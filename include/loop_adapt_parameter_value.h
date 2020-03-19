#ifndef LOOP_ADAPT_PARAMETER_VALUE_H
#define LOOP_ADAPT_PARAMETER_VALUE_H

#include <loop_adapt_parameter_value_types.h>

ParameterValue loop_adapt_new_param_value(ParameterValueType_t type);
int loop_adapt_copy_param_value(ParameterValue in, ParameterValue *out);

int loop_adapt_less_param_value(ParameterValue a, ParameterValue b);
int loop_adapt_greater_param_value(ParameterValue a, ParameterValue b);
int loop_adapt_equal_param_value(ParameterValue a, ParameterValue b);
int loop_adapt_add_param_value(ParameterValue a, ParameterValue b, ParameterValue *result);
int loop_adapt_mult_param_value(ParameterValue a, ParameterValue b, ParameterValue *result);

void loop_adapt_print_param_value(ParameterValue v);
char* loop_adapt_print_param_valuetype(ParameterValueType_t t);
char* loop_adapt_param_value_str(ParameterValue v);

int loop_adapt_parse_param_value(char* string, ParameterValueType_t type, ParameterValue* value);


void loop_adapt_destroy_param_value(ParameterValue v);


#define DEC_NEW_INVALID_PARAM_VALUE { .type = LOOP_ADAPT_PARAMETER_TYPE_INVALID, .value.ival = -1}
#define DEC_NEW_INT_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_INT, \
                                    .value.ival = (int)(x) }
#define DEC_NEW_UINT_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_UINT, \
                                     .value.uval = (unsigned int)(x) }
#define DEC_NEW_LONG_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_LONG, \
                                     .value.lval = (long)(x) }
#define DEC_NEW_ULONG_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_ULONG, \
                                     .value.ulval = (unsigned long)(x) }
#define DEC_NEW_STR_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_STR, \
                                    .value.sval = (char*)(x) }
#define DEC_NEW_FLOAT_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_FLOAT, \
                                      .value.fval = (float)(x) }
#define DEC_NEW_DOUBLE_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE, \
                                       .value.dval = (double)(x) }
#define DEC_NEW_CHAR_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_CHAR, \
                                     .value.cval = (char)(x) }
#define DEC_NEW_BOOL_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_BOOL, \
                                     .value.bval = (unsigned int)(x) }
#define DEC_NEW_PTR_PARAM_VALUE(x) {.type = LOOP_ADAPT_PARAMETER_TYPE_PTR, \
                                     .value.pval = (void*)(x) }



#endif /* LOOP_ADAPT_PARAMETER_VALUE_H */
