include ../common/make.defs
include ../common/PRKVERSION

ifndef RADIUS
  RADIUS=2
endif

STARFLAG   = -DSTAR
DOUBLEFLAG = -DDOUBLE

CFLAGS  = $(DEFAULT_OPT_FLAGS)
CFLAGS += -DPRKVERSION=$(CPRKVERSION)
CFLAGS += -DRADIUS=$(RADIUS) $(STARFLAG) $(DOUBLEFLAG)

.PHONY: all clean

all: p2p stencil transpose stencil-omp transpose-omp # p2p-omp # transpose-1d

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
	-rm -f p2p-omp
	-rm -f stencil
	-rm -f stencil-omp
	-rm -f transpose
	-rm -f transpose-omp
	-rm -f transpose-1d
	-rm -f transpose-omp-*int*_t

