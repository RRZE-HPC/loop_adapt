

CFLAGS   = -g -O2 -std=gnu99 -Wno-format -fPIC -fopenmp  -fsanitize=address -fsanitize=leak
INCLUDES = -I..
LIBPATH = -L..
LIBS =  -lloop_adapt

all: jacobi-2D-5pt-block

jacobi-2D-5pt-block: jacobi-2D-5pt-block.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBPATH) jacobi-2D-5pt-block.c -o $@ -lasan $(LIBS)

clean:
	rm -f jacobi-2D-5pt-block
