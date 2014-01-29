#include "petsc-private/fortranimpl.h"
#include "tao-private/taosolver_impl.h"


#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define taosetobjectiveroutine_             TAOSETOBJECTIVEROUTINE
#define taosetgradientroutine_              TAOSETGRADIENTROUTINE
#define taosetobjectiveandgradientroutine_  TAOSETOBJECTIVEANDGRADIENTROUTINE
#define taosethessianroutine_               TAOSETHESSIANROUTINE
#define taosetseparableobjectiveroutine_    TAOSETSEPARABLEOBJECTIVEROUTINE
#define taosetjacobianroutine_              TAOSETJACOBIANROUTINE
#define taosetjacobianstateroutine_         TAOSETJACOBIANSTATEROUTINE
#define taosetjacobiandesignroutine_        TAOSETJACOBIANDESIGNROUTINE
#define taosetjacobianinequalityroutine_    TAOSETJACOBIANINEQUALITYROUTINE
#define taosetjacobianequalityroutine_      TAOSETJACOBIANEQUALITYROUTINE
#define taosetinequalityconstraintsroutine_ TAOSETINEQUALITYCONSTRAINTSROUTINE
#define taosetequalityconstraintsroutine_   TAOSETEQUALITYCONSTRAINTSROUTINE
#define taosetvariableboundsroutine_        TAOSETVARIABLEBOUNDSROUTINE
#define taosetconstraintsroutine_           TAOSETCONSTRAINTSROUTINE
#define taosetmonitor_                      TAOSETMONITOR
#define taosettype_                         TAOSETTYPE
#define taoview_                            TAOVIEW
#define taogethistory_                      TAOGETHISTORY
#define taosetconvergencetest_              TAOSETCONVERGENCETEST
#define taogetoptionsprefix_                TAOGETOPTIONSPREFIX
#define taosetoptionsprefix_                TAOSETOPTIONSPREFIX
#define taoappendoptionsprefix_             TAOAPPENDOPTIONSPREFIX
#define taogettype_                         TAOGETTYPE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)

#define taosetobjectiveroutine_             taosetobjectiveroutine
#define taosetgradientroutine_              taosetgradientroutine
#define taosetobjectiveandgradientroutine_  taosetobjectiveandgradientroutine
#define taosethessianroutine_               taosethessianroutine
#define taosetseparableobjectiveroutine_    taosetseparableobjectiveroutine
#define taosetjacobianroutine_              taosetjacobianroutine
#define taosetjacobianstateroutine_         taosetjacobianstateroutine
#define taosetjacobiandesignroutine_        taosetjacobiandesignroutine
#define taosetjacobianinequalityroutine_    taosetjacobianinequalityroutine
#define taosetjacobianequalityroutine_      taosetjacobianequalityroutine
#define taosetinequalityconstraintsroutine_ taosetinequalityconstraintsroutine
#define taosetequalityconstraintsroutine_   taosetequalityconstraintsroutine
#define taosetvariableboundsroutine_        taosetvariableboundsroutine
#define taosetconstraintsroutine_           taosetconstraintsroutine
#define taosetmonitor_                      taosetmonitor
#define taosettype_                         taosettype
#define taoview_                            taoview
#define taogethistory_                      taogethistory
#define taosetconvergencetest_              taosetconvergencetest
#define taogetoptionsprefix_                taogetoptionsprefix
#define taosetoptionsprefix_                taosetoptionsprefix
#define taoappendoptionsprefix_             taoappendoptionsprefix
#define taogettype_                         taogettype
#endif

static int OBJ=0;       /*  objective routine index */
static int GRAD=1;      /*  gradient routine index */
static int OBJGRAD=2;   /*  objective and gradient routine */
static int HESS=3;      /*  hessian routine index */
static int SEPOBJ=4;    /*  separable objective routine index */
static int JAC=5;       /*  jacobian routine index */
static int JACSTATE=6;  /*  jacobian state routine index */
static int JACDESIGN=7; /*  jacobian design routine index */
static int BOUNDS=8;
static int MON=9;       /*  monitor routine index */
static int MONCTX=10;       /*  monitor routine index */
static int MONDESTROY=11; /*  monitor destroy index */
static int CONVTEST=12;  /*  */
static int CONSTRAINTS=13;
static int JACINEQ=14;
static int JACEQ=15;
static int CONINEQ=16;
static int CONEQ=17;
static int NFUNCS=18;

static PetscErrorCode ourtaoobjectiveroutine(TaoSolver tao, Vec x, PetscReal *f, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,PetscReal*,void*,PetscErrorCode*))
        (((PetscObject)tao)->fortran_func_pointers[OBJ]))(&tao,&x,f,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaogradientroutine(TaoSolver tao, Vec x, Vec g, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
       (((PetscObject)tao)->fortran_func_pointers[GRAD]))(&tao,&x,&g,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
    
}

static PetscErrorCode ourtaoobjectiveandgradientroutine(TaoSolver tao, Vec x, PetscReal *f, Vec g, void* ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,PetscReal*,Vec*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[OBJGRAD]))(&tao,&x,f,&g,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaohessianroutine(TaoSolver tao, Vec x, Mat *H, Mat *Hpre, MatStructure *type, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,Mat*,MatStructure*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[HESS]))(&tao,&x,H,Hpre,type,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaojacobianroutine(TaoSolver tao, Vec x, Mat *H, Mat *Hpre, MatStructure *type, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,Mat*,MatStructure*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[JAC]))(&tao,&x,H,Hpre,type,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaojacobianstateroutine(TaoSolver tao, Vec x, Mat *H, Mat *Hpre, Mat *Hinv, MatStructure *type, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,Mat*,Mat*,MatStructure*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[JACSTATE]))(&tao,&x,H,Hpre,Hinv,type,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaojacobiandesignroutine(TaoSolver tao, Vec x, Mat *H, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[JACDESIGN]))(&tao,&x,H,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaoboundsroutine(TaoSolver tao, Vec xl, Vec xu, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[BOUNDS]))(&tao,&xl,&xu,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}
static PetscErrorCode ourtaoseparableobjectiveroutine(TaoSolver tao, Vec x, Vec f, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[SEPOBJ]))(&tao,&x,&f,ctx,&ierr);
    return 0;
}

static PetscErrorCode ourtaomonitor(TaoSolver tao, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver *, void*, PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[MON]))(&tao,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaomondestroy(void **ctx) 
{
    PetscErrorCode ierr = 0;
    TaoSolver tao = *(TaoSolver*)ctx;
    (*(void (PETSC_STDCALL *)(void*,PetscErrorCode*))(((PetscObject)tao)->fortran_func_pointers[MONDESTROY]))
      ((void*)(PETSC_UINTPTR_T)((PetscObject)tao)->fortran_func_pointers[MONCTX],&ierr);
    CHKERRQ(ierr);
    return 0;
}
static PetscErrorCode ourtaoconvergencetest(TaoSolver tao, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver *, void*, PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[CONVTEST]))(&tao,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
}


static PetscErrorCode ourtaoconstraintsroutine(TaoSolver tao, Vec x, Vec c, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
       (((PetscObject)tao)->fortran_func_pointers[CONSTRAINTS]))(&tao,&x,&c,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
    
}

static PetscErrorCode ourtaojacobianinequalityroutine(TaoSolver tao, Vec x, Mat *J, Mat *Jpre, MatStructure *type, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,Mat*,MatStructure*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[JACINEQ]))(&tao,&x,J,Jpre,type,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaojacobianequalityroutine(TaoSolver tao, Vec x, Mat *J, Mat *Jpre, MatStructure *type, void *ctx) 
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Mat*,Mat*,MatStructure*,void*,PetscErrorCode*))
     (((PetscObject)tao)->fortran_func_pointers[JACEQ]))(&tao,&x,J,Jpre,type,ctx,&ierr); CHKERRQ(ierr);
    return 0;
}

static PetscErrorCode ourtaoinequalityconstraintsroutine(TaoSolver tao, Vec x, Vec c, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
       (((PetscObject)tao)->fortran_func_pointers[CONINEQ]))(&tao,&x,&c,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
    
}

static PetscErrorCode ourtaoequalityconstraintsroutine(TaoSolver tao, Vec x, Vec c, void *ctx)
{
    PetscErrorCode ierr = 0;
    (*(void (PETSC_STDCALL *)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*))
       (((PetscObject)tao)->fortran_func_pointers[CONEQ]))(&tao,&x,&c,ctx,&ierr);
    CHKERRQ(ierr);
    return 0;
    
}


EXTERN_C_BEGIN


void PETSC_STDCALL taosetobjectiveroutine_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, PetscReal *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetObjectiveRoutine(*tao,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[OBJ] = (PetscVoidFunction)func;
        *ierr = TaoSetObjectiveRoutine(*tao, ourtaoobjectiveroutine,ctx);
    }
}

void PETSC_STDCALL taosetgradientroutine_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetGradientRoutine(*tao,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[GRAD] = (PetscVoidFunction)func;
        *ierr = TaoSetGradientRoutine(*tao, ourtaogradientroutine,ctx);
    }
}

void PETSC_STDCALL taosetobjectiveandgradientroutine_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, PetscReal *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetObjectiveAndGradientRoutine(*tao,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[OBJGRAD] = (PetscVoidFunction)func;
        *ierr = TaoSetObjectiveAndGradientRoutine(*tao, ourtaoobjectiveandgradientroutine,ctx);
    }
}




void PETSC_STDCALL taosetseparableobjectiveroutine_(TaoSolver *tao, Vec *F, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetSeparableObjectiveRoutine(*tao,*F,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[SEPOBJ] = (PetscVoidFunction)func;
        *ierr = TaoSetSeparableObjectiveRoutine(*tao,*F, ourtaoseparableobjectiveroutine,ctx);
    }
}



void PETSC_STDCALL taosetjacobianroutine_(TaoSolver *tao, Mat *J, Mat *Jp, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, Mat *, MatStructure *,void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetJacobianRoutine(*tao,*J,*Jp,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[JAC] = (PetscVoidFunction)func;
        *ierr = TaoSetJacobianRoutine(*tao,*J, *Jp, ourtaojacobianroutine,ctx);
    }
}

void PETSC_STDCALL taosetjacobianstateroutine_(TaoSolver *tao, Mat *J, Mat *Jp, Mat*Jinv, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, Mat *, Mat*, MatStructure *,void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
      *ierr = TaoSetJacobianStateRoutine(*tao,*J,*Jp,*Jinv,0,ctx);
    } else {
      ((PetscObject)*tao)->fortran_func_pointers[JACSTATE] = (PetscVoidFunction)func;
      *ierr = TaoSetJacobianStateRoutine(*tao,*J, *Jp, *Jinv, ourtaojacobianstateroutine,ctx);
    }
}

void PETSC_STDCALL taosetjacobiandesignroutine_(TaoSolver *tao, Mat *J, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetJacobianDesignRoutine(*tao,*J,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[JACDESIGN] = (PetscVoidFunction)func;
        *ierr = TaoSetJacobianDesignRoutine(*tao,*J, ourtaojacobiandesignroutine,ctx);
    }
}


void PETSC_STDCALL taosethessianroutine_(TaoSolver *tao, Mat *J, Mat *Jp, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, Mat *, MatStructure *,void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetHessianRoutine(*tao,*J,*Jp,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[HESS] = (PetscVoidFunction)func;
        *ierr = TaoSetHessianRoutine(*tao,*J, *Jp, ourtaohessianroutine,ctx);
    }
}

void PETSC_STDCALL taosetvariableboundsroutine_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*,Vec*,Vec*,void*,PetscErrorCode*),void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (func) {
        ((PetscObject)*tao)->fortran_func_pointers[BOUNDS] = (PetscVoidFunction)func;
        *ierr = TaoSetVariableBoundsRoutine(*tao,ourtaoboundsroutine,ctx);
    } else {
        *ierr = TaoSetVariableBoundsRoutine(*tao,0,ctx); 
    }
    
}    
void PETSC_STDCALL taosetmonitor_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*,void*,PetscErrorCode*),void *ctx, void (PETSC_STDCALL *mondestroy)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (func) {
        ((PetscObject)*tao)->fortran_func_pointers[MON] = (PetscVoidFunction)func;
        if (FORTRANNULLFUNCTION(mondestroy)){
          *ierr = TaoSetMonitor(*tao,ourtaomonitor,*tao,NULL);
        } else {
          *ierr = TaoSetMonitor(*tao,ourtaomonitor,*tao,ourtaomondestroy);
        }
    }
}

void PETSC_STDCALL taosetconvergencetest_(TaoSolver *tao, void (PETSC_STDCALL *func)(TaoSolver*,void*,PetscErrorCode*),void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetConvergenceTest(*tao,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[CONVTEST] = (PetscVoidFunction)func;
        *ierr = TaoSetConvergenceTest(*tao,ourtaoconvergencetest,ctx);
    }
}

        
void PETSC_STDCALL taosetconstraintsroutine_(TaoSolver *tao, Vec *C, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
      *ierr = TaoSetConstraintsRoutine(*tao,*C,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[CONSTRAINTS] = (PetscVoidFunction)func;
        *ierr = TaoSetConstraintsRoutine(*tao, *C, ourtaoconstraintsroutine,ctx);
    }
}
    

void PETSC_STDCALL taosettype_(TaoSolver *tao, CHAR type_name PETSC_MIXED_LEN(len), PetscErrorCode *ierr PETSC_END_LEN(len))

{
    char *t;
    
    FIXCHAR(type_name,len,t);
    *ierr = TaoSetType(*tao,t);
    FREECHAR(type_name,t);
        
}

void PETSC_STDCALL taoview_(TaoSolver *tao, PetscViewer *viewer, PetscErrorCode *ierr)
{
    PetscViewer v;
    PetscPatchDefaultViewers_Fortran(viewer,v);
    *ierr = TaoView(*tao,v);
}

void PETSC_STDCALL taogethistory_(TaoSolver *tao, PetscInt *nhist, PetscErrorCode *ierr) 
{
  *nhist  = (*tao)->hist_len;
  *ierr = 0;
}

void PETSC_STDCALL taogetoptionsprefix_(TaoSolver *tao, CHAR prefix PETSC_MIXED_LEN(len), PetscErrorCode *ierr PETSC_END_LEN(len))
{
  const char *name;
  *ierr = TaoGetOptionsPrefix(*tao,&name);
  *ierr = PetscStrncpy(prefix,name,len); if (*ierr) return;
  FIXRETURNCHAR(PETSC_TRUE,prefix,len);

}

void PETSC_STDCALL taoappendoptionsprefix_(TaoSolver *tao, CHAR prefix PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *name;
  FIXCHAR(prefix,len,name);
  *ierr = TaoAppendOptionsPrefix(*tao,name);
  FREECHAR(prefix,name);
}

void PETSC_STDCALL taosetoptionsprefix_(TaoSolver *tao, CHAR prefix PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;
  FIXCHAR(prefix,len,t);
  *ierr = TaoSetOptionsPrefix(*tao,t);
  FREECHAR(prefix,t);
}

void PETSC_STDCALL taogettype_(TaoSolver *tao, CHAR name PETSC_MIXED_LEN(len), PetscErrorCode *ierr  PETSC_END_LEN(len))
{
  const char *tname;
  *ierr = TaoGetType(*tao,&tname);
  *ierr = PetscStrncpy(name,tname,len); if (*ierr) return;
  FIXRETURNCHAR(PETSC_TRUE,name,len);
  
}


void PETSC_STDCALL taosetjacobianinequalityroutine_(TaoSolver *tao, Mat *J, Mat *Jp, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, Mat *, MatStructure *,void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetJacobianInequalityRoutine(*tao,*J,*Jp,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[JACINEQ] = (PetscVoidFunction)func;
        *ierr = TaoSetJacobianInequalityRoutine(*tao,*J, *Jp, ourtaojacobianinequalityroutine,ctx);
    }
}

void PETSC_STDCALL taosetjacobianequalityroutine_(TaoSolver *tao, Mat *J, Mat *Jp, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Mat *, Mat *, MatStructure *,void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
        *ierr = TaoSetJacobianEqualityRoutine(*tao,*J,*Jp,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[JACEQ] = (PetscVoidFunction)func;
        *ierr = TaoSetJacobianEqualityRoutine(*tao,*J, *Jp, ourtaojacobianequalityroutine,ctx);
    }
}


void PETSC_STDCALL taosetinequalityconstraintsroutine_(TaoSolver *tao, Vec *C, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
      *ierr = TaoSetInequalityConstraintsRoutine(*tao,*C,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[CONINEQ] = (PetscVoidFunction)func;
        *ierr = TaoSetInequalityConstraintsRoutine(*tao, *C, ourtaoinequalityconstraintsroutine,ctx);
    }
}

void PETSC_STDCALL taosetequalityconstraintsroutine_(TaoSolver *tao, Vec *C, void (PETSC_STDCALL *func)(TaoSolver*, Vec *, Vec *, void *, PetscErrorCode *), void *ctx, PetscErrorCode *ierr)
{
    CHKFORTRANNULLOBJECT(ctx);
    PetscObjectAllocateFortranPointers(*tao,NFUNCS);
    if (!func) {
      *ierr = TaoSetEqualityConstraintsRoutine(*tao,*C,0,ctx);
    } else {
        ((PetscObject)*tao)->fortran_func_pointers[CONEQ] = (PetscVoidFunction)func;
        *ierr = TaoSetEqualityConstraintsRoutine(*tao, *C, ourtaoequalityconstraintsroutine,ctx);
    }
}


EXTERN_C_END


