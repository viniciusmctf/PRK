include ../common/UPC.defs
include ../common/PRKVERSION

ifndef RADIUS
  RADIUS=2
endif

STARFLAG   = -DSTAR
DOUBLEFLAG = -DDOUBLE

UPCFLAGS  = $(DEFAULT_OPT_FLAGS)
UPCFLAGS += -DPRKVERSION=$(CPRKVERSION)
UPCFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)

.PHONY: all clean

all: 

clean:
	-rm -f *.o
	-rm -f *.optrpt
	-rm -f *.dwarf
	-rm -rf *.dSYM
	-rm -f p2p
	-rm -f p2p-upc
	-rm -f stencil-upc
	-rm -f transpose-upc

