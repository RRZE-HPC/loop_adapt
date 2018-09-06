#ifndef LOOP_ADAPT_POLICY_H
#define LOOP_ADAPT_POLICY_H

#include <hwloc.h>
#include <loop_adapt_types.h>

#define POL_FUNCS(NAME)  \
    int loop_adapt_policy_ ## NAME##_init(int num_cpus, int* cpulist, int num_profiles); \
    int loop_adapt_policy_ ## NAME##_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj); \
    int loop_adapt_policy_ ## NAME##_eval(hwloc_topology_t tree, hwloc_obj_t obj); \
    int loop_adapt_policy_ ## NAME##_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj); \
    void loop_adapt_policy_ ## NAME##_close(); \

#define STR_POL_FUNCS(NAME) \
    .loop_adapt_policy_eval = loop_adapt_policy_## NAME##_eval, \
    .loop_adapt_policy_init = loop_adapt_policy_## NAME##_init, \
    .loop_adapt_policy_begin = loop_adapt_policy_##NAME##_begin, \
    .loop_adapt_policy_end = loop_adapt_policy_## NAME##_end, \
    .loop_adapt_policy_close = loop_adapt_policy_## NAME##_close, \

extern GHashTable* loop_adapt_global_hash;

int loop_adapt_add_policy(hwloc_topology_t tree, Policy *p, int num_cpus, int *cpulist, int num_profiles, int iters_per_profile);
int loop_adapt_begin_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_end_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_exec_policies(hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_add_int_parameter(hwloc_obj_t obj, char* name, char* desc, int cur, int min, int max);
int loop_adapt_add_double_parameter(hwloc_obj_t obj, char* name, char* desc, double cur, double min, double max);

int loop_adapt_init_parameter(hwloc_obj_t obj, Policy_t policy);

TimerData* loop_adapt_policy_profile_get_timer(PolicyProfile_t pp);
double* loop_adapt_policy_profile_get_runtime(PolicyProfile_t pp);
double* loop_adapt_policy_profile_get_values(PolicyProfile_t pp);


#endif
