#ifndef LOOP_ADAPT_PARAMETER_H
#define LOOP_ADAPT_PARAMETER_H

#include <loop_adapt_parameter_types.h>
#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_threads_types.h>

int loop_adapt_parameter_initialize();
int loop_adapt_parameter_getnames(int* count, char*** parameters);


int loop_adapt_parameter_add(char* name, LoopAdaptScope_t scope,
                             ParameterValueType_t valuetype,
                             parameter_set_function set,
                             parameter_get_function get,
                             parameter_available_function avail);
int loop_adapt_parameter_add_user(char* name, LoopAdaptScope_t scope, ParameterValue value);
int loop_adapt_parameter_set(ThreadData_t thread, char* parameter, ParameterValue value);
int loop_adapt_parameter_get(ThreadData_t thread, char* parameter, ParameterValue* value);
int loop_adapt_parameter_getcurrent(ThreadData_t thread, char* parameter, ParameterValue* value);

ParameterValueType_t loop_adapt_parameter_type(char* name);
LoopAdaptScope_t loop_adapt_parameter_scope(char* name);
int loop_adapt_parameter_scope_count(char* name);

void loop_adapt_parameter_finalize();

#endif /* LOOP_ADAPT_PARAMETER_H */