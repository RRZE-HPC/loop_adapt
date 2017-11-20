#ifndef LOOP_ADAPT_EVAL_DVFS_H
#define LOOP_ADAPT_EVAL_DVFS_H


#include <loop_adapt_eval.h>

POL_FUNCS(dvfs)


Policy POL_DVFS = {
    .likwid_group = "L3",
    .name = "DVFS",
    .desc = "This policy tries to adapt the cpu and uncore frequency.",
    STR_POL_FUNCS(dvfs)
    .scope = LOOP_ADAPT_SCOPE_SOCKET,
    
    .num_parameters = 2,
    .parameters = { {"cfreq",
                     "CPU frequency",
                     "MIN_CPUFREQ/1000000000",
                     "MAX_CPUFREQ/1000000000",
                     "mem_cur < mem_opt"},
                    {"ufreq",
                     "Uncore frequency",
                     "MIN_UNCOREFREQ/1000000000",
                     "MAX_UNCOREFREQ/1000000000"} },
    .num_metrics = 2,
    .metrics = {{ "time", "Runtime (RDTSC) [s]"},
                { "mem", "Memory bandwidth [MByte/s]"} },
};



#endif
