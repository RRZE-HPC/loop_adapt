
include config.mk
LIKWID_INCDIR ?= ${HOME}/Apps/hwloc/include
LIKWID_LIBDIR ?= ${HOME}/Apps/hwloc/lib
INCLUDES  += -I./include -I$(LIKWID_INCDIR)
OBJ       = $(patsubst src/%.c, BUILD/%.o,$(wildcard src/*.c))
OBJ      += $(patsubst src/%.cc, BUILD/%.o,$(wildcard src/*.cc))
Q         ?= @
DEFINES += -D_GNU_SOURCE

ANSI_CFLAGS   =

CFLAGS   = -g -O2 -std=gnu99 -Wno-format -fPIC
CPPFLAGS   = -g -O2 -Wno-format -fPIC
SHARED_CFLAGS = -fPIC
SHARED_LFLAGS = -shared -L$(LIKWID_LIBDIR)
DYNAMIC_TARGET_LIB = libloop_adapt.so
LIBS =  -llikwid -lhwloc


all: $(DYNAMIC_TARGET_LIB)

$(DYNAMIC_TARGET_LIB): $(OBJ)
	@echo "===>  Link  $@"
	$(Q)${CC} $(DEFINES) $(INCLUDES) $(SHARED_LFLAGS) $(CFLAGS) $(ANSI_CFLAGS) $(SHARED_CFLAGS) -o $(DYNAMIC_TARGET_LIB) $(OBJ) $(LIBS)

BUILD/%.o:  src/%.c create_build_dir
	@echo "===>  COMPILE (C) $@"
	$(Q)$(CC) $(DEFINES) $(INCLUDES) -c $(CFLAGS) $(ANSI_CFLAGS) $(CPPFLAGS) $< -o $@

BUILD/%.o:  src/%.cc create_build_dir
	@echo "===>  COMPILE (C++) $@"
	$(Q)$(CXX) $(DEFINES) $(INCLUDES) -c $(ANSI_CFLAGS) $(CPPFLAGS) $< -o $@

create_build_dir:
	@mkdir -p BUILD

examples:
	make -C example all

install: $(DYNAMIC_TARGET_LIB)
	$(Q)install -m 755 $(DYNAMIC_TARGET_LIB) $(PREFIX)/lib/$(DYNAMIC_TARGET_LIB)
	$(Q)install -m 644 loop_adapt.h $(PREFIX)/include/loop_adapt.h

uninstall:
	$(Q)rm $(PREFIX)/lib/$(DYNAMIC_TARGET_LIB)
	$(Q)rm $(PREFIX)/include/loop_adapt.h

clean:
	@rm -rf BUILD $(DYNAMIC_TARGET_LIB)

distclean: clean

.PHONY: clean
