#ifndef LOOP_ADAPT_EVAL_DVFS_H
#define LOOP_ADAPT_EVAL_DVFS_H


#include <loop_adapt_eval.h>

POL_FUNCS(ompthreads)


Policy POL_OMPTHREADS = {
    .likwid_group = "MEMTEST",
    .name = "OMPTHREADS",
    .desc = "This policy tries to adapt number of OpenMP threads.",
    STR_POL_FUNCS(ompthreads)
    .scope = LOOP_ADAPT_SCOPE_MACHINE,
    
    .num_parameters = 1,
    .parameters = { {"nthreads",
                     "Number of threads",
                     "1",
                     "AVAILABLE_CPUS",
                     "time_opt < time_cur"} },
    .num_metrics = 2,
    .metrics = {{ "time", "Runtime (RDTSC) [s]"},
                { "energy", "Energy PKG [W]"}},
};



#endif
