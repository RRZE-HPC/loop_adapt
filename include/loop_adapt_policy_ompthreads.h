#ifndef LOOP_ADAPT_EVAL_DVFS_H
#define LOOP_ADAPT_EVAL_DVFS_H


#include <loop_adapt_policy.h>

POL_FUNCS(ompthreads)

int loop_adapt_policy_ompthreads_post_param(char* location, Nodeparameter_t param);

Policy POL_OMPTHREADS = {
    .likwid_group = "ENERGY",
    .internalname = "POL_OMPTHREADS",
    .name = "OMPTHREADS",
    .desc = "This policy tries to adapt number of OpenMP threads.",
    STR_POL_FUNCS(ompthreads)
    .scope = LOOP_ADAPT_SCOPE_MACHINE,
    
    .num_parameters = 1,
    .parameters = { {"nthreads",
                     "Number of threads",
                     "1",
                     "AVAILABLE_CPUS",
                     "time_cur < time_opt",
                     NULL,
                     loop_adapt_policy_ompthreads_post_param} },
    .num_metrics = 1,
    .metrics = {{ "time", "Runtime (RDTSC) [s]"}},
};



#endif
