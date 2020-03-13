#ifndef LOOP_ADAPT_MEASUREMENT_TYPES_H
#define LOOP_ADAPT_MEASUREMENT_TYPES_H

#include <hwloc.h>
#include <bstrlib.h>
#include <loop_adapt_parameter_types.h>
#include <loop_adapt_scopes.h>

#define LOOP_ADAPT_MEASUREMENT_SCOPE_MIN HWLOC_OBJ_TYPE_MIN
typedef enum {
    LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD = HWLOC_OBJ_PU,
    LOOP_ADAPT_MEASUREMENT_SCOPE_CORE = HWLOC_OBJ_CORE,
    LOOP_ADAPT_MEASUREMENT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
    LOOP_ADAPT_MEASUREMENT_SCOPE_SYSTEM = HWLOC_OBJ_MACHINE,
    LOOP_ADAPT_MEASUREMENT_SCOPE_NUMANODE = HWLOC_OBJ_NUMANODE,
    LOOP_ADAPT_MEASUREMENT_SCOPE_LLCACHE = HWLOC_OBJ_L3CACHE,
    LOOP_ADAPT_MEASUREMENT_SCOPE_MAX
} MeasurementScope_t;

#define LOOP_ADAPT_MEASUREMENT_VALID_SCOPE(scope) \
    ((scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD || \\
     (scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_CORE || \\
     (scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_SOCKET || \\
     (scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_SYSTEM || \\
     (scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_NUMANODE || \\
     (scope) == LOOP_ADAPT_MEASUREMENT_SCOPE_LLCACHE)

typedef enum {
    LOOP_ADAPT_MEASUREMENT_STATE_NONE = 0,
    LOOP_ADAPT_MEASUREMENT_STATE_SETUP,
    LOOP_ADAPT_MEASUREMENT_STATE_RUNNING,
    LOOP_ADAPT_MEASUREMENT_STATE_STOPPED,
} MeasurementState_t;

typedef struct {
    bstring configuration;
    bstring metrics;
    int measure_list_idx;
    int instance;
    int responsible;
    MeasurementState_t state;
} Measurement;
typedef Measurement* Measurement_t;


typedef int (*measurement_init_function)();
typedef int (*measurement_setup_function)(int instance, bstring configuration, bstring metrics);
typedef void (*measurement_start_function)(int instance);
typedef void (*measurement_stop_function)(int instance);
typedef int (*measurement_result_function)(int instance, int num_values, ParameterValue* values);
typedef void (*measurement_finalize_function)();

typedef struct {
    char* name;
    LoopAdaptScope_t scope;
    measurement_init_function init;
    measurement_setup_function setup;
    measurement_start_function start;
    measurement_stop_function stop;
    measurement_result_function result;
    measurement_finalize_function finalize;
} MeasurementDefinition;

#endif /* LOOP_ADAPT_MEASUREMENT_TYPES_H */
