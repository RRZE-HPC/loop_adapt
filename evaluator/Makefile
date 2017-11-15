
CFLAGS   = -g -O2 -Wno-format -fPIC -fsanitize=address -fsanitize=leak
SHARED_CFLAGS = -fPIC
SHARED_LFLAGS = -shared -L$(LIKWID_LIBDIR)
DYNAMIC_TARGET_LIB = libevaluator.so

all: $(DYNAMIC_TARGET_LIB)

$(DYNAMIC_TARGET_LIB):
	g++ -fPIC -I. -L. *.cpp -c
	g++ -fPIC -I. -L. interface.c -c
	g++ $(SHARED_CFLAGS) $(SHARED_LFLAGS) -I. -L. *.o -o $(DYNAMIC_TARGET_LIB)

test_parser: $(DYNAMIC_TARGET_LIB)
	g++ $(CFLAGS) -I. -L. test_parser.c -o test_parser -levaluator

clean:
	@rm -rf *.o test_parser $(DYNAMIC_TARGET_LIB)

.PHONY: clean
