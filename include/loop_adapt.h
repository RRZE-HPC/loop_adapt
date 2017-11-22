#ifndef LOOP_ADAPT_H
#define LOOP_ADAPT_H

#include <hwloc.h>

#ifdef __cplusplus
extern "C"
{
#endif

//#include <loop_adapt_types.h>
//#include <loop_adapt_eval.h>
static int loop_adapt_debug = 0;

typedef enum {
    LOOP_ADAPT_SCOPE_NONE = -1,
    LOOP_ADAPT_SCOPE_MACHINE = HWLOC_OBJ_MACHINE,
    LOOP_ADAPT_SCOPE_NUMA = HWLOC_OBJ_NUMANODE,
    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU,
    LOOP_ADAPT_SCOPE_MAX,
} AdaptScope;

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

typedef enum {
    NODEPARAMETER_INT = 0,
    NODEPARAMETER_DOUBLE,
} Nodeparametertype;

#define REGISTER_LOOP(s) loop_adapt_register(s);
#define REGISTER_POLICY(s, p, num_profiles, iters_per_profile) \
    loop_adapt_register_policy((s), (p), (num_profiles), (iters_per_profile));
#define REGISTER_PARAMETER(s, scope, name, cpu, type, cur, min, max) \
    if (type == NODEPARAMETER_INT) \
        loop_adapt_register_int_param((s), (scope), (cpu), (name), NULL, (cur), (min), (max)); \
    else if (type == NODEPARAMETER_DOUBLE) \
        loop_adapt_register_double_param((s), (scope), (cpu), (name), NULL, (cur), (min), (max));

#define GET_INT_PARAMETER(s, name, cpu) \
    loop_adapt_get_int_param((s), LOOP_ADAPT_SCOPE_THREAD, (cpu), (name));
#define GET_DBL_PARAMETER(s, name, cpu) \
    loop_adapt_get_double_param((s), LOOP_ADAPT_SCOPE_THREAD, (cpu), (name));

#define LOOP_BEGIN(s) loop_adapt_begin((s), __FILE__, __LINE__)
#define LOOP_END(s) loop_adapt_end((s))


#define LA_FOR(name, start, cond, inc) \
    for ((start); (cond) && LOOP_BEGIN((name)); (inc), LOOP_END((name)))

#define LA_WHILE(name, cond) \
    while (LOOP_BEGIN((name)) && (cond) && LOOP_END((name)))

#define LA_DO(name) \
    LOOP_BEGIN((name)); do


// Register a new loop in the loop_adapt library
// You have to give the name in the loop macros LA_WHILE, LA_FOR, LA_DO
void loop_adapt_register(char* string);
// This function is called before a loop body executes
int loop_adapt_begin(char* string, char* filename, int linenumber);
// This function is called after the execution of a loop body
int loop_adapt_end(char* string);
void loop_adapt_print(char *string, int profile_num);

void loop_adapt_register_tcount_func(int (*handle)());
void loop_adapt_get_tcount_func(int (**handle)());
void loop_adapt_register_tid_func(int (*handle)());
void loop_adapt_get_tid_func(int (**handle)());

// Register a policy for a loop. Multiple policies can be registered for a loop
// and they are processed in order
void loop_adapt_register_policy( char* string, char* polname, int num_profiles, int iters_per_profile);
// Register a parameter in the topology tree belonging to a loop.
// These parameters are used as knobs for manipulating the runtime by the loop
// policies.
void loop_adapt_register_int_param( char* string,
                                    AdaptScope scope,
                                    int cpu,
                                    char* name,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max);
int loop_adapt_get_int_param( char* string, AdaptScope scope, int cpu, char* name);
void loop_adapt_register_double_param( char* string,
                                       AdaptScope scope,
                                       int cpu,
                                       char* name,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max);
double loop_adapt_get_double_param( char* string, AdaptScope scope, int cpu, char* name);

int loop_adapt_list_policy();

#ifdef __cplusplus
}
#endif

#endif
