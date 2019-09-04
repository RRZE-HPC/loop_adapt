#ifndef LOOP_ADAPT_CONFIG_H
#define LOOP_ADAPT_CONFIG_H

int loop_adapt_write_profiles(char* loopname, hwloc_topology_t tree);
int loop_adapt_write_parameters(char* loopname, hwloc_topology_t tree);

int loop_adapt_read_parameters(char* loopname, hwloc_topology_t tree);

#endif
