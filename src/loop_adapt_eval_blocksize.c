#include <stdarg.h>
#include <loop_adapt.h>
#include <loop_adapt_eval_blocksize.h>
#include <loop_adapt_calc.h>

#include <hwloc.h>


int loop_adapt_eval_blocksize_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    //affinity_init();
    timer_init();
    perfmon_init(num_cpus, cpulist);
    int npros = num_profiles;
    npros++;
    
    CpuTopology_t cputopo = get_cpuTopology();
    for (int i = 0; i < cputopo->numCacheLevels; i++)
    {
        char level[20];
        snprintf(level, 19, "L%d_CACHESIZE", cputopo->cacheLevels[i].level);
        char linesize[20];
        snprintf(linesize, 19, "L%d_LINESIZE", cputopo->cacheLevels[i].level);
        for (int c=0; c < num_cpus; c++)
        {
            add_def(level, cputopo->cacheLevels[i].size, cpulist[c]);
            add_def(linesize, cputopo->cacheLevels[i].lineSize, cpulist[c]);
        }
    }
    
    int gid = perfmon_addEventSet(POL_BLOCKSIZE.likwid_group);
    if (gid >= 0)
    {
        for (int m = 0; m < POL_BLOCKSIZE.num_metrics; m++)
        {
            for (int i = 0; i < perfmon_getNumberOfMetrics(gid); i++)
            {
                if (strcmp(POL_BLOCKSIZE.metrics[m].name, perfmon_getMetricName(gid, i)) == 0)
                {
                    POL_BLOCKSIZE.metric_idx[m] = i;
                }
            }
        }
        perfmon_setupCounters(gid);
        perfmon_startCounters();
    }
    return gid;
}

int loop_adapt_eval_blocksize_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        if (vals->cur_profile == 0)
        {
            for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)
            {
                set_param_min(POL_BLOCKSIZE.parameters[i].name, vals);
            }
        }
        TimerData *t = vals->runtimes + vals->cur_profile;
        timer_start(t);
        ret = perfmon_readCountersCpu(cpuid);
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
        TimerData *t = vals->runtimes + vals->cur_profile;
        ret = perfmon_readCountersCpu(cpuid);
        timer_stop(t);
        Policy_t p = tdata->cur_policy;
        int max_vals = perfmon_getNumberOfMetrics(p->likwid_gid);
        
        for (int m = 0; m < max_vals; m++)
        {
            //vals->profiles[vals->cur_profile][m] = loop_adapt_round(perfmon_getLastMetric(p->likwid_gid, m, obj->logical_index));
            vals->profiles[vals->cur_profile][m] = perfmon_getLastMetric(p->likwid_gid, m, obj->logical_index);
            //printf("%d Metric %s : %f\n", m, perfmon_getMetricName(p->likwid_gid, m), vals->profiles[vals->cur_profile][m]);
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
    double *cur_values = v->profiles[cur_profile];
    double *opt_values = v->profiles[opt_profile];
    unsigned int (*tcount_func)() = NULL;

    // Evaluate the condition whether this profile is better than the old
    // optimal one
    // evaluate() returns 1 if better, 0 if not

    int eval = evaluate(p, opt_values, cur_values);

    if (eval)
    {
        p->optimal_profile = cur_profile;
        // we search through all parameters and set the best setting to the
        // current setting. After the policy is completely evaluated, the
        // best setting is returned by GET_[INT/DBL]_PARAMETER
        update_best(p, obj, obj);

        // Check the current thread count whether the setting of this object
        // should be propagated to the other ones
        loop_adapt_get_tcount_func(&tcount_func);
        if (tcount_func && tcount_func() == 1)
        {
            int len = hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)p->scope);
            for (int k = 0; k < len; k++)
            {
                //printf("K %d\n", k);
                hwloc_obj_t new = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)p->scope, k);
                if (new == obj)
                {
                    //printf("Skip %d\n", k);
                    continue;
                }
                update_best(p, obj, new);
            }
        }
    }
    
    // If we are in the scope of the policy (at which level should changes be
    // done) the parameters are adjusted.
    if (getLoopAdaptType(obj->type) == p->scope)
    {
        for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)
        {
            set_param_step(POL_BLOCKSIZE.parameters[i].name, v, cur_profile-1);
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
    perfmon_stopCounters();
    perfmon_finalize();
    return;
}
