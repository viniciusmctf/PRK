PROGRAM=stencil

include ../../common/make.defs
include ../../common/PRKVERSION

ifndef RADIUS
  RADIUS=2
endif

STARFLAG   = -DSTAR
#DOUBLEFLAG = -DDOUBLE

CXXFLAGS  = $(DEFAULT_OPT_FLAGS)
CXXFLAGS += -DPRKVERSION=$(CPRKVERSION)
CXXFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)
CXXFLAGS += -I..

.PHONY: all clean run

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).cc
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(PROGRAM)
	$(PROGRAM) 10 4000 32

clean:
	-rm -f *.o
	-rm -f *.optrpt
	-rm -f *.dwarf
	-rm -rf *.dSYM
	-rm -f $(PROGRAM)

