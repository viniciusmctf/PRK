include ../common/make.defs
include ../common/PRKVERSION

ifndef RADIUS
  RADIUS=2
endif

STARFLAG = -DSTAR

FCFLAGS  = $(DEFAULT_OPT_FLAGS)
FCFLAGS += -DPRKVERSION=$(FPRKVERSION)
FCFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG)

.PHONY: all clean serial openmp coarray pretty

all: serial openmp coarray pretty

serial: stencil transpose p2p
openmp: stencil-omp transpose-omp # p2p-omp
coarray: stencil-coarray transpose-coarray p2p-coarray
pretty: transpose-pretty # stencil-pretty # p2p-pretty

%: %.f90
	$(FC) $(FCFLAGS) $< -o $@

%-omp: %.f90
	$(FC) $(FCFLAGS) $(OPENMPFLAG) $< -o $@

%-coarray: %-coarray.f90
	$(FC) $(FCFLAGS) $< $(COARRAYFLAG) -o $@

%-pretty: %-pretty.f90
	$(FC) $(FCFLAGS) $< -o $@

clean:
	-rm -f *.o
	-rm -f *.i90
	-rm -f *__genmod.f90
	-rm -f *__genmod.mod
	-rm -f *.optrpt
	-rm -f *.dwarf
	-rm -rf *.dSYM
	-rm -f p2p
	-rm -f p2p-omp
	-rm -f p2p-coarray
	#-rm -f p2p-pretty
	-rm -f stencil
	-rm -f stencil-omp
	-rm -f stencil-coarray
	-rm -f stencil-pretty
	-rm -f transpose
	-rm -f transpose-omp
	-rm -f transpose-coarray
	-rm -f transpose-pretty
