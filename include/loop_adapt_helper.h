#ifndef LOOP_ADAPT_HELPER_H
#define LOOP_ADAPT_HELPER_H

//int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int num_profiles, int num_values);
int populate_tree(hwloc_topology_t tree, Policy_t pol, int num_profiles);
void tidy_tree(hwloc_topology_t tree);
void update_tree(hwloc_obj_t obj, int profile);
double loop_adapt_round(double val);


int set_param_min(char* param, Nodevalues_t vals);
int set_param_max(char* param, Nodevalues_t vals);
int set_param_step(char* param, Nodevalues_t vals, int step);

#endif
