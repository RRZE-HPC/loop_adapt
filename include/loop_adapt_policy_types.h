#ifndef LOOP_ADAPT_POLICY_TYPES_H
#define LOOP_ADAPT_POLICY_TYPES_H

#include <bstrlib.h>

// Used for the list of builtin policies
typedef struct {
    char* name;
    char* description;
    char* backend;
    char* config;
    char* match;
    int (*eval)(int num_values, ParameterValue* values, double* result);
} _PolicyDefinition;



typedef struct {
    bstring name;
    bstring backend;
    bstring description;
    bstring config;
    bstring match;
    int (*eval)(int num_values, ParameterValue* values, double* result);
} PolicyDefinition;
typedef PolicyDefinition* PolicyDefinition_t;

#endif
