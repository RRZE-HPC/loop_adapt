#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <likwid.h>

#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_threads.h>
#include <error.h>

/*int (*loop_adapt_likwid_setup)(int instance, char* configuration);*/
/*void (*loop_adapt_likwid_start)(int instance);*/
/*void (*loop_adapt_likwid_stop)(int instance);*/
/*ParameterValue (*loop_adapt_likwid_result)(int instance);*/


static int likwid_init = 0;
static int current_group = -1;
static bstring current_group_str = NULL;
/*static struct bstrList* current_metrics = NULL;*/
static int* cpus = NULL;
static int num_cpus = 0;
static int* current_metric_ids = NULL;
static int num_current_metric_ids = 0;

int loop_adapt_measurement_likwid_init()
{
    int i = 0;
    int c = 0;
    int err = 0;

    err = topology_init();
    if (err)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Failed to init LIKWID topology);
        return -EFAULT;
    }
    CpuTopology_t cputopo = get_cpuTopology();
    num_cpus = loop_adapt_threads_get_application_cpus(&cpus);

    setenv("LIKWID_FORCE", "1", 1);

    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Init LIKWID perfmon with %d CPUs, num_cpus);
    err = perfmon_init(num_cpus, cpus);
    if (!err)
    {
        likwid_init = 1;
    }
    else
    {
        ERROR_PRINT(Failed to initialize LIKWID);
    }
    current_group_str = bfromcstr("");
}

void loop_adapt_measurement_likwid_finalize()
{
    if (likwid_init)
    {
        if (current_metric_ids)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing LIKWID metric IDs);
            free(current_metric_ids);
            current_metric_ids = NULL;
            num_current_metric_ids = 0;
        }
        if (cpus)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing LIKWID CPU list);
            free(cpus);
            cpus = NULL;
            num_cpus = 0;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize LIKWID);
        perfmon_finalize();
        bdestroy(current_group_str);
        current_group_str = NULL;
        current_group = -1;
        likwid_init = 0;
    }
}

int loop_adapt_measurement_likwid_setup(int instance, bstring configuration, bstring metrics)
{
    int err = 0;
    int i = 0;
    int j = 0;

    if (likwid_init)
    {
        if (bstrncmp(configuration, current_group_str, blength(configuration)) == BSTR_OK)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Reusing current configuration %s, bdata(configuration));
            return 0;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Adding eventset %s, bdata(configuration));
        int gid = perfmon_addEventSet(bdata(configuration));
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Eventset %s got GID %d, bdata(configuration), gid);
        if (gid >= 0)
        {
            current_group = gid;
            int active_group = perfmon_getIdOfActiveGroup();
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Active LIKWID group: %d, active_group);
            if (active_group >= 0)
            {
                err = perfmon_switchActiveGroup(gid);
            }
            struct bstrList* metriclist = bsplit(metrics, ',');

            if (current_metric_ids)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing current LIKWID metric IDs);
                free(current_metric_ids);
                current_metric_ids = 0;
                num_current_metric_ids = 0;
            }
            num_current_metric_ids = 0;
            current_metric_ids = malloc(metriclist->qty * sizeof(int));
/*            int* ids = malloc(metriclist->qty * sizeof(int));*/
/*            memset(ids, -1, metriclist->qty * sizeof(int));*/


            int count = 0;
            for (i = 0; i < metriclist->qty; i++)
            {
                int tmp = 0;
                for (j = 0; j < perfmon_getNumberOfMetrics(gid); j++)
                {
                    bstring bname = bfromcstr(perfmon_getMetricName(gid, j));
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Comparing '%s' with '%s', bdata(bname), bdata(metriclist->entry[i]));
                    if (bstrncmp(bname, metriclist->entry[i], blength(metriclist->entry[i])) == BSTR_OK)
                    {

                        tmp++;
                    }
                    bdestroy(bname);
                }
                if (tmp > 1)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Metric string %s matches %d metrics taking just first match, bdata(metriclist->entry[i]), tmp);
                }
                for (j = 0; j < perfmon_getNumberOfMetrics(gid); j++)
                {
                    bstring bname = bfromcstr(perfmon_getMetricName(gid, j));
                    if (bstrncmp(bname, metriclist->entry[i], blength(metriclist->entry[i])) == BSTR_OK)
                    {
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Using LIKWID metric '%s' (idx %d) for '%s', bdata(bname), j, bdata(metriclist->entry[i]));
                        current_metric_ids[num_current_metric_ids] = j;
                        num_current_metric_ids++;
                        break;
                    }
                    bdestroy(bname);
                }
            }
            bstrListDestroy(metriclist);

            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Specifying %d new LIKWID metric IDs for LIKWID group %d, count, gid);

            perfmon_setupCounters(gid);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Start LIKWID group %d on CPU %d, gid, cpus[instance]);
            perfmon_startCounters();
            current_group = gid;
            bdestroy(current_group_str);
            current_group_str = bstrcpy(configuration);
        }
    }
    return 0;
}

void loop_adapt_measurement_likwid_start(int instance)
{
    if (instance < 0 || instance >= num_cpus || likwid_init == 0)
    {
        return;
    }
    int cpu = cpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (start) LIKWID counters for instance %d (CPU %d), instance, cpu);
    int err = perfmon_readCountersCpu(cpu);
    if (err)
    {
        ERROR_PRINT(Failed to read (start) LIKWID counters for instance %d (CPU %d), instance, cpu);
    }
}

void loop_adapt_measurement_likwid_startall()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (start) LIKWID counters for all instances);
    int err = perfmon_readCounters();
    if (err)
    {
        ERROR_PRINT(Failed to read (start) LIKWID counters for all instances);
    }
}
void loop_adapt_measurement_likwid_stop(int instance)
{
    if (instance < 0 || instance >= num_cpus || likwid_init == 0)
    {
        return;
    }
    int cpu = cpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (stop) LIKWID counters for instance %d (CPU %d), instance, cpu);
    int err = perfmon_readCountersCpu(cpu);
    if (err)
    {
        ERROR_PRINT(Failed to read (stop) LIKWID counters for instance %d (CPU %d), instance, cpu);
    }
}

void loop_adapt_measurement_likwid_stopall()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (stop) LIKWID counters for all instances);
    int err = perfmon_readCounters();
    if (err)
    {
        ERROR_PRINT(Failed to read (stop) LIKWID counters for all instances);
    }
}
int loop_adapt_measurement_likwid_result(int instance, int num_values, ParameterValue* values)
{
    int i = 0;
    if (likwid_init == 0)
        return 0;
    int loop = num_values > num_current_metric_ids ? num_current_metric_ids : num_values;
    int cpu = cpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Getting %d results from LIKWID counters for instance %d (CPU %d), loop, instance, cpu );
    for (i = 0; i < loop; i++)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get result for LIKWID group %d metric %d and thread %d, current_group, current_metric_ids[i], instance)
        double r = perfmon_getLastMetric(current_group, current_metric_ids[i], instance);
        ParameterValue *v = &values[i];
        v->type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE;
        v->value.dval = r;
    }
    return loop;
}

int loop_adapt_measurement_likwid_configs(struct bstrList* configs)
{
    int i = 0;
    char** groups = NULL;

    int num_groups = perfmon_getGroups(&groups, NULL, NULL);

    for (i = 0; i < num_groups; i++)
    {
        bstrListAddChar(configs, groups[i]);
    }
    return num_groups;

}
