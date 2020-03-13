/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_cpufrequency.h
 *
 *      Description:  Parameter functions for CPU frequency manipulation
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

#include <likwid.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <error.h>

static int _loop_adapt_parameter_cpufrequency_initialized = 0;

int loop_adapt_parameter_cpufrequency_init()
{
    if (!_loop_adapt_parameter_cpufrequency_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initializing CPU frequency backend)
        topology_init();
        freq_init();
        _loop_adapt_parameter_cpufrequency_initialized = 1;
    }
    return 0;
}

void loop_adapt_parameter_cpufrequency_finalize()
{
    if (_loop_adapt_parameter_cpufrequency_initialized)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalizing CPU frequency backend)
        freq_finalize();
        topology_finalize();
        _loop_adapt_parameter_cpufrequency_initialized = 0;
    }
}


// This function is called when more operations are required to reflect the parameter change
int loop_adapt_parameter_cpufrequency_set(int instance, ParameterValue value)
{
    if (value.type != LOOP_ADAPT_PARAMETER_TYPE_INT)
    {
        return -EINVAL;
    }
    if (_loop_adapt_parameter_cpufrequency_initialized)
    {
        int cpu = loop_adapt_threads_get_cpu(instance);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set CPU frequency for CPU %d, cpu);
        if (cpu >= 0)
        {
            value.type = LOOP_ADAPT_PARAMETER_TYPE_INT;
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set minimal CPU frequency for CPU %d to %d, cpu, value.value.ival);
            int f = freq_setCpuClockMin(cpu, value.value.ival);
            if (f == value.value.ival)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Set maximal CPU frequency for CPU %d to %d, cpu, value.value.ival);
                f = freq_setCpuClockMax(cpu, value.value.ival);
                if (f == value.value.ival)
                {
                    return 0;
                }
            }
            return -EFAULT;
        }
        return -EINVAL;
    }
    return -EFAULT;
}

// This function is called to get the actual value at system level (e.g. state of a prefetcher)
int loop_adapt_parameter_cpufrequency_get(int instance, ParameterValue* value)
{
    if (_loop_adapt_parameter_cpufrequency_initialized)
    {
        int cpu = loop_adapt_threads_get_cpu(instance);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get current CPU frequency for CPU %d, cpu);
        if (cpu >= 0 && value)
        {
            value->type = LOOP_ADAPT_PARAMETER_TYPE_INT;
            int freq = (int)freq_getCpuClockCurrent(cpu);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get current CPU frequency for CPU %d: %d, cpu, freq);
            value->value.ival = freq;
            return 0;
        }
        return -EINVAL;
    }
    return -ENODEV;
}

// This function is called to get the available parameter values for validation and iterating
int loop_adapt_parameter_cpufrequency_avail(int instance, ParameterValueLimit* limit)
{
    int i = 0;
    int cpu = 0;
    int (*ownatoi)(const char* nptr) = &atoi;
    if (!limit)
    {
        return -EINVAL;
    }
    if (_loop_adapt_parameter_cpufrequency_initialized)
    {
        if (limit->type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST && limit->limit.list.num_values > 0 && limit->limit.list.values)
        {
            int i = 0;
            for (i = 0; i < limit->limit.list.num_values; i++)
            {
                loop_adapt_destroy_param_value(limit->limit.list.values[i]);
            }
            free(limit->limit.list.values);
        }
        limit->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST;
        limit->limit.list.num_values = 0;
        limit->limit.list.value_idx = -1;
        limit->limit.list.values = NULL;

        cpu = loop_adapt_threads_get_cpu(instance);
        if (cpu >= 0)
        {
            fprintf(stderr, "TODO - support both drivers");
            char* cfreqs = freq_getAvailFreq(cpu);
            bstring bfreqs = bfromcstr(cfreqs);
            struct bstrList* freqlist = bsplit(bfreqs, ' ');
            if (freqlist && freqlist->qty > 0)
            {
                for (i = 0; i < freqlist->qty; i++)
                {
                    ParameterValue v;
                    v.type = LOOP_ADAPT_PARAMETER_TYPE_INT;
                    v.value.ival = ownatoi(bdata(freqlist->entry[i]));
                    loop_adapt_add_param_limit_list(limit, v);
                }
            }
            //free(cfreqs);
        }
    }

    return 0;
}
