include ../config.mk

LIBDIRS = -L .. -L $(HWLOC_LIBDIR) -L $(LIKWID_LIBDIR)
INCDIRS = -I ../include -I $(HWLOC_INCDIR) -I $(LIKWID_INCDIR)

all: simple_omp simple_omp_parallel

simple_omp: simple_omp.c
	$(CXX) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) simple_omp.c -o simple_omp -lloop_adapt -llikwid
	@echo "export LD_LIBRARY_PATH=$$(pwd)/..:${LD_LIBRARY_PATH}"

simple_omp_parallel: simple_omp_parallel.c
	$(CXX) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) simple_omp_parallel.c -o simple_omp_parallel -lloop_adapt -llikwid
clean:
	rm -f simple_omp simple_omp_parallel

.PHONY: clean simple_omp simple_omp_parallel
