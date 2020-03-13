/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_prefetcher.h
 *
 *      Description:  Parameter functions for prefetcher manipulation
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@fau.de
 *      Project:  loop_adapt
 *
 *      Copyright (C) 2020 RRZE, University Erlangen-Nuremberg
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

#include <loop_adapt_parameter_types.h>
#include <loop_adapt_threads.h>
#include <error.h>
#include <likwid.h>

static int loop_adapt_parameter_prefetcher_initialized = 0;

int loop_adapt_parameter_prefetcher_avail(int instance, ParameterValueLimit* limit)
{
    if (limit->type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST && limit->limit.list.num_values > 0 && limit->limit.list.values)
    {
        loop_adapt_destroy_param_limit(*limit);
    }
    limit->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST;
    ParameterValue t = DEC_NEW_BOOL_PARAM_VALUE(1);
    ParameterValue f = DEC_NEW_BOOL_PARAM_VALUE(0);
    loop_adapt_add_param_limit_list(limit, t);
    loop_adapt_add_param_limit_list(limit, f);

    return 2;
}

int loop_adapt_parameter_prefetcher_init()
{
    if (!loop_adapt_parameter_prefetcher_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initializing LIKWID cpuFeatures);
        cpuFeatures_init();
        loop_adapt_parameter_prefetcher_initialized = 1;
    }
    return 0;
}

void loop_adapt_parameter_prefetcher_finalize()
{
    if (loop_adapt_parameter_prefetcher_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize LIKWID cpuFeatures);
        //cpuFeatures_finalize();
        loop_adapt_parameter_prefetcher_initialized = 0;
    }
}

//int loop_adapt_parameter_prefetcher_hwpf_set(int instance, ParameterValue value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    if (value.value.bval == TRUE)
//    {
//        cpuFeatures_enable(instance, FEAT_HW_PREFETCHER, 0);
//    }
//    else
//    {
//        cpuFeatures_disable(instance, FEAT_HW_PREFETCHER, 0);
//    }
//}


//int loop_adapt_parameter_prefetcher_hwpf_get(int instance, ParameterValue* value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    value->value.bval = cpuFeatures_get(instance, FEAT_HW_PREFETCHER);
//    value->type = LOOP_ADAPT_PARAMETER_TYPE_BOOL;
//    return 0;
//}

#define LOOP_ADAPT_PREF_FUNCS(NAME, PFNAME) \
    int loop_adapt_parameter_prefetcher_##NAME##_set(int instance, ParameterValue value) \
    { \
        if (!loop_adapt_parameter_prefetcher_initialized) \
        { \
            return -1; \
        } \
        int cpu = loop_adapt_threads_get_cpu(instance); \
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set current setting of PFNAME for CPU %d to %u, cpu, value.value.bval); \
        if (cpu >= 0) \
        { \
            if (value.value.bval == TRUE) \
            { \
                cpuFeatures_enable(instance, PFNAME, 0); \
            } \
            else \
            { \
                cpuFeatures_disable(instance, PFNAME, 0); \
            } \
            return 0; \
        } \
        return -EINVAL; \
    } \
    int loop_adapt_parameter_prefetcher_##NAME##_get(int instance, ParameterValue* value) \
    { \
        if (!loop_adapt_parameter_prefetcher_initialized) \
        { \
            return -1; \
        } \
        int cpu = loop_adapt_threads_get_cpu(instance); \
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get current setting of PFNAME for CPU %d, cpu); \
        if (cpu >= 0) \
        { \
            value->value.bval = cpuFeatures_get(instance, PFNAME); \
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get current setting of PFNAME for CPU %d: %u, cpu, value->value.bval); \
            value->type = LOOP_ADAPT_PARAMETER_TYPE_BOOL; \
            return 0; \
        } \
        return -EINVAL; \
    }

LOOP_ADAPT_PREF_FUNCS(hwpf, FEAT_HW_PREFETCHER)
LOOP_ADAPT_PREF_FUNCS(clpf, FEAT_CL_PREFETCHER)
LOOP_ADAPT_PREF_FUNCS(dcupf, FEAT_DCU_PREFETCHER)
LOOP_ADAPT_PREF_FUNCS(ippf, FEAT_IP_PREFETCHER)

//int loop_adapt_parameter_prefetcher_dcupf_set(int instance, ParameterValue value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    if (value.value.bval == TRUE)
//    {
//        cpuFeatures_enable(instance, FEAT_DCU_PREFETCHER, 0);
//    }
//    else
//    {
//        cpuFeatures_disable(instance, FEAT_DCU_PREFETCHER, 0);
//    }
//}


//int loop_adapt_parameter_prefetcher_dcupf_get(int instance, ParameterValue* value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    value->value.bval = cpuFeatures_get(instance, FEAT_DCU_PREFETCHER);
//    value->type = LOOP_ADAPT_PARAMETER_TYPE_BOOL;
//    return 0;
//}

//int loop_adapt_parameter_prefetcher_ippf_set(int instance, ParameterValue value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    if (value.value.bval == TRUE)
//    {
//        cpuFeatures_enable(instance, FEAT_IP_PREFETCHER, 0);
//    }
//    else
//    {
//        cpuFeatures_disable(instance, FEAT_IP_PREFETCHER, 0);
//    }
//}


//int loop_adapt_parameter_prefetcher_ippf_get(int instance, ParameterValue* value)
//{
//    if (!loop_adapt_parameter_prefetcher_initialized)
//    {
//        return -1;
//    }
//    value->value.bval = cpuFeatures_get(instance, FEAT_IP_PREFETCHER);
//    value->type = LOOP_ADAPT_PARAMETER_TYPE_BOOL;
//    return 0;
//}
