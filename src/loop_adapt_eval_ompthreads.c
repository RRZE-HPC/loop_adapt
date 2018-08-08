#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <loop_adapt_eval_ompthreads.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

#include <omp.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;


static int ompthreads_num_cpus = 0;
static int ompthreads_active_cpus = 0;
static int* ompthreads_cpus = NULL;

static int thread_inc = 0;
static int thread_start = 0;
static int thread_init = 0;

static cpu_set_t loop_adapt_ompthreads_cpuset;

int loop_adapt_eval_ompthreads_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    timer_init();
    int npros = num_profiles;
    npros++;


    CpuTopology_t cputopo = get_cpuTopology();
    char* avail_cpus = "AVAILABLE_CPUS";
    char* avail_cores = "AVAILABLE_CORES";
    char* active_cpus = "ACTIVE_CPUS";
    if (loop_adapt_debug == 2)
    {
        fprintf(stderr, "DEBUG POL_OMPTHREADS: Add define %s = %d globally\n", avail_cpus, cputopo->numHWThreads,  -1);
        fprintf(stderr, "DEBUG POL_OMPTHREADS: Add define %s = %d globally\n", avail_cores, cputopo->numHWThreads/cputopo->numThreadsPerCore,  -1);
        fprintf(stderr, "DEBUG POL_OMPTHREADS: Add define %s = %d globally\n", active_cpus, cputopo->activeHWThreads,  -1);
    }
    la_calc_add_def(avail_cpus, cputopo->numHWThreads, -1);
    la_calc_add_def(avail_cores, cputopo->numHWThreads/cputopo->numThreadsPerCore, -1);
    la_calc_add_def(active_cpus, cputopo->activeHWThreads, -1);

    ompthreads_num_cpus = num_cpus;
    ompthreads_cpus = (int*)malloc(ompthreads_num_cpus * sizeof(int));
    if (!ompthreads_cpus)
        return -ENOMEM;
    memcpy(ompthreads_cpus, cpulist, ompthreads_num_cpus * sizeof(int));

    if (getenv("OMP_NUM_THREADS"))
        thread_init = atoi(getenv("OMP_NUM_THREADS"));
    CPU_ZERO(&loop_adapt_ompthreads_cpuset);
    int ret = sched_getaffinity(0, sizeof(loop_adapt_ompthreads_cpuset), &loop_adapt_ompthreads_cpuset);
    ompthreads_active_cpus = CPU_COUNT(&loop_adapt_ompthreads_cpuset);
    return 0;
}


void _loop_adapt_eval_ompthreads(hwloc_topology_t tree, hwloc_obj_t obj)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t v = (Nodevalues_t)obj->userdata;
    int cur_profile = v->cur_profile - 1;
    Policy_t p = tdata->cur_policy;
    int opt_profile = v->opt_profiles[v->cur_policy];
    printf("Opt %d Cur %d Type %s\n", opt_profile, cur_profile, loop_adapt_type_name((AdaptScope)obj->type));
    double *cur_values = v->profiles[v->cur_policy][cur_profile];
    double *opt_values = v->profiles[v->cur_policy][opt_profile];
    int (*tcount_func)() = NULL;

    // Evaluate the condition whether this profile is better than the old
    // optimal one
    // evaluate() returns 1 if better, 0 if not
    for (int i = 0; i < p->num_parameters; i++)
    {
        PolicyParameter_t pp = &p->parameters[i];
        int eval = la_calc_evaluate(p, pp, opt_values, cur_values);
        printf("Eval %d\n", eval);
        if (eval)
        {
            //p->optimal_profile = cur_profile;
            v->opt_profiles[v->cur_policy] = cur_profile;
            //if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Evaluate param %s for %s with idx %d (opt:%d, cur:%d)\n", pp->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index, opt_profile, cur_profile);
            // we search through all parameters and set the best setting to the
            // current setting. After the policy is completely evaluated, the
            // best setting is returned by GET_[INT/DBL]_PARAMETER
            update_best(pp, obj, obj);

            // Check the current thread count whether the setting of this object
            // should be propagated to the other ones.
            // Often the LA_LOOP macros are used in serial regions and therefore
            // the best parameter is set for all other objects of the scope.
            loop_adapt_get_cpucount_func(&tcount_func);
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
        if (getLoopAdaptType(obj->type) == p->scope && cur_profile < v->num_profiles[v->cur_policy]-1)
        {
            for (int i = 0; i < POL_OMPTHREADS.num_parameters; i++)
            {
                Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer)pp->name);
                Value old;
                Value* oldptr = np->old_vals + cur_profile;
                old.ival = np->cur.ival;
                if (thread_inc == 0)
                {
                    thread_inc = (np->max.ival - np->min.ival)/np->inter;
                }
                np->cur.ival = np->min.ival + ((v->cur_profile-1)*thread_inc);
                fprintf(stderr, "Setting OMP_NUM_THREADS to %d\n", np->cur.ival);
                fprintf(stderr, "Storing old param value %d at pos %d\n", old, cur_profile);
                np->old_vals[cur_profile] = old;
                np->num_old_vals = cur_profile + 1;
                memcpy(oldptr, &old, sizeof(Value));
                omp_set_num_threads(np->cur.ival);
                ompthreads_active_cpus = np->cur.ival;
            }
        }
    }
}

int loop_adapt_eval_ompthreads(hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Policy_t p = tdata->cur_policy;
    Nodevalues_t v = (Nodevalues_t)obj->userdata;
    Nodevalues_t wv = NULL;

    if (!likwid_started)
        return 0;

    if (obj->type == p->scope)
    {
        _loop_adapt_eval_ompthreads(tree, obj);
    }
    return ret;
}

int loop_adapt_eval_ompthreads_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    char spid[30];
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    ret = snprintf(spid, 29, "%d", getpid());
    spid[ret] = '\0';
    ret = 0;
    setenv("LIKWID_PERF_PID", spid, 1);
    if (tdata && vals)
    {
        if (!likwid_initialized)
        {
            perfmon_init(ompthreads_num_cpus, ompthreads_cpus);
        }
        if (!likwid_configured)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Adding group %s to LIKWID\n", POL_OMPTHREADS.likwid_group);
            likwid_gid = perfmon_addEventSet(POL_OMPTHREADS.likwid_group);
            if (likwid_gid >= 0)
            {
                // We store which indicies we need later for evaluation. We can also use
                // special performance groups that offer only the required metrics but
                // with this way we can use default performance groups.
                for (int m = 0; m < POL_OMPTHREADS.num_metrics; m++)
                {
                    for (int i = 0; i < perfmon_getNumberOfMetrics(likwid_gid); i++)
                    {
                        if (strcmp(POL_OMPTHREADS.metrics[m].name, perfmon_getMetricName(likwid_gid, i)) == 0)
                        {
                            POL_OMPTHREADS.metric_idx[m] = i;
                            //if (loop_adapt_debug == 2)
                                fprintf(stderr, "DEBUG POL_OMPTHREADS: Using metric with ID %d\n", i);
                        }
                    }
                }
                likwid_configured = 1;
            }
        }
        if (likwid_configured && likwid_gid >= 0 && !likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Setup LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_setupCounters(likwid_gid);
            if (ret < 0)
            {
                printf("Cannot setup LIKWID counters for group ID %d\n", likwid_gid);
            }
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Starting LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_startCounters();
            if (ret < 0)
            {
                printf("Cannot start LIKWID counters for group ID %d\n", likwid_gid);
            }
            likwid_started = 1;
        }

        timer_start(&vals->timers[vals->cur_policy][vals->cur_profile]);
        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Reading LIKWID counters on CPU %d into [%d][%d]\n",cpuid, vals->cur_policy, vals->cur_profile);
            ret = perfmon_readCountersCpu(cpuid);
        }
    }
    return ret;
}

int loop_adapt_eval_ompthreads_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        TimerData *t = vals->timers[vals->cur_policy] + vals->cur_profile;
        double *r = vals->runtimes[vals->cur_policy] + vals->cur_profile;
        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Reading LIKWID counters on CPU %d into [%d][%d]\n",cpuid, vals->cur_policy, vals->cur_profile);
            ret = perfmon_readCountersCpu(cpuid);
            timer_stop(t);
            *r = timer_print(t);
            Policy_t p = tdata->cur_policy;
            int max_vals = perfmon_getNumberOfMetrics(p->likwid_gid);
            double* pdata = vals->profiles[vals->cur_policy][vals->cur_profile];
            for (int m = 0; m < p->num_metrics; m++)
            {
                pdata[m] = perfmon_getLastMetric(p->likwid_gid, p->metric_idx[m], obj->logical_index);
            }
        }
    }
    return ret;
}


void loop_adapt_eval_ompthreads_close()
{

    omp_set_num_threads(thread_init);

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

