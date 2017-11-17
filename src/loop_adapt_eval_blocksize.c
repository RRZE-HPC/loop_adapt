#include <stdarg.h>
#include <loop_adapt.h>
#include <loop_adapt_eval_blocksize.h>
#include <loop_adapt_calc.h>

#include <hwloc.h>

static int likwid_initialized = 0;
static int likwid_configured = 0;
static int likwid_started = 0;
static int likwid_gid = -1;

static int blocksize_num_cpus = 0;
static int* blocksize_cpus = NULL;

static void loop_adapt_eval_blocksize_next_param_step(char* param, Nodevalues_t vals, int step)
{
    Nodeparameter_t np = g_hash_table_lookup(vals->param_hash, (gpointer)param);
    if (np)
    {
        if (np->type == NODEPARAMETER_INT)
        {
            int min = np->min.imin;
            int max = np->max.imax;
            int s = (max-min) / np->inter;
            np->cur.icur = min + (step * s);
            if (loop_adapt_debug == 2)
                fprintf(stderr, "Next param value for %s : %d\n", param, np->cur.icur);
        }
        else if (np->type == NODEPARAMETER_DOUBLE)
        {
            double min = np->min.dmin;
            double max = np->max.imax;
            double s = (max-min) / np->inter;
            np->cur.dcur = min + (step * s);
            if (loop_adapt_debug == 2)
                fprintf(stderr, "Next param value for %s : %f\n", param, np->cur.dcur);
        }
    }
}


int loop_adapt_eval_blocksize_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    //affinity_init();
    timer_init();
    
    // In this policy the num_profiles input is not needed. This is just to
    // avoid warnings.
    int npros = num_profiles;
    npros++;
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
        // Defines are CPU specific, so we have to loop over all CPUs and add
        // the defines there
        for (int c=0; c < num_cpus; c++)
        {
            la_calc_add_def(level, cputopo->cacheLevels[i].size, cpulist[c]);
            la_calc_add_def(linesize, cputopo->cacheLevels[i].lineSize, cpulist[c]);
            if (loop_adapt_debug == 2)
            {
                fprintf(stderr, "DEBUG: Add define %s = %ld for CPU %d\n", level, cputopo->cacheLevels[i].size,  cpulist[c]);
                fprintf(stderr, "DEBUG: Add define %s = %ld for CPU %d\n", linesize, cputopo->cacheLevels[i].lineSize,  cpulist[c]);
            }
        }
    }

    // Initialize LIKWID
    // There might be policies which do not require LIKWID, therefore all
    // LIKWID handling is done by the policies themselves and not by the
    // remaining library.
    
    return 0;
}

int loop_adapt_eval_blocksize_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        if (!likwid_initialized)
        {
            perfmon_init(blocksize_num_cpus, blocksize_cpus);
        }
        if (!likwid_configured)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Adding group %s to LIKWID\n", POL_BLOCKSIZE.likwid_group);
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

int loop_adapt_eval_blocksize_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        TimerData *t = vals->runtimes[vals->cur_policy] + vals->cur_profile;
        if (likwid_started)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Reading LIKWID counters on CPU %d\n",cpuid);
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



void loop_adapt_eval_blocksize(hwloc_topology_t tree, hwloc_obj_t obj)
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
    if (getLoopAdaptType(obj->type) == p->scope)
    {
        for (int i = 0; i < p->num_parameters; i++)
        {
            PolicyParameter_t pp = &p->parameters[i];
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Evaluate param %s for %s with idx %d\n", pp->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index);
            int eval = la_calc_evaluate(p, pp, opt_values, cur_values);

            if (eval)
            {
                p->optimal_profile = cur_profile;
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG: New optimal profile %d\n", p->optimal_profile);
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
                    if (loop_adapt_debug == 2)
                        fprintf(stderr, "DEBUG: Updating parameter for all\n");
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
            loop_adapt_eval_blocksize_next_param_step(pp->name, v, cur_profile-1);
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
            loop_adapt_eval_blocksize(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return;
}

void loop_adapt_eval_blocksize_close()
{
    if (likwid_started)
    {
        if (loop_adapt_debug == 2)
            fprintf(stderr, "DEBUG: Stopping LIKWID counters\n");
        perfmon_stopCounters();
        likwid_started = 0;
    }
    if (likwid_configured)
    {
        if (loop_adapt_debug == 2)
            fprintf(stderr, "DEBUG: Finalizing LIKWID counters\n");
        perfmon_finalize();
        likwid_configured = 0;
    }
    return;
}
