LIBDIRS = -L .. -L ${HOME}/Apps/lib -L /mnt/opt/likwid-5.0.1/lib
INCDIRS = -I ../include -I${HOME}/Apps/hwloc/include -I /mnt/opt/likwid-5.0.1/include

all: simple_omp simple_omp_parallel

simple_omp: simple_omp.c
	$(CXX) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) simple_omp.c -o simple_omp -lloop_adapt -llikwid
	@echo "export LD_LIBRARY_PATH=$$(pwd)/..:${LD_LIBRARY_PATH}"

simple_omp_parallel: simple_omp_parallel.c
	$(CXX) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) simple_omp_parallel.c -o simple_omp_parallel -lloop_adapt -llikwid
clean:
	rm -f simple_omp simple_omp_parallel

.PHONY: clean simple_omp simple_omp_parallel
