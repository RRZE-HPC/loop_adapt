#ifndef LOOP_ADAPT_MEASUREMENT_H
#define LOOP_ADAPT_MEASUREMENT_H

#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_configuration_types.h>
#include <loop_adapt_threads.h>
#include <bstrlib.h>

int loop_adapt_measurement_initialize();

int loop_adapt_measurement_setup(ThreadData_t thread, char* measurement, bstring configuration, bstring metrics);
int loop_adapt_measurement_start(ThreadData_t thread, char* measurement);
int loop_adapt_measurement_start_all();
int loop_adapt_measurement_stop(ThreadData_t thread, char* measurement);
int loop_adapt_measurement_stop_all();
int loop_adapt_measurement_result(ThreadData_t thread, char* measurement, int num_values, ParameterValue* value);

int loop_adapt_measurement_available(char* measurement);
int loop_adapt_measurement_num_metrics(ThreadData_t thread);

int loop_adapt_measurement_configs(struct bstrList* configs);
int loop_adapt_measurement_getbnames(struct bstrList* measurements);

void loop_adapt_measurement_finalize();

#endif /* LOOP_ADAPT_MEASUREMENT_H */
