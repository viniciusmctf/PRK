include ../common/make.defs

include ../common/PRKVERSION

ifndef NUMBER_OF_FUNCTIONS
  NUMBER_OF_FUNCTIONS=40
endif

ifndef MATRIX_RANK
  MATRIX_RANK=10
endif

ifndef RADIUS
  RADIUS=2
endif

ifndef LOOPGEN
  LOOPGEN=0
endif

STARFLAG     = -DSTAR
DOUBLEFLAG   = -DDOUBLE
RESTRICTFLAG = #restrict

CFLAGS  = $(DEFAULT_OPT_FLAGS)
CFLAGS += -std=c89 -ansi -pedantic
CFLAGS += -DPRKVERSION=$(CPRKVERSION)
CFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)
CFLAGS += -DRESTRICT=$(RESTRICTFLAG)

SERIAL_PROGRAMS = branch-ser dgemm-ser nstream-ser p2p-ser \
                  random-ser reduce-ser sparse-ser \
                  stencil-ser transpose-ser pic-ser
OPENMP_PROGRAMS = branch-omp dgemm-omp nstream-omp p2p-omp \
                  random-omp reduce-omp sparse-omp \
                  stencil-omp transpose-omp \
                  global-omp private-omp shared-omp
SHMEM_PROGRAMS = p2p-shmem stencil-shmem transpose-shmem
MPI1_PROGRAMS = branch-mpi1 dgemm-mpi1 nstream-mpi1 p2p-mpi1 \
                random-mpi1 reduce-mpi1 sparse-mpi1 \
                stencil-mpi1 transpose-mpi1
AMPI_PROGRAMS = branch-ampi dgemm-ampi nstream-ampi p2p-ampi \
                random-ampi reduce-ampi sparse-ampi \
                stencil-ampi transpose-ampi
FGMPI_PROGRAMS = branch-fgmpi dgemm-fgmpi nstream-fgmpi p2p-fgmpi \
                 random-fgmpi reduce-fgmpi sparse-fgmpi \
                 stencil-fgmpi transpose-fgmpi
MPIRMA_PROGRAMS = p2p-mpirma stencil-mpirma transpose-mpirma
MPISHM_PROGRAMS = p2p-mpishm stencil-mpishm transpose-mpishm
MPIOMP_PROGRAMS = p2p-mpiomp stencil-mpiomp transpose-mpiomp

.PHONY: all clean allser allomp allshmem allmpi allmpi1 allmpirma allmpishm allmpiomp allampi allfgmpi

all: allser allomp allshmem allmpi allmpi1 allmpirma allmpishm allmpiomp allampi allfgmpi

allmpi: allmpi1 allmpirma allmpiomp allmpishm allampi allfgmpi

allser: $(SERIAL_PROGRAMS)

allomp: $(OPENMP_PROGRAMS)

allmpiomp: $(MPIOMP_PROGRAMS)

allmpirma: $(MPIRMA_PROGRAMS)

allmpishm: $(MPISHM_PROGRAMS)

allmpi1: $(MPI1_PROGRAMS)

allampi: $(AMPI_PROGRAMS)

allfgmpi: $(FGMPI_PROGRAMS)

allshmem: $(SHMEM_PROGRAMS)

func.o: func.c
	$(CC) $(CFLAGS) -c $< -o $@

random_draw.o: random_draw.c
	$(CC) $(CFLAGS) -c $< -o $@

pic-ser: pic-ser.c random_draw.o
	$(CC) $(CFLAGS) $< random_draw.o -o $@

branch-ser: branch-ser.c func.o
	$(CC) $(CFLAGS) $< func.o -o $@

branch-omp: branch-omp.c func.o
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< func.o -o $@

branch-mpi1: branch-mpi1.c func.o
	$(MPICC) $(CFLAGS) $< func.o -o $@

stencil-ser: stencil-ser.c loop_body_star.incl loop_body_compact.incl
	$(CC) $(CFLAGS) $< -o $@

stencil-omp: stencil-omp.c loop_body_star.incl loop_body_compact.incl
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

stencil-mpi1: stencil-mpi1.c loop_body_star.incl loop_body_compact.incl
	$(MPICC) $(CFLAGS) $< -o $@

stencil-ampi: stencil-ampi.c loop_body_star.incl loop_body_compact.incl
	$(AMPICC) $(CFLAGS) $< -o $@

stencil-fgmpi: stencil-fgmpi.c loop_body_star.incl loop_body_compact.incl
	$(FGMPICC) $(CFLAGS) $< -o $@

stencil-shmem: stencil-shmem.c loop_body_star.incl loop_body_compact.incl
	$(OSHCC) $(CFLAGS) $< -o $@

%-ser: %-ser.c
	$(CC) $(CFLAGS) $< -o $@

%-omp: %-omp.c
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

%-mpi1: %-mpi1.c
	$(MPICC) $(CFLAGS) $< -o $@

%-mpirma: %-mpirma.c
	$(MPICC) $(CFLAGS) $< -o $@

%-mpishm: %-mpishm.c
	$(MPICC) $(CFLAGS) $< -o $@

%-mpiomp: %-mpiomp.c
	$(MPICC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

%-ampi: %-ampi.c
	$(AMPICC) $(CFLAGS) $< -o $@

%-fgmpi: %-fgmpi.c
	$(FGMPICC) $(CFLAGS) $< -o $@

%-shmem: %-shmem.c
	$(OSHCC) $(CFLAGS) $< -o $@

func.c:
	@echo "############################################################"
	@echo "##### No file func.c -- invoking func_gen to create it #####"
	@echo "############################################################"
	./func_gen ${MATRIX_RANK} ${NUMBER_OF_FUNCTIONS}

loop_body_star.incl:
	@echo "#########################################################################"
	@echo "##### No file loop_body_star.incl -- invoking loop_gen to create it #####"
	@echo "#########################################################################"
	./loop_gen $(RADIUS) 1

loop_body_compact.incl:
	@echo "############################################################################"
	@echo "##### No file loop_body_compact.incl -- invoking loop_gen to create it #####"
	@echo "############################################################################"
	./loop_gen $(RADIUS) 0

clean:
	-rm -f *.o
	-rm -f *.optrpt
	-rm -f *.dwarf
	-rm -rf *.dSYM
	-rm -f $(SERIAL_PROGRAMS)
	-rm -f $(OPENMP_PROGRAMS)
	-rm -f $(MPI1_PROGRAMS)
	-rm -f $(MPISHM_PROGRAMS)
	-rm -f $(MPIRMA_PROGRAMS)
	-rm -f $(MPIOMP_PROGRAMS)
	-rm -f $(AMPI_PROGRAMS)
	-rm -f $(FGMPI_PROGRAMS)
	-rm -f $(SHMEM_PROGRAMS)

veryclean: clean
	-rm -f func.c
	-rm -f loop_body_star.incl loop_body_compact.incl
	-rm -f ___*.c

