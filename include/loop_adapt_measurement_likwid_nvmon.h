#ifndef LOOP_ADAPT_MEASUREMENT_LIKWID_NVMON_H
#define LOOP_ADAPT_MEASUREMENT_LIKWID_NVMON_H

#include <bstrlib.h>

#include <loop_adapt_parameter_value_types.h>

int loop_adapt_measurement_likwid_nvmon_init();

int loop_adapt_measurement_likwid_nvmon_setup(int instance, bstring configuration, bstring metrics);
void loop_adapt_measurement_likwid_nvmon_start(int instance);
void loop_adapt_measurement_likwid_nvmon_stop(int instance);
int loop_adapt_measurement_likwid_nvmon_result(int instance, int num_values, ParameterValue* values);
int loop_adapt_measurement_likwid_nvmon_configs(struct bstrList* configs);
void loop_adapt_measurement_likwid_nvmon_finalize();

#endif
