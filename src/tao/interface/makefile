ALL: lib 

CFLAGS   =-DTAO_LIB_DIR='"${TAO_LIB_DIR}"'
FFLAGS   =
SOURCEC  = taosolver.c taosolver_fg.c taosolverregi.c taosolver_hj.c taosolver_bounds.c dlregistao.c fdiff.c fdtest.c
SOURCEF  =
SOURCEH  = ../../../include/petsc-private/taoimpl.h ../../../include/petsctao.h
LIBBASE  = libpetsctao
MANSEC   = Tao
LOCDIR   = src/tao/interface/


include ${PETSC_DIR}/conf/variables
include ${PETSC_DIR}/conf/rules
include ${PETSC_DIR}/conf/test
