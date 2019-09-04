#ifndef LOOP_ADAPT_H
#define LOOP_ADAPT_H

#include <hwloc.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file loop_adapt.h
 * @author Thomas Roehl
 * @date 19.08.2018
 * @brief File containing the main API of loop_adapt
 *
 * bla bla
 */

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
\def REGISTER_SEARCHALGO(string, policy, search)
Shortcut for loop_adapt_register_searchalgo(string, policy, search) if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/
/*!
\def LA_FOR(name, start, cond, inc)
Shortcut for for (start; cond && LOOP_BEGIN(name); inc, LOOP_END(name)) if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is resolved to for (start; cond; inc)
*/
/*!
\def REGISTER_PARAMETER(string, name, scope, cpu, type, cur, min, max)
Shortcut for loop_adapt_register_int/double_param if compiled with -DLOOP_ADAPT_ACTIVE based on the given type. Otherwise macro is empty
*/

/*!
\def GET_INT_PARAMETER(string, name, orig, scope, cpu)
Shortcut for loop_adapt_get_int_param if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/

/*!
\def GET_DBL_PARAMETER(string, name, orig, scope, cpu)
Shortcut for loop_adapt_get_double_param if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/

/*!
\def REGISTER_THREAD_COUNT_FUNC(func)
Shortcut for loop_adapt_register_cpucount_func if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/
/*!
\def REGISTER_THREAD_ID_FUNC(func)
Shortcut for loop_adapt_register_getcpu_func if compiled with -DLOOP_ADAPT_ACTIVE. Otherwise macro is empty
*/
/** @}*/




#ifdef LOOP_ADAPT_ACTIVE

/** \addtogroup LA_API loop_adapt API
*  @{
*/
int loop_adapt_debug; /*! \brief  Debug level */
int loop_adapt_active; /*! \brief Internal variable whether loop_adapt is active */

/*! \brief  List of topology entities where policies and parameters can be associated to */
typedef enum {
    LOOP_ADAPT_SCOPE_NONE = -1, /**< \brief  No type */
    LOOP_ADAPT_SCOPE_MACHINE = HWLOC_OBJ_MACHINE, /**< \brief  Same value as HWLOC_OBJ_MACHINE to ease casting */
    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE, /**< \brief  Same value as HWLOC_OBJ_PACKAGE to ease casting */
    LOOP_ADAPT_SCOPE_NUMA = HWLOC_OBJ_NUMANODE, /**< \brief  Same value as HWLOC_OBJ_NUMANODE to ease casting */
    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU, /**< \brief  Same value as HWLOC_OBJ_PU to ease casting */
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

Currently only integer and double parameters are supported. Internal values
can also take pointers.
*/
typedef enum {
    NODEPARAMETER_INT = 0, /**< \brief Integer parameter */
    NODEPARAMETER_DOUBLE, /**< \brief Double parameter */
    NODEPARAMETER_VOIDPTR, /**< \brief Double parameter */
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

#define REGISTER_PARAMETER_LIST(string, name, scope, cpu, type, num_values, values) \
    if (type == NODEPARAMETER_INT) \
        loop_adapt_register_int_paramlist(string, name, scope, cpu, NULL, num_values, (int*)values); \
    else if (type == NODEPARAMETER_DOUBLE) \
        loop_adapt_register_double_paramlist(string, name, scope, cpu, NULL, num_values, (double*)values); \
    else if (type == NODEPARAMETER_VOIDPTR) \
        loop_adapt_register_voidptr_paramlist(string, name, scope, cpu, NULL, num_values, (void**)values);

#define GET_INT_PARAMETER(string, name, orig, scope, cpu) \
    orig = loop_adapt_get_int_param(string, scope, cpu, name);

#define GET_DBL_PARAMETER(string, name, orig, scope, cpu) \
    orig = loop_adapt_get_double_param(string, scope, cpu, name);
    
#define GET_VOIDPTR_PARAMETER(string, name, orig, scope, cpu) \
    orig = loop_adapt_get_voidptr_param(string, scope, cpu, name);

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
    loop_adapt_register_tidcount_func(func);

#define REGISTER_THREAD_ID_FUNC(func) \
    loop_adapt_register_gettid_func(func);

#define REGISTER_CPU_ID_FUNC(func) \
    loop_adapt_register_getcpuid_func(func);

#define REGISTER_EVENT(s, scope, cpu, name, var, type, ptr) \
    loop_adapt_register_event(s, scope, cpu, name, var, type, ptr);


/** \addtogroup LA_API loop_adapt API
*  @{
*/
/*! \brief Register a function that return the number of processing units
(e.g. threads or tasks)

For OpenMP you can use omp_get_num_threads().
@param handle function pointer (int(*)()) to a function returning the number of processing units
 */
void loop_adapt_register_tidcount_func(int (*handle)());

/*! \brief Register a function that return the ID of the calling processing unit
(e.g. thread or task)

@param handle function pointer (int(*)()) to a function returning the ID of the calling processing unit
*/
void loop_adapt_register_gettid_func(int (*handle)());

/*! \brief Register a function that return the ID of the current CPU

@param handle function pointer (int(*)()) to a function returning the ID of the current CPU
*/
void loop_adapt_register_getcpuid_func(int (*handle)());

/*! \brief Get the function pointer to retrieve the number of processing units */
void loop_adapt_get_tidcount_func(int (**handle)());

/*! \brief Get the function pointer to retrieve the identifier of a processing
unit */
void loop_adapt_get_gettid_func(int (**handle)());

/*! \brief Get the function pointer to retrieve the identifier of a processing
unit */
void loop_adapt_get_getcpuid_func(int (**handle)());

/*! \brief Register a new loop to loop_adapt

This function adds a new loop to loop_adapt. This is done by duplicating the
base topology tree and register the tree in the global hash table using the loop
name as key.
@param string Loop identifier
*/
void loop_adapt_register(char* string);



/*! \brief Register a policy for a loop 

This function adds a new policy to a loop identified by the loop string. The
policy is searched in the current binary using dlsym to be more flexible (extra
shared libs per policy to add them without recompilation). A policy will take
num_profiles profiles with iters_per_profile iterations per profile.

@param string Loop identifier
@param polname Name of the policy
@param num_profiles Number of profiles for this policy
@param iters_per_profile Number of iterations for each profile
*/
void loop_adapt_register_policy( char* string, char* polname, int num_profiles, int iters_per_profile);

/*! \brief Register a search algorithm for policy in a loop 

This function adds a new search algorithm to a policy in a loop both identified
by strings. After resolving the appropriate policy, it checks whether the search
algorithm exists and if yes, it adds it to the policy.

NOTE: Currently, the search algorithm is per policy. I thought it might be better to
have it per parameter. Was too complicated for now, so I did it per policy.
*/
void loop_adapt_register_searchalgo( char* string, char* polname, char *searchname);

/*! \brief  This function is called at the beginning of each loop iteration.

At the beginning of each loop body execution this function should be called. If looks
up the given loop name string in the hash table to get the topology tree
associated with the loop. If this function is called in parallel, it operates
only for the current thread. Otherwise it operates on all threads.
If there is already a profile to analyse (cur_policy > 0) it calls the
evaluation function of the current policy on the profile (comparing the new
profile with either base or the current optimum).
The function itself does not perform any measurements or anything, it just calls
the begin-function of the current policy of the loop.
*/
int loop_adapt_begin(char* string, char* filename, int linenumber);

/*! \brief  This function is called at the end of each loop iteration.

At the end of each loop body execution this function should be called. If looks
up the given loop name string in the hash table to get the topology tree
associated with the loop. If this function is called in parallel, it operates
only for the current thread. Otherwise it operates on all threads. The function
itself does not perform any measurements or anything, it just calls the
end-function of the current policy of the loop.
*/
int loop_adapt_end(char* string);

/*! \brief Register an integer parameter in the loop's topology tree

Register an integer parameter for later manipulation through a policy It is
save to register also unused parameters. It is assumed that each thread is
registering the parameter for itself. If the scope is not thread, it walks
up the tree until it found the parent node of the CPU with the proper scope.
 */
void loop_adapt_register_int_param( char* string,
                                    char* name,
                                    AdaptScope scope,
                                    int cpu,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max);

void loop_adapt_register_int_paramlist( char* string,
                                        char* name,
                                        AdaptScope scope,
                                        int cpu,
                                        char* desc,
                                        int num_values,
                                        int* values);

/*! \brief Get an integer parameter from the loop tree

This function searches for the parameter of the current CPU (or other scope if
known) and returns the current value of the paramter. At first it resolves the
loop name to the loop's topology tree. Afterwards it gets the tree node id to
access the right node. If the scope it thread, just look up the id in the
cpu-to-id mapping. If not, use the current tree node as starting point to walk
up the tree until you find the appropriate tree node of scope. When found,
look up the parameter name in the node's parameter hash and return the value.
*/
int loop_adapt_get_int_param( char* string, AdaptScope scope, int cpu, char* name);


/*! \brief Register a double parameter in the loop's topology tree

Register an double parameter for later manipulation through a policy It is
save to register also unused parameters. It is assumed that each thread is
registering the parameter for itself. If the scope is not thread, it walks
up the tree until it found the parent node of the CPU with the proper scope.
 */
void loop_adapt_register_double_param( char* string,
                                       char* name,
                                       AdaptScope scope,
                                       int cpu,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max);

void loop_adapt_register_double_paramlist( char* string,
                                        char* name,
                                        AdaptScope scope,
                                        int cpu,
                                        char* desc,
                                        int num_values,
                                        double* values);
                                        

/*! \brief Get a double parameter from the loop tree

This function searches for the parameter of the current CPU (or other scope if
known) and returns the current value of the paramter. At first it resolves the
loop name to the loop's topology tree. Afterwards it gets the tree node id to
access the right node. If the scope it thread, just look up the id in the
cpu-to-id mapping. If not, use the current tree node as starting point to walk
up the tree until you find the appropriate tree node of scope. When found,
look up the parameter name in the node's parameter hash and return the value.
*/
double loop_adapt_get_double_param( char* string, AdaptScope scope, int cpu, char* name);

/*! \brief Register a void* parameter in the loop's topology tree

Register an void* parameter for later manipulation through a policy It is
save to register also unused parameters. It is assumed that each thread is
registering the parameter for itself. If the scope is not thread, it walks
up the tree until it found the parent node of the CPU with the proper scope.
 */
void loop_adapt_register_voidptr_paramlist( char* string,
                                            char* name,
                                            AdaptScope scope,
                                            int cpu,
                                            char* desc,
                                            int num_values,
                                            void** values);


/*! \brief Get a void* parameter from the loop tree

This function searches for the parameter of the current CPU (or other scope if
known) and returns the current value of the paramter. At first it resolves the
loop name to the loop's topology tree. Afterwards it gets the tree node id to
access the right node. If the scope it thread, just look up the id in the
cpu-to-id mapping. If not, use the current tree node as starting point to walk
up the tree until you find the appropriate tree node of scope. When found,
look up the parameter name in the node's parameter hash and return the value.
*/
void* loop_adapt_get_voidptr_param( char* string, AdaptScope scope, int cpu, char* name);


//int loop_adapt_list_policy();
//void loop_adapt_register_event(char* string, AdaptScope scope, int cpu, char* name, char* var, Nodeparametertype type, void* ptr);
/** @}*/
#else /* LOOP_ADAPT_ACTIVE */
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

#endif /* LOOP_ADAPT_ACTIVE */

#ifdef __cplusplus
}
#endif

#endif /* LOOP_ADAPT_H */
