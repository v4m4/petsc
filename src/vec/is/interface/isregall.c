#define PETSCVEC_DLL

#include "private/isimpl.h"     /*I  "petscis.h"  I*/
EXTERN_C_BEGIN
EXTERN PetscErrorCode PETSCVEC_DLLEXPORT ISCreate_General(IS);
EXTERN PetscErrorCode PETSCVEC_DLLEXPORT ISCreate_Stride(IS);
EXTERN PetscErrorCode PETSCVEC_DLLEXPORT ISCreate_Block(IS);
EXTERN_C_END

#undef __FUNCT__  
#define __FUNCT__ "ISRegisterAll"
/*@C
  ISRegisterAll - Registers all of the index set components in the IS package.

  Not Collective

  Input parameter:
. path - The dynamic library path

  Level: advanced

.keywords: IS, register, all
.seealso:  ISRegister(), ISRegisterDestroy(), ISRegisterDynamic()
@*/
PetscErrorCode PETSCVEC_DLLEXPORT ISRegisterAll(const char path[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ISRegisterAllCalled = PETSC_TRUE;

  ierr = ISRegisterDynamic(ISGENERAL,     path, "ISCreate_General",    ISCreate_General);CHKERRQ(ierr);
  ierr = ISRegisterDynamic(ISSTRIDE,      path, "ISCreate_Stride",     ISCreate_Stride);CHKERRQ(ierr);
  ierr = ISRegisterDynamic(ISBLOCK,       path, "ISCreate_Block",      ISCreate_Block);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
