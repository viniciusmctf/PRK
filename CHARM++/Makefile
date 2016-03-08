include ../common/make.defs
ifeq ($(CHARMTOP),)
    CXX=true
else
    CXX=$(CHARMTOP)/bin/charmc
endif
CCOMPILER=$(CXX)
CITRANSLATOR=$(CXX) -E

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

# These are specific to Charm++
CXXFLAGS += -DCHARMXX -language charm++

CHARM_PROGRAMS = p2p-charm stencil-charm transpose-charm

.PHONY: all clean allcharm

all: allcharm

allcharm: $(CHARM_PROGRAMS)

#random_draw.o: random_draw.c
#	$(CXX) $(CXXFLAGS) -c $< -o $@

#pic-charm: pic-charm.C random_draw.o
#	$(CXX) $(CXXFLAGS) $< random_draw.o -o $@

p2p.decl.h: p2p-charm.ci
	$(CXX) -E $<

stencil.decl.h: stencil-charm.ci
	$(CXX) -E $<

transpose.decl.h: transpose-charm.ci
	$(CXX) -E $<

p2p-charm: p2p-charm.C p2p.decl.h
	$(CXX) $(CXXFLAGS) $< -o $@

stencil-charm: stencil-charm.C stencil.decl.h loop_body_star.incl loop_body_compact.incl
	$(CXX) $(CXXFLAGS) $< -o $@

transpose-charm: transpose-charm.C transpose.decl.h
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
	-rm -f $(CHARM_PROGRAMS)

veryclean: clean
	-rm -f charmrun
	-rm -f func.c
	-rm -f loop_body_star.incl loop_body_compact.incl
	-rm -f *.decl.h
	-rm -f *.def.h
	-rm -f ___*.c

