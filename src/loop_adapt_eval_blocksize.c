#include <stdarg.h>
#include <loop_adapt_eval_blocksize.h>




int loop_adapt_eval_blocksize_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    affinity_init();
    timer_init();
    perfmon_init(num_cpus, cpulist);
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
        vals->cur_profile++;
    }
    return ret;
}



void loop_adapt_eval_blocksize(hwloc_topology_t tree, hwloc_obj_t obj)
{
    Nodevalues *v = (Nodevalues*)obj->userdata;
    int cur_profile = v->cur_profile - 1;
    int last_profile = cur_profile - 1;
    double *cur_values = v->profiles[cur_profile];
    double cur_runtime = timer_print(&v->runtimes[cur_profile]);
    double *last_values = v->profiles[last_profile];
    double last_runtime = timer_print(&v->runtimes[last_profile]);
    
    if (cur_profile == 1)
    {
        for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)
        {
            set_param_max(POL_BLOCKSIZE.parameters[i].name, v);
        }
    }
    else
    {
        for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)
        {
            set_param_step(POL_BLOCKSIZE.parameters[i].name, v, cur_profile-1);
        }
    }
    
/*    int match = 0;*/
/*    for (int m = 0; m < POL_BLOCKSIZE.num_metrics; m++)*/
/*    {*/
/*        int idx = POL_BLOCKSIZE.metric_idx[m];*/
/*        switch (POL_BLOCKSIZE.metrics[m].compare)*/
/*        {*/
/*            case LESS_THAN:*/
/*                if (cur_values[idx] < last_values[idx])*/
/*                    match++;*/
/*                break;*/
/*            case LESS_EQUAL_THEN:*/
/*                if (cur_values[idx] <= last_values[idx])*/
/*                    match++;*/
/*                break;*/
/*            case GREATER_THAN:*/
/*                if (cur_values[idx] > last_values[idx])*/
/*                    match++;*/
/*                break;*/
/*            case GREATER_EQUAL_THEN:*/
/*                if (cur_values[idx] >= last_values[idx])*/
/*                    match++;*/
/*                break;*/
/*        }*/
/*    }*/
/*    PolicyMetricDestination dest = NONE;*/
/*    if (match > POL_BLOCKSIZE.num_metrics/2)*/
/*    {*/
/*        dest = POL_BLOCKSIZE.metrics[0].destination;*/
/*    }*/
/*    else*/
/*    {*/
/*        if (POL_BLOCKSIZE.metrics[0].destination == UP)*/
/*            POL_BLOCKSIZE.metrics[0].destination = DOWN;*/
/*    }*/
/*    if (dest == UP)*/
/*    {*/
/*        // get parameter blksize from obj and increase*/
/*        printf("Increase\n");*/
/*        for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)*/
/*        {*/
/*            for (int j = 0; j < v->num_parameters; j++)*/
/*            {*/
/*                if (strcmp(v->parameters[j]->name, POL_BLOCKSIZE.parameters[i].name) == 0)*/
/*                {*/
/*                    int* ptr = (int*)v->parameters[j]->ptr;*/
/*                    *ptr += v->parameters[j]->stepsize;*/
/*                    printf("New blksize %d\n", *ptr);*/
/*                    break;*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
/*    else if (dest == DOWN)*/
/*    {*/
/*        printf("Decrease\n");*/
/*        // get parameter blksize from obj and decrease*/
/*        for (int i = 0; i < POL_BLOCKSIZE.num_parameters; i++)*/
/*        {*/
/*            for (int j = 0; j < v->num_parameters; j++)*/
/*            {*/
/*                if (strcmp(v->parameters[j]->name, POL_BLOCKSIZE.parameters[i].name) == 0)*/
/*                {*/
/*                    int* ptr = (int*)v->parameters[j]->ptr;*/
/*                    *ptr -= v->parameters[j]->stepsize;*/
/*                    printf("New blksize %d\n", *ptr);*/
/*                    break;*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
/*    else if (dest == NONE)*/
/*    {*/
/*        printf("Keep setting\n");*/
/*    }*/
    hwloc_obj_t walker = obj->parent;
    while (walker)
    {
        if (walker->type == HWLOC_OBJ_PACKAGE ||
            walker->type == HWLOC_OBJ_NUMANODE ||
            walker->type == HWLOC_OBJ_MACHINE)
        {
            v = (Nodevalues*)walker->userdata;
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
    return;
}
