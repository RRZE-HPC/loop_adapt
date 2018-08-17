#ifndef LOOP_ADAPT_H
#define LOOP_ADAPT_H

#include <hwloc.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** \addtogroup LA_Macros loop_adapt macros
*  @{
*/

/*!
\def REGISTER_LOOP(string)
Shortcut for loop_adapt_register(string) if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/
/*!
\def REGISTER_POLICY(string, policy, num_profiles, iters_per_profile)
Shortcut for loop_adapt_register_policy(string, policy, num_profiles, iters_per_profile) if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/
/*!
\def LA_FOR(name, start, cond, inc)
Shortcut for for (start; cond && LOOP_BEGIN(name); inc, LOOP_END(name)) if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is resolved to for (start; cond; inc)
*/
/** @}*/

//#include <loop_adapt_types.h>
//#include <loop_adapt_policy.h>
static int loop_adapt_debug = 1; /*! \brief  Debug level */
static int loop_adapt_active = 1; /*! \brief Internal variable whether loop_adapt is active */

#ifdef LOOP_ADAPT_ACTIVE

/** \addtogroup LA_API loop_adapt API
*  @{
*/
/*! \brief  List of topology entities where policies and parameters can be associated to */
typedef enum {
    /*! \brief  No type */
    LOOP_ADAPT_SCOPE_NONE = -1,
    /*! \brief  Same value as HWLOC_OBJ_MACHINE to ease casting */
    LOOP_ADAPT_SCOPE_MACHINE = HWLOC_OBJ_MACHINE,
    /*! \brief  Same value as HWLOC_OBJ_PACKAGE to ease casting */
    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
    /*! \brief  Same value as HWLOC_OBJ_NUMANODE to ease casting */
    LOOP_ADAPT_SCOPE_NUMA = HWLOC_OBJ_NUMANODE,
    /*! \brief  Same value as HWLOC_OBJ_PU to ease casting */
    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU,
    LOOP_ADAPT_SCOPE_MAX,
} AdaptScope;

/*! \brief  Return a string describing the loop_adapt type */
static char* loop_adapt_type_name(AdaptScope scope)
{
    switch(scope)
    {
        case LOOP_ADAPT_SCOPE_MACHINE:
            return "Machine";
        case LOOP_ADAPT_SCOPE_NUMA:
            return "NUMA node";
        case LOOP_ADAPT_SCOPE_SOCKET:
            return "Socket";
        case LOOP_ADAPT_SCOPE_THREAD:
            return "Thread";
        default:
            return "Invalid type";
    }
    return "Invalid type";
}
/*! \brief  List of types a parameter can have.

Currently only integer and double parameters are supported.
*/
typedef enum {
    /*! \brief  Integer parameter */
    NODEPARAMETER_INT = 0,
    /*! \brief  Double parameter */
    NODEPARAMETER_DOUBLE,
} Nodeparametertype;
/** @}*/

#define REGISTER_LOOP(string) loop_adapt_register(string);

#define REGISTER_POLICY(string, policy, num_profiles, iters_per_profile) \
    loop_adapt_register_policy(string, policy, num_profiles, iters_per_profile);
    
#define REGISTER_SEARCHALGO(string, policy, search) \
    loop_adapt_register_searchalgo(string, policy, search);

#define REGISTER_PARAMETER(string, name, scope, cpu, type, cur, min, max) \
    if (type == NODEPARAMETER_INT) \
        loop_adapt_register_int_param(string, name, scope, cpu, NULL, cur, min, max); \
    else if (type == NODEPARAMETER_DOUBLE) \
        loop_adapt_register_double_param(string, name, scope, cpu, NULL, cur, min, max);

#define GET_INT_PARAMETER(string, name, orig, scope, cpu) \
    orig = loop_adapt_get_int_param(string, scope, cpu, name);

#define GET_DBL_PARAMETER(string, name, orig, scope, cpu) \
    orig = loop_adapt_get_double_param(string, scope, cpu, name);

#define LOOP_BEGIN(string) loop_adapt_begin(string, __FILE__, __LINE__)
#define LOOP_END(string) loop_adapt_end(string)


#define LA_FOR(name, start, cond, inc) \
    for (start; cond && LOOP_BEGIN(name); inc, LOOP_END(name))

#define LA_FOR_BEGIN(name, ...) \
    for (__VA_ARGS__) \
    { \
        LOOP_BEGIN(name);

#define LA_FOR_END(name) \
        LOOP_END(name); \
    }

#define LA_WHILE(name, cond) \
    while (LOOP_BEGIN(name) && (cond) && LOOP_END(name))

#define LA_DO(name) \
    LOOP_BEGIN(name); do

#define REGISTER_THREAD_COUNT_FUNC(func) \
    loop_adapt_register_cpucount_func(func);

#define REGISTER_THREAD_ID_FUNC(func) \
    loop_adapt_register_getcpu_func(func);

#define REGISTER_EVENT(s, scope, cpu, name, var, type, ptr) \
    loop_adapt_register_event(s, scope, cpu, name, var, type, ptr);


/** \addtogroup LA_API loop_adapt API
*  @{
*/
/*! \brief  Register a function that returns the number of active threads/tasks/...

The function is used internally to detect whether the loop is executed serially (with a parallel region in the loop body) or in parallel.
For OpenMP you can use omp_get_num_threads()
*/
void loop_adapt_register_cpucount_func(int (*handle)());
/*! \brief  Register a function that returns the current CPU id
*/
void loop_adapt_register_getcpu_func(int (*handle)());
/*! \brief  Get the function pointer of function which returns the number of active threads/tasks/...
*/
void loop_adapt_get_cpucount_func(int (**handle)());
/*! \brief  Get the function pointer of function which returns  the current CPU id
*/
void loop_adapt_get_getcpu_func(int (**handle)());

/*! \brief  Register a new loop in the loop_adapt library

Each loop is handled separately, so you can have multiple loops that get adapted with the registered policies. It is _strongly_ recommended to use the macros.
*/
void loop_adapt_register(char* string);
/*! \brief  This function is called before a loop body executes

It is _strongly_ recommended to use the macros.
*/
int loop_adapt_begin(char* string, char* filename, int linenumber);
/*! \brief  This function is called after the execution of a loop body

It is _strongly_ recommended to use the macros.
*/
int loop_adapt_end(char* string);
/*! \brief  This function prints the profile with number profile_num of the loop named string
*/
void loop_adapt_print(char *string, int profile_num);



/*! \brief  Register a policy for a loop

Multiple policies can be registered for a loop and they are processed in order. It is _strongly_ recommended to use the macros.
*/
void loop_adapt_register_policy( char* string, char* polname, int num_profiles, int iters_per_profile);
/*! \brief  Register an integer parameter for a loop

These parameters are used as knobs for manipulating the runtime by the loop policies. It is _strongly_ recommended to use the macros.
*/
void loop_adapt_register_int_param( char* string,
                                    char* name,
                                    AdaptScope scope,
                                    int cpu,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max);

/*! \brief  Get the integer value for a loop parameter

This returns the current value of a parameter. During the profiling runs the value changes, so you should call this function before you setup the next loop iteration config. It is _strongly_ recommended to use the macros.
*/
int loop_adapt_get_int_param( char* string, AdaptScope scope, int cpu, char* name);
/*! \brief  Register a double parameter for a loop

These parameters are used as knobs for manipulating the runtime by the loop policies. It is _strongly_ recommended to use the macros.
*/
void loop_adapt_register_double_param( char* string,
                                       char* name,
                                       AdaptScope scope,
                                       int cpu,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max);

/*! \brief  Get the double value for a loop parameter

Get the double value for a loop parameter. This returns the current value of a parameter. During the profiling runs the value changes, so you should call this function before you setup the next loop iteration config. It is _strongly_ recommended to use the macros.
*/
double loop_adapt_get_double_param( char* string, AdaptScope scope, int cpu, char* name);

/*! \brief  Register a search algorithm to a policy.

Register a search algorithm to a policy. All afterwards registered parameters that are handled by a policy
are added to the search algorithm automatically. Currently this doesn't work with paramters registered before
calling this function (TODO).
*/
void loop_adapt_register_searchalgo( char* string, char* polname, char *searchname);

int loop_adapt_list_policy();
void loop_adapt_register_event(char* string, AdaptScope scope, int cpu, char* name, char* var, Nodeparametertype type, void* ptr);
/** @}*/
#else
#define REGISTER_LOOP(s)
#define REGISTER_POLICY(s, p, num_profiles, iters_per_profile)
#define REGISTER_SEARCHALGO(string, policy, search)
#define REGISTER_PARAMETER(s, scope, name, cpu, type, cur, min, max)
#define GET_INT_PARAMETER(s, orig, name, cpu) orig = orig;
#define GET_DBL_PARAMETER(s, orig, name, cpu)
#define LOOP_BEGIN(s)
#define LOOP_END(s)

#define LA_FOR(name, start, cond, inc) \
    for (start; cond; inc)

#define LA_FOR_BEGIN(name, ...) \
    for (__VA_ARGS__) {
#define LA_FOR_END(name) }

#define LA_WHILE(name, cond) while(cond)
#define LA_DO(name) do

#define REGISTER_THREAD_COUNT_FUNC(func)
#define REGISTER_THREAD_ID_FUNC(func)

#define REGISTER_EVENT(s, scope, cpu, name, var, type, ptr)

#endif

#ifdef __cplusplus
}
#endif

#endif
