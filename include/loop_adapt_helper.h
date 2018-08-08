#ifndef LOOP_ADAPT_HELPER_H
#define LOOP_ADAPT_HELPER_H

#include <loop_adapt_types.h>
#include <syscall.h>

//int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int num_profiles, int num_values);

#define MIN(a,b) ( (a) < (b) ? (a) : (b))
#define MAX(a,b) ( (a) >= (b) ? (a) : (b))
#define gettid() syscall(SYS_gettid)

void *realloc_buffer(void *ptrmem, size_t size);

int populate_tree(hwloc_topology_t tree, int polidx, int num_profiles, int profile_iterations);
void update_cur_policy_in_tree(hwloc_topology_t tree, int polidx);
void tidy_tree(hwloc_topology_t tree);
void update_tree(hwloc_obj_t obj, int profile);
AdaptScope getLoopAdaptType(hwloc_obj_type_t t);

double get_param_def_min(hwloc_topology_t tree, char* name);
double get_param_def_max(hwloc_topology_t tree, char* name);

int get_objidx_above_cpuidx(hwloc_topology_t tree, AdaptScope scope, int cpuidx);
void update_best(PolicyParameter_t pp, hwloc_obj_t baseobj, hwloc_obj_t setobj);

void set_param_min(char* param, Nodevalues_t vals);
void set_param_max(char* param, Nodevalues_t vals);
void set_param_step(char* param, Nodevalues_t vals, int step);



#endif
