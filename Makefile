
include config.mk
LIKWID_INCDIR ?= /usr/local/include
LIKWID_LIBDIR ?= /usr/local/lib
INCLUDES  += -I. -I$(LIKWID_INCDIR)
OBJ       = $(patsubst %.c, %.o,$(wildcard *.c))
Q         ?= @
DEFINES += -D_GNU_SOURCE

ANSI_CFLAGS   =

CFLAGS   = -g -O2 -std=gnu99 -Wno-format -fPIC -fsanitize=address -fsanitize=leak
SHARED_CFLAGS = -fPIC
SHARED_LFLAGS = -shared -L$(LIKWID_LIBDIR)
DYNAMIC_TARGET_LIB = libloop_adapt.so
LIBS =  -llikwid -lhwloc


all: $(DYNAMIC_TARGET_LIB)

$(DYNAMIC_TARGET_LIB): $(OBJ)
	@echo "===>  Link  $@"
	$(Q)${CC} $(DEFINES) $(INCLUDES) $(SHARED_LFLAGS) $(CFLAGS) $(ANSI_CFLAGS) $(SHARED_CFLAGS) -o $(DYNAMIC_TARGET_LIB) $(OBJ) -lasan $(LIBS)

%.o:  %.c
	@echo "===>  COMPILE  $@"
	$(Q)$(CC) $(DEFINES) $(INCLUDES) -c $(CFLAGS) $(ANSI_CFLAGS) $(CPPFLAGS) $< -o $@

examples:
	make -C example all

install: $(DYNAMIC_TARGET_LIB) usermetric.h
	$(Q)install -m 755 $(DYNAMIC_TARGET_LIB) $(PREFIX)/lib/$(DYNAMIC_TARGET_LIB)
	$(Q)install -m 644 loop_adapt.h $(PREFIX)/include/loop_adapt.h

uninstall:
	$(Q)rm $(PREFIX)/lib/$(DYNAMIC_TARGET_LIB)
	$(Q)rm $(PREFIX)/include/loop_adapt.h

clean:
	@rm -rf *.o $(DYNAMIC_TARGET_LIB)

.PHONY: clean
