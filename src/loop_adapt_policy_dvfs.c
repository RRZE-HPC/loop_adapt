#include <stdint.h>
#include <loop_adapt_policy_dvfs.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;

static pthread_mutex_t dvfs_lock = PTHREAD_MUTEX_INITIALIZER;

static int dvfs_num_cpus = 0;
static int* dvfs_cpus = NULL;

static char* avail_freqs = NULL;
static int num_freqs = 0;

static double* freqs = NULL;




static void parse_avail_freqs(char* s)
{
    struct bstrList* flist;
    bstring bs = bfromcstr(s);
    flist = bsplit(bs, ' ');
    freqs = malloc(flist->qty * sizeof(double));
    memset(freqs, 0.0, flist->qty * sizeof(double));
    for (int i = 0; i < flist->qty; i++)
    {
        double f = atof(bdata(flist->entry[i]));
        freqs[i] = f;
    }
    num_freqs = flist->qty;
    bstrListPrint(flist);
    for (int i = 0; i < num_freqs; i++)
    {
        printf("%f ", freqs[i]);
    }
    printf("\n");
    bdestroy(bs);
    bstrListDestroy(flist);
}

int loop_adapt_policy_dvfs_post_param_cpu(char* location, Nodeparameter_t param)
{
    if (strncmp(location, "S", 1) == 0)
    {
        int socketid = 0;
        int threadid = 0;
        int f_valid = 0;
        double f = param->cur.dval;
        printf("%s %f\n", location, f);
        CpuTopology_t topo = get_cpuTopology();
        if (f > 0)
        {
            for (int i = 0; i < num_freqs; i++)
            {
                if (freqs[i] == f)
                {
                    f_valid = 1;
                    break;
                }
            }
        }
        if (f <= 0 || !f_valid)
        {
            fprintf(stderr, "ERROR POL_DVFS: Given frequency %f not available or not valid\n", f);
            return -EINVAL;
        }

        int ret = sscanf(location, "S%d", &socketid);
        uint64_t ufreq = (uint64_t)(f*1E6);
        uint64_t testfreq = (uint64_t)(f*1E9);
        if (ret == 1 && socketid >= 0 && socketid < topo->numSockets )
        {
            for (int i=0; i < topo->numHWThreads; i++)
            {
                if (topo->threadPool[i].packageId == socketid && topo->threadPool[i].inCpuSet)
                {
                    int done_any = 0;
                    //printf("%lu -> %lu %lu\n", testfreq, freq_getCpuClockMin(topo->threadPool[i].apicId), freq_getCpuClockMax(topo->threadPool[i].apicId));
                    if (freq_getCpuClockMin(topo->threadPool[i].apicId) != testfreq)
                    {
                        freq_setCpuClockMin(topo->threadPool[i].apicId, ufreq);
                    }
                    if (freq_getCpuClockMax(topo->threadPool[i].apicId) != testfreq)
                    {
                        freq_setCpuClockMax(topo->threadPool[i].apicId, ufreq);
                    }
                    if (freq_getCpuClockMin(topo->threadPool[i].apicId) != testfreq)
                    {
                        freq_setCpuClockMin(topo->threadPool[i].apicId, ufreq);
                    }
                    if (freq_getCpuClockMax(topo->threadPool[i].apicId) != testfreq)
                    {
                        freq_setCpuClockMax(topo->threadPool[i].apicId, ufreq);
                    }
                    if (loop_adapt_debug)
                        fprintf(stdout, "Pinning frequency to %f for CPU %d\n", f, topo->threadPool[i].apicId);
                }
            }
            return 0;
        }
        else
        {
            fprintf(stderr, "ERROR POL_DVFS: Thread ID not valid\n");
        }
    }
    else
    {
        fprintf(stderr, "ERROR POL_DVFS: Location argument %s for post function invalid. Accepts just 'Sx' where x is the socket ID.\n", location);
    }
    return -EINVAL;
}

int loop_adapt_policy_dvfs_post_param_uncore(char* location, Nodeparameter_t param)
{
    topology_init();
    if (strncmp(location, "S", 1) == 0)
    {
        int socketid = 0;
        CpuTopology_t topo = get_cpuTopology();
        int ret = sscanf(location, "S%d", &socketid);
        if (ret == 1 && socketid >= 0 && socketid < topo->numSockets )
        {
            double f = param->cur.dval;
            if (f > 0)
            {
                freq_setUncoreFreqMin(socketid, (uint64_t)(f*1E6));
                freq_setUncoreFreqMax(socketid, (uint64_t)(f*1E6));
            }
            else
            {
                fprintf(stderr, "ERROR POL_DVFS: Uncore frequency %f must be greater than zero\n", f);
            }
        }
        else
        {
            fprintf(stderr, "ERROR POL_DVFS: Socket ID not valid\n");
        }
    }
    else
    {
        fprintf(stderr, "ERROR POL_DVFS: Location argument %s for post function invalid. Accepts just 'Sx' where x is the socket ID.\n", location);
    }
    return -EINVAL;
}


int loop_adapt_policy_dvfs_init(int num_cpus, int* cpulist, int num_profiles)
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
    CpuTopology_t cputopo = get_cpuTopology();
    
    parse_avail_freqs(freq_getAvailFreq(cpulist[0]));
    
    for (int c = 0; c < num_cpus; c++)
    {
        if (loop_adapt_debug == 2)
        {
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", minclock, freqs[0],  cpulist[c]);
            fprintf(stderr, "DEBUG: Add define %s = %f for CPU %d\n", maxclock, freqs[num_freqs-1],  cpulist[c]);
        }
        la_calc_add_def(minclock, freqs[0], cpulist[c]);
        la_calc_add_def(maxclock, freqs[num_freqs-1], cpulist[c]);
    }
    
    dvfs_num_cpus = num_cpus;
    dvfs_cpus = (int*)malloc(dvfs_num_cpus * sizeof(int));
    if (!dvfs_cpus)
        return -ENOMEM;
    memcpy(dvfs_cpus, cpulist, dvfs_num_cpus * sizeof(int));

    return 0;
}


int loop_adapt_policy_dvfs_eval(hwloc_topology_t tree, hwloc_obj_t obj)
{
    int eval = 0;
    unsigned int (*tcount_func)() = NULL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t v = (Nodevalues_t)obj->userdata;
    double *cur_values = NULL;
    double *opt_values = NULL;
    double cur_runtime = 0;
    double opt_runtime = 0;
    if (tdata && v)
    {
        Policy_t p = tdata->cur_policy;
        PolicyProfile_t pp = v->policies[v->cur_policy];
        if (pp->cur_profile >= 0)
        {
            cur_values = pp->profiles[pp->cur_profile-1]->values;
            cur_runtime = pp->profiles[pp->cur_profile-1]->runtime;
        }
        else
        {
            return 0;
        }
        if (pp->opt_profile >= 0)
        {
            opt_values = pp->profiles[pp->opt_profile]->values;
            opt_runtime = pp->profiles[pp->opt_profile]->runtime;
        }
        else
        {
            opt_values = pp->base_profile->values;
            opt_runtime = pp->base_profile->runtime;
        }
        if (getLoopAdaptType(obj->type) == p->scope)
        {
            for (int i = 0; i < p->num_parameters; i++)
            {
                PolicyParameter_t pparam = &p->parameters[i];
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_DVFS: Evaluate param %s for %s with idx %d (opt:%d, cur:%d)\n", pparam->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index, pp->opt_profile, pp->cur_profile-1);

                eval = la_calc_evaluate(p, pparam, opt_values, opt_runtime, cur_values, cur_runtime);
                //if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_DVFS: Evaluation %s = %d\n", pparam->eval, eval);
            }
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
            walker->type == HWLOC_OBJ_MACHINE ||
            walker->type == HWLOC_OBJ_PU)
        {
            eval += loop_adapt_policy_dvfs_eval(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return eval;
}

int loop_adapt_policy_dvfs_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    TimerData* t = NULL;

    if (tdata && vals)
    {
        pthread_mutex_lock(&dvfs_lock);
        if (!likwid_initialized)
        {
            
            perfmon_init(dvfs_num_cpus, dvfs_cpus);
            
        }
        if (!likwid_configured)
        {
            Policy_t p = tdata->cur_policy;
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_DVFS: Adding group %s to LIKWID\n", p->likwid_group);
            likwid_gid = perfmon_addEventSet(p->likwid_group);
            if (likwid_gid >= 0)
            {
                // We store which indicies we need later for evaluation. We can also use
                // special performance groups that offer only the required metrics but
                // with this way we can use default performance groups.
                for (int m = 0; m < p->num_metrics; m++)
                {
                    for (int i = 0; i < perfmon_getNumberOfMetrics(likwid_gid); i++)
                    {
                        if (strcmp(p->metrics[m].name, perfmon_getMetricName(likwid_gid, i)) == 0)
                        {
                            p->metric_idx[m] = i;
                            if (loop_adapt_debug == 2)
                                fprintf(stderr, "DEBUG POL_DVFS: Using metric with ID %d\n", i);
                        }
                    }
                }
                likwid_configured = 1;
            }
        }
        if (likwid_configured && likwid_gid >= 0 && !likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_DVFS: Setup LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_setupCounters(likwid_gid);
            if (ret < 0)
            {
                printf("Cannot setup LIKWID counters for group ID %d\n", likwid_gid);
            }
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_DVFS: Starting LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_startCounters();
            if (ret < 0)
            {
                printf("Cannot start LIKWID counters for group ID %d\n", likwid_gid);
            }
            likwid_started = 1;
        }
        pthread_mutex_unlock(&dvfs_lock);
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        t = loop_adapt_policy_profile_get_timer(pp);
        if (t)
            timer_stop(t);

        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_DVFS: Reading LIKWID counters on CPU %d Profile %d\n", cpuid, pp->cur_profile);
            ret = perfmon_readCountersCpu(cpuid);
        }
    }
    return ret;
}

int loop_adapt_policy_dvfs_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    double* pdata = NULL;
    double *runtime = NULL;
    Policy_t p = NULL;
    TimerData *t = NULL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        p = tdata->cur_policy;
        PolicyProfile_t pp = vals->policies[vals->cur_policy];

        if (likwid_started)
        {
            ret = perfmon_readCountersCpu(cpuid);

            t = loop_adapt_policy_profile_get_timer(pp);
            if (t)
                timer_stop(t);
            runtime = loop_adapt_policy_profile_get_runtime(pp);
            pdata = loop_adapt_policy_profile_get_values(pp);


            for (int m = 0; m < p->num_metrics; m++)
            {
                pdata[m] = perfmon_getLastMetric(p->likwid_gid, p->metric_idx[m], obj->logical_index);
            }
            pp->num_values = p->num_metrics;
            *runtime = timer_print(t);
        }
    }
    return ret;
}



void loop_adapt_policy_dvfs_close()
{
    pthread_mutex_lock(&dvfs_lock);
    if (dvfs_cpus)
    {
        free(dvfs_cpus);
        dvfs_cpus = NULL;
        dvfs_num_cpus = 0;
    }
    if (freqs)
    {
        free(freqs);
        freqs = NULL;
    }
    if (likwid_started)
    {
        if (loop_adapt_debug == 2)
            fprintf(stderr, "DEBUG POL_DVFS: Stopping LIKWID counters\n");
        perfmon_stopCounters();
        likwid_started = 0;

        if (likwid_configured && likwid_initialized)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_DVFS: Finalizing LIKWID counters\n");
            perfmon_finalize();
            likwid_configured = 0;
            likwid_initialized = 0;
        }
    }
    pthread_mutex_unlock(&dvfs_lock);
    return;
}

