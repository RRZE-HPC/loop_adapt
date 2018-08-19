#include <stdarg.h>
#include <math.h>
#include <loop_adapt.h>
#include <loop_adapt_policy_blocksize.h>
#include <loop_adapt_calc.h>
#include <loop_adapt_search.h>

#include <hwloc.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;

static pthread_mutex_t blocksize_lock = PTHREAD_MUTEX_INITIALIZER;

static int blocksize_num_cpus = 0;
static int* blocksize_cpus = NULL;



int loop_adapt_policy_blocksize_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    //affinity_init();
    timer_init();
    
    // In this policy the num_profiles input is not needed. This is just to
    // avoid warnings.
    int npros = num_profiles;
    npros++;
    pthread_mutex_lock(&blocksize_lock);
    blocksize_num_cpus = num_cpus;
    blocksize_cpus = (int*)malloc(blocksize_num_cpus * sizeof(int));
    if (!blocksize_cpus)
        return -ENOMEM;
    memcpy(blocksize_cpus, cpulist, blocksize_num_cpus * sizeof(int));

    // Add some defines to the calculator that can be used in the evaluation
    // of limits and conditions
    CpuTopology_t cputopo = get_cpuTopology();
    for (int i = 0; i < cputopo->numCacheLevels; i++)
    {
        char level[20];
        snprintf(level, 19, "L%d_CACHESIZE", cputopo->cacheLevels[i].level);
        char linesize[20];
        snprintf(linesize, 19, "L%d_LINESIZE", cputopo->cacheLevels[i].level);

        la_calc_add_def(level, cputopo->cacheLevels[i].size, -1);
        la_calc_add_def(linesize, cputopo->cacheLevels[i].lineSize, -1);
        if (loop_adapt_debug)
        {
            fprintf(stderr, "DEBUG POL_BLOCKSIZE: Add define %s = %ld for all nodes\n", level, cputopo->cacheLevels[i].size);
            fprintf(stderr, "DEBUG POL_BLOCKSIZE: Add define %s = %ld for all nodes\n", linesize, cputopo->cacheLevels[i].lineSize);
        }
    }
    pthread_mutex_unlock(&blocksize_lock);

    // Initialize LIKWID
    // There might be policies which do not require LIKWID, therefore all
    // LIKWID handling is done by the policies themselves and not by the
    // remaining library.
    
    return 0;
}

int loop_adapt_policy_blocksize_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    TimerData* t = NULL;

    if (tdata && vals)
    {
        pthread_mutex_lock(&blocksize_lock);
        if (!likwid_initialized)
        {
            
            perfmon_init(blocksize_num_cpus, blocksize_cpus);
            
        }
        if (!likwid_configured)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Adding group %s to LIKWID\n", POL_BLOCKSIZE.likwid_group);
            likwid_gid = perfmon_addEventSet(POL_BLOCKSIZE.likwid_group);
            if (likwid_gid >= 0)
            {
                // We store which indicies we need later for evaluation. We can also use
                // special performance groups that offer only the required metrics but
                // with this way we can use default performance groups.
                for (int m = 0; m < POL_BLOCKSIZE.num_metrics; m++)
                {
                    for (int i = 0; i < perfmon_getNumberOfMetrics(likwid_gid); i++)
                    {
                        if (strcmp(POL_BLOCKSIZE.metrics[m].name, perfmon_getMetricName(likwid_gid, i)) == 0)
                        {
                            POL_BLOCKSIZE.metric_idx[m] = i;
                            if (loop_adapt_debug == 2)
                                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Using metric with ID %d\n", i);
                        }
                    }
                }
                likwid_configured = 1;
            }
        }
        if (likwid_configured && likwid_gid >= 0 && !likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Setup LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_setupCounters(likwid_gid);
            if (ret < 0)
            {
                printf("Cannot setup LIKWID counters for group ID %d\n", likwid_gid);
            }
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Starting LIKWID counters with group ID %d\n", likwid_gid);
            ret = perfmon_startCounters();
            if (ret < 0)
            {
                printf("Cannot start LIKWID counters for group ID %d\n", likwid_gid);
            }
            likwid_started = 1;
        }
        pthread_mutex_unlock(&blocksize_lock);
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        t = loop_adapt_policy_profile_get_timer(pp);
        if (t)
            timer_stop(t);

        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Reading LIKWID counters on CPU %d Profile %d\n", cpuid, pp->cur_profile);
            ret = perfmon_readCountersCpu(cpuid);
        }
    }
    return ret;
}

int loop_adapt_policy_blocksize_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
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


// Data is read in end and the Nodes' cur_profile value is updated but
// the evaluation is in the startloop afterwards, so cur_profle is set to
// nodes' cur_profile - 1
int loop_adapt_policy_blocksize_eval(hwloc_topology_t tree, hwloc_obj_t obj)
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
                    fprintf(stderr, "DEBUG POL_BLOCKSIZE: Evaluate param %s for %s with idx %d (opt:%d, cur:%d)\n", pparam->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index, pp->opt_profile, pp->cur_profile-1);

                eval = la_calc_evaluate(p, pparam, opt_values, opt_runtime, cur_values, cur_runtime);
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_BLOCKSIZE: Evaluation %s = %d\n", pparam->eval, eval);
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
            eval = loop_adapt_policy_blocksize_eval(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return eval;
}

void loop_adapt_policy_blocksize_close()
{
    pthread_mutex_lock(&blocksize_lock);

    if (likwid_started)
    {
        if (loop_adapt_debug == 2)
            fprintf(stderr, "DEBUG POL_BLOCKSIZE: Stopping LIKWID counters\n");
        perfmon_stopCounters();
        likwid_started = 0;

        if (likwid_configured && likwid_initialized)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG POL_BLOCKSIZE: Finalizing LIKWID counters\n");
            perfmon_finalize();
            likwid_configured = 0;
            likwid_initialized = 0;
        }
    }
    pthread_mutex_unlock(&blocksize_lock);
    return;
}
