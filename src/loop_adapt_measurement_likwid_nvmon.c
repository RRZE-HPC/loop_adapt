#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <likwid.h>

#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_threads.h>
#include <error.h>

static int nvmon_init = 0;
static int current_group = -1;
static bstring current_group_str = NULL;
static int* gpus = NULL;
static int num_gpus = 0;
static int* current_metric_ids = NULL;
static int num_current_metric_ids = 0;


int loop_adapt_measurement_likwid_nvmon_init()
{
    int i = 0;
    int c = 0;
    int err = 0;

    err = topology_gpu_init();
    if (err)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Failed to init LIKWID GPU topology);
        return -EFAULT;
    }
    GpuTopology_t gputopo = get_gpuTopology();
    num_gpus = loop_adapt_accel_get_application_gpus(&gpus);

    setenv("LIKWID_FORCE", "1", 1);

    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Init LIKWID nvmon with %d GPUs, num_gpus);
    err = nvmon_init(num_gpus, gpus);
    if (!err)
    {
        nvmon_init = 1;
    }
    else
    {
        ERROR_PRINT(Failed to initialize LIKWID nvmon);
    }
    current_group_str = bfromcstr("");
}


void loop_adapt_measurement_likwid_nvmon_finalize()
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
        if (gpus)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing LIKWID GPU list);
            free(gpus);
            gpus = NULL;
            num_gpus = 0;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize LIKWID);
        nvmon_finalize();
        bdestroy(current_group_str);
        current_group_str = NULL;
        current_group = -1;
        likwid_init = 0;
    }
}


int loop_adapt_measurement_likwid_nvmon_setup(int instance, bstring configuration, bstring metrics)
{
    int err = 0;
    int i = 0;
    int j = 0;

    if (likwid_init)
    {
        if (bstrncmp(configuration, current_group_str, blength(configuration)) == BSTR_OK)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Reusing current nvmon configuration %s, bdata(configuration));
            return 0;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Adding nvmon eventset %s, bdata(configuration));
        int gid = nvmon_addEventSet(bdata(configuration));
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Eventset %s got nvmon GID %d, bdata(configuration), gid);
        if (gid >= 0)
        {
            current_group = gid;
            int active_group = nvmon_getIdOfActiveGroup();
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Active LIKWID nvmon group: %d, active_group);
            if (active_group >= 0)
            {
                err = nvmon_switchActiveGroup(gid);
            }
            struct bstrList* metriclist = bsplit(metrics, ',');

            if (current_metric_ids)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing current LIKWID nvmon metric IDs);
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
                for (j = 0; j < nvmon_getNumberOfMetrics(gid); j++)
                {
                    bstring bname = bfromcstr(nvmon_getMetricName(gid, j));
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Comparing '%s' with '%s', bdata(bname), bdata(metriclist->entry[i]));
                    if (bstrncmp(bname, metriclist->entry[i], blength(metriclist->entry[i])) == BSTR_OK)
                    {

                        tmp++;
                    }
                    bdestroy(bname);
                }
                if (tmp > 1)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Nvmon Metric string %s matches %d metrics taking just first match, bdata(metriclist->entry[i]), tmp);
                }
                for (j = 0; j < nvmon_getNumberOfMetrics(gid); j++)
                {
                    bstring bname = bfromcstr(nvmon_getMetricName(gid, j));
                    if (bstrncmp(bname, metriclist->entry[i], blength(metriclist->entry[i])) == BSTR_OK)
                    {
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Using LIKWID nvmon metric '%s' (idx %d) for '%s', bdata(bname), j, bdata(metriclist->entry[i]));
                        current_metric_ids[num_current_metric_ids] = j;
                        num_current_metric_ids++;
                        break;
                    }
                    bdestroy(bname);
                }
            }
            bstrListDestroy(metriclist);

            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Specifying %d new LIKWID nvmon metric IDs for LIKWID nvmon group %d, count, gid);

            nvmon_setupCounters(gid);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Start LIKWID nvmon group %d on GPU %d, gid, gpus[instance]);
            nvmon_startCounters();
            current_group = gid;
            bdestroy(current_group_str);
            current_group_str = bstrcpy(configuration);
        }
    }
    return 0;
}

void loop_adapt_measurement_likwid_nvmon_start(int instance)
{
    if (instance < 0 || instance >= num_cpus)
    {
        return;
    }
    int gpu = gpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (start) LIKWID nvmon counters for instance %d (GPU %d), instance, gpu);
    int err = nvmon_readCounters();
    if (err)
    {
        ERROR_PRINT(Failed to read (start) LIKWID nvmon counters for instance %d (GPU %d), instance, gpu);
    }
}

void loop_adapt_measurement_likwid_nvmon_stop(int instance)
{
    if (instance < 0 || instance >= num_cpus)
    {
        return;
    }
    int gpu = gpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Reading (stop) LIKWID nvmon counters for instance %d (GPU %d), instance, gpu);
    int err = nvmon_readCounters();
    if (err)
    {
        ERROR_PRINT(Failed to read (stop) LIKWID nvmon counters for instance %d (GPU %d), instance, gpu);
    }
}


int loop_adapt_measurement_likwid_nvmon_result(int instance, int num_values, ParameterValue* values)
{
    int i = 0;
    int loop = num_values > num_current_metric_ids ? num_current_metric_ids : num_values;
    int gpu = gpus[instance];
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Getting %d results from LIKWID nvmon counters for instance %d (GPU %d), loop, instance, gpu );
    for (i = 0; i < loop; i++)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get result for LIKWID nvmon group %d metric %d and gpu %d (GPU %d), current_group, current_metric_ids[i], instance, gpu)
        double r = perfmon_getLastMetric(current_group, current_metric_ids[i], instance);
        ParameterValue *v = &values[i];
        v->type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE;
        v->value.dval = r;
    }
    return loop;
}

int loop_adapt_measurement_likwid_nvmon_configs(struct bstrList* configs)
{
    int i = 0;
    char** groups = NULL;

    int num_groups = nvmon_getGroups(&groups, NULL, NULL);

    for (i = 0; i < num_groups; i++)
    {
        bstrListAddChar(configs, groups[i]);
    }
    return num_groups;

}