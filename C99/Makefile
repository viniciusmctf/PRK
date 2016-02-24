include ../common/UPC.defs
include ../common/PRKVERSION

ifndef RADIUS
  RADIUS=2
endif

STARFLAG   = -DSTAR
DOUBLEFLAG = -DDOUBLE

CFLAGS  = $(DEFAULT_OPT_FLAGS)
CFLAGS += -DPRKVERSION=$(CPRKVERSION)
CFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)

UPCFLAGS = $(CFLAGS)

.PHONY: all clean allser allomp allupc

all: allser allomp allupc

allser: p2p stencil transpose # transpose-1d

allomp: stencil-omp transpose-omp # p2p-omp

allupc: p2p-upc stencil-upc transpose-upc

stencil: stencil.c
	$(CC) $(CFLAGS) $< -o $@

p2p: p2p.c
	$(CC) $(CFLAGS) $< -o $@

transpose: transpose.c
	$(CC) $(CFLAGS) $< -o $@

stencil-omp: stencil.c
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

p2p-omp: p2p.c
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

transpose-omp: transpose.c
	$(CC) $(CFLAGS) $(OPENMPFLAG) $< -o $@

stencil-upc: stencil.upc
	$(UPCC) $(UPCFLAGS) $< -o $@

p2p-upc: p2p.upc
	$(UPCC) $(UPCFLAGS) $< -o $@

transpose-upc: transpose.upc
	$(UPCC) $(UPCFLAGS) $< -o $@

extra:
	make PRK_LOOP_INDEX_TYPE=int32_t transpose-omp-int32_t
	make PRK_LOOP_INDEX_TYPE=uint32_t transpose-omp-uint32_t
	make PRK_LOOP_INDEX_TYPE=int64_t transpose-omp-int64_t
	make PRK_LOOP_INDEX_TYPE=uint64_t transpose-omp-uint64_t

transpose-omp-$(PRK_LOOP_INDEX_TYPE): transpose.c
	$(CC) $(CFLAGS) $(OPENMPFLAG) -DPRK_LOOP_INDEX_TYPE=$(PRK_LOOP_INDEX_TYPE) $< -o $@

clean:
	-rm -f *.o
	-rm -f *.optrpt
	-rm -f *.dwarf
	-rm -rf *.dSYM
	-rm -f p2p
	-rm -f stencil
	-rm -f transpose
	-rm -f p2p-omp
	-rm -f stencil-omp
	-rm -f transpose-omp
	-rm -f p2p-upc
	-rm -f stencil-upc
	-rm -f transpose-upc
	-rm -f transpose-1d
	-rm -f transpose-omp-*int*_t

