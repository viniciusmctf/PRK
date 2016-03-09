#Copyright (c) 2013, Intel Corporation
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions
#are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products
#      derived from this software without specific prior written
#      permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#POSSIBILITY OF SUCH DAMAGE.
#
# ******************************************************************

ifndef number_of_functions
  number_of_functions=40
endif

ifndef matrix_rank
  matrix_rank=5
endif

ifndef PRK_FLAGS
  PRK_FLAGS=-O3
endif

default: allserial allopenmp allmpi

help:
	@echo "Usage: \"make all\"          (re-)builds all targets"
	@echo "       \"make allserial\"    (re-)builds all serial targets"
	@echo "       \"make allopenmp\"    (re-)builds all OpenMP targets"
	@echo "       \"make allmpi1\"      (re-)builds all conventional MPI targets"
	@echo "       \"make allfgmpi\"     (re-)builds all Fine-Grain MPI targets"
	@echo "       \"make allmpiopenmp\" (re-)builds all MPI + OpenMP targets"
	@echo "       \"make allmpiomp\"    (re-)builds all MPI + OpenMP targets"
	@echo "       \"make allmpishm\"    (re-)builds all MPI+MPI aka MPI+Shm targets"
	@echo "       \"make allmpirma\"    (re-)builds all MPI-3 RMA targets"
	@echo "       \"make allmpi\"       (re-)builds all MPI targets"
	@echo "       \"make allshmem\"     (re-)builds all SHMEM targets"
	@echo "       \"make allmpishm\"    (re-)builds all MPI-3 shared memory segments targets"
	@echo "       \"make allupc\"       (re-)builds all UPC targets"
	@echo "       \"make allpgas\"      (re-)builds all PGAS (UPC, SHMEM, MPI-3 RMA) targets"
	@echo "       \"make alldarwin\"    (re-)builds all of the above targets"
	@echo "       \"make allcharm++\"   (re-)builds all Charm++ targets"
	@echo "       \"make allampi\"      (re-)builds all Adaptive MPI targets"
	@echo "       \"make allgrappa\"    (re-)builds all Grappa targets"
	@echo "       \"make allfortran\"   (re-)builds all Fortran targets"
	@echo "       \"make allfreaks\"    (re-)builds the above two targets"
	@echo "       optionally, specify   \"matrix_rank=<n> number_of_functions=<m>\""
	@echo "       optionally, specify   \"default_opt_flags=<list of optimization flags>\""
	@echo "       \"make clean\"        removes all objects and executables"
	@echo "       \"make veryclean\"    removes some generated source files as well"

all: alldarwin allfreaks

alldarwin: allserial allopenmp allmpi1 allampi allfgmpi allmpiopenmp allmpirma allshmem allmpishm allupc allfortran allc99

allfreaks: allcharm++ allgrappa

allmpi1:
	make -C C89 allmpi1 MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allampi:
	make -C C89 allampi MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allfgmpi:
	make -C C89 allfgmpi MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)
	cd scripts/small; $(MAKE) -f  Makefile_FG_MPI runfgmpi
	cd scripts/wide;  $(MAKE) -f  Makefile_FG_MPI runfgmpi

allmpiopenmp:
	make -C C89 allmpiomp MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allmpiomp: allmpiopenmp

allmpirma:
	make -C C89 allmpirma MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allshmem:
	make -C C89 allshmem MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allmpishm:
	make -C C89 allmpishm MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allmpi: allmpi1 allmpiomp allmpirma allmpishm

allupc:
	make -C C99 allupc

allpgas: allshmem allupc allmpirma

allopenmp:
	make -C C89 allomp MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allcharm++:
	make -C CHARM++ all

allgrappa:
	make -C Cxx11 allgrappa

allc99:
	make -C C99 all

allserial:
	make -C C89 allser MATRIX_RANK=$(matrix_rank) NUMBER_OF_FUNCTIONS=$(number_of_functions)

allfortran:
	make -C FORTRAN all

allfortranserial:
	make -C FORTRAN serial

allfortranopenmp:
	make -C FORTRAN openmp

allfortrancoarray:
	make -C FORTRAN coarray

allfortranpretty:
	make -C FORTRAN pretty

clean:
	make -C C89 clean
	make -C C99 clean
	make -C FORTRAN clean
	make -C CHARM++ clean
	make -C Cxx11 clean
	rm -f stats.json

veryclean: clean
	make -C C89 veryclean
	cd scripts/small;           $(MAKE) -f  Makefile_FG_MPI veryclean
	cd scripts/wide;            $(MAKE) -f  Makefile_FG_MPI veryclean
	#rm -f common/make.defs
