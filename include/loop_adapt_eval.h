#ifndef LOOP_ADAPT_EVAL_H
#define LOOP_ADAPT_EVAL_H

#include <hwloc.h>
#include <loop_adapt_types.h>



int loop_adapt_add_policy(hwloc_topology_t tree, Policy *p, int num_cpus, int *cpulist, int num_profiles);
int loop_adapt_begin_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_end_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_exec_policies(hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_add_int_parameter(hwloc_obj_t obj, char* name, char* desc, int cur, int min, int max, void(*pre)(char* fmt, ...), void(*post)(char* fmt, ...));
int loop_adapt_add_double_parameter(hwloc_obj_t obj, char* name, char* desc, double cur, double min, double max, void(*pre)(char* fmt, ...), void(*post)(char* fmt, ...));


#endif
