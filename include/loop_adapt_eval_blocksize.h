#ifndef LOOP_ADAPT_EVAL_BLOCKSIZE_H
#define LOOP_ADAPT_EVAL_BLOCKSIZE_H


#include <loop_adapt_eval.h>
#include <loop_adapt_helper.h>


POL_FUNCS(blocksize)

Policy POL_BLOCKSIZE = {
    .likwid_group = "CACHES",
    .name = "Loop Blocksize",
    .desc = "This policy tries to adapt the blocksize of a loop to improve locality in the caches.",
    STR_POL_FUNCS(blocksize)
    .scope = LOOP_ADAPT_SCOPE_THREAD,
    
    .num_parameters = 1,
    .parameters = { {"blksize",
                     "Variable that needs to be incremented",
                     "L1_SIZE/SIZEOF_DOUBLE",
                     "L3_SIZE/SIZEOF_DOUBLE"} },
    .num_metrics = 2,
    .metrics = {{ "time", "Runtime (RDTSC) [s]"},
                { "l2l3", "L2 to/from L3 data volume [GBytes]"},
               },
    .eval = "l2l3_opt > l2l3_cur "
};


#endif
