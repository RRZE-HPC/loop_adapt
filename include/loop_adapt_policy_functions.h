#ifndef LOOP_ADAPT_POLICY_FUNCTIONS_H
#define LOOP_ADAPT_POLICY_FUNCTIONS_H

#include <loop_adapt_parameter_value.h>

int loop_adapt_policy_function_min(int num_values, ParameterValue *values, double *result);
int loop_adapt_policy_function_max(int num_values, ParameterValue *values, double *result);
int loop_adapt_policy_function_sum(int num_values, ParameterValue *values, double *result);

#endif