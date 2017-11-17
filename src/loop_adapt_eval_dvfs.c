#include <stdint.h>
#include <loop_adapt_eval_dvfs.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;


static int dvfs_num_cpus = 0;
static int* dvfs_cpus = NULL;

static char* avail_freqs = NULL;
static int num_freqs = 0;
static double* freqs = NULL;

static void loop_adapt_eval_dvfs_next_param_step(char* param, Nodevalues_t vals, int step, int cpu)
{
    Nodeparameter_t np = g_hash_table_lookup(vals->param_hash, (gpointer)param);
    if (np)
    {
    
        double min = np->min.dmin;
        double max = np->max.dmax;
        int s = num_freqs / np->inter;
        np->cur.dcur = freqs[(int)(s*step)];
        if (strncmp(param, "cfreq", 5) == 0)
        {
            uint64_t f = (uint64_t)(np->cur.dcur*1000000);
            freq_setCpuClockMin(cpu, f);
            freq_setCpuClockMax(cpu, f);
            if (loop_adapt_debug)
                printf("Setting core frequency to %lu MHz\n", f);
        }
/*        else if (strncmp(param, "ufreq", 5) == 0)*/
/*        {*/
/*            CpuTopology_t cputopo = get_cpuTopology();*/
/*            uint64_t f = (uint64_t)(np->cur.dcur*1000000);*/
/*            freq_setUncoreFreqMin(cputopo->threadPool[cpu].packageId, f);*/
/*            freq_setUncoreFreqMax(cputopo->threadPool[cpu].packageId, f);*/
/*            if (loop_adapt_debug)*/
/*                printf("Setting uncore frequency to %lu MHz\n", f);*/
/*        }*/
    }
}

static void parse_avail_freqs(char* s)
{
    int num_f = 0;
    const char split[2] = " ";
    char *token, *eptr;
    
    for (int i = 0; i < strlen(s); i++)
    {
        if (s[i] == '.')
            num_f++;
    }
    num_freqs = num_f;
    freqs = malloc(num_f * sizeof(double));
    memset(freqs, 0, num_f * sizeof(double));
    num_f = 0;
    token = strtok(s, split);
    while(token)
    {
        double f = strtod(token, &eptr);
        if (token != eptr)
        {
            freqs[num_f] = f;
            printf("%d -> %f MHz\n", num_f, freqs[num_f]);
            num_f++;
        }
        token = strtok(NULL, split);
    }
}

int loop_adapt_eval_dvfs_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    //affinity_init();
    timer_init();
    //perfmon_init(num_cpus, cpulist);
    int npros = num_profiles;
    npros++;
    
    char minclock[20];
    char maxclock[20];
    char minuclock[20];
    char maxuclock[20];
    snprintf(minclock, 19, "MIN_CPUFREQ");
    snprintf(maxclock, 19, "MAX_CPUFREQ");
    snprintf(minuclock, 19, "MIN_UNCOREFREQ");
    snprintf(maxuclock, 19, "MAX_UNCOREFREQ");
    CpuTopology_t cputopo = get_cpuTopology();
    
    parse_avail_freqs(freq_getAvailFreq(cpulist[0]));
    
    for (int c = 0; c < num_cpus; c++)
    {
        if (loop_adapt_debug == 2)
        {
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", minclock, freqs[0],  cpulist[c]);
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", maxclock, freqs[num_freqs-1],  cpulist[c]);
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", minuclock, freqs[0],  cpulist[c]);
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", maxuclock, freqs[num_freqs-1],  cpulist[c]);
        }
        la_calc_add_def(minclock, freqs[0], cpulist[c]);
        la_calc_add_def(maxclock, freqs[num_freqs-1], cpulist[c]);
        la_calc_add_def(minuclock, freqs[0], cpulist[c]);
        la_calc_add_def(maxuclock, freqs[num_freqs-1], cpulist[c]);
    }
    
    dvfs_num_cpus = num_cpus;
    dvfs_cpus = (int*)malloc(dvfs_num_cpus * sizeof(int));
    if (!dvfs_cpus)
        return -ENOMEM;
    memcpy(dvfs_cpus, cpulist, dvfs_num_cpus * sizeof(int));
    
    
    return 0;
}

void loop_adapt_eval_dvfs(hwloc_topology_t tree, hwloc_obj_t obj)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t v = (Nodevalues_t)obj->userdata;
    int cur_profile = v->cur_profile - 1;
    Policy_t p = tdata->cur_policy;
    int opt_profile = p->optimal_profile;
    double *cur_values = v->profiles[v->cur_policy][cur_profile];
    double *opt_values = v->profiles[v->cur_policy][opt_profile];
    unsigned int (*tcount_func)() = NULL;

    // Evaluate the condition whether this profile is better than the old
    // optimal one
    // evaluate() returns 1 if better, 0 if not
    for (int i = 0; i < p->num_parameters; i++)
    {
        PolicyParameter_t pp = &p->parameters[i];
        int eval = la_calc_evaluate(p, pp, opt_values, cur_values);

        if (eval)
        {
            p->optimal_profile = cur_profile;
            // we search through all parameters and set the best setting to the
            // current setting. After the policy is completely evaluated, the
            // best setting is returned by GET_[INT/DBL]_PARAMETER
            update_best(pp, obj, obj);

            // Check the current thread count whether the setting of this object
            // should be propagated to the other ones.
            // Often the LA_LOOP macros are used in serial regions and therefore
            // the best parameter is set for all other objects of the scope.
            loop_adapt_get_tcount_func(&tcount_func);
            if (tcount_func && tcount_func() == 1)
            {
                int len = hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)p->scope);
                for (int k = 0; k < len; k++)
                {
                    hwloc_obj_t new = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)p->scope, k);
                    if (new == obj)
                    {
                        continue;
                    }
                    update_best(pp, obj, new);
                }
            }
        }
        
        // If we are in the scope of the policy (at which level should changes be
        // done) the parameters are adjusted.
        if (getLoopAdaptType(obj->type) == p->scope)
        {
/*            for (int i = 0; i < POL_DVFS.num_parameters; i++)*/
/*            {*/
                char* pname = pp->name;
                loop_adapt_eval_dvfs_next_param_step(pname, v, v->cur_profile-1, obj->os_index);
/*            }*/
        }
    }

    // Walk up the tree to see whether adjustments are required.
    // Currently this does not wait until all subnodes have updated the
    // values.
    hwloc_obj_t walker = obj->parent;
    while (walker)
    {
        if (walker->type == HWLOC_OBJ_PACKAGE ||
            walker->type == HWLOC_OBJ_NUMANODE ||
            walker->type == HWLOC_OBJ_MACHINE)
        {
            loop_adapt_eval_dvfs(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return;
}

int loop_adapt_eval_dvfs_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        if (!likwid_initialized)
        {
            perfmon_init(dvfs_num_cpus, dvfs_cpus);
        }
        if (!likwid_configured)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Adding group %s to LIKWID\n", POL_DVFS.likwid_group);
            likwid_gid = perfmon_addEventSet(POL_DVFS.likwid_group);
            if (likwid_gid >= 0)
            {
                // We store which indicies we need later for evaluation. We can also use
                // special performance groups that offer only the required metrics but
                // with this way we can use default performance groups.
                for (int m = 0; m < POL_DVFS.num_metrics; m++)
                {
                    for (int i = 0; i < perfmon_getNumberOfMetrics(likwid_gid); i++)
                    {
                        if (strcmp(POL_DVFS.metrics[m].name, perfmon_getMetricName(likwid_gid, i)) == 0)
                        {
                            POL_DVFS.metric_idx[m] = i;
                            if (loop_adapt_debug == 2)
                                fprintf(stderr, "DEBUG: Using metric with ID %d\n", i);
                        }
                    }
                }
                likwid_configured = 1;
            }
        }
        if (likwid_configured && likwid_gid < 0 && !likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Setup LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_setupCounters(likwid_gid);
            if (ret < 0)
            {
                printf("Cannot setup LIKWID counters for group ID %d\n", likwid_gid);
            }
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Starting LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_startCounters();
            if (ret < 0)
            {
                printf("Cannot start LIKWID counters for group ID %d\n", likwid_gid);
            }
            likwid_started = 1;
        }
        TimerData *t = vals->runtimes[vals->cur_policy] + vals->cur_profile;
        timer_start(t);
        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Reading LIKWID counters on CPU %d\n",cpuid);
            ret = perfmon_readCountersCpu(cpuid);
        }
    }
    return ret;
}

int loop_adapt_eval_dvfs_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        TimerData *t = vals->runtimes[vals->cur_policy] + vals->cur_profile;
        if (likwid_started)
        {
            ret = perfmon_readCountersCpu(cpuid);
            timer_stop(t);
            Policy_t p = tdata->cur_policy;
            int max_vals = perfmon_getNumberOfMetrics(p->likwid_gid);
            double* pdata = vals->profiles[vals->cur_policy][vals->cur_profile];
            for (int m = 0; m < max_vals; m++)
            {
                pdata[m] = perfmon_getLastMetric(p->likwid_gid, m, obj->logical_index);
            }
        }
    }
    return ret;
}


void loop_adapt_eval_dvfs_close()
{
    if (likwid_started)
    {
        perfmon_stopCounters();
        likwid_started = 0;
    }
    if (likwid_configured)
    {
        perfmon_finalize();
        likwid_configured = 0;
    }
    return;
}

