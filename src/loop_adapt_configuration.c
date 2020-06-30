#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <error.h>

#include <pthread.h>

#ifdef USE_MPI
#include <mpi.h>
#endif

/*#include <map.h>*/
#include <loop_adapt_configuration_types.h>
#include <loop_adapt_configuration_backends.h>


static LoopAdaptInputConfigurationFunctions* loop_adapt_configuration_funcs_input = NULL;
static LoopAdaptOutputConfigurationFunctions* loop_adapt_configuration_funcs_output = NULL;

/*typedef struct {*/
/*    int stop;*/
/*    pthread_t _pthread;*/
/*    int exitcode;*/

/*} ConfigurationThreadData;*/

/*void _loop_adapt_configuration_thread(void* data)*/
/*{*/
/*    int err = 0;*/
/*    ConfigurationThreadData* tdata = (ConfigurationThreadData*)data;*/
/*    if (!tdata)*/
/*    {*/
/*        pthread_exit(NULL);*/
/*    }*/
/*    while (!tdata->stop)*/
/*    {*/
/*        // do sql query*/
/*        SQLResponse_t sql = NULL;*/
/*        err = _loop_adapt_analyse_sql(&sql);*/
/*        if (err == 0)*/
/*        {*/
/*            LoopAdaptConfiguration_t config = NULL;*/
/*            err = get_smap_by_key(loop_adapt_new_configuration_hash, sql->loop, (void**)&config);*/
/*            if (err != 0 && !config)*/
/*            {*/
/*                config = malloc(sizeof(LoopAdaptConfiguration));*/
/*                if (!config)*/
/*                {*/
/*                    fprintf(stderr, "Failed to parse SQL input\n");*/
/*                }*/
/*                memset(config, 0, sizeof(LoopAdaptConfiguration));*/
/*            }*/
/*            // update Configuration*/
/*            loop_adapt_new_configuration_available = 1;*/
/*        }*/
/*        else*/
/*        {*/
/*            fprintf(stderr, "Failed to parse SQL input\n");*/
/*        }*/
/*    }*/
/*    return;*/
/*}*/


void loop_adapt_configuration_destroy_config(LoopAdaptConfiguration_t config)
{
    int i = 0;
    if (config)
    {
        if (config->parameters)
        {
            for (i = 0; i < config->num_parameters; i++)
            {
                LoopAdaptConfigurationParameter* p = &config->parameters[i];
                if (p)
                {
                    bdestroy(p->parameter);
                    free(p->values);
                    p->values = NULL;
                    p->num_values = 0;
                }
            }
            free(config->parameters);
            config->parameters = NULL;
            config->num_parameters = 0;
        }
        memset(config, 0, sizeof(LoopAdaptConfiguration));
        free(config);
        config = NULL;
    }
}


int loop_adapt_configuration_resize_config(LoopAdaptConfiguration_t *configuration, int num_parameters)
{
    LoopAdaptConfiguration_t config = *configuration;
    if (!config)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Allocating new configuration);
        config = malloc(sizeof(LoopAdaptConfiguration));
        memset(config, 0, sizeof(LoopAdaptConfiguration));
        config->parameters = NULL;
        config->configuration_id = -1;
    }
    if (config && num_parameters > 0)
    {
        if (num_parameters > config->num_parameters)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Increase size of parameters from %d to %d, config->num_parameters, num_parameters);
            LoopAdaptConfigurationParameter* ptmp = realloc(config->parameters, (num_parameters)*sizeof(LoopAdaptConfigurationParameter));
            if (!ptmp)
            {
                return -ENOMEM;
            }
            config->parameters = ptmp;
        }
        *configuration = config;
        return 0;
    }
    return -EINVAL;
}

int loop_adapt_configuration_initialize()
{
    int err_input = 0, err_output = 0;
    ConfigurationInputBackendTypes type = LA_CONFIG_IN_TXT;

    char* env_in_type = getenv("LA_CONFIG_INPUT_TYPE");
    if (!env_in_type)
    {
        ERROR_PRINT(Environment variable LA_CONFIG_INPUT_TYPE not set. Supported: %d-%d, LA_CONFIG_IN_MIN, LA_CONFIG_IN_MAX-1);
        return -1;
    }
    char* env_out_type = getenv("LA_CONFIG_OUTPUT_TYPE");
    if (!env_out_type)
    {
        ERROR_PRINT(Environment variable LA_CONFIG_OUTPUT_TYPE not set. Supported: %d-%d, LA_CONFIG_OUT_MIN, LA_CONFIG_OUT_MAX-1);
        return -1;
    }

    type = (ConfigurationInputBackendTypes)atoi(env_in_type);
    if (type >= LA_CONFIG_IN_MIN && type < LA_CONFIG_IN_MAX)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Configuring input functions to type %d, type);
        loop_adapt_configuration_funcs_input = &loop_adapt_configuration_input_list[type];
        if (   loop_adapt_configuration_funcs_input
            && loop_adapt_configuration_funcs_input->init)
        {
            err_input = loop_adapt_configuration_funcs_input->init();
            if (err_input)
            {
                ERROR_PRINT(Initialization of input backend failed)
            }
        }
    }
    else
    {
        ERROR_PRINT(Environment variable LA_CONFIG_INPUT_TYPE contains invalid type. Supported: %d-%d, LA_CONFIG_IN_MIN, LA_CONFIG_IN_MAX-1);
        err_input = -1;
    }

    type = (ConfigurationOutputBackendTypes)atoi(env_out_type);
    if (type >= LA_CONFIG_OUT_MIN && type < LA_CONFIG_OUT_MAX)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Configuring output functions to type %d, type);
        loop_adapt_configuration_funcs_output = &loop_adapt_configuration_output_list[type];
        if (   loop_adapt_configuration_funcs_output
            && loop_adapt_configuration_funcs_output->init)
        {
            err_output = loop_adapt_configuration_funcs_output->init();
            if (err_output)
            {
                ERROR_PRINT(Initialization of output backend failed)
            }
        }
    }
    else
    {
        ERROR_PRINT(Environment variable LA_CONFIG_OUTPUT_TYPE contains invalid type. Supported: %d-%d, LA_CONFIG_OUT_MIN, LA_CONFIG_OUT_MAX-1);
        err_output = -1;
    }

    return err_input + err_output;
}

void loop_adapt_configuration_finalize()
{
    if (loop_adapt_configuration_funcs_input)
    {
        if (loop_adapt_configuration_funcs_input->finalize)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling finalize function of input backend);
            loop_adapt_configuration_funcs_input->finalize();
        }
        loop_adapt_configuration_funcs_input = NULL;
    }
    if (loop_adapt_configuration_funcs_output)
    {
        if (loop_adapt_configuration_funcs_output->finalize)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling finalize function of output backend);
            loop_adapt_configuration_funcs_output->finalize();
        }
        loop_adapt_configuration_funcs_output = NULL;
    }
}



int loop_adapt_get_new_configuration(char* string, int config_id, LoopAdaptConfiguration_t *config)
{
    int err = -EINVAL;
    if (   string
        && config
        && loop_adapt_configuration_funcs_input
        && loop_adapt_configuration_funcs_input->getnew)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling getnew function of input backend);
        err = loop_adapt_configuration_funcs_input->getnew(string, config_id, config);
    }
    return err;
}

// int loop_adapt_get_current_configuration(char* string, LoopAdaptConfiguration_t *config)
// {
//     int err = -EINVAL;
//     if (   string
//         && config
//         && loop_adapt_configuration_funcs_input
//         && loop_adapt_configuration_funcs_input->getcurrent)
//     {
//         DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling getcurrent function of input backend);
//         *config = loop_adapt_configuration_funcs_input->getcurrent(string);
//         err = 0;
//     }
//     return err;
// }

int loop_adapt_write_configuration_results(ThreadData_t thread, char* loopname, PolicyDefinition_t policy, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
    if (   policy && results
        && loop_adapt_configuration_funcs_output
        && loop_adapt_configuration_funcs_output->write)
    {
#ifdef MPI
        if (thread->mpirank == 0)
        {
#endif
            if (thread->thread == 0)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling write function of output backend);
                return loop_adapt_configuration_funcs_output->write(thread, loopname, policy, config, num_results, results);
            }
            else
            {
                return 0;
            }
#ifdef MPI
        }
#endif
    }
    else
    {
        ERROR_PRINT(Cannot write configuration because of missing input data or results != measurements);
    }
    return -EINVAL;
}


int loop_adapt_config_parse_default_entry(bstring b, bstring* first, bstring* second, bstring* third)
{
    int i = 0;
    int eq = 0;
    int col = -1;
    int (*ownatoi)(const char *nptr) = &atoi;
    if ((!first) || (!second) || (!third))
    {
        return -EINVAL;
    }
    *first = NULL;
    *second = NULL;
    *third = NULL;

    eq = bstrchrp(b, '=', 0);
    if (eq == BSTR_ERR)
    {
        return -EINVAL;
    }
    col = bstrchrp(b, ':', eq);
    if (eq == BSTR_ERR)
    {
        return -EINVAL;
    }
    if (eq > 0 && col == -1)
    {
        *first = bmidstr(b, 0, eq);
        *second = bmidstr(b, eq+1, blength(b)-eq);
    }
    else
    {
        *first = bmidstr(b, 0, eq);
        *second = bmidstr(b, eq+1, col-eq-1);
        *third = bmidstr(b, col+1, blength(b));
    }
    return 0;
}
