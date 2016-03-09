include ../common/make.defs

ifneq ($(GRAPPATOP),)
include $(GRAPPATOP)/share/Grappa/grappa.mk
endif

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

CXXFLAGS  = $(DEFAULT_OPT_FLAGS)
CXXFLAGS += -std=c++11
CXXFLAGS += -DPRKVERSION=$(CPRKVERSION)
CXXFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)
CXXFLAGS += -DRESTRICT=$(RESTRICTFLAG)

CXX11_PROGRAMS = p2p-cxx11 stencil-cxx11 transpose-cxx11
GRAPPA_PROGRAMS = p2p-grappa stencil-grappa transpose-grappa

.PHONY: all clean allcxx allgrappa

all: allcxx11 allgrappa

allcxx11: $(CXX11_PROGRAMS)
allgrappa: $(GRAPPA_PROGRAMS)

stencil-cxx11: stencil-cxx11.cc loop_body_star.incl loop_body_compact.incl
	$(CXX) $(CXXFLAGS) $< -o $@

stencil-grappa: stencil-grappa.cpp loop_body_star.incl loop_body_compact.incl
	$(CXX) $(CXXFLAGS) $< -o $@

%-cxx11: %-cxx11.cc
	$(CXX) $(CXXFLAGS) $< -o $@

%-grappa: %-grappa.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

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
	-rm -f $(GRAPPA_PROGRAMS)

veryclean: clean
	-rm -f func.c
	-rm -f loop_body_star.incl loop_body_compact.incl
	-rm -f ___*.c

