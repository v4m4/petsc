#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: random.c,v 1.41 1998/05/18 19:26:21 bsmith Exp bsmith $";
#endif

/*
    This file contains routines for interfacing to random number generators.
    This provides more than just an interface to some system random number
    generator:

    Numbers can be shuffled for use as random tuples

    Multiple random number generators may be used

    We're still not sure what interface we want here.  There should be
    one to reinitialize and set the seed.
 */

#include "petsc.h"
#include "sys.h"        /*I "sys.h" I*/
#include <stdlib.h>

/* Private data */
struct _p_PetscRandom {
  PETSCHEADER(int)
  unsigned long seed;
  Scalar        low, width;       /* lower bound and width of the interval over
                                     which the random numbers are distributed */
  PetscTruth    iset;             /* if true, indicates that the user has set the interval */
  /* array for shuffling ??? */
};


#undef __FUNC__  
#define __FUNC__ "PetscRandomDestroy" 
/*@C
   PetscRandomDestroy - Destroys a context that has been formed by 
   PetscRandomCreate().

   Collective on PetscRandom

   Intput Parameter:
.  r  - the random number generator context

.keywords: random, destroy

.seealso: PetscRandomGetValue(), PetscRandomCreate(), VecSetRandom()
@*/
int PetscRandomDestroy(PetscRandom r)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSCRANDOM_COOKIE);
  if (--r->refct > 0) PetscFunctionReturn(0);

  PLogObjectDestroy((PetscObject)r);
  PetscHeaderDestroy((PetscObject)r);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscRandomSetInterval"
/*@C
   PetscRandomSetInterval - Sets the interval over which the random numbers
   will be randomly distributed.  By default, this interval is [0,1).

   Collective on PetscRandom

   Input Parameters:
.  r  - the random number generator context

   Example of Usage:
.vb
      PetscRandomCreate(PETSC_COMM_WORLD,RANDOM_DEFAULT,&r);
      PetscRandomSetInterval(RANDOM_DEFAULT,&r);
      PetscRandomGetValue(r,&value1);
      PetscRandomGetValue(r,&value2);
      PetscRandomDestroy(r);
.ve

.keywords: random, set, interval

.seealso: PetscRandomCreate()
@*/
int PetscRandomSetInterval(PetscRandom r,Scalar low,Scalar high)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSCRANDOM_COOKIE);
#if defined(USE_PETSC_COMPLEX)
  if (PetscReal(low) >= PetscReal(high))           SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"only low < high");
  if (PetscImaginary(low) >= PetscImaginary(high)) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"only low < high");
#else
  if (low >= high) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"only low < high");
#endif
  r->low   = low;
  r->width = high-low;
  r->iset  = PETSC_TRUE;
  PetscFunctionReturn(0);
}

/*
   For now we've set up using the DRAND48() generater. We need to deal 
   with other variants of random number generators. We should also add
   a routine to enable restarts [seed48()] 
*/
#if defined(HAVE_DRAND48)
EXTERN_C_BEGIN
extern double drand48();
extern void   srand48(long);
EXTERN_C_END

#undef __FUNC__  
#define __FUNC__ "PetscRandomCreate" 
/*@C
   PetscRandomCreate - Creates a context for generating random numbers,
   and initializes the random-number generator.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
-  type - the type of random numbers to be generated, usually RANDOM_DEFAULT

   Output Parameter:
.  r  - the random number generator context

   Notes:
   By default, we generate random numbers via srand48()/drand48() that
   are uniformly distributed over [0,1).  The user can shift and stretch
   this interval by calling PetscRandomSetInterval().
  
   Currently three types of random numbers are supported. These types
   are equivalent when working with real numbers.
.     RANDOM_DEFAULT - both real and imaginary components are random
.     RANDOM_DEFAULT_REAL - real component is random; imaginary component is 0
.     RANDOM_DEFAULT_IMAGINARY - imaginary component is random; real component is 0

   Use VecSetRandom() to set the elements of a vector to random numbers.

   Example of Usage:
.vb
      PetscRandomCreate(PETSC_COMM_SELF,RANDOM_DEFAULT,&r);
      PetscRandomGetValue(r,&value1);
      PetscRandomGetValue(r,&value2);
      PetscRandomGetValue(r,&value3);
      PetscRandomDestroy(r);
.ve

.keywords: random, create

.seealso: PetscRandomGetValue(), PetscRandomSetInterval(), PetscRandomDestroy(), VecSetRandom()
@*/
int PetscRandomCreate(MPI_Comm comm,PetscRandomType type,PetscRandom *r)
{
  PetscRandom rr;
  int      rank;

  PetscFunctionBegin;
  *r = 0;
  if (type != RANDOM_DEFAULT && type != RANDOM_DEFAULT_REAL && type != RANDOM_DEFAULT_IMAGINARY){
    SETERRQ(PETSC_ERR_SUP,0,"Not for this random number type");
  }
  PetscHeaderCreate(rr,_p_PetscRandom,int,PETSCRANDOM_COOKIE,type,comm,PetscRandomDestroy,0);
  PLogObjectCreate(rr);
  rr->low   = 0.0;
  rr->width = 1.0;
  rr->iset  = PETSC_FALSE;
  rr->seed  = 0;
  MPI_Comm_rank(comm,&rank);
  srand48(0x12345678+rank);
  *r = rr;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscRandomGetValue"
/*@C
   PetscRandomGetValue - Generates a random number.  Call this after first calling
   PetscRandomCreate().

   Not Collective

   Intput Parameter:
.  r  - the random number generator context

   Notes:
   Use VecSetRandom() to set the elements of a vector to random numbers.

   Example of Usage:
.vb
      PetscRandomCreate(PETSC_COMM_WORLD,RANDOM_DEFAULT,&r);
      PetscRandomGetValue(r,&value1);
      PetscRandomGetValue(r,&value2);
      PetscRandomGetValue(r,&value3);
      PetscRandomDestroy(r);
.ve

.keywords: random, get, value

.seealso: PetscRandomCreate(), PetscRandomDestroy(), VecSetRandom()
@*/
int PetscRandomGetValue(PetscRandom r,Scalar *val)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSCRANDOM_COOKIE);
#if defined(USE_PETSC_COMPLEX)
  if (r->type == RANDOM_DEFAULT) {
    if (r->iset == PETSC_TRUE) {
         *val = PetscReal(r->width)*drand48() + PetscReal(r->low) +
                (PetscImaginary(r->width)*drand48() + PetscImaginary(r->low)) * PETSC_i;
    }
    else *val = drand48() + drand48()*PETSC_i;
  } else if (r->type == RANDOM_DEFAULT_REAL) {
    if (r->iset == PETSC_TRUE) *val = PetscReal(r->width)*drand48() + PetscReal(r->low);
    else                       *val = drand48();
  } else if (r->type == RANDOM_DEFAULT_IMAGINARY) {
    if (r->iset == PETSC_TRUE) *val = (PetscImaginary(r->width)*drand48()+PetscImaginary(r->low))*PETSC_i;
    else                       *val = drand48()*PETSC_i;
  } else {
    SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Invalid random number type");
  }
#else
  if (r->iset == PETSC_TRUE) *val = r->width * drand48() + r->low;
  else                       *val = drand48();
#endif
  PetscFunctionReturn(0);
}

#elif defined(HAVE_RAND)

#undef __FUNC__  
#define __FUNC__ "PetscRandomCreate" 
int PetscRandomCreate(MPI_Comm comm,PetscRandomType type,PetscRandom *r)
{
  PetscRandom rr;
  int      rank;

  PetscFunctionBegin;
  PLogInfo(0,"PetscRandomCreate: using rand(). not as efficinet as dran48\n");
  *r = 0;
  if (type != RANDOM_DEFAULT && type != RANDOM_DEFAULT_REAL && type != RANDOM_DEFAULT_IMAGINARY) {
    SETERRQ(PETSC_ERR_SUP,0,"Not for this random number type");
  }
  PetscHeaderCreate(rr,_p_PetscRandom,int,PETSCRANDOM_COOKIE,type,comm,PetscRandomDestroy,0);
  PLogObjectCreate(rr);
  rr->low   = 0.0;
  rr->width = 1.0;
  rr->iset  = PETSC_FALSE;
  rr->seed  = 0;
  MPI_Comm_rank(comm,&rank);
  srand(0x12345678+rank);
  *r = rr;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscRandomGetValue"
int PetscRandomGetValue(PetscRandom r,Scalar *val)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSCRANDOM_COOKIE);
#if defined(USE_PETSC_COMPLEX)
  if (r->type == RANDOM_DEFAULT) {
    if (r->iset == PETSC_TRUE)
         *val = PetscReal(r->width)*rand()/(double)RAND_MAX + PetscReal(r->low) +
                (PetscImaginary(r->width)*rand()/(double)RAND_MAX + PetscImaginary(r->low)) * PETSC_i;
    else *val = rand()/(double)RAND_MAX + rand()/(double)RAND_MAX*PETSC_i;
  } else if (r->type == RANDOM_DEFAULT_REAL) {
    if (r->iset == PETSC_TRUE) *val = PetscReal(r->width)*rand()/(double)RAND_MAX + PetscReal(r->low);
    else                       *val = rand()/(double)RAND_MAX;
  } else if (r->type == RANDOM_DEFAULT_IMAGINARY) {
    if (r->iset == PETSC_TRUE) *val = (PetscImaginary(r->width)*rand()/(double)RAND_MAX+PetscImaginary(r->low))*PETSC_i;
    else                       *val = rand()/(double)RAND_MAX*PETSC_i;
  } else SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Invalid random number type");
#else
  if (r->iset == PETSC_TRUE) *val = r->width * rand()/(double)RAND_MAX + r->low;
  else                       *val = rand()/(double)RAND_MAX;
#endif
  PetscFunctionReturn(0);
}

#else
/* Should put a simple, portable random number generator here? */

extern double drand48();

#undef __FUNC__  
#define __FUNC__ "PetscRandomCreate" 
int PetscRandomCreate(MPI_Comm comm,PetscRandomType type,PetscRandom *r)
{
  PetscRandom rr;
  char   arch[10];

  PetscFunctionBegin;
  *r = 0;
  if (type != RANDOM_DEFAULT) SETERRQ(PETSC_ERR_SUP,0,"Not for this random number type");
  PetscHeaderCreate(rr,_p_PetscRandom,int,PETSCRANDOM_COOKIE,type,comm,PetscRandomDestroy,0);
  PLogObjectCreate(rr);
  *r = rr;
  PetscGetArchType(arch,10);
  PetscPrintf(comm,"PetscRandomCreate: Warning: Random number generator not set for machine %s; using fake random numbers.\n",arch);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscRandomGetValue"
int PetscRandomGetValue(PetscRandom r,Scalar *val)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSCRANDOM_COOKIE);
#if defined(USE_PETSC_COMPLEX)
  *val = (0.5,0.5);
#else
  *val = 0.5;
#endif
  PetscFunctionReturn(0);
}
#endif


