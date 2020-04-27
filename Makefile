
include config.mk

HWLOC_INCDIR ?= ${HOME}/Apps/include
HWLOC_LIBDIR ?= ${HOME}/Apps/lib
LIKWID_INCDIR ?= /mnt/opt/likwid-5.0.1/include
LIKWID_LIBDIR ?= /mnt/opt/likwid-5.0.1/lib
BOOST_INCDIR ?= ${BOOST_INCLUDEDIR}
BOOST_LIBDIR ?= ${BOOST_LIBRARYDIR}
MYSQL_INCDIR ?= ${HOME}/Apps/libmysqlcppconn-1.1.9/include
MYSQL_LIBDIR ?= ${HOME}/Apps/libmysqlcppconn-1.1.9/lib

INCLUDES  += -I./include -I$(HWLOC_INCDIR) -I$(LIKWID_INCDIR) -I$(BOOST_INCDIR) -I$(MYSQL_INCDIR)
OBJ       = $(patsubst src/%.c, BUILD/%.o,$(wildcard src/*.c))
OBJ      += $(patsubst src/%.cc, BUILD/%.o,$(wildcard src/*.cc))
OBJ      += $(patsubst src/%.cpp, BUILD/%.o,$(wildcard src/*.cpp))
Q         ?= @
DEFINES += -D_GNU_SOURCE

ANSI_CFLAGS   =

CFLAGS   = -g -fopenmp -std=gnu99 -Wno-format -fPIC
CPPFLAGS   = -g -fopenmp -Wno-format -fPIC
SHARED_CFLAGS = -fPIC
SHARED_LFLAGS = -shared -L$(HWLOC_LIBDIR) -L$(LIKWID_LIBDIR) -L$(BOOST_LIBDIR) -L$(MYSQL_LIBDIR)
DYNAMIC_TARGET_LIB = libloop_adapt.so
LIBS =  -llikwid -lhwloc -lmysqlcppconn


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

BUILD/%.o:  src/%.cpp create_build_dir
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

allclean: clean
	make -C examples clean
	make -C tests  clean

distclean: allclean

.PHONY: clean
