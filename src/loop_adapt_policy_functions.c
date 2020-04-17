#include <stdlib.h>
#include <stdio.h>
#include <error.h>


#include <loop_adapt_parameter_value.h>


int loop_adapt_policy_function_min(int num_values, ParameterValue *values, double *result)
{
    if (num_values == 0 || (!values) || (!result))
        return -EINVAL;
    ParameterValue r = DEC_NEW_INVALID_PARAM_VALUE;
    loop_adapt_copy_param_value(values[0], &r);
    int i = 0;
    for (i = 1; i < num_values; i++)
    {
        // values[i] < r
        if (loop_adapt_less_param_value(values[i], r))
        {
            loop_adapt_copy_param_value(values[i], &r);
        }
    }
    loop_adapt_cast_param_value(&r, LOOP_ADAPT_PARAMETER_TYPE_DOUBLE);
    *result = (double)r.value.dval;
    loop_adapt_destroy_param_value(r);
    return 0;
}

int loop_adapt_policy_function_max(int num_values, ParameterValue *values, double *result)
{
    if (num_values == 0 || (!values) || (!result))
        return -EINVAL;
    ParameterValue r = DEC_NEW_INVALID_PARAM_VALUE;
    loop_adapt_copy_param_value(values[0], &r);
    int i = 0;
    for (i = 1; i < num_values; i++)
    {
        // values[i] > r
        if (loop_adapt_greater_param_value(values[i], r))
        {
            loop_adapt_copy_param_value(values[i], &r);
        }
    }
    loop_adapt_cast_param_value(&r, LOOP_ADAPT_PARAMETER_TYPE_DOUBLE);
    *result = (double)r.value.dval;
    loop_adapt_destroy_param_value(r);
    return 0;
}

int loop_adapt_policy_function_sum(int num_values, ParameterValue *values, double *result)
{
    if (num_values == 0 || (!values) || (!result))
        return -EINVAL;
    ParameterValue r = loop_adapt_new_param_value(values[0].type);
    int i = 0;
    for (i = 0; i < num_values; i++)
    {
        loop_adapt_add_param_value(r, values[i], &r);
    }
    loop_adapt_cast_param_value(&r, LOOP_ADAPT_PARAMETER_TYPE_DOUBLE);
    *result = (double)r.value.dval;
    loop_adapt_destroy_param_value(r);
    return 0;
}

int loop_adapt_policy_function_avg(int num_values, ParameterValue *values, double *result)
{
    double sum = 0;
    int err = loop_adapt_policy_function_sum(num_values, values, &sum);
    if (err == 0)
    {
        *result = sum/num_values;
    }
    return err;
}