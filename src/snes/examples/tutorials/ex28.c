static const char help[] = "Uses analytic Jacobians to solve individual problems and a coupled problem\n\n";

/* Solve a PDE coupled to an algebraic system in 1D
 *
 * PDE (U):
 *     -(k u_x)_x = 1 on (0,1), subject to u(0) = 0, u(1) = 1
 * Algebraic (K):
 *     exp(k) + k = u + 1/(1 + u_x^2)
 *
 * The discretization places k at staggered points, and a separate DMDA is used for each "physics".
 *
 * This example is a prototype for coupling in multi-physics problems, therefore residual evaluation and assembly for
 * each problem (referred to as U and K) are written separately.  This permits the same "physics" code to be used for
 * solving each uncoupled problem as well as the coupled system.  In all cases, a fully-assembled analytic Jacobian is
 * available, so the systems can be solved with a direct solve.  Additionally, by running with
 *
 *   -pack_dm_mat_type nest
 *
 * The same code assembles a coupled matrix where each block is stored separately, which allows the use of PCFieldSplit
 * without copying values to extract submatrices.
 */

#include <petscsnes.h>

PetscErrorCode PETSCDM_DLLEXPORT DMDACreateOwnershipRanges(DM); /* Import an internal function */

typedef struct _UserCtx *User;
struct _UserCtx {
  PetscInt ptype;
  DM       pack;
  Vec      Uloc,Kloc;
};

#undef __FUNCT__  
#define __FUNCT__ "FormFunctionLocal_U"
static PetscErrorCode FormFunctionLocal_U(User user,DMDALocalInfo *info,const PetscScalar u[],const PetscScalar k[],PetscScalar f[])
{
  PetscInt i;

  PetscFunctionBegin;
  for (i=info->xs; i<info->xs+info->xm; i++) {
    if (i == 0) f[i] = u[i];
    else if (i == info->mx-1) f[i] = u[i] - 1.0;
    else f[i] = k[i-1]*(u[i]-u[i-1]) - k[i]*(u[i+1]-u[i]) - 1.0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormFunctionLocal_K"
static PetscErrorCode FormFunctionLocal_K(User user,DMDALocalInfo *info,const PetscScalar u[],const PetscScalar k[],PetscScalar f[])
{
  PetscReal hx = 1./info->mx;
  PetscInt  i;

  PetscFunctionBegin;
  for (i=info->xs; i<info->xs+info->xm; i++) {
    const PetscScalar
      ubar = 0.5*(u[i+1]+u[i]),
      gradu = (u[i+1]-u[i])/hx,
      g = 1. + gradu*gradu,
      w = 1./g;
    f[i] = PetscExpScalar(k[i]) + k[i] - (ubar + w);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormFunction_All"
static PetscErrorCode FormFunction_All(SNES snes,Vec X,Vec F,void *ctx)
{
  User              user = (User)ctx;
  DM                dau,dak;
  DMDALocalInfo     infou,infok;
  const PetscScalar *u,*k;
  PetscScalar       *fu,*fk;
  PetscErrorCode    ierr;
  Vec               Uloc,Kloc,Fu,Fk;

  PetscFunctionBegin;
  ierr = DMCompositeGetEntries(user->pack,&dau,&dak);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dau,&infou);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dak,&infok);CHKERRQ(ierr);
  ierr = DMCompositeGetLocalVectors(user->pack,&Uloc,&Kloc);CHKERRQ(ierr);
  switch (user->ptype) {
  case 0:
    ierr = DMGlobalToLocalBegin(dau,X,INSERT_VALUES,Uloc);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd  (dau,X,INSERT_VALUES,Uloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,user->Kloc,&k);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,F,&fu);CHKERRQ(ierr);
    ierr = FormFunctionLocal_U(user,&infou,u,k,fu);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,F,&fu);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,user->Kloc,&k);CHKERRQ(ierr);
    break;
  case 1:
    ierr = DMGlobalToLocalBegin(dak,X,INSERT_VALUES,Kloc);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd  (dak,X,INSERT_VALUES,Kloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,user->Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,Kloc,&k);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,F,&fk);CHKERRQ(ierr);
    ierr = FormFunctionLocal_K(user,&infok,u,k,fk);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,F,&fk);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,user->Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,Kloc,&k);CHKERRQ(ierr);
    break;
  case 2:
    ierr = DMCompositeScatter(user->pack,X,Uloc,Kloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,Kloc,&k);CHKERRQ(ierr);
    ierr = DMCompositeGetAccess(user->pack,F,&Fu,&Fk);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,Fu,&fu);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,Fk,&fk);CHKERRQ(ierr);
    ierr = FormFunctionLocal_U(user,&infou,u,k,fu);CHKERRQ(ierr);
    ierr = FormFunctionLocal_K(user,&infok,u,k,fk);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,Fu,&fu);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,Fk,&fk);CHKERRQ(ierr);
    ierr = DMCompositeRestoreAccess(user->pack,F,&Fu,&Fk);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,Kloc,&k);CHKERRQ(ierr);
    break;
  }
  ierr = DMCompositeRestoreLocalVectors(user->pack,&Uloc,&Kloc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormJacobianLocal_U"
static PetscErrorCode FormJacobianLocal_U(User user,DMDALocalInfo *info,const PetscScalar u[],const PetscScalar k[],Mat Buu)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  for (i=info->xs; i<info->xs+info->xm; i++) {
    PetscInt row = i-info->gxs,cols[] = {row-1,row,row+1};
    PetscScalar one = 1.0;
    if (i == 0) {ierr = MatSetValuesLocal(Buu,1,&row,1,&row,&one,INSERT_VALUES);CHKERRQ(ierr);}
    else if (i == info->mx-1) {ierr = MatSetValuesLocal(Buu,1,&row,1,&row,&one,INSERT_VALUES);CHKERRQ(ierr);}
    else {
      PetscScalar vals[] = {-k[i-1],k[i-1]+k[i],-k[i]};
      ierr = MatSetValuesLocal(Buu,1,&row,3,cols,vals,INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormJacobianLocal_K"
static PetscErrorCode FormJacobianLocal_K(User user,DMDALocalInfo *info,const PetscScalar u[],const PetscScalar k[],Mat Bkk)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  for (i=info->xs; i<info->xs+info->xm; i++) {
    PetscInt row = i-info->gxs;
    PetscScalar vals[] = {PetscExpScalar(k[i])+1.};
    ierr = MatSetValuesLocal(Bkk,1,&row,1,&row,vals,INSERT_VALUES);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormJacobianLocal_UK"
static PetscErrorCode FormJacobianLocal_UK(User user,DMDALocalInfo *info,DMDALocalInfo *infok,const PetscScalar u[],const PetscScalar k[],Mat Buk)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  if (!Buk) PetscFunctionReturn(0); /* Not assembling this block */
  for (i=info->xs; i<info->xs+info->xm; i++) {
    if (i == 0 || i == info->mx-1) continue;
    PetscInt row = i-info->gxs,cols[] = {i-1-infok->gxs,i-infok->gxs};
    PetscScalar vals[] = {u[i]-u[i-1],u[i]-u[i+1]};
    ierr = MatSetValuesLocal(Buk,1,&row,2,cols,vals,INSERT_VALUES);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormJacobianLocal_KU"
static PetscErrorCode FormJacobianLocal_KU(User user,DMDALocalInfo *info,DMDALocalInfo *infok,const PetscScalar u[],const PetscScalar k[],Mat Bku)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscReal      hx = 1./(info->mx-1);

  PetscFunctionBegin;
  if (!Bku) PetscFunctionReturn(0); /* Not assembling this block */
  for (i=infok->xs; i<infok->xs+infok->xm; i++) {
    PetscInt row = i-infok->gxs,cols[] = {i-info->gxs,i+1-info->gxs};
    const PetscScalar
      ubar_L  = 0.5,
      ubar_R  = 0.5,
      gradu   = (u[i+1]-u[i])/hx,
      gradu_L = -1./hx,
      gradu_R = 1./hx,
      g       = 1. + gradu*gradu,
      g_gradu = 2.*gradu,
      w       = 1./g,
      w_gradu = -g_gradu*w*w,
      vals[]  = {-ubar_L - w_gradu*gradu_L, -ubar_R - w_gradu*gradu_R};
    ierr = MatSetValuesLocal(Bku,1,&row,2,cols,vals,INSERT_VALUES);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormJacobian_All"
static PetscErrorCode FormJacobian_All(SNES snes,Vec X,Mat *J,Mat *B,MatStructure *mstr,void *ctx)
{
  User              user = (User)ctx;
  DM                dau,dak;
  DMDALocalInfo     infou,infok;
  const PetscScalar *u,*k;
  PetscErrorCode    ierr;
  Vec               Uloc,Kloc;

  PetscFunctionBegin;
  ierr = DMCompositeGetEntries(user->pack,&dau,&dak);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dau,&infou);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dak,&infok);CHKERRQ(ierr);
  ierr = DMCompositeGetLocalVectors(user->pack,&Uloc,&Kloc);CHKERRQ(ierr);
  switch (user->ptype) {
  case 0:
    ierr = DMGlobalToLocalBegin(dau,X,INSERT_VALUES,Uloc);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd  (dau,X,INSERT_VALUES,Uloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,user->Kloc,&k);CHKERRQ(ierr);
    ierr = FormJacobianLocal_U(user,&infou,u,k,*B);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,user->Kloc,&k);CHKERRQ(ierr);
    break;
  case 1:
    ierr = DMGlobalToLocalBegin(dak,X,INSERT_VALUES,Kloc);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd  (dak,X,INSERT_VALUES,Kloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,user->Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,Kloc,&k);CHKERRQ(ierr);
    ierr = FormJacobianLocal_K(user,&infok,u,k,*B);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,user->Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,Kloc,&k);CHKERRQ(ierr);
    break;
  case 2: {
    Mat Buu,Buk,Bku,Bkk;
    IS  *is;
    ierr = DMCompositeScatter(user->pack,X,Uloc,Kloc);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecGetArray(dak,Kloc,&k);CHKERRQ(ierr);
    ierr = DMCompositeGetLocalISs(user->pack,&is);CHKERRQ(ierr);
    ierr = MatGetLocalSubMatrix(*B,is[0],is[0],&Buu);CHKERRQ(ierr);
    ierr = MatGetLocalSubMatrix(*B,is[0],is[1],&Buk);CHKERRQ(ierr);
    ierr = MatGetLocalSubMatrix(*B,is[1],is[0],&Bku);CHKERRQ(ierr);
    ierr = MatGetLocalSubMatrix(*B,is[1],is[1],&Bkk);CHKERRQ(ierr);
    ierr = FormJacobianLocal_U(user,&infou,u,k,Buu);CHKERRQ(ierr);
    ierr = FormJacobianLocal_UK(user,&infou,&infok,u,k,Buk);CHKERRQ(ierr);
    ierr = FormJacobianLocal_KU(user,&infou,&infok,u,k,Bku);CHKERRQ(ierr);
    ierr = FormJacobianLocal_K(user,&infok,u,k,Bkk);CHKERRQ(ierr);
    ierr = MatRestoreLocalSubMatrix(*B,is[0],is[0],&Buu);CHKERRQ(ierr);
    ierr = MatRestoreLocalSubMatrix(*B,is[0],is[1],&Buk);CHKERRQ(ierr);
    ierr = MatRestoreLocalSubMatrix(*B,is[1],is[0],&Bku);CHKERRQ(ierr);
    ierr = MatRestoreLocalSubMatrix(*B,is[1],is[1],&Bkk);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dau,Uloc,&u);CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(dak,Kloc,&k);CHKERRQ(ierr);

    ierr = ISDestroy(is[0]);CHKERRQ(ierr);
    ierr = ISDestroy(is[1]);CHKERRQ(ierr);
    ierr = PetscFree(is);CHKERRQ(ierr);
  } break;
  }
  ierr = DMCompositeRestoreLocalVectors(user->pack,&Uloc,&Kloc);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd  (*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (*J != *B) {
    ierr = MatAssemblyBegin(*J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd  (*J,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  *mstr = SAME_NONZERO_PATTERN;
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "FormInitial_Coupled"
static PetscErrorCode FormInitial_Coupled(User user,Vec X)
{
  PetscErrorCode ierr;
  DM             dau,dak;
  DMDALocalInfo  infou,infok;
  Vec            Xu,Xk;
  PetscScalar    *u,*k,hx;
  PetscInt       i;

  PetscFunctionBegin;
  ierr = DMCompositeGetEntries(user->pack,&dau,&dak);CHKERRQ(ierr);
  ierr = DMCompositeGetAccess(user->pack,X,&Xu,&Xk);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(dau,Xu,&u);CHKERRQ(ierr);
  ierr = DMDAVecGetArray(dak,Xk,&k);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dau,&infou);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(dak,&infok);CHKERRQ(ierr);
  hx = 1./(infok.mx);
  for (i=infou.xs; i<infou.xs+infou.xm; i++) u[i] = (PetscScalar)i*hx * (1.-(PetscScalar)i*hx);
  for (i=infok.xs; i<infok.xs+infok.xm; i++) k[i] = 5.0;
  ierr = DMDAVecRestoreArray(dau,Xu,&u);CHKERRQ(ierr);
  ierr = DMDAVecRestoreArray(dak,Xk,&k);CHKERRQ(ierr);
  ierr = DMCompositeRestoreAccess(user->pack,X,&Xu,&Xk);CHKERRQ(ierr);
  ierr = DMCompositeScatter(user->pack,X,user->Uloc,user->Kloc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "main"
int main(int argc, char *argv[])
{
  PetscErrorCode ierr;
  DM             dau,dak,pack;
  const PetscInt *lxu;
  PetscInt       *lxk,m,nprocs;
  User           user;
  SNES           snes;
  Vec            X,F,Xu,Xk,Fu,Fk;
  Mat            B;
  PetscBool      view_draw;

  PetscInitialize(&argc,&argv,0,help);
  ierr = DMDACreate1d(PETSC_COMM_WORLD,DMDA_NONPERIODIC,-10,1,1,PETSC_NULL,&dau);CHKERRQ(ierr);
  ierr = DMDACreateOwnershipRanges(dau);CHKERRQ(ierr); /* Ensure that the ownership ranges agree so that we can get a compatible grid for the coefficient */
  ierr = DMDAGetOwnershipRanges(dau,&lxu,0,0);CHKERRQ(ierr);
  ierr = DMDAGetInfo(dau,0, &m,0,0, &nprocs,0,0, 0,0,0,0);CHKERRQ(ierr);
  ierr = PetscMalloc(nprocs*sizeof(*lxk),&lxk);CHKERRQ(ierr);
  ierr = PetscMemcpy(lxk,lxu,nprocs*sizeof(*lxk));CHKERRQ(ierr);
  lxk[0]--;
  ierr = DMDACreate1d(PETSC_COMM_WORLD,DMDA_NONPERIODIC,m-1,1,1,lxk,&dak);CHKERRQ(ierr);
  ierr = PetscFree(lxk);CHKERRQ(ierr);

  ierr = DMCompositeCreate(PETSC_COMM_WORLD,&pack);CHKERRQ(ierr);
  ierr = DMCompositeAddDM(pack,dau);CHKERRQ(ierr);
  ierr = DMCompositeAddDM(pack,dak);CHKERRQ(ierr);
  ierr = DMDASetFieldName(dau,0,"u");CHKERRQ(ierr);
  ierr = DMDASetFieldName(dak,0,"k");CHKERRQ(ierr);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)dau,"u_");CHKERRQ(ierr);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)dak,"k_");CHKERRQ(ierr);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)pack,"pack_");CHKERRQ(ierr);

  ierr = DMCreateGlobalVector(pack,&X);CHKERRQ(ierr);
  ierr = VecDuplicate(X,&F);CHKERRQ(ierr);

  ierr = PetscNew(struct _UserCtx,&user);CHKERRQ(ierr);
  user->pack = pack;
  ierr = DMCompositeGetLocalVectors(pack,&user->Uloc,&user->Kloc);CHKERRQ(ierr);
  ierr = DMCompositeScatter(pack,X,user->Uloc,user->Kloc);CHKERRQ(ierr);

  ierr = PetscOptionsBegin(PETSC_COMM_WORLD,PETSC_NULL,"Coupled problem options","SNES");CHKERRQ(ierr);
  {
    user->ptype = 0; view_draw = PETSC_FALSE;
    ierr = PetscOptionsInt("-problem_type","0: solve for u only, 1: solve for k only, 2: solve for both",0,user->ptype,&user->ptype,0);CHKERRQ(ierr);
    ierr = PetscOptionsBool("-view_draw","Draw the final coupled solution regardless of whether only one physics was solved",0,view_draw,&view_draw,0);CHKERRQ(ierr);
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);

  ierr = FormInitial_Coupled(user,X);CHKERRQ(ierr);

  ierr = SNESCreate(PETSC_COMM_WORLD,&snes);CHKERRQ(ierr);
  switch (user->ptype) {
  case 0:
    ierr = DMCompositeGetAccess(pack,X,&Xu,0);CHKERRQ(ierr);
    ierr = DMCompositeGetAccess(pack,F,&Fu,0);CHKERRQ(ierr);
    ierr = DMGetMatrix(dau,PETSC_NULL,&B);CHKERRQ(ierr);
    ierr = SNESSetFunction(snes,Fu,FormFunction_All,user);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,B,B,FormJacobian_All,user);CHKERRQ(ierr);
    ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);
    ierr = SNESSolve(snes,PETSC_NULL,Xu);CHKERRQ(ierr);
    ierr = DMCompositeRestoreAccess(pack,X,&Xu,0);CHKERRQ(ierr);
    ierr = DMCompositeRestoreAccess(pack,F,&Fu,0);CHKERRQ(ierr);
    break;
  case 1:
    ierr = DMCompositeGetAccess(pack,X,0,&Xk);CHKERRQ(ierr);
    ierr = DMCompositeGetAccess(pack,F,0,&Fk);CHKERRQ(ierr);
    ierr = DMGetMatrix(dak,PETSC_NULL,&B);CHKERRQ(ierr);
    ierr = SNESSetFunction(snes,Fk,FormFunction_All,user);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,B,B,FormJacobian_All,user);CHKERRQ(ierr);
    ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);
    ierr = SNESSolve(snes,PETSC_NULL,Xk);CHKERRQ(ierr);
    ierr = DMCompositeRestoreAccess(pack,X,0,&Xk);CHKERRQ(ierr);
    ierr = DMCompositeRestoreAccess(pack,F,0,&Fk);CHKERRQ(ierr);
    break;
  case 2:
    ierr = DMGetMatrix(pack,PETSC_NULL,&B);CHKERRQ(ierr);
    ierr = SNESSetFunction(snes,F,FormFunction_All,user);CHKERRQ(ierr);
    ierr = SNESSetJacobian(snes,B,B,FormJacobian_All,user);CHKERRQ(ierr);
    ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);
    ierr = SNESSolve(snes,PETSC_NULL,X);CHKERRQ(ierr);
    break;
  }
  if (view_draw) {ierr = VecView(X,PETSC_VIEWER_DRAW_WORLD);CHKERRQ(ierr);}

  ierr = DMCompositeRestoreLocalVectors(pack,&user->Uloc,&user->Kloc);CHKERRQ(ierr);
  ierr = PetscFree(user);CHKERRQ(ierr);

  ierr = VecDestroy(X);CHKERRQ(ierr);
  ierr = VecDestroy(F);CHKERRQ(ierr);
  ierr = MatDestroy(B);CHKERRQ(ierr);
  ierr = DMDestroy(dau);CHKERRQ(ierr);
  ierr = DMDestroy(dak);CHKERRQ(ierr);
  ierr = DMDestroy(pack);CHKERRQ(ierr);
  ierr = SNESDestroy(snes);CHKERRQ(ierr);
  PetscFinalize();
  return 0;
}