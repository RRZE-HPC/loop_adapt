/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_measurement.c
 *
 *      Description:  Implementation of the measurement backend for loop_adapt
 *
 *      Version:   5.0
 *      Released:  10.11.2019
 *
 *      Author:   Thoams Roehl (tr), thomas.roehl@gmail.com
 *      Project:  likwid
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
#include <error.h>
#include <stdint.h>

#include <loop_adapt_measurement_types.h>
#include <loop_adapt_measurement_list.h>
#include <loop_adapt_configuration_types.h>
#include <loop_adapt_hwloc_tree.h>
#include <loop_adapt_lock.h>
#include <loop_adapt_threads.h>


static MeasurementDefinition* loop_adapt_active_measurements = NULL;
static int loop_adapt_num_active_measurements = 0;

static hwloc_topology_t loop_adapt_measurement_tree = NULL;

static Measurement_t _loop_adapt_new_measurement()
{
    Measurement_t p = malloc(sizeof(Measurement));
    if (!p)
    {
        fprintf(stderr, "ERROR: Cannot allocate new measurement\n");
        return NULL;
    }
    memset(p, 0, sizeof(Measurement));
    p->responsible = LOOP_ADAPT_LOCK_INIT;
    p->instance = -1;
    p->measure_list_idx = -1;
    p->state = LOOP_ADAPT_MEASUREMENT_STATE_NONE;
    return p;
}

static int _loop_adapt_copy_measurement_definition(MeasurementDefinition* in, MeasurementDefinition*out)
{
    if (in && out)
    {
        out->name = malloc(sizeof(char) * (strlen(in->name)+2));
        int err = snprintf(out->name, strlen(in->name)+1, "%s", in->name);
        if (err > 0)
        {
            out->name[err] = '\0';
        }
        out->scope = in->scope;
        out->init = in->init;
        out->setup = in->setup;
        out->start = in->start;
        out->startall = in->startall;
        out->stop = in->stop;
        out->stopall = in->stopall;
        out->result = in->result;
        out->finalize = in->finalize;

        return 0;
    }
    return -1;
}

void loop_adapt_measurement_finalize()
{
    int j = 0;
    if (loop_adapt_active_measurements)
    {
        if (loop_adapt_num_active_measurements > 0)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize measurement system with %d backends, loop_adapt_num_active_measurements);
            for (j = 0; j < loop_adapt_num_active_measurements; j++)
            {
                MeasurementDefinition* md = &loop_adapt_active_measurements[j];
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize measurement system %s, md->name);
                if (md->finalize)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling finalize function of measurement system %s, md->name);
                    md->finalize();
                }
                free(md->name);
            }
            loop_adapt_num_active_measurements = 0;
        }
        free(loop_adapt_active_measurements);
        loop_adapt_active_measurements = NULL;
    }

    if (loop_adapt_measurement_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize measurement system tree);
        hwloc_topology_destroy(loop_adapt_measurement_tree);
        loop_adapt_measurement_tree = NULL;
    }
}

int loop_adapt_measurement_initialize()
{
    int md_idx = 0;
/*    MeasurementDefinition* md = &loop_adapt_measurement_list[md_idx];*/
/*    while (md->name != NULL)*/
/*    {*/
/*        // Next parameter in list*/
/*        md_idx++;*/
/*        md = &loop_adapt_measurement_list[md_idx];*/
/*    }*/
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize measurement system with %d backends, loop_adapt_measurement_list_count);
    loop_adapt_active_measurements = malloc(loop_adapt_measurement_list_count * sizeof(MeasurementDefinition));
    if (!loop_adapt_active_measurements)
    {
        fprintf(stderr, "Failed to allocate space for active measurements\n");
        return 1;
    }
    for (int j = 0; j < loop_adapt_measurement_list_count; j++)
    {
        MeasurementDefinition* in = &loop_adapt_measurement_list[j];
        MeasurementDefinition* out = &loop_adapt_active_measurements[j];
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize measurement system %s, in->name);
        _loop_adapt_copy_measurement_definition(in, out);
        if (out->init)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling init function of measurement system %s, in->name);
            out->init();
        }
        loop_adapt_num_active_measurements++;
    }
    if (!loop_adapt_measurement_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize measurement system tree);
        int err = loop_adapt_copy_hwloc_tree(&loop_adapt_measurement_tree);
        if (err)
        {
            ERROR_PRINT(Failed to copy hwloc tree for measurements);
            loop_adapt_measurement_finalize();
            return err;
        }
    }
}

static void _loop_adapt_measurement_destroy(gpointer mptr)
{
    Measurement_t m = (Measurement_t)mptr;
    if (m)
    {
        m->responsible = LOOP_ADAPT_LOCK_INIT;
        m->measure_list_idx = 0;
        bdestroy(m->configuration);
        bdestroy(m->metrics);
        m->instance = 0;
        m->state = LOOP_ADAPT_MEASUREMENT_STATE_NONE;
        free(m);
    }
}

int loop_adapt_measurement_setup(ThreadData_t thread, char* measurement, bstring configuration, bstring metrics)
{
    int i = 0;
    int s = 0;
    int md_idx = -1;
    MeasurementDefinition* md = NULL;
    for (i = 0; i < loop_adapt_num_active_measurements; i++)
    {
        if (strncmp(measurement, loop_adapt_active_measurements[i].name, strlen(loop_adapt_active_measurements[i].name)) == 0)
        {
            md = &loop_adapt_active_measurements[i];
            md_idx = i;
            break;
        }
    }
    if (!md)
    {
        ERROR_PRINT(No measurement backend %s, measurement);
        return -EINVAL;
    }


    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Setup measurement system %s for thread %d (scope %s %d), md->name, thread->thread, hwloc_obj_type_string(md->scope), thread->scopeOffsets[md->scope]);
    for (s=0;s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
    	if (LoopAdaptScopeList[s] == md->scope)
    		break;
    }
    hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_measurement_tree, md->scope, thread->scopeOffsets[s]);
    if (obj)
    {
        Map_t measurements = (Map_t)obj->userdata;
        if (!measurements)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize measurement map at %s %d, hwloc_obj_type_string(obj->type), obj->logical_index);
            init_smap(&measurements, _loop_adapt_measurement_destroy);
            obj->userdata = (void*)measurements;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Setup measurement %s at %s %d, measurement, hwloc_obj_type_string(md->scope), obj->logical_index);
        Measurement_t m = _loop_adapt_new_measurement();
        if (m)
        {
            m->measure_list_idx = md_idx;
            lock_acquire(&m->responsible, thread->objidx);
            if (md->scope == LOOP_ADAPT_SCOPE_THREAD)
                m->instance = thread->thread;
            else
                m->instance = obj->logical_index;
            m->configuration = bstrcpy(configuration);
            m->metrics = bstrcpy(metrics);
            loop_adapt_active_measurements[md_idx].setup(m->instance, m->configuration, m->metrics);
            m->state = LOOP_ADAPT_MEASUREMENT_STATE_SETUP;
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Adding measurement %s=%s:%s at %s %d (state %d) %p, measurement, bdata(m->configuration), bdata(m->metrics), hwloc_obj_type_string(obj->type), obj->logical_index, m->state, m);
            add_smap(measurements, measurement, (void*)m);
        }
    }
    return 0;
}

int loop_adapt_measurement_start(ThreadData_t thread, char* measurement)
{
    int i = 0;
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Starting measurement %s for thread %d, measurement, thread->thread);
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_measurement_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Measurement_t m = NULL;
            Map_t measurements = (Map_t)obj->userdata;
            if (!measurements)
            {
                continue;
            }
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Checking measurement map at %s %d for thread %d, hwloc_obj_type_string(obj->type), obj->logical_index, thread->thread);
            int err = get_smap_by_key(measurements, measurement, (void**)&m);
            if (err == 0)
            {
                if (m->responsible != thread->objidx)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Thread %d not responsible for measurement %s, thread->thread, measurement);
                    return 0;
                }
                if (m->state == LOOP_ADAPT_MEASUREMENT_STATE_STOPPED ||
                    m->state == LOOP_ADAPT_MEASUREMENT_STATE_SETUP)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling start function for measurement %s (%d) with instance %d, measurement, m->measure_list_idx, m->instance);
                    loop_adapt_active_measurements[m->measure_list_idx].start(m->instance);
                    m->state = LOOP_ADAPT_MEASUREMENT_STATE_RUNNING;
                    return 0;
                }
            }
            else
            {
                WARN_PRINT(Measurement %s not set up, measurement);
            }
        }
    }
    return 0;
}


int loop_adapt_measurement_start_all()
{
    int i = 0;
    for (i = 0; i < loop_adapt_num_active_measurements; i++)
    {
        
        if (loop_adapt_active_measurements[i].startall)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling startall function for measurement %s, loop_adapt_active_measurements[i].name);
            loop_adapt_active_measurements[i].startall();
        }
        else
        {
            int j = 0;
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling start function for measurement %s for all threads, loop_adapt_active_measurements[i].name);
            for (j = 0; j < loop_adapt_threads_get_count(); j++)
            {
                ThreadData_t thread = loop_adapt_threads_getthread(i);
                loop_adapt_measurement_start(thread, loop_adapt_active_measurements[i].name);
            }
        }
    }
    return 0;
}


int loop_adapt_measurement_stop(ThreadData_t thread, char* measurement)
{
    int i = 0;
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Stopping measurement %s for thread %d, measurement, thread->thread);
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_measurement_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Measurement_t m = NULL;
            Map_t measurements = (Map_t)obj->userdata;
            if (!measurements)
            {
                continue;
            }
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Checking measurement map at %s %d for thread %d, hwloc_obj_type_string(obj->type), obj->logical_index, thread->thread);
            int err = get_smap_by_key(measurements, measurement, (void**)&m);
            if (err == 0)
            {
                if (m->responsible != thread->objidx)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Thread %d not responsible for measurement %s, thread->thread, measurement);
                    return 0;
                }
                if (m->state == LOOP_ADAPT_MEASUREMENT_STATE_RUNNING)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling stop function for measurement %s (%d) with instance %d, measurement, m->measure_list_idx, m->instance);
                    loop_adapt_active_measurements[m->measure_list_idx].stop(m->instance);
                    m->state = LOOP_ADAPT_MEASUREMENT_STATE_STOPPED;
                    return 0;
                }
            }
        }
    }
    return 0;
}



int loop_adapt_measurement_stop_all()
{
    int i = 0;
    for (i = 0; i < loop_adapt_num_active_measurements; i++)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling stopall function for measurement %s, loop_adapt_active_measurements[i].name);
        loop_adapt_active_measurements[i].stopall();
    }
    return 0;
}

int loop_adapt_measurement_result(ThreadData_t thread, char* measurement, int num_values, ParameterValue* values)
{
    int i = 0;
    int err = 0;
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting measurement results (thread: %d, measurement: %s), thread->thread, measurement);
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES ; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_measurement_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Map_t measurements = (Map_t)obj->userdata;
            if (!measurements) continue;
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Checking measurement map at %s %d for thread %d, hwloc_obj_type_string(obj->type), obj->logical_index, thread->thread);
            Measurement_t m = NULL;
            err = get_smap_by_key(measurements, measurement, (void**)&m);
            if (err == 0)
            {
                if (m->responsible == thread->objidx)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Calling getresult function for measurement %s (%d) with instance %d, measurement, m->measure_list_idx, m->instance);
                    err = loop_adapt_active_measurements[m->measure_list_idx].result(m->instance, num_values, values);
                }
            }
        }
    }
    return err;
}


int loop_adapt_measurement_available(char* measurement)
{
    int i = 0;
    for (i = 0; i < loop_adapt_num_active_measurements; i++)
    {
        if (strncmp(measurement, loop_adapt_active_measurements[i].name, strlen(loop_adapt_active_measurements[i].name)) == 0)
        {
            return 1;
        }
    }
    return 0;
}


int loop_adapt_measurement_num_metrics(ThreadData_t thread)
{
    int count = 0;
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES ; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_measurement_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Map_t measurements = (Map_t)obj->userdata;
            if (!measurements) continue;
            count += get_smap_size(measurements);
        }
    }
    return count;
}


int loop_adapt_measurement_announce(LoopAdaptAnnounce_t announce)
{
    int i = 0;
    int ret = 0;
    struct bstrList* configs = bstrListCreate();
    for (i = 0; i < loop_adapt_num_active_measurements; i++)
    {
        if (loop_adapt_active_measurements[i].configs)
        {
            ret += loop_adapt_active_measurements[i].configs(configs);
        }
    }
    return ret;
}