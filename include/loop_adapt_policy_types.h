#ifndef LOOP_ADAPT_POLICY_TYPES_H
#define LOOP_ADAPT_POLICY_TYPES_H


// Used for the list of builtin policies
typedef struct {
    char* name;
    char* backend;
    char* config;
    char* match;
    int (*eval)(int num_values, ParameterValue* values, double* result);
} _PolicyDefinition;


// Used at runtime
typedef struct {
    bstring name;
    bstring measurement;
    int (*eval)(int num_values, ParameterValue* values, double* result);
} PolicyDefinition;
typedef PolicyDefinition* PolicyDefinition_t;

#endif
