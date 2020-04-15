#ifndef LOOP_ADAPT_PARAMETER_LIMIT_H
#define LOOP_ADAPT_PARAMETER_LIMIT_H

#include <loop_adapt_parameter_limit_types.h>

char* loop_adapt_param_limit_type(ParameterValueLimitType_t t);
char* loop_adapt_param_limit_str(ParameterValueLimit l);

ParameterValueLimit loop_adapt_new_param_limit_list();
int loop_adapt_add_param_limit_list(ParameterValueLimit *l, ParameterValue v);
void loop_adapt_destroy_param_limit(ParameterValueLimit l);

int loop_adapt_next_limit_value(ParameterValueLimit *l, ParameterValue* v);
int loop_adapt_param_limit_tolist(ParameterValueLimit in, ParameterValueLimit *out);

int loop_adapt_check_param_limit(ParameterValue p, ParameterValueLimit l);

#define DEC_NEW_INVALID_PARAM_LIMIT \
    { \
        .type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_INVALID, \
    }

#define DEC_NEW_INTRANGE_PARAM_LIMIT(START, END, STEP) \
    { \
        .type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE, \
        .limit.range.current = DEC_NEW_INVALID_PARAM_VALUE, \
        .limit.range.start = DEC_NEW_INT_PARAM_VALUE((START)), \
        .limit.range.end = DEC_NEW_INT_PARAM_VALUE((END)), \
        .limit.range.step = DEC_NEW_INT_PARAM_VALUE((STEP)), \
    }

#define DEC_NEW_DOUBLERANGE_PARAM_LIMIT(START, END, STEP) \
    { \
        .type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE, \
        .limit.range.current = DEC_NEW_INVALID_PARAM_VALUE, \
        .limit.range.start = DEC_NEW_DOUBLE_PARAM_VALUE((START)), \
        .limit.range.end = DEC_NEW_DOUBLE_PARAM_VALUE((END)), \
        .limit.range.step = DEC_NEW_DOUBLE_PARAM_VALUE((STEP)), \
    }

#define DEC_NEW_LIST_PARAM_LIMIT() \
    { \
        .type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST, \
        .limit.list.num_values = 0, \
        .limit.list.value_idx = -1, \
        .limit.list.values = NULL, \
    }

#endif /* LOOP_ADAPT_PARAMETER_LIMIT_H */
