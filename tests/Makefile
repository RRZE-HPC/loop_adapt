
Q?=@

HWLOC_INCLUDE=-I${HOME}/Apps/hwloc/include
HWLOC_LIBDIR=-L${HOME}/Apps/hwloc/lib
HWLOC_LIB=-lhwloc

LIKWID_INCLUDE=-I/usr/local/include
LIKWID_LIBDIR=-L/usr/local/lib
LIKWID_LIB=-llikwid

LA_INCLUDE=-I../include
LA_LIBDIR=-L..
LA_LIB=-lloop_adapt

INCLUDES= $(LA_INCLUDE) $(HWLOC_INCLUDE) $(LIKWID_INCLUDE)
LIBDIRS= $(LA_LIBDIR) $(HWLOC_LIBDIR) $(LIKWID_LIBDIR)

BUILD_DIR=./BUILD



LIBS=$(LA_LIB) $(HWLOC_LIB) $(LIKWID_LIB)

DEFINES= -D_GNU_SOURCE
ACTIVE_DEFINE=-DLOOP_ADAPT_ACTIVATE
CFLAGS   = -g -O2 -std=gnu99 -Wno-format -fPIC
CPPFLAGS   = -g -O2 -Wno-format -fPIC
SHARED_CFLAGS = -fPIC
SHARED_LFLAGS = -L$(LIKWID_LIBDIR)

BSTRLIB_FILES = $(wildcard ../src/bstrlib*.c)
BSTRLIB_HEADERS = $(wildcard ../include/bstrlib*.h)

MAP_FILES     = ../src/map.c ../src/ghash.c
MAP_HEADERS   = ../include/map.h

HWLOCTREE_FILES = ../src/loop_adapt_hwloc_tree.c
HWLOCTREE_HEADERS = ../include/loop_adapt_hwloc_tree.h

THREADS_FILES = $(wildcard ../src/loop_adapt_threads*.c)
THREADS_HEADERS = $(wildcard ../include/loop_adapt_threads*.h)

MEASUREMENT_FILES = $(wildcard ../src/loop_adapt_measurement*.c)
MEASUREMENT_HEADERS = $(wildcard ../include/loop_adapt_measurement*.h)

PARAMETER_VALUE_FILES = ../src/loop_adapt_parameter_value.c
PARAMETER_VALUE_HEADERS = ../include/loop_adapt_parameter_value.h

PARAMETER_LIMIT_FILES = ../src/loop_adapt_parameter_limit.c
PARAMETER_LIMIT_HEADERS = ../include/loop_adapt_parameter_limit.h

PARAMETER_FILES = $(wildcard ../src/loop_adapt_parameter*.c)
PARAMETER_HEADERS = $(wildcard ../include/loop_adapt_parameter*.h)

CONFIGURATION_FILES = $(wildcard ../src/loop_adapt_config*.c)
CONFIGURATION_HEADERS = $(wildcard ../include/loop_adapt_config*.h)

RINGBUFFER_FILES = ../src/loop_adapt_configuration_socket_ringbuffer.c
RINGBUFFER_HEADERS = $(wildcard ../include/loop_adapt_configuration_socket_ringbuffer*.h)

all: parameter_value_test parameter_limit_test threads_test measurement_test smap_test imap_test bstrlib_helper_test parameter_test configuration_test ringbuffer_test

create_build_dir:
	@mkdir -p $(BUILD_DIR)

PARAMETER_VALUE_OBJS = parameter_value_test.c $(PARAMETER_VALUE_FILES)
parameter_value_test: $(PARAMETER_VALUE_OBJS) $(PARAMETER_VALUE_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(PARAMETER_VALUE_OBJS) -o $@

PARAMETER_LIMIT_OBJS = parameter_limit_test.c $(PARAMETER_VALUE_FILES) $(PARAMETER_LIMIT_FILES)
parameter_limit_test: $(PARAMETER_LIMIT_OBJS) $(PARAMETER_LIMIT_HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(PARAMETER_LIMIT_OBJS) -o $@


THREADS_OBJ = threads_test.c $(THREADS_FILES) $(MAP_FILES) $(HWLOCTREE_FILES)
threads_test: $(THREADS_OBJ) $(THREADS_HEADERS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(INCLUDES) $(LIBDIRS) $(THREADS_OBJ) -o $@ $(HWLOC_LIB) -ldl


MEASUREMENT_OBJS  = measurement_test.c $(MAP_FILES) $(BSTRLIB_FILES) $(PARAMETER_VALUE_FILES)
MEASUREMENT_OBJS += $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(MEASUREMENT_FILES))
MEASUREMENT_OBJS += $(THREADS_FILES) $(HWLOCTREE_FILES)
MEASUREMENT_INCLUDES = $(HWLOC_INCLUDE) $(LA_INCLUDE) $(LIKWID_INCLUDE)
MEASUREMENT_LIBDIRS = $(HWLOC_LIBDIR) $(LA_LIBDIR) $(LIKWID_LIBDIR)
MEASUREMENT_LIBS = $(HWLOC_LIB) $(LIKWID_LIB)
measurement_test: $(MEASUREMENT_OBJS) $(MEASUREMENT_HEADERS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(MEASUREMENT_INCLUDES) $(MEASUREMENT_LIBDIRS) $(MEASUREMENT_OBJS) -o $@ $(MEASUREMENT_LIBS) -ldl

SMAP_OBJS = smap_test.c $(MAP_FILES) $(MAP_HEADERS)
smap_test: $(SMAP_OBJS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(INCLUDES) $(LIBDIRS) $(SMAP_OBJS) -o $@

IMAP_OBJS = imap_test.c $(MAP_FILES) $(MAP_HEADERS)
imap_test: $(IMAP_OBJS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(INCLUDES) $(LIBDIRS) $(IMAP_OBJS) -o $@

BSTRLIB_OBJS = bstrlib_helper_test.c $(BSTRLIB_FILES)
bstrlib_helper_test: $(BSTRLIB_OBJS) $(BSTRLIB_HEADERS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(INCLUDES) $(LIBDIRS) $(BSTRLIB_OBJS) -o $@

PARAMETER_OBJS = parameter_test.c $(MAP_FILES) $(BSTRLIB_FILES)
PARAMETER_OBJS += $(THREADS_FILES) $(HWLOCTREE_FILES)
PARAMETER_OBJS += $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(PARAMETER_FILES))
PARAMETER_INCLUDES = $(HWLOC_INCLUDE) $(LA_INCLUDE) $(LIKWID_INCLUDE)
PARAMETER_LIBDIRS = $(HWLOC_LIBDIR) $(LA_LIBDIR) $(LIKWID_LIBDIR)
PARAMETER_LIBS = $(HWLOC_LIB) $(LIKWID_LIB)
parameter_test: $(PARAMETER_OBJS) $(PARAMETER_HEADERS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(PARAMETER_INCLUDES) $(PARAMETER_LIBDIRS) $(PARAMETER_OBJS) -o $@ $(PARAMETER_LIBS) -ldl


BUILD_CONFIGURATION_FILES = $(MAP_FILES) $(BSTRLIB_FILES)
BUILD_CONFIGURATION_FILES += $(THREADS_FILES) $(HWLOCTREE_FILES)
BUILD_CONFIGURATION_FILES += $(PARAMETER_FILES)
BUILD_CONFIGURATION_FILES += $(MEASUREMENT_FILES)
BUILD_CONFIGURATION_FILES += $(CONFIGURATION_FILES)
CONFIGURATION_OBJS = $(patsubst ../src/%.c, $(BUILD_DIR)/%.o, $(BUILD_CONFIGURATION_FILES))
#CONFIGURATION_OBJS += $(patsubst ../src/%.cc, $(BUILD_DIR)/%.o, ../src/loop_adapt_configuration_cc_client.cc)
CONFIGURATION_OBJS += configuration_test.c
CONFIGURATION_INCLUDES = $(HWLOC_INCLUDE) $(LA_INCLUDE) $(LIKWID_INCLUDE)
CONFIGURATION_LIBDIRS = $(HWLOC_LIBDIR) $(LA_LIBDIR) $(LIKWID_LIBDIR)
CONFIGURATION_LIBS = $(HWLOC_LIB) $(LIKWID_LIB)
configuration_test: $(CONFIGURATION_OBJS) $(CONFIGURATION_HEADERS)
	@echo "===>  LINK $@"
	$(CC) -fopenmp -pthread $(CFLAGS) $(DEFINES) $(CONFIGURATION_INCLUDES) $(CONFIGURATION_LIBDIRS) $(CONFIGURATION_OBJS) ../BUILD/loop_adapt_configuration_cc_client.o -o $@ $(CONFIGURATION_LIBS) -ldl -lstdc++

BUIL_RINGBUFFER_FILES = $(RINGBUFFER_FILES)
RINGBUFFER_OBJS = $(patsubst ../src/%.c, $(BUILD_DIR)/%.o, $(BUIL_RINGBUFFER_FILES))
RINGBUFFER_OBJS += ringbuffer_test.c
RINGBUFFER_INCLUDES = $(LA_INCLUDE)
ringbuffer_test: $(RINGBUFFER_OBJS) $(RINGBUFFER_HEADERS)
	$(CC) -pthread $(CFLAGS) $(DEFINES) $(RINGBUFFER_INCLUDES) $(RINGBUFFER_OBJS) -o $@

$(BUILD_DIR)/%.o:  ../src/%.c create_build_dir
	@echo "===>  COMPILE (C) $@"
	$(Q)$(CC) $(DEFINES) $(INCLUDES) -c $(CFLAGS) $(ANSI_CFLAGS) $< -o $@

$(BUILD_DIR)/%.o:  ../src/%.cc create_build_dir
	@echo "===>  COMPILE (C++) $@"
	$(Q)$(CXX) -shared -fPIC $(DEFINES) $(INCLUDES) -c $(ANSI_CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	@rm -f parameter_value_test parameter_limit_test threads_test measurement_test smap_test imap_test bstrlib_helper_test parameter_test configuration_test ../src/loop_adapt_configuration_cc_client.o ringbuffer_test
	@rm -rf BUILD

.PHONY: clean
