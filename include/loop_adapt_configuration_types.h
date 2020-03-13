#ifndef LOOP_ADAPT_CONFIGURATION_TYPES_H
#define LOOP_ADAPT_CONFIGURATION_TYPES_H

#include <loop_adapt_parameter_value_types.h>
#include <bstrlib.h>


typedef struct {
    bstring parameter;
    ParameterValueType_t type;
    int num_values;
    ParameterValue* values;
} LoopAdaptConfigurationParameter;

typedef struct {
    bstring measurement;
    bstring config;
    bstring metric;
} LoopAdaptConfigurationMeasurement;

typedef struct {
    int configuration_id;
    int num_parameters;
    LoopAdaptConfigurationParameter* parameters;
    int num_measurements;
    LoopAdaptConfigurationMeasurement* measurements;
} LoopAdaptConfiguration;
typedef LoopAdaptConfiguration* LoopAdaptConfiguration_t;

typedef struct {
    int (*init)();
    LoopAdaptConfiguration_t (*getnew)(char* string);
    LoopAdaptConfiguration_t (*getcurrent)(char* string);
    void (*finalize)();
} LoopAdaptInputConfigurationFunctions;

typedef struct {
    int (*init)();
    int (*write)(LoopAdaptConfiguration_t config, int num_results, ParameterValue* results);
    void (*finalize)();
} LoopAdaptOutputConfigurationFunctions;


#endif /* LOOP_ADAPT_CONFIGURATION_TYPES_H */
