LIBDIRS = -L .. -L ${HOME}/Apps/lib #-L /mnt/opt/likwid-5.0.1/lib
INCDIRS = -I ../include -I${HOME}/Apps/hwloc/include #-I /mnt/opt/likwid-5.0.1/include

all: loop_adapt_avail


loop_adapt_avail: loop_adapt_avail.c
	$(CC) -g -DLOOP_ADAPT_ACTIVATE -fopenmp $(INCDIRS) $(LIBDIRS) loop_adapt_avail.c -o loop_adapt_avail -lloop_adapt
clean:
	rm -f loop_adapt_avail

.PHONY: clean loop_adapt_avail
