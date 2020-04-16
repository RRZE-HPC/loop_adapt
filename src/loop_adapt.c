/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt.c
 *
 *      Description:  Main file of loop_adapt
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@fau.de
 *      Project:  loop_adapt
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <dlfcn.h>
#include <sched.h>

#include <hwloc.h>
#include <likwid.h>
#include <map.h>
#include <error.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>

#include <loop_adapt.h>
#include <loop_adapt_hwloc_tree.h>
#include <loop_adapt_threads.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_parameter_value.h>
#include <loop_adapt_measurement.h>
#include <loop_adapt_configuration.h>

/*! \brief  This is the base hwloc topology tree used later to create copies for each registered loop */
/*static hwloc_topology_t topotree = NULL;*/

/*! \brief  This is the global hash table, the main entry point to the loop_adapt data */
static Map_t loop_adapt_global_hash = NULL;
/*! \brief  This is the lock for the global hash table */
static pthread_mutex_t loop_adapt_global_hash_lock = PTHREAD_MUTEX_INITIALIZER;
/*! \brief Basic lock securing the loop_adapt data structures */
static pthread_mutex_t loop_adapt_lock = PTHREAD_MUTEX_INITIALIZER;




int loop_adapt_active = 1;
LoopAdaptDebugLevel loop_adapt_verbosity = LOOP_ADAPT_DEBUGLEVEL_ONLYERROR;



/*
###############################################################################
# Helper functions for allocation and deallocation of loop objects
###############################################################################
*/
static LoopThreadData_t _loop_adapt_new_loopdata_thread()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, New loop data thread);
    LoopThreadData_t ldata = malloc(sizeof(LoopThreadData));
    if (!ldata)
    {
        ERROR_PRINT(Cannot allocate new loop data thread);
        return NULL;
    }
    memset(ldata, 0, sizeof(LoopThreadData));
    return ldata;
}

/* This is a callback function that we hand over to the hash map at initialization
 * so that it cleans the values itself at finalization */
static void _loop_adapt_free_loopdata_thread(void* t)
{
    if (t)
    {
        free(t);
    }
}

static LoopData_t _loop_adapt_new_loopdata()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, New loop data);
    LoopData_t ldata = malloc(sizeof(LoopData));
    if (!ldata)
    {
        ERROR_PRINT(Cannot allocate new loop data);
        return NULL;
    }
    /* Initialize data struct representing a single loop */
    memset(ldata, 0, sizeof(LoopData));
    ldata->status = LOOP_STOPPED;
    ldata->filename = NULL;
    ldata->linenumber = -1;
    ldata->parameters = bstrListCreate();
    ldata->policy = bfromcstr("");

    //ldata->threads = g_hash_table_new(g_direct_hash, g_direct_equal);
    //init_map(&ldata->threads, MAP_KEY_TYPE_INT, -1, _loop_adapt_free_loopdata_thread);

    /* Create the hash map for the loops used in this loop execution. Register value deletion callback */
/*    init_imap(&ldata->threads, _loop_adapt_free_loopdata_thread);*/
    pthread_mutex_init(&ldata->lock, NULL);
    return ldata;
}

static void _loop_adapt_destroy_loopdata(gpointer val)
{
    LoopData_t loopdata = (LoopData_t)val;
    if (loopdata)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Destroy loop data);
/*        if (loopdata->threads)*/
/*        {*/
/*            destroy_imap(loopdata->threads);*/
/*        }*/
        pthread_mutex_destroy(&loopdata->lock);
        bstrListDestroy(loopdata->parameters);
        bdestroy(loopdata->policy);
        memset(loopdata, 0, sizeof(LoopData));
        free(loopdata);
    }
}

/*
###############################################################################
# Internal functions
###############################################################################
*/



/*
###############################################################################
# Exported functions
###############################################################################
*/

void loop_adapt_debug_level(int level)
{
    if (level >= LOOP_ADAPT_DEBUGLEVEL_MIN && level <= LOOP_ADAPT_DEBUGLEVEL_MAX)
    {
        loop_adapt_verbosity = (LoopAdaptDebugLevel)level;
    }
}

void loop_adapt_enable()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt activate);
    loop_adapt_active = 1;
}

void _loop_adapt_disable()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt deactivate);
    loop_adapt_active = 0;
}

void loop_adapt_finalize()
{
    // Finalize configuration system
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize configuration system);
    loop_adapt_configuration_finalize();
    // Finalize measurement system
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize measurement system);
    loop_adapt_measurement_finalize();
    // Finalize parameter tree
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize parameter tree);
    loop_adapt_parameter_finalize();
    // Finalize thread storage
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize thread storage);
    loop_adapt_threads_finalize();

/*    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Finalize 2nd cpuset);*/
/*    CPU_ZERO(&loop_adapt_cpuset_master);*/


    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Finalize central hwloc tree);
    loop_adapt_hwloc_tree_finalize();

    if (loop_adapt_global_hash)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Finalize global hash);
        destroy_smap(loop_adapt_global_hash);
        loop_adapt_global_hash = NULL;
    }
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt finalize);
    loop_adapt_active = 0;
}


int loop_adapt_initialize()
{
    int err = 0;
    /* Do the initialization only once if loop_adapt is active.
     * We check whether the loop hash map and the hwloc topology tree exists
     */
    if (loop_adapt_active && (!loop_adapt_global_hash))
    {
        /* Initialize the hash map for the loopname -> string relation */
        if (!loop_adapt_global_hash)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Initialize global hash);
            pthread_mutex_lock(&loop_adapt_global_hash_lock);
            err = init_smap(&loop_adapt_global_hash, _loop_adapt_destroy_loopdata);
            pthread_mutex_unlock(&loop_adapt_global_hash_lock);
            if (err != 0)
            {
                ERROR_PRINT(Failed to initialize global hash table);
                loop_adapt_global_hash = NULL;
                _loop_adapt_disable();
                return 1;
            }
        }


        /* Not sure why we get it twice but there was a reason for storing a second version of the cpuset */
/*        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Initialize 2nd cpuset);*/
/*        CPU_ZERO(&loop_adapt_cpuset_master);*/
/*        err = sched_getaffinity(0, sizeof(cpu_set_t), &loop_adapt_cpuset_master);*/
/*        if (err < 0)*/
/*        {*/
/*            ERROR_PRINT(Failed to get cpuset);*/
/*            if (loop_adapt_global_hash)*/
/*            {*/
                /* Destroy the previously created hash map in case of error */
/*                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Destroy global hash);*/
/*                pthread_mutex_lock(&loop_adapt_global_hash_lock);*/
/*                destroy_smap(loop_adapt_global_hash);*/
/*                loop_adapt_global_hash = NULL;*/
/*                pthread_mutex_unlock(&loop_adapt_global_hash_lock);*/
/*            }*/
            /* Disable loop_adapt */
/*            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Disable loop_adapt);*/
/*            _loop_adapt_disable();*/
/*            return 1;*/
/*        }*/
        // Initialize timer
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Initialize timer);
        timer_init();
        // Initialize thread storage
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize thread storage);
        loop_adapt_threads_initialize();
        loop_adapt_threads_register(0);
        // Initialize parameter tree
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize parameter tree);
        loop_adapt_parameter_initialize();
        // Initialize measurement system
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize measurement system);
        loop_adapt_measurement_initialize();
        // Initialize configuration system
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize configuration system);
        loop_adapt_configuration_initialize();
    }
    return 0;
}

void loop_adapt_register(char* string, int num_iterations)
{
    int err = 0;
    hwloc_topology_t dup_tree = NULL;
    if (loop_adapt_active)
    {
        loop_adapt_initialize();
        // Check whether there is already an element in the loopname -> loopdata
        // hash map
        if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&dup_tree) == 0)
        {
            ERROR_PRINT(Loop string %s already registered, string);
            return;
        }
        // Create a copy of the main hwloc topology tree used to represent the loop
        err = loop_adapt_copy_hwloc_tree(&dup_tree);
        if (err)
        {
            ERROR_PRINT(Cannot duplicate topology tree %s: error %d, string, err);
            return;
        }
        LoopData_t ldata = _loop_adapt_new_loopdata();
        ldata->max_iterations = num_iterations;
        ldata->num_iterations = 0;
        ldata->status = LOOP_STOPPED;

        // Safe the global data struct in the topology tree. We can store an
        // object at the root of the tree. It does not work for tree elements later.
        hwloc_topology_set_userdata(dup_tree, ldata);

        // insert the loop-specific tree into the hash table using the string as key
        pthread_mutex_lock(&loop_adapt_global_hash_lock);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Registering loop '%s' with %d iterations per profile %d, string, ldata->max_iterations, ldata->status);
        add_smap(loop_adapt_global_hash, string, (void*) dup_tree);
        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
    }
}

/* This function is called when the application registers a new loop thread */
int loop_adapt_register_thread(int threadid)
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Registering loop thread %d, threadid);
    return loop_adapt_threads_register(threadid);
}

int loop_adapt_add_loop_parameter(char* string, char* parameter)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;
        if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&tree) == 0)
        {
            LoopData_t ldata = NULL;
            

            ldata = (LoopData_t)hwloc_topology_get_userdata(tree);
            if (ldata)
            {
                int i = 0;
                int found = 0;
                bstring bparam = bfromcstr(parameter);
                for (i = 0; i < ldata->parameters->qty; i++)
                {
                    if (bstrcmp(bparam, ldata->parameters->entry[i]) == BSTR_OK)
                    {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    bstrListAdd(ldata->parameters, bparam);
                }
                bdestroy(bparam);
            }
        }
    }
    return 0;
}

int loop_adapt_add_loop_policy(char* string, char* policy)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;
        if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&tree) == 0)
        {
            LoopData_t ldata = NULL;
            
            ldata = (LoopData_t)hwloc_topology_get_userdata(tree);
            if (ldata)
            {
                if (blength(ldata->policy) > 0)
                {
                    ERROR_PRINT(Policy %s already registered for loop %s, bdata(ldata->policy), string);
                    return -EFAULT;
                }
                bdestroy(ldata->policy);
                ldata->policy = bfromcstr(policy);
            }
        }
    }
    return 0;
}

int loop_adapt_get_loop_parameter(char* string, struct bstrList* parameters)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;
        if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&tree) == 0)
        {
            LoopData_t ldata = NULL;
            
            ldata = (LoopData_t)hwloc_topology_get_userdata(tree);
            if (ldata)
            {
                int i = 0;
                for (i = 0; i < ldata->parameters->qty; i++)
                {
                    bstrListAdd(parameters, ldata->parameters->entry[i]);
                }
            }
        }
    }
    return 0;
}

// static void loop_adapt_announce_destroy(LoopAdaptAnnounce_t announce)
// {
//     int i = 0;
//     int j = 0;
//     if (announce)
//     {
//         bdestroy(announce->loopname);

//         if (announce->num_parameters > 0 && announce->parameters)
//         {
//             for (i = 0; i < announce->num_parameters; i++)
//             {
//                 LoopAdaptConfigurationParameter* p = &announce->parameters[i];
//                 for (j = 0; j < p->num_values; j++)
//                 {
//                     loop_adapt_destroy_param_value(p->values[j]);
//                 }
//                 bdestroy(p->parameter);
//                 p->type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
//                 free(p->values);
//                 p->values = NULL;
//                 p->num_values = 0;
//             }
//             free(announce->parameters);
//             announce->parameters = NULL;
//             announce->num_parameters = 0;
//         }
//         if (announce->num_measurements > 0 && announce->measurements)
//         {
//             for (i = 0; i < announce->num_measurements; i++)
//             {
//                 LoopAdaptConfigurationMeasurement* m = &announce->measurements[i];
//                 bdestroy(m->measurement);
//                 bdestroy(m->metric);
//                 bdestroy(m->config);
//             }
//             free(announce->measurements);
//             announce->measurements = NULL;
//             announce->num_measurements = 0;
//         }
//     }
//     return;
// }

// static int loop_adapt_announce(char* string)
// {
//     int err = 0;
//     hwloc_topology_t loop_tree = NULL;
//     if (loop_adapt_active)
//     {
//         if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&loop_tree) == 0)
//         {
//             LoopAdaptAnnounce_t announce = NULL;
//             announce = malloc(sizeof(LoopAdaptAnnounce));
//             if (!announce)
//             {
//                 return -ENOMEM;
//             }
//             announce->loopname = bfromcstr(string);
//             err = loop_adapt_parameter_configs(announce);
//             if (err)
//             {
//                 loop_adapt_announce_destroy(announce);
//                 return err;
//             }
//             err = loop_adapt_parameter_configs(announce);
//             if (err)
//             {
//                 loop_adapt_announce_destroy(announce);
//                 return err;
//             }
//             loop_adapt_configuration_announce(announce);
//         }
//     }
// }




/* This function is called when the loop starts and at the beginning of each loop iteration */
int loop_adapt_start_loop( char* string, char* file, int linenumber )
{
    int i = 0;
    int j = 0;
    int err = 0;
    hwloc_topology_t tree = NULL;
    LoopData_t ldata = NULL;
    ThreadData_t thread = NULL;
    if (loop_adapt_active)
    {
        if (get_smap_by_key(loop_adapt_global_hash, string, (void**)&tree) < 0)
        {
            fprintf(stderr, "ERROR: Loop string %s not registered\n", string);
            return 1;
        }
        ldata = (LoopData_t)hwloc_topology_get_userdata(tree);
        if (ldata)
        {
            if (ldata->status == LOOP_STARTED)
            {
                if (ldata->num_iterations < ldata->max_iterations)
                {
                    ldata->num_iterations++;
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Need more iterations for loop '%s', string);
                    return 1;
                }
                else
                {
                    LoopAdaptConfiguration_t config = NULL;
                    err = loop_adapt_get_current_configuration(string, &config);
                    if (err == 0 && config)
                    {
/*                        if (config->configuration_id != ldata->current_config_id)*/
/*                        {*/
/*                            fprintf(stderr, "ERROR: Configuration mismatch (%d vs %d) for loop %s\n", config->configuration_id, ldata->current_config_id, string);*/
/*                        }*/
                        if (loop_adapt_threads_in_parallel() == 0)
                        {
                            loop_adapt_measurement_stop_all();
                            for (i = 0; i < loop_adapt_threads_get_count(); i++)
                            {
                                thread = loop_adapt_threads_getthread(i);
                                int nmetrics = loop_adapt_measurement_num_metrics(thread);
                                ParameterValue* v = malloc(nmetrics * sizeof(ParameterValue));
                                if (v)
                                {
                                    int tmp = 0;

                                    for (j = 0; j < config->num_measurements; j++)
                                    {
                                        LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                                        tmp += loop_adapt_measurement_result(thread, bdata(cm->measurement), nmetrics - tmp, &v[tmp]);
                                    }
                                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Write %d metrics for config with %d measurements, nmetrics, config->num_measurements);
                                    loop_adapt_write_configuration_results(thread, string, config, nmetrics, v);
                                    free(v);
                                }
                                loop_adapt_parameter_loop_end(thread);
                            }
                            

                        }
                        else
                        {
                            thread = loop_adapt_threads_get();
                            for (j = 0; j < config->num_measurements; j++)
                            {
                                LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                                loop_adapt_measurement_stop(thread, bdata(cm->measurement));
                            }
                            int nmetrics = loop_adapt_measurement_num_metrics(thread);
                            ParameterValue* v = malloc(nmetrics * sizeof(ParameterValue));
                            if (v)
                            {
                                int tmp = 0;

                                for (j = 0; j < config->num_measurements; j++)
                                {
                                    LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                                    tmp += loop_adapt_measurement_result(thread, bdata(cm->measurement), nmetrics - tmp, &v[tmp]);
                                }
                                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Write %d metrics for config with %d measurements, nmetrics, config->num_measurements);
                                loop_adapt_write_configuration_results(thread, string, config, nmetrics, v);
                                free(v);
                            }
                            loop_adapt_parameter_loop_end(thread);
                        }
                        // stop profiling and send data
/*                        loop_adapt_measurement_stop_all(thread);*/
                        
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt stop measurements);
                    }
                    ldata->status = LOOP_STOPPED;
                    ldata->num_iterations = 0;
                    ldata->current_config_id = -1;


                    LoopAdaptConfiguration_t new = NULL;
                    err = loop_adapt_get_new_configuration(string, &new);
                    if (err == 0 && new)
                    {
                        // apply new parameter
                        // configure and start measurements
                        if (loop_adapt_threads_in_parallel() == 0)
                        {
                            for (i = 0; i < loop_adapt_threads_get_count(); i++)
                            {
                                thread = loop_adapt_threads_getthread(i);
                                //_loop_adapt_set_setup_and_start(thread, new);
                                for (j = 0; j < new->num_parameters; j++)
                                {
                                    LoopAdaptConfigurationParameter* cp = &new->parameters[j];
                                    if (cp->num_values > 1 && thread->thread < cp->num_values)
                                    {
                                        loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[thread->thread]);
                                    }
                                    else if (cp->num_values == 1)
                                    {
                                        loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[0]);
                                    }
                                }
                                for (j = 0; j < new->num_measurements; j++)
                                {
                                    LoopAdaptConfigurationMeasurement* cm = &new->measurements[j];
                                    loop_adapt_measurement_setup(thread, bdata(cm->measurement), cm->config, cm->metric);
                                }
                            }
                            loop_adapt_measurement_start_all();

                            // for (i = 0; i < loop_adapt_threads_get_count(); i++)
                            // {
                            //     thread = loop_adapt_threads_getthread(i);
                            //     for (i = 0; i < new->num_measurements; i++)
                            //     {
                            //         LoopAdaptConfigurationMeasurement* cm = &new->measurements[i];
                            //         loop_adapt_measurement_start(thread, bdata(cm->measurement));
                            //     }
                            // }
                        }
                        else
                        {
                            thread = loop_adapt_threads_get();
                            for (i = 0; i < new->num_parameters; i++)
                            {
                                LoopAdaptConfigurationParameter* cp = &new->parameters[i];
                                if (cp->num_values > 1 && thread->thread < cp->num_values)
                                {
                                    loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[thread->thread]);
                                }
                                else if (cp->num_values == 1)
                                {
                                    loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[0]);
                                }
                            }
                            for (i = 0; i < new->num_measurements; i++)
                            {
                                LoopAdaptConfigurationMeasurement* cm = &new->measurements[i];
                                loop_adapt_measurement_setup(thread, bdata(cm->measurement), cm->config, cm->metric);
                            }
                            for (i = 0; i < new->num_measurements; i++)
                            {
                                LoopAdaptConfigurationMeasurement* cm = &new->measurements[i];
                                loop_adapt_measurement_start(thread, bdata(cm->measurement));
                            }
                        }
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt start measurements);
                        ldata->current_config_id = config->configuration_id;
                        ldata->status = LOOP_STARTED;
                        ldata->num_iterations = 1;
                    }
                }
            }
            else
            {
                LoopAdaptConfiguration_t config = NULL;
                err = loop_adapt_get_new_configuration(string, &config);
                if (err == 0 && config)
                {
                    // apply new parameter
                    // configure and start measurements
                    if (loop_adapt_threads_in_parallel() == 0)
                    {
                        for (i = 0; i < loop_adapt_threads_get_count(); i++)
                        {
                            thread = loop_adapt_threads_getthread(i);
                            loop_adapt_parameter_loop_start(thread);
                            //_loop_adapt_set_setup_and_start(thread, config);
                            for (j = 0; j < config->num_parameters; j++)
                            {
                                LoopAdaptConfigurationParameter* cp = &config->parameters[j];
                                if (cp->num_values > 1 && thread->thread < cp->num_values)
                                {
                                    loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[thread->thread]);
                                }
                                else if (cp->num_values == 1)
                                {
                                    loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[0]);
                                }
                            }
                            for (j = 0; j < config->num_measurements; j++)
                            {
                                LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                                loop_adapt_measurement_setup(thread, bdata(cm->measurement), cm->config, cm->metric);
                            }
                        }
                        loop_adapt_measurement_start_all();
                    }
                    else
                    {
                        thread = loop_adapt_threads_get();
                        for (j = 0; j < config->num_parameters; j++)
                        {
                            LoopAdaptConfigurationParameter* cp = &config->parameters[j];
                            if (cp->num_values > 1 && thread->thread < cp->num_values)
                            {
                                loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[thread->thread]);
                            }
                            else if (cp->num_values == 1)
                            {
                                loop_adapt_parameter_set(thread, bdata(cp->parameter), cp->values[0]);
                            }
                        }
                        for (j = 0; j < config->num_measurements; j++)
                        {
                            LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                            loop_adapt_measurement_setup(thread, bdata(cm->measurement), cm->config, cm->metric);
                        }
                        for (j = 0; j < config->num_measurements; j++)
                        {
                            LoopAdaptConfigurationMeasurement* cm = &config->measurements[j];
                            loop_adapt_measurement_start(thread, bdata(cm->measurement));
                        }
                        loop_adapt_parameter_loop_start(thread);
                    }
                    
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt start measurements);
                    ldata->current_config_id = config->configuration_id;
                    ldata->status = LOOP_STARTED;
                    ldata->num_iterations = 1;
                }
                else
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, loop_adapt received no new configuration);
                    return 1;
                }
            }
            //hwloc_topology_set_userdata(tree, (void*)ldata);
        }
    }
    return 1;
}

int loop_adapt_end(char* name)
{
    fprintf(stderr, "loop_adapt_end\n");
    return 1;
}


#define _LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(NAME, TYPE, VAR) \
    NAME loop_adapt_get_##NAME##_parameter(char* string) \
    { \
        ThreadData_t thread = loop_adapt_threads_get(); \
        if (thread) \
        { \
            ParameterValue v; \
            loop_adapt_parameter_get(thread, string, &v); \
            if (v.type == TYPE) \
                return v.value.VAR; \
        } \
        return 0; \
    } \
    int loop_adapt_set_##NAME##_parameter(char* string, NAME value) \
    { \
        ThreadData_t thread = loop_adapt_threads_get(); \
        if (thread) \
        { \
            ParameterValue v; \
            v.type = TYPE; \
            v.value.VAR = value; \
            return loop_adapt_parameter_set(thread, string, v); \
        } \
        return 1; \
    } \
    int loop_adapt_new_##NAME##_parameter(char* string, LoopAdaptScope_t scope, NAME value) \
    { \
        ParameterValue v; \
        v.type = TYPE; \
        v.value.VAR = value; \
        return loop_adapt_parameter_add_user(string, scope, v); \
    }

_LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(char, LOOP_ADAPT_PARAMETER_TYPE_CHAR, cval)
_LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(double, LOOP_ADAPT_PARAMETER_TYPE_DOUBLE, dval)
_LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(float, LOOP_ADAPT_PARAMETER_TYPE_FLOAT, fval)
_LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(int, LOOP_ADAPT_PARAMETER_TYPE_INT, ival)
_LOOP_ADAPT_DEFINE_PARAM_GET_SET_FUNCS(boolean, LOOP_ADAPT_PARAMETER_TYPE_BOOL, bval)

int loop_adapt_get_string_parameter(char* string, char** value)
{
    if ((!string) || (!value))
    {
        return -EINVAL;
    }
    ThreadData_t thread = loop_adapt_threads_get();
    if (thread)
    {
        ParameterValue v;
        loop_adapt_parameter_get(thread, string, &v);
        if (v.type == LOOP_ADAPT_PARAMETER_TYPE_STR)
        {
            int len = strlen(v.value.sval);
            char* s = malloc((len+2) * sizeof(char));
            if (s)
            {
                int ret = snprintf(s, len+1, "%s", v.value.sval);
                if (ret > 0)
                {
                    s[ret] = '\0';
                }
            }
        }
    }
}

int loop_adapt_set_string_parameter(char* string, char* value)
{
    if ((!string) || (!value))
    {
        return -EINVAL;
    }
    ThreadData_t thread = loop_adapt_threads_get();
    if (thread)
    {
        ParameterValue v;
        v.type = LOOP_ADAPT_PARAMETER_TYPE_STR;
        int len = strlen(value);
        v.value.sval = malloc((len+2) * sizeof(char));
        if (v.value.sval)
        {
            int ret = snprintf(v.value.sval, len+1, "%s", value);
            if (ret > 0)
            {
                v.value.sval[ret] = '\0';
            }
            return loop_adapt_parameter_set(thread, string, v);
        }
    }
}

int loop_adapt_register_inparallel_function(int (*in_parallel)(void))
{
    if (in_parallel)
    {
        loop_adapt_threads_register_inparallel_func(in_parallel);
    }
}
