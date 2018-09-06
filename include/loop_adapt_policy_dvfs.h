#ifndef LOOP_ADAPT_EVAL_DVFS_H
#define LOOP_ADAPT_EVAL_DVFS_H


#include <loop_adapt_policy.h>

POL_FUNCS(dvfs)
int loop_adapt_policy_dvfs_post_param_cpu(char* location, Nodeparameter_t param);
int loop_adapt_policy_dvfs_post_param_uncore(char* location, Nodeparameter_t param);

Policy POL_DVFS = {
    .likwid_group = "L3",
    .internalname = "POL_DVFS",
    .name = "DVFS",
    .desc = "This policy tries to adapt the cpu and uncore frequency.",
    STR_POL_FUNCS(dvfs)
    .scope = LOOP_ADAPT_SCOPE_SOCKET,
    
    .num_parameters = 2,
    .parameters = { {"cfreq",
                     "CPU frequency",
                     "MIN_CPUFREQ/1000000000",
                     "MAX_CPUFREQ/1000000000",
                     "data_cur < 0.2 and data_opt < data_cur and mem_cur < mem_opt",
                     NULL,
                     loop_adapt_policy_dvfs_post_param_cpu},
                    {"ufreq",
                     "Uncore frequency",
                     "MIN_UNCOREFREQ/1000000000",
                     "MAX_UNCOREFREQ/1000000000",
                     "data > 0.2 and data_opt > data_cur and mem_cur > mem_opt",
                     NULL,
                     loop_adapt_policy_dvfs_post_param_uncore} },
    .num_metrics = 3,
    .metrics = {{ "time", "Runtime (RDTSC) [s]"},
                { "mem", "Memory bandwidth [MBytes/s]"},
                { "data", "Data access ratio"} },
};



#endif
