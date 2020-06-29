#ifndef LOOP_ADAPT_PARAMETER_H
#define LOOP_ADAPT_PARAMETER_H

#include <loop_adapt_parameter_types.h>
#include <loop_adapt_parameter_value.h>
#include <loop_adapt_parameter_limit.h>
#include <loop_adapt_threads_types.h>

int loop_adapt_parameter_initialize();
int loop_adapt_parameter_getnames(int* count, char*** parameters);
int loop_adapt_parameter_getbnames(struct bstrList* parameters);


int loop_adapt_parameter_add(char* name, LoopAdaptScope_t scope,
                             ParameterValueType_t valuetype,
                             parameter_set_function set,
                             parameter_get_function get,
                             parameter_available_function avail);
int loop_adapt_parameter_add_user(char* name, LoopAdaptScope_t scope, ParameterValue value);
int loop_adapt_parameter_add_user_with_limit(char* name, LoopAdaptScope_t scope, ParameterValue value, ParameterValueLimit limit);
int loop_adapt_parameter_set(ThreadData_t thread, char* parameter, ParameterValue value);
int loop_adapt_parameter_get(ThreadData_t thread, char* parameter, ParameterValue* value);
int loop_adapt_parameter_getcurrent(ThreadData_t thread, char* parameter, ParameterValue* value);
int loop_adapt_parameter_configs(struct bstrList* configs);


ParameterValueType_t loop_adapt_parameter_type(char* name);
LoopAdaptScope_t loop_adapt_parameter_scope(char* name);
int loop_adapt_parameter_scope_count(char* name);

int loop_adapt_parameter_loop_start(ThreadData_t thread);
int loop_adapt_parameter_loop_end(ThreadData_t thread);

int loop_adapt_parameter_loop_best(ThreadData_t thread, ParameterValue v);

void loop_adapt_parameter_finalize();

// internal use
bstring loop_adapt_parameter_str_long(bstring name);


#endif /* LOOP_ADAPT_PARAMETER_H */
