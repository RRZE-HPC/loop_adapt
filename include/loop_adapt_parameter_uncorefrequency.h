/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_uncorefrequency.h
 *
 *      Description:  Parameter functions for Uncore frequency manipulation
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

static int loop_adapt_parameter_uncorefrequency_initialized = 0;


int loop_adapt_parameter_uncorefrequency_init()
{
    if (!loop_adapt_parameter_uncorefrequency_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initializing Uncore frequency backend)
        topology_init();
        if (freq_init() == 0)
        {
            loop_adapt_parameter_uncorefrequency_initialized = 1;
        }
        else
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initializing Uncore frequency backend failed)
            topology_finalize();
            return 1;
        }
    }
    return 0;
}

void loop_adapt_parameter_uncorefrequency_finalize()
{
    if (loop_adapt_parameter_uncorefrequency_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalizing Uncore frequency backend)
        freq_finalize();
        topology_finalize();
        loop_adapt_parameter_uncorefrequency_initialized = 0;
    }
    return;
}

// This function is called when more operations are required to reflect the parameter change
int loop_adapt_parameter_uncorefrequency_set(int instance, ParameterValue value)
{
    if (value.type != LOOP_ADAPT_PARAMETER_TYPE_UINT)
    {
        return -EINVAL;
    }
    if (loop_adapt_parameter_uncorefrequency_initialized)
    {
        int socket = loop_adapt_threads_get_socket(instance);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set Uncore frequency for socket %d, socket);
        if (socket >= 0)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set minimal Uncore frequency for socket %d to %u, socket, value.value.uval);
            int err = freq_setUncoreFreqMin(socket, value.value.uval);
            if (err == 0)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set maximal Uncore frequency for socket %d to %u, socket, value.value.uval);
                err = freq_setUncoreFreqMax(socket, value.value.uval);
                if (err == 0)
                {
                    return 0;
                }
            }
        }
    }
    return -EFAULT;
}

// This function is called to get the actual value at system level (e.g. state of a prefetcher)
int loop_adapt_parameter_uncorefrequency_get(int instance, ParameterValue* value)
{
    if (loop_adapt_parameter_uncorefrequency_initialized)
    {
        int socket = loop_adapt_threads_get_socket(instance);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get Uncore frequency for socket %d, socket);
        if (socket >= 0)
        {
            unsigned int x = freq_getUncoreFreqCur(socket);
            if (x != 0)
            {
                value->value.uval = x;
                value->type = LOOP_ADAPT_PARAMETER_TYPE_UINT;
                return 0;
            }
        }
    }
    return -EFAULT;
}

// This function is called to get the available parameter values for validation and iterating
int loop_adapt_parameter_uncorefrequency_avail(int instance, ParameterValueLimit* limit)
{
    if (loop_adapt_parameter_uncorefrequency_initialized)
    {
        int err = power_init(0);
        if (err)
        {
            PowerInfo_t pi = get_powerInfo();
            unsigned int umin = (unsigned int)(pi->uncoreMinFreq * 1E6);
            unsigned int umax = (unsigned int)(pi->uncoreMaxFreq * 1E6);
            limit->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE;
            limit->limit.range.start.type = LOOP_ADAPT_PARAMETER_TYPE_UINT;
            limit->limit.range.start.value.uval = umin;
            limit->limit.range.end.type = LOOP_ADAPT_PARAMETER_TYPE_UINT;
            limit->limit.range.end.value.uval = umax + 1000000U;
            limit->limit.range.step.type = LOOP_ADAPT_PARAMETER_TYPE_UINT;
            limit->limit.range.step.value.uval = 1000000U;
            limit->limit.range.current.type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
            return 0;
        }
    }
    return -EFAULT;
}
