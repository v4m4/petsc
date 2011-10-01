static const char help[] = "p-Bratu nonlinear PDE in 2d.\n\
We solve the  p-Laplacian (nonlinear diffusion) combined with\n\
the Bratu (solid fuel ignition) nonlinearity in a 2D rectangular\n\
domain, using distributed arrays (DAs) to partition the parallel grid.\n\
The command line options include:\n\
  -p <2>: `p' in p-Laplacian term\n\
  -epsilon <1e-05>: Strain-regularization in p-Laplacian\n\
  -lambda <6>: Bratu parameter\n\
\n";


/* ------------------------------------------------------------------------

    p-Laplacian and Solid Fuel Ignition problem.  This problem is modeled by
    the partial differential equation

        -div(eta grad(u)) - lambda exp(u) = 0,  0 < x,y < 1,

    with closure

        eta(gamma) = (epsilon^2 + gamma)^((p-2)/2),   gamma = 1/2 |grad u|^2

    with boundary conditions

        u = 0  for  x = 0, x = 1, y = 0, y = 1.

    A finite difference approximation a 9-point stencil is used to discretize
    the boundary value problem to obtain a nonlinear system of equations.
    This would be a 5-point stencil if not for the p-Laplacian's nonlinearity.

    Program usage:  mpiexec -n <procs> ./pbratu [-help] [all PETSc options]
     e.g.,
      ./pbratu -fd_jacobian -mat_fd_coloring_view_draw -draw_pause -1
      mpiexec -n 2 ./pbratu -fd_jacobian_ghosted -log_summary

  ------------------------------------------------------------------------- */

/*
   Include "petscda.h" so that we can use distributed arrays (DAs).
   Include "petscsnes.h" so that we can use SNES solvers.  Note that this
   file automatically includes:
     petsc.h       - base PETSc routines   petscvec.h - vectors
     petscsys.h    - system routines       petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers
*/
#include <petscdmda.h>
#include <petscsnes.h>

/*
   User-defined application context - contains data needed by the 
   application-provided call-back routines, FormJacobianLocal() and
   FormFunctionLocal().
*/
typedef struct {
  PassiveReal lambda;         /* Bratu parameter */
  PassiveReal p;              /* Exponent in p-Laplacian */
  PassiveReal epsilon;        /* Regularization */
  PetscInt    jtype;          /* What type of Jacobian to assemble */
} AppCtx;

/*
   User-defined routines
*/
static PetscErrorCode FormInitialGuess(AppCtx*,DM,Vec);
static PetscErrorCode FormFunctionLocal(DMDALocalInfo*,PetscScalar**,PetscScalar**,AppCtx*);
static PetscErrorCode FormJacobianLocal(DMDALocalInfo*,PetscScalar**,Mat,AppCtx*);
static PetscErrorCode MySNESDefaultComputeJacobianColor(SNES,Vec,Mat*,Mat*,MatStructure*,void*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  SNES                   snes;                 /* nonlinear solver */
  Vec                    x,r;                  /* solution, residual vectors */
  Mat                    A,B;                  /* Jacobian and preconditioning matrices */
  AppCtx                 user;                 /* user-defined work context */
  PetscInt               its;                  /* iterations for convergence */
  SNESConvergedReason    reason;               /* Check convergence */
  PetscBool              alloc_star,fd_jacobian = PETSC_FALSE,fd_jacobian_ghosted = PETSC_FALSE;
  PetscReal              bratu_lambda_max = 6.81,bratu_lambda_min = 0.;
  MatFDColoring          matfdcoloring = 0;
  ISColoring             iscoloring;
  DM                     da,dastar;            /* distributed array data structure */
  PetscErrorCode         ierr;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  PetscInitialize(&argc,&argv,(char *)0,help);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize problem parameters
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  user.lambda = 6.0; user.p = 2.0; user.epsilon = 1e-5; user.jtype = 4; alloc_star = PETSC_FALSE;
  ierr = PetscOptionsBegin(PETSC_COMM_WORLD,NULL,"p-Bratu options",__FILE__);CHKERRQ(ierr);
  {
    ierr = PetscOptionsReal("-lambda","Bratu parameter","",user.lambda,&user.lambda,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsReal("-p","Exponent `p' in p-Laplacian","",user.p,&user.p,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsReal("-epsilon","Strain-regularization in p-Laplacian","",user.epsilon,&user.epsilon,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsInt("-jtype","Jacobian type, 1=plain, 2=first term 3=star 4=full","",user.jtype,&user.jtype,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsBool("-alloc_star","Allocate for STAR stencil (5-point)","",alloc_star,&alloc_star,NULL);CHKERRQ(ierr);
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  if (user.lambda > bratu_lambda_max || user.lambda < bratu_lambda_min) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"WARNING: lambda %g out of range for p=2\n",user.lambda);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create nonlinear solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESCreate(PETSC_COMM_WORLD,&snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create distributed array (DA) to manage parallel grid and vectors
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DMDACreate2d(PETSC_COMM_WORLD,DMDA_BOUNDARY_NONE,DMDA_BOUNDARY_NONE,DMDA_STENCIL_BOX,-4,-4,PETSC_DECIDE,PETSC_DECIDE,
                      1,1,PETSC_NULL,PETSC_NULL,&da);CHKERRQ(ierr);
  ierr = DMDACreate2d(PETSC_COMM_WORLD,DMDA_BOUNDARY_NONE,DMDA_BOUNDARY_NONE,DMDA_STENCIL_STAR,-4,-4,PETSC_DECIDE,PETSC_DECIDE,
                      1,1,PETSC_NULL,PETSC_NULL,&dastar);CHKERRQ(ierr);


  /*  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Extract global vectors from DA; then duplicate for remaining
     vectors that are the same types
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DMCreateGlobalVector(da,&x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&r);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create matrix data structure; set Jacobian evaluation routine

     Set Jacobian matrix data structure and default Jacobian evaluation
     routine. User can override with:
     -snes_mf : matrix-free Newton-Krylov method with no preconditioning
                (unless user explicitly sets preconditioner)
     -snes_mf_operator : form preconditioning matrix as set by the user,
                         but use matrix-free approx for Jacobian-vector
                         products within Newton-Krylov method

     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /* B can be type of MATAIJ,MATBAIJ or MATSBAIJ */
  ierr = DMGetMatrix(alloc_star ? dastar : da,MATAIJ,&B);CHKERRQ(ierr);

  A    = B;
  ierr = PetscOptionsGetBool(PETSC_NULL,"-fd_jacobian",&fd_jacobian,0);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(PETSC_NULL,"-fd_jacobian_ghosted",&fd_jacobian_ghosted,0);CHKERRQ(ierr);

  if (fd_jacobian) {
    ierr = DMGetColoring(da,IS_COLORING_GLOBAL,MATAIJ,&iscoloring);CHKERRQ(ierr);
    ierr = MatFDColoringCreate(B,iscoloring,&matfdcoloring);CHKERRQ(ierr);
    ierr = ISColoringDestroy(&iscoloring);CHKERRQ(ierr);
    ierr = MatFDColoringSetFunction(matfdcoloring,(PetscErrorCode (*)(void))SNESDAFormFunction,&user);CHKERRQ(ierr);
    ierr = MatFDColoringSetFromOptions(matfdcoloring);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,A,B,SNESDefaultComputeJacobianColor,matfdcoloring);CHKERRQ(ierr);
  } else if (fd_jacobian_ghosted) {
    ierr = DMGetColoring(da,IS_COLORING_GHOSTED,MATAIJ,&iscoloring);CHKERRQ(ierr);
    ierr = MatFDColoringCreate(B,iscoloring,&matfdcoloring);CHKERRQ(ierr);
    ierr = ISColoringDestroy(&iscoloring);CHKERRQ(ierr);
    ierr = MatFDColoringSetFunction(matfdcoloring,(PetscErrorCode (*)(void))SNESDAFormFunction,&user);CHKERRQ(ierr);
    ierr = MatFDColoringSetFromOptions(matfdcoloring);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,A,B,MySNESDefaultComputeJacobianColor,matfdcoloring);CHKERRQ(ierr);
  } else {
    ierr = SNESSetJacobian(snes,A,B,SNESDAComputeJacobian,&user);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set local function evaluation routine
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = DMDASetLocalFunction(da,(DMDALocalFunction1)FormFunctionLocal);CHKERRQ(ierr);
  ierr = DMDASetLocalJacobian(da,(DMDALocalFunction1)FormJacobianLocal);CHKERRQ(ierr);
  ierr = SNESSetDM(snes,da);CHKERRQ(ierr);
  ierr = SNESSetFunction(snes,r,SNESDAFormFunction,&user);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Customize nonlinear solver; set runtime options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Evaluate initial guess
     Note: The user should initialize the vector, x, with the initial guess
     for the nonlinear solver prior to calling SNESSolve().  In particular,
     to employ an initial guess of zero, the user should explicitly set
     this vector to zero by calling VecSet().
  */

  ierr = FormInitialGuess(&user,da,x);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = SNESSolve(snes,PETSC_NULL,x);CHKERRQ(ierr);
  ierr = SNESGetIterationNumber(snes,&its);CHKERRQ(ierr);
  ierr = SNESGetConvergedReason(snes,&reason);CHKERRQ(ierr);

  ierr = PetscPrintf(PETSC_COMM_WORLD,"%s Number of Newton iterations = %D\n",SNESConvergedReasons[reason],its);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if (A != B) {
    ierr = MatDestroy(&A);CHKERRQ(ierr);
  }
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatFDColoringDestroy(&matfdcoloring);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&r);CHKERRQ(ierr);
  ierr = SNESDestroy(&snes);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = DMDestroy(&dastar);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}

/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormInitialGuess"
/*
   FormInitialGuess - Forms initial approximation.

   Input Parameters:
   user - user-defined application context
   X - vector

   Output Parameter:
   X - vector
 */
static PetscErrorCode FormInitialGuess(AppCtx *user,DM da,Vec X)
{
  PetscInt       i,j,Mx,My,xs,ys,xm,ym;
  PetscErrorCode ierr;
  PetscReal      temp1,temp,hx,hy;
  PetscScalar    **x;

  PetscFunctionBegin;
  ierr = DMDAGetInfo(da,PETSC_IGNORE,&Mx,&My,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,
                     PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE,PETSC_IGNORE);

  hx     = 1.0/(PetscReal)(Mx-1);
  hy     = 1.0/(PetscReal)(My-1);
  temp1  = user->lambda / (user->lambda + 1.);

  /*
     Get a pointer to vector data.
       - For default PETSc vectors, VecGetArray() returns a pointer to
         the data array.  Otherwise, the routine is implementation dependent.
       - You MUST call VecRestoreArray() when you no longer need access to
         the array.
  */
  ierr = DMDAVecGetArray(da,X,&x);CHKERRQ(ierr);

  /*
     Get local grid boundaries (for 2-dimensional DA):
       xs, ys   - starting grid indices (no ghost points)
       xm, ym   - widths of local grid (no ghost points)

  */
  ierr = DMDAGetCorners(da,&xs,&ys,PETSC_NULL,&xm,&ym,PETSC_NULL);CHKERRQ(ierr);

  /*
     Compute initial guess over the locally owned part of the grid
  */
  for (j=ys; j<ys+ym; j++) {
    temp = (PetscReal)(PetscMin(j,My-j-1))*hy;
    for (i=xs; i<xs+xm; i++) {
      if (i == 0 || j == 0 || i == Mx-1 || j == My-1) {
        /* boundary conditions are all zero Dirichlet */
        x[j][i] = 0.0;
      } else {
        if (user->lambda != 0) {
          x[j][i] = temp1*sqrt(PetscMin((PetscReal)(PetscMin(i,Mx-i-1))*hx,temp));
        } else {
          /* The solution above is an exact solution for lambda=0, this avoids "accidentally" starting
           * with an exact solution. */
          const PetscReal
            xx = 2*(PetscReal)i/(Mx-1) - 1,
            yy = 2*(PetscReal)j/(My-1) - 1;
          x[j][i] = (1 - xx*xx) * (1-yy*yy);
        }
      }
    }
  }

  /*
     Restore vector
  */
  ierr = DMDAVecRestoreArray(da,X,&x);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

/* p-Laplacian diffusivity */
static inline PetscScalar eta(const AppCtx *ctx,PetscScalar ux,PetscScalar uy)
{return pow(PetscSqr(ctx->epsilon)+0.5*(ux*ux + uy*uy),0.5*(ctx->p-2.));}
static inline PetscScalar deta(const AppCtx *ctx,PetscScalar ux,PetscScalar uy)
{
  return (ctx->p == 2)
    ? 0
    : pow(PetscSqr(ctx->epsilon)+0.5*(ux*ux + uy*uy),0.5*(ctx->p-4)) * 0.5 * (ctx->p-2.);
}


/* ------------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "FormFunctionLocal"
/*
   FormFunctionLocal - Evaluates nonlinear function, F(x).
 */
static PetscErrorCode FormFunctionLocal(DMDALocalInfo *info,PetscScalar **x,PetscScalar **f,AppCtx *user)
{
  PetscReal      hx,hy,dhx,dhy,sc;
  PetscInt       i,j;

  PetscFunctionBegin;
  hx     = 1.0/(PetscReal)(info->mx-1);
  hy     = 1.0/(PetscReal)(info->my-1);
  sc     = hx*hy*user->lambda;
  dhx    = 1/hx;
  dhy    = 1/hy;
  /*
     Compute function over the locally owned part of the grid
  */
  for (j=info->ys; j<info->ys+info->ym; j++) {
    for (i=info->xs; i<info->xs+info->xm; i++) {
      if (i == 0 || j == 0 || i == info->mx-1 || j == info->my-1) {
        f[j][i] = x[j][i];
      } else {
        const PetscScalar
          u = x[j][i],
          ux_E = dhx*(x[j][i+1]-x[j][i]),
          uy_E = 0.25*dhy*(x[j+1][i]+x[j+1][i+1]-x[j-1][i]-x[j-1][i+1]),
          ux_W = dhx*(x[j][i]-x[j][i-1]),
          uy_W = 0.25*dhy*(x[j+1][i-1]+x[j+1][i]-x[j-1][i-1]-x[j-1][i]),
          ux_N = 0.25*dhx*(x[j][i+1]+x[j+1][i+1]-x[j][i-1]-x[j+1][i-1]),
          uy_N = dhy*(x[j+1][i]-x[j][i]),
          ux_S = 0.25*dhx*(x[j-1][i+1]+x[j][i+1]-x[j-1][i-1]-x[j][i-1]),
          uy_S = dhy*(x[j][i]-x[j-1][i]),
          e_E = eta(user,ux_E,uy_E),
          e_W = eta(user,ux_W,uy_W),
          e_N = eta(user,ux_N,uy_N),
          e_S = eta(user,ux_S,uy_S),
          uxx = -hy * (e_E*ux_E - e_W*ux_W),
          uyy = -hx * (e_N*uy_N - e_S*uy_S);
        /** For p=2, these terms decay to:
        * uxx = (2.0*u - x[j][i-1] - x[j][i+1])*hydhx
        * uyy = (2.0*u - x[j-1][i] - x[j+1][i])*hxdhy
        **/
        f[j][i] = uxx + uyy - sc*PetscExpScalar(u);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "FormJacobianLocal"
/*
   FormJacobianLocal - Evaluates Jacobian matrix.
*/
static PetscErrorCode FormJacobianLocal(DMDALocalInfo *info,PetscScalar **x,Mat jac,AppCtx *user)
{
  PetscErrorCode ierr;
  PetscInt       i,j;
  MatStencil     col[9],row;
  PetscScalar    v[9];
  PetscReal      hx,hy,hxdhy,hydhx,dhx,dhy,sc;

  PetscFunctionBegin;
  hx     = 1.0/(PetscReal)(info->mx-1);
  hy     = 1.0/(PetscReal)(info->my-1);
  sc     = hx*hy*user->lambda;
  dhx    = 1/hx;
  dhy    = 1/hy;
  hxdhy  = hx/hy;
  hydhx  = hy/hx;

  /*
     Compute entries for the locally owned part of the Jacobian.
      - PETSc parallel matrix formats are partitioned by
        contiguous chunks of rows across the processors.
      - Each processor needs to insert only elements that it owns
        locally (but any non-local elements will be sent to the
        appropriate processor during matrix assembly).
      - Here, we set all entries for a particular row at once.
  */
  for (j=info->ys; j<info->ys+info->ym; j++) {
    for (i=info->xs; i<info->xs+info->xm; i++) {
      row.j = j; row.i = i;
      /* boundary points */
      if (i == 0 || j == 0 || i == info->mx-1 || j == info->my-1) {
        v[0] = 1.0;
        ierr = MatSetValuesStencil(jac,1,&row,1,&row,v,INSERT_VALUES);CHKERRQ(ierr);
      } else {
      /* interior grid points */
        const PetscScalar
          ux_E = dhx*(x[j][i+1]-x[j][i]),
          uy_E = 0.25*dhy*(x[j+1][i]+x[j+1][i+1]-x[j-1][i]-x[j-1][i+1]),
          ux_W = dhx*(x[j][i]-x[j][i-1]),
          uy_W = 0.25*dhy*(x[j+1][i-1]+x[j+1][i]-x[j-1][i-1]-x[j-1][i]),
          ux_N = 0.25*dhx*(x[j][i+1]+x[j+1][i+1]-x[j][i-1]-x[j+1][i-1]),
          uy_N = dhy*(x[j+1][i]-x[j][i]),
          ux_S = 0.25*dhx*(x[j-1][i+1]+x[j][i+1]-x[j-1][i-1]-x[j][i-1]),
          uy_S = dhy*(x[j][i]-x[j-1][i]),
          u = x[j][i],
          e_E = eta(user,ux_E,uy_E),
          e_W = eta(user,ux_W,uy_W),
          e_N = eta(user,ux_N,uy_N),
          e_S = eta(user,ux_S,uy_S),
          de_E = deta(user,ux_E,uy_E),
          de_W = deta(user,ux_W,uy_W),
          de_N = deta(user,ux_N,uy_N),
          de_S = deta(user,ux_S,uy_S),
          skew_E = de_E*ux_E*uy_E,
          skew_W = de_W*ux_W*uy_W,
          skew_N = de_N*ux_N*uy_N,
          skew_S = de_S*ux_S*uy_S,
          cross_EW = 0.25*(skew_E - skew_W),
          cross_NS = 0.25*(skew_N - skew_S),
          newt_E = e_E+de_E*PetscSqr(ux_E),
          newt_W = e_W+de_W*PetscSqr(ux_W),
          newt_N = e_N+de_N*PetscSqr(uy_N),
          newt_S = e_S+de_S*PetscSqr(uy_S);
      /* interior grid points */
        switch (user->jtype) {
          case 1:
            /* Jacobian from p=2 */
            v[0] = -hxdhy;                                           col[0].j = j-1;   col[0].i = i;
            v[1] = -hydhx;                                           col[1].j = j;     col[1].i = i-1;
            v[2] = 2.0*(hydhx + hxdhy) - sc*PetscExpScalar(u);       col[2].j = row.j; col[2].i = row.i;
            v[3] = -hydhx;                                           col[3].j = j;     col[3].i = i+1;
            v[4] = -hxdhy;                                           col[4].j = j+1;   col[4].i = i;
            ierr = MatSetValuesStencil(jac,1,&row,5,col,v,INSERT_VALUES);CHKERRQ(ierr);
            break;
          case 2:
            /* Jacobian arising from Picard linearization */
            v[0] = -hxdhy*e_S;                                           col[0].j = j-1;   col[0].i = i;
            v[1] = -hydhx*e_W;                                           col[1].j = j;     col[1].i = i-1;
            v[2] = (e_W+e_E)*hydhx + (e_S+e_N)*hxdhy - sc*PetscExpScalar(u); col[2].j = row.j; col[2].i = row.i;
            v[3] = -hydhx*e_E;                                           col[3].j = j;     col[3].i = i+1;
            v[4] = -hxdhy*e_N;                                           col[4].j = j+1;   col[4].i = i;
            ierr = MatSetValuesStencil(jac,1,&row,5,col,v,INSERT_VALUES);CHKERRQ(ierr);
            break;
          case 3:
            /* Full Jacobian, but only a star stencil */
            col[0].j = j-1; col[0].i = i;
            col[1].j = j;   col[1].i = i-1;
            col[2].j = j;   col[2].i = i;
            col[3].j = j;   col[3].i = i+1;
            col[4].j = j+1; col[4].i = i;
            v[0] = -hxdhy*newt_S + cross_EW;
            v[1] = -hydhx*newt_W + cross_NS;
            v[2] = hxdhy*(newt_N + newt_S) + hydhx*(newt_E + newt_W) - sc*PetscExpScalar(u);
            v[3] = -hydhx*newt_E - cross_NS;
            v[4] = -hxdhy*newt_N - cross_EW;
            ierr = MatSetValuesStencil(jac,1,&row,5,col,v,INSERT_VALUES);CHKERRQ(ierr);
            break;
          case 4:
            /** The Jacobian is
            *
            * -div [ eta (grad u) + deta (grad u0 . grad u) grad u0 ] - (eE u0) u
            *
            **/
            col[0].j = j-1; col[0].i = i-1;
            col[1].j = j-1; col[1].i = i;
            col[2].j = j-1; col[2].i = i+1;
            col[3].j = j;   col[3].i = i-1;
            col[4].j = j;   col[4].i = i;
            col[5].j = j;   col[5].i = i+1;
            col[6].j = j+1; col[6].i = i-1;
            col[7].j = j+1; col[7].i = i;
            col[8].j = j+1; col[8].i = i+1;
            v[0] = -0.25*(skew_S + skew_W);
            v[1] = -hxdhy*newt_S + cross_EW;
            v[2] =  0.25*(skew_S + skew_E);
            v[3] = -hydhx*newt_W + cross_NS;
            v[4] = hxdhy*(newt_N + newt_S) + hydhx*(newt_E + newt_W) - sc*PetscExpScalar(u);
            v[5] = -hydhx*newt_E - cross_NS;
            v[6] =  0.25*(skew_N + skew_W);
            v[7] = -hxdhy*newt_N - cross_EW;
            v[8] = -0.25*(skew_N + skew_E);
            ierr = MatSetValuesStencil(jac,1,&row,9,col,v,INSERT_VALUES);CHKERRQ(ierr);
            break;
          default:
            SETERRQ1(((PetscObject)info->da)->comm,PETSC_ERR_SUP,"Jacobian type %d not implemented",user->jtype);
        }
      }
    }
  }

  /*
     Assemble matrix, using the 2-step process:
       MatAssemblyBegin(), MatAssemblyEnd().
  */
  ierr = MatAssemblyBegin(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(jac,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  /*
     Tell the matrix we will never add a new nonzero location to the
     matrix. If we do, it will generate an error.
  */
  ierr = MatSetOption(jac,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MySNESDefaultComputeJacobianColor"
/*
  MySNESDefaultComputeJacobianColor - Computes the Jacobian using
    finite differences and coloring to exploit matrix sparsity.
    It is customized from SNESDefaultComputeJacobianColor.
    The input global vector x1 is scattered to x1_local
    which then is passed into MatFDColoringApply() for reducing the
    VecScatterBingin/End.
*/
static PetscErrorCode MySNESDefaultComputeJacobianColor(SNES snes,Vec x1,Mat *J,Mat *B,MatStructure *flag,void *ctx)
{
  MatFDColoring  color = (MatFDColoring) ctx;
  PetscErrorCode ierr;
  Vec            f;
  PetscErrorCode (*ff)(void),(*fd)(void);
  void           *fctx;
  DM             da;
  Vec            x1_loc;

  PetscFunctionBegin;
  *flag = SAME_NONZERO_PATTERN;
  ierr  = SNESGetFunction(snes,&f,(PetscErrorCode (**)(SNES,Vec,Vec,void*))&ff,0);CHKERRQ(ierr);
  ierr  = MatFDColoringGetFunction(color,&fd,&fctx);CHKERRQ(ierr);
  if (fd == ff) { /* reuse function value computed in SNES */
    ierr  = MatFDColoringSetF(color,f);CHKERRQ(ierr);
  }
  /* Now, get x1_loc and scatter global x1 onto x1_loc */
  da = *(DM*)fctx;
  ierr = DMGetLocalVector(da,&x1_loc);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(da,x1,INSERT_VALUES,x1_loc);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(da,x1,INSERT_VALUES,x1_loc);CHKERRQ(ierr);
  ierr = MatFDColoringApply(*B,color,x1_loc,flag,snes);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(da,&x1_loc);CHKERRQ(ierr);
  if (*J != *B) {
    ierr = MatAssemblyBegin(*J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(*J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}