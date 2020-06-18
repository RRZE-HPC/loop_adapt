#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <errno.h>
#include <loop_adapt_configuration_cc_client_base.hpp>
#include <bstrlib.h>
#include <loop_adapt_threads.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_parameter_value.h>


using namespace std;

extern "C"
{
// Loop_adapt internal functions accessing the loop data
#include <loop_adapt_internal.h>
// A parameter value is a container for different types like integer, double, string, boolean, ...
#include <loop_adapt_parameter_value.h>
// To access main parameter functions like "get parameter names"
#include <loop_adapt_parameter.h>
// A configuration is a runtime configuration that tells loop_adapt to run the loop with these parameter settings
// and measure the desired metrics
#include <loop_adapt_configuration.h>
// That's the header exporting the function to the remaining C-library
#include <loop_adapt_configuration_cc_client.h>
// Include the hash table implementation
#include <map.h>
}

// Global hash table for loopname -> Client relation
static Map_t cc_client_hash = NULL;
// Read the data, user and password only once
static char* data = NULL;
static char* user = NULL;
static char* pass = NULL;
static char prog_name[300];


typedef struct {
    Client* client;
    LoopAdaptConfiguration_t current;
} CCConfig;


// This function is called when destroying the hash table to close all clients
static void _cc_client_hash_final_delete(mpointer p)
{
    CCConfig* cc_config = (CCConfig* )p;
    if (cc_config)
    {
        //cc_config->client->close();
        if (cc_config->current != NULL)
        {
            loop_adapt_configuration_destroy_config(cc_config->current);
        }
    }
}

// Read the current executable name from /proc filesystem
static void read_executable_name()
{
    int ret = 0;
    FILE* fp = NULL;
    fp = fopen("/proc/self/comm", "r");
    if (fp)
    {
        ret = fread(prog_name, sizeof(char), 199, fp);
        if (ret > 0)
        {
            prog_name[ret] = '\n';
        }
        fclose(fp);
    }

}

// Currently the init function does not get any information from loop_adapt but here we need
// username/password etc. Reading it from enviroment is currently only used for
// demonstration.
extern "C" int loop_adapt_config_cc_client_init()
{
    int err = 0;
    // Create the hash table if it does not exist
    if (!cc_client_hash)
    {
        err = init_smap(&cc_client_hash, _cc_client_hash_final_delete);
        if (err)
        {
            ERROR_PRINT(Failed to initialize hash table for loopname -> OpenTuner client);
        }
    }

    // Read some basic information from the environment
    data = getenv("LA_CONFIG_CC_DATA");
    user = getenv("LA_CONFIG_CC_USER");
    pass = getenv("LA_CONFIG_CC_PASS");
    read_executable_name();
    // The project is the loop_name

}

extern "C" void loop_adapt_config_cc_client_finalize()
{
    if (cc_client_hash)
    {
        // Destroy the hash table. This calls the function _cc_client_hash_final_delete for each element
        destroy_smap(cc_client_hash);
        cc_client_hash = NULL;
    }
}


// We get a string identifying the loop. If we should be able to handle multiple
// loops simultaneously, we should hand it over to client.set_config() which
// returns the per-loop config. Moreover, the client.current variable should be
// a hash map string -> current config
extern "C" int loop_adapt_get_new_config_cc_client(char* string, int config_id, LoopAdaptConfiguration_t* configuration)
{
    int i = 0;
    int j = 0;
    int err = 0;
    if ((!cc_client_hash) || (!string))
        return -EINVAL;
    CCConfig * cc_config = NULL;

    // Check whether there is already a cc_config for the loop
    err = get_smap_by_key(cc_client_hash, string, (void**)&cc_config);
    if (err != 0)
    {
        std::string proj_name = string;
        cc_config = (CCConfig*) malloc(sizeof(CCConfig));
        if (!cc_config)
        {
            return -ENOMEM;
        }
        cc_config->current = NULL;

        // Create an empry list of strings
        struct bstrList* loop_params = bstrListCreate();
        int num_params = loop_adapt_get_loop_parameter(string, loop_params);

        struct bstrList* available_configs = bstrListCreate();
        int avail_configs = loop_adapt_parameter_configs(available_configs);

        std::vector<std::string> otparameters;
        for (i = 0; i < num_params; i++)
        {
            for (j = 0; j < avail_configs; j++)
            {
                if (bstrncmp(loop_params->entry[i], available_configs->entry[j], blength(loop_params->entry[i])) == BSTR_OK)
                {
                    otparameters.push_back(bdata(available_configs->entry[j]));
                }
            }
        }
        bstrListDestroy(loop_params);
        bstrListDestroy(available_configs);
        std::vector<std::string> otarguments;
        // Add default arguments

        // Hash table empty for loop, create a new client
        cc_config->client = new Client(data, user, pass, otarguments, otparameters, prog_name, proj_name, NULL);


        // Add new client to hash map;
        add_smap(cc_client_hash, string, (void*)cc_config);
    }
    // Allocate a fresh Configuration
    LoopAdaptConfiguration_t config = *configuration;
    if (!config)
        config = (LoopAdaptConfiguration_t) malloc(sizeof(LoopAdaptConfiguration));


    // Get all parameter names
    struct bstrList *param_names = bstrListCreate();
    loop_adapt_get_loop_parameter(string, param_names);

    loop_adapt_configuration_resize_config(configuration, param_names->qty);

    cc_config->client->nextConfig();

    // We check for all available parameters whether the client has some values for
    for (i = 0; i < param_names->qty; i++)
    {
        // This here will be tricky because not all parameters will be int;
        std::string param = cc_config->client->getConfigAt(bdata(param_names->entry[i]));
        // add param to config;
        bstring bparam = bfromcstr(param.c_str());
        struct bstrList* tmp = bsplit(bparam, ',');
        ParameterValueType_t type = loop_adapt_parameter_type(bdata(tmp->entry[0]));
        ParameterValue paramvalue = loop_adapt_new_param_value(type);
        loop_adapt_parse_param_value(bdata(tmp->entry[1]), type, &paramvalue);

        for (int j = 0; j < loop_adapt_threads_get_count(); j++)
        {
            ThreadData_t t = loop_adapt_threads_getthread(j);
            loop_adapt_parameter_set(t, bdata(tmp->entry[0]), paramvalue);
        }
        loop_adapt_destroy_param_value(paramvalue);
        bstrListDestroy(tmp);
        bdestroy(bparam);
    }

    if (cc_config->current)
    {
        // We don't need the last configuration anymore, so destroy it.
        loop_adapt_configuration_destroy_config(cc_config->current);
    }
    // Update current configuration for easy access later
    cc_config->current = config;
    bstrListDestroy(param_names);
    *configuration = config;
    return 0;
}

// Return the current configuration. See comment at loop_adapt_get_new_config_cc_client
// for string function argument for per-loop configurations
// extern "C" LoopAdaptConfiguration_t loop_adapt_get_current_config_cc_client(char* string)
// {
//     if (cc_client_hash)
//     {
//         CCConfig * cc_config = NULL;
//         // Get loop-specific config from hash table
//         int err = get_smap_by_key(cc_client_hash, string, (void**)&cc_config);
//         if (err == 0 && cc_config != NULL)
//         {
//             // Return the current configuration
//             return cc_config->current;
//         }
//     }
//     return NULL;
// }


// This is just an example (sum all values). This is going to be an extra module or part of the measurement module
double loop_adapt_policy_eval(char* loopname, int num_results, ParameterValue* results)
{
    ParameterValue r = loop_adapt_new_param_value(results[0].type);
    int i = 0;
    for (i = 0; i < num_results; i++)
    {
        loop_adapt_add_param_value(r, results[i], &r);
    }
    loop_adapt_cast_param_value(&r, LOOP_ADAPT_PARAMETER_TYPE_DOUBLE);
    return (double)r.value.dval;
}

// Write out measurement results
extern "C" int loop_adapt_config_cc_client_write(ThreadData_t thread, char* loopname, PolicyDefinition_t policy, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
    if (cc_client_hash)
    {
        CCConfig * cc_config = NULL;
        // Get loop-specific config from hash table
        int err = get_smap_by_key(cc_client_hash, loopname, (void**)&cc_config);
        if (err == 0 && cc_config != NULL)
        {
            // Evaluate the measurements using the policy registered for the loop
            double result = 0;//loop_adapt_policy_eval(loopname, num_results, results);
            if (policy->eval)
            {
                policy->eval(num_results, results, &result);
            }
            // Send result to OpenTuner
            TODO_PRINT(WTF happened to report config?);
            //cc_config->client->report_config(result);
            return 0;
        }
    }
    return -ENODEV;
}

//L3[hostname,threadid]=value|...
