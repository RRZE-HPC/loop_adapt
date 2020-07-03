#ifndef LOOP_ADAPT_HWLOC_TREE_H
#define LOOP_ADAPT_HWLOC_TREE_H

#include <hwloc.h>

int loop_adapt_copy_hwloc_tree(hwloc_topology_t* copy);
void loop_adapt_copydestroy_hwloc_tree(hwloc_topology_t tree);
void loop_adapt_hwloc_tree_finalize();

#endif /* LOOP_ADAPT_HWLOC_TREE_H */
