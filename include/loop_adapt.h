#ifndef LOOP_ADAPT_H
#define LOOP_ADAPT_H



#ifdef __cplusplus
extern "C"
{
#endif

#include <loop_adapt_types.h>
#include <loop_adapt_scopes.h>
#include <hwloc.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifdef LOOP_ADAPT_ACTIVATE

#include <loop_adapt_parameter_value_types.h>




extern int loop_adapt_active;
extern LoopAdaptDebugLevel loop_adapt_verbosity;

int loop_adapt_initialize();
void loop_adapt_register(char * name, int num_iterations);
int loop_adapt_register_thread(int threadid);
int loop_adapt_start_loop( char* name, char* file, int linenumber );
int loop_adapt_start_loop_new( char* name, char* file, int linenumber );
void loop_adapt_finalize();
void loop_adapt_debug_level(int level);
int loop_adapt_register_inparallel_function(int (*in_parallel)(void));

//#define _LOOP_ADAPT_DEFINE_PARAM_GET_SET_DEFS(NAME) \
//    int loop_adapt_get_##NAME##_parameter(char* name); \
//    int loop_adapt_set_##NAME##_parameter(char* name, NAME value);
//_LOOP_ADAPT_DEFINE_PARAM_GET_SET_DEFS(int)
//_LOOP_ADAPT_DEFINE_PARAM_GET_SET_DEFS(bool)
/*int loop_adapt_get_bool_parameter(char* name);*/
/*int loop_adapt_set_bool_parameter(char* name, unsigned int value);*/

#define LA_INIT loop_adapt_initialize();

#define LA_FINALIZE loop_adapt_finalize();

#define LA_REGISTER(name, count) loop_adapt_register(((char *)name), (count));
#define LA_REGISTER_THREAD(threadid) loop_adapt_register_thread((threadid));
#define LA_REGISTER_INPARALLEL_FUNC(func) loop_adapt_register_inparallel_function((func));

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

#define LOOP_BEGIN(name) loop_adapt_start_loop(((char *)name), (char *)__FILE__, __LINE__)
#define LOOP_END(name) loop_adapt_end(((char *)name))

#define LA_FOR(name, start, cond, inc) \
    for (start; cond && LOOP_BEGIN(((char *)name)); inc)

#define LA_FOR_BEGIN(name, ...) \
    for (__VA_ARGS__) \
    { \
        LOOP_BEGIN(((char *)name));

#define LA_FOR_END(name) \
        LOOP_END(((char *)name)); \
    }

#else /* !LOOP_ADAPT_ACTIVATE */

#define LA_REGISTER(name, count)
#define LA_REGISTER_THREAD(threadid)
#define LA_REGISTER_INPARALLEL_FUNC(func)
#define LA_NEW_PARAMETER(name, scope, type)
#define LA_GET_INT_PARAMETER(name, x)

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
