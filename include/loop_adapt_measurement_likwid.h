#ifndef LOOP_ADAPT_MEASUREMENT_LIKWID_H
#define LOOP_ADAPT_MEASUREMENT_LIKWID_H

#include <bstrlib.h>

#include <loop_adapt_parameter_value_types.h>

int loop_adapt_measurement_likwid_init();

int loop_adapt_measurement_likwid_setup(int instance, bstring configuration, bstring metrics);
void loop_adapt_measurement_likwid_start(int instance);
void loop_adapt_measurement_likwid_startall();
void loop_adapt_measurement_likwid_stop(int instance);
void loop_adapt_measurement_likwid_stopall();
int loop_adapt_measurement_likwid_result(int instance, int num_values, ParameterValue* values);
int loop_adapt_measurement_likwid_configs(struct bstrList* configs);
void loop_adapt_measurement_likwid_finalize();

#endif
