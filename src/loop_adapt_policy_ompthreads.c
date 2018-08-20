#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <loop_adapt_policy_ompthreads.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

#include <omp.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;

static pthread_mutex_t ompthreads_lock = PTHREAD_MUTEX_INITIALIZER;

static int ompthreads_num_cpus = 0;
static int ompthreads_active_cpus = 0;
static int* ompthreads_cpus = NULL;

static int ompthreads_init_value = 0;

static cpu_set_t loop_adapt_ompthreads_cpuset;

int loop_adapt_policy_ompthreads_post_param(char* location, Nodeparameter_t param)
{
    if (strncmp(location, "N", 1) == 0)
    {
        int num_threads = param->cur.ival;
        if (num_threads > 0)
        {
            if (loop_adapt_debug)
                fprintf(stderr, "DEBUG: Setting OpenMP threads to %d\n", num_threads);
            omp_set_num_threads(num_threads);
            return 0;
        }
        else
        {
            fprintf(stderr, "ERROR POL_OMPTHREADS: Parameter value for post function invalid. Accepts just > 0\n");
        }
    }
    else
    {
        fprintf(stderr, "ERROR POL_OMPTHREADS: Location argument for post function invalid. Accepts just 'N'\n");
    }
    return -EINVAL;
}

int loop_adapt_policy_ompthreads_init(int num_cpus, int* cpulist, int num_profiles)
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
    {
        ompthreads_init_value = atoi(getenv("OMP_NUM_THREADS"));
    }
    CPU_ZERO(&loop_adapt_ompthreads_cpuset);
    int ret = sched_getaffinity(0, sizeof(loop_adapt_ompthreads_cpuset), &loop_adapt_ompthreads_cpuset);
    ompthreads_active_cpus = CPU_COUNT(&loop_adapt_ompthreads_cpuset);
    return 0;
}


// Data is read in end and the Nodes' cur_profile value is updated but
// the evaluation is in the startloop afterwards, so cur_profle is set to
// nodes' cur_profile - 1
int loop_adapt_policy_ompthreads_eval(hwloc_topology_t tree, hwloc_obj_t obj)
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
                    fprintf(stderr, "DEBUG POL_OMPTHREADS: Evaluate param %s for %s with idx %d (opt:%d, cur:%d)\n", pparam->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index, pp->opt_profile, pp->cur_profile-1);

                eval = la_calc_evaluate(p, pparam, opt_values, opt_runtime, cur_values, cur_runtime);
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_OMPTHREADS: Evaluation %s = %d\n", pparam->eval, eval);
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
            eval = loop_adapt_policy_ompthreads_eval(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return eval;
}


int loop_adapt_policy_ompthreads_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    TimerData* t = NULL;

    if (tdata && vals)
    {
        pthread_mutex_lock(&ompthreads_lock);
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
                            if (loop_adapt_debug == 2)
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
        pthread_mutex_unlock(&ompthreads_lock);
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        t = loop_adapt_policy_profile_get_timer(pp);
        if (t)
            timer_stop(t);

        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Reading LIKWID counters on CPU %d Profile %d\n", cpuid, pp->cur_profile);
            ret = perfmon_readCountersCpu(cpuid);
        }
    }
    return ret;
}


int loop_adapt_policy_ompthreads_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
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

void loop_adapt_policy_ompthreads_close()
{
    pthread_mutex_lock(&ompthreads_lock);

    if (likwid_started)
    {
        if (loop_adapt_debug == 2)
            fprintf(stderr, "DEBUG POL_OMPTHREADS: Stopping LIKWID counters\n");
        perfmon_stopCounters();
        likwid_started = 0;

        if (likwid_configured && likwid_initialized)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_OMPTHREADS: Finalizing LIKWID counters\n");
            perfmon_finalize();
            likwid_configured = 0;
            likwid_initialized = 0;
        }
    }
    pthread_mutex_unlock(&ompthreads_lock);
    return;
}

