#ifndef LOOP_ADAPT_H
#define LOOP_ADAPT_H



#ifdef __cplusplus
extern "C"
{
#endif

#include <loop_adapt_types.h>
#include <loop_adapt_scopes.h>
#include <loop_adapt_parameter_value_types.h>
#include <hwloc.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef int (*policy_eval_function)(int num_values, ParameterValue *values, double *result);

#ifdef LOOP_ADAPT_ACTIVATE


extern int loop_adapt_active;
extern LoopAdaptDebugLevel loop_adapt_verbosity;


int loop_adapt_initialize();
void loop_adapt_register(char * name, int num_iterations);
int loop_adapt_register_thread(int threadid);
int loop_adapt_register_policy(char* name, char* backend, char* config, char* metric, policy_eval_function func );
int loop_adapt_add_loop_parameter(char* string, char* parameter);
int loop_adapt_add_loop_policy(char* string, char* policy);
int loop_adapt_start_loop( char* name, char* file, int linenumber );
int loop_adapt_end_loop(char* string);

void loop_adapt_finalize();
void loop_adapt_debug_level(int level);
int loop_adapt_register_inparallel_function(int (*in_parallel)(void));


#define LA_INIT loop_adapt_initialize();

#define LA_FINALIZE loop_adapt_finalize();

#define LA_REGISTER(name, count) loop_adapt_register(((char *)name), (count));
#define LA_REGISTER_THREAD(threadid) loop_adapt_register_thread((threadid));
#define LA_REGISTER_POLICY(name, backend, config, metric, func) loop_adapt_register_policy((name), (backend), (config), (metric), (func));
#define LA_REGISTER_INPARALLEL_FUNC(func) loop_adapt_register_inparallel_function((func));
#define LA_USE_LOOP_PARAMETER(name, parameter) loop_adapt_add_loop_parameter(((char *)name), ((char *)parameter));
#define LA_USE_LOOP_POLICY(name, policy) loop_adapt_add_loop_policy(((char *)name), ((char *)policy));




#define LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(TYPE) \
    int loop_adapt_new_##TYPE##_parameter(char* name, LoopAdaptScope_t scope, TYPE value); \
    TYPE loop_adapt_get_##TYPE##_parameter(char* name); \
    int loop_adapt_set_##TYPE##_parameter(char* name, TYPE value);

LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(int)
LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(boolean)
LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(char)
LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(double)
LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(float)

#define LA_GET_INT_PARAMETER(name, x) x = loop_adapt_get_int_parameter(((char *)name));
#define LA_SET_INT_PARAMETER(name, x) loop_adapt_set_int_parameter(((char *)name), (x));
#define LA_NEW_INT_PARAMETER(name, scope, value) loop_adapt_new_int_parameter(((char *)name), (scope), (value))

#define LA_GET_BOOL_PARAMETER(name, x) x = loop_adapt_get_bool_parameter((char *)name);
#define LA_SET_BOOL_PARAMETER(name, x) loop_adapt_set_bool_parameter(((char *)name), (x));
#define LA_NEW_BOOL_PARAMETER(name, scope, value) loop_adapt_new_bool_parameter(((char *)name), (scope), (value))

int loop_adapt_new_loop_start(char* string, char* file, int linenumber);
int loop_adapt_new_loop_end(char* string);

#define LOOP_BEGIN(name) loop_adapt_start_loop(((char *)name), (char *)__FILE__, __LINE__)
#define LOOP_END(name) loop_adapt_end_loop(((char *)name))

#define LA_FOR(name, start, cond, inc) \
    for (start; cond && LOOP_BEGIN(((char *)name)); inc, LOOP_END(name))

#define LA_FOR_BEGIN(name, ...) \
    for (__VA_ARGS__) \
    { \
        LOOP_BEGIN(((char *)name));

#define LA_FOR_END(name) \
        LOOP_END(((char *)name)); \
    }

#else /* !LOOP_ADAPT_ACTIVATE */

#define LA_INIT
#define LA_FINALIZE

#define LA_REGISTER(name, count)
#define LA_REGISTER_THREAD(threadid)
#define LA_REGISTER_INPARALLEL_FUNC(func)
#define LA_REGISTER_POLICY(name, backend, config, metric, func)
#define LA_USE_LOOP_PARAMETER(name, parameter)
#define LA_USE_LOOP_POLICY(name, policy)

#define LA_NEW_INT_PARAMETER(name, scope, type)
#define LA_GET_INT_PARAMETER(name, x)
#define LA_SET_INT_PARAMETER(name, x)

#define LA_NEW_BOOL_PARAMETER(name, scope, type)
#define LA_GET_BOOL_PARAMETER(name, x)
#define LA_SET_BOOL_PARAMETER(name, x)

#define LOOP_BEGIN(name)
#define LOOP_END(name)

#define LA_FOR(name, start, cond, inc) \
    for (start; cond; inc)

#define LA_FOR_BEGIN(name, ...) \
    for (__VA_ARGS__) {
#define LA_FOR_END(name) }


#endif /* LOOP_ADAPT_ACTIVATE */


#ifdef __cplusplus
}
#endif

#endif /* LOOP_ADAPT_H */
