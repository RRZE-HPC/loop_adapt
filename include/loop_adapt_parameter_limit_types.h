#ifndef LOOP_ADAPT_PARAMETER_LIMIT_TYPES_H
#define LOOP_ADAPT_PARAMETER_LIMIT_TYPES_H

#include <loop_adapt_parameter_value_types.h>

#define LOOP_ADAPT_PARAMETER_LIMIT_TYPE_MIN LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE
typedef enum {
    LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE = 0,
    LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST,
    LOOP_ADAPT_PARAMETER_LIMIT_TYPE_MAX,
} ParameterValueLimitType_t;

typedef struct {
    ParameterValueLimitType_t type;
    union {
        struct {
            ParameterValue start;
            ParameterValue end;
            ParameterValue step;
            ParameterValue current;
        } range;
        struct {
            int num_values;
            int value_idx;
            ParameterValue* values;
        } list;
    } limit;
} ParameterValueLimit;


#endif /* LOOP_ADAPT_PARAMETER_LIMIT_TYPES_H */
