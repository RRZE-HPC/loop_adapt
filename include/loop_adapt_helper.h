#ifndef LOOP_ADAPT_HELPER_H
#define LOOP_ADAPT_HELPER_H

#include <loop_adapt_types.h>

//int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int num_profiles, int num_values);
int populate_tree(hwloc_topology_t tree, int num_profiles);
void tidy_tree(hwloc_topology_t tree);
void update_tree(hwloc_obj_t obj, int profile);
AdaptScope getLoopAdaptType(hwloc_obj_type_t t);

double get_param_def_min(hwloc_topology_t tree, char* name);
double get_param_def_max(hwloc_topology_t tree, char* name);

void update_best(Policy_t p, hwloc_obj_t baseobj, hwloc_obj_t setobj);

void set_param_min(char* param, Nodevalues_t vals);
void set_param_max(char* param, Nodevalues_t vals);
void set_param_step(char* param, Nodevalues_t vals, int step);

#endif
