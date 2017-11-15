#ifndef LOOP_ADAPT_EVAL_BLOCKSIZE_H
#define LOOP_ADAPT_EVAL_BLOCKSIZE_H


#include <loop_adapt_eval.h>
#include <loop_adapt_helper.h>

int loop_adapt_eval_blocksize_init(int num_cpus, int* cpulist, int num_profiles);
int loop_adapt_eval_blocksize_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
void loop_adapt_eval_blocksize(hwloc_topology_t tree, hwloc_obj_t obj);
int loop_adapt_eval_blocksize_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
void loop_adapt_eval_blocksize_close();

Policy POL_BLOCKSIZE = {
    .likwid_group = "CACHES",
    .name = "Loop Blocksize",
    .desc = "This policy tries to adapt the blocksize of a loop to improve locality in the caches.",
    .loop_adapt_eval = loop_adapt_eval_blocksize,
    .loop_adapt_eval_init = loop_adapt_eval_blocksize_init,
    .loop_adapt_eval_begin = loop_adapt_eval_blocksize_begin,
    .loop_adapt_eval_end = loop_adapt_eval_blocksize_end,
    .loop_adapt_eval_close = loop_adapt_eval_blocksize_close,
    .scope = LOOP_ADAPT_SCOPE_THREAD,
    
    .num_parameters = 1,
    .parameters = { {"blksize", "Variable that needs to be incremented"} },
    .num_metrics = 2,
    .metrics = {{ "l1l2", "L2 to L1 load bandwidth [MBytes/s]"},
                { "l2l3", "L3 to L2 load bandwidth [MBytes/s]"},
               }
};


#endif
