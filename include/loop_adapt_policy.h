#ifndef LOOP_ADAPT_POLICY_H
#define LOOP_ADAPT_POLICY_H

#include <bstrlib.h>
#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_policy_types.h>

int loop_adapt_policy_initialize();
void loop_adapt_policy_finalize();

int loop_adapt_policy_available(char* policy);

int loop_adapt_policy_eval(char* loop, int num_results, ParameterValue* inputs, ParameterValue* output);

bstring loop_adapt_policy_get_measurement(bstring policy);

#endif