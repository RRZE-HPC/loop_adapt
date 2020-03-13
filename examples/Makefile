
all: simple_omp

simple_omp: simple_omp.c
	$(CXX) -DLOOP_ADAPT_ACTIVATE -fopenmp -L.. -I../include -I${HOME}/Apps/hwloc/include simple_omp.c -o simple_omp -lloop_adapt
	@echo "export LD_LIBRARY_PATH=$$(pwd)/..:${LD_LIBRARY_PATH}"

clean:
	rm -f simple_omp

.PHONY: clean simple_omp
