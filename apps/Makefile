include ../config.mk

LIBDIRS = -L .. -L ${HWLOC_LIBDIR} -L ${LIKWID_LIBDIR}
INCDIRS = -I ../include -I ${HWLOC_INCDIR} -I ${LIKWID_INCDIR}
LIBS = -lloop_adapt

all: loop_adapt_avail


loop_adapt_avail: loop_adapt_avail.c
	$(CC) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) loop_adapt_avail.c -o loop_adapt_avail $(LIBS)
clean:
	rm -f loop_adapt_avail

.PHONY: clean loop_adapt_avail
