/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_ompnumthreads.h
 *
 *      Description:  Parameter functions for manipulation of OpenMP thread counts
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
#include <error.h>
#include <omp.h>

#ifdef _OPENMP

static int loop_adapt_parameter_ompnumthreads_threads = -1;

// This function is called when more operations are required to reflect the parameter change
int loop_adapt_parameter_ompnumthreads_set(int instance, ParameterValue value)
{
    if (instance = 0 && value.type == LOOP_ADAPT_PARAMETER_TYPE_INT)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Setting OMP_NUM_THREADS to %d, value.value.ival);
        omp_set_num_threads(value.value.ival);
        omp_set_num_threads(value.value.ival);
        loop_adapt_parameter_ompnumthreads_threads = value.value.ival;
    }
    return 0;
}

// This function is called to get the actual value at system level (e.g. state of a prefetcher)
int loop_adapt_parameter_ompnumthreads_get(int instance, ParameterValue* value)
{
    if (instance != 0 || (!value))
        return -EINVAL;

    int ompthreads = 1;
    if (loop_adapt_parameter_ompnumthreads_threads < 0)
    {
        char* env = getenv("OMP_NUM_THREADS");
        if (env)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from env);
            ompthreads = atoi(env);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from env: %d, ompthreads);
        }
        else
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from omp_get_max_threads);
#pragma omp parallel
{
            ompthreads = omp_get_max_threads();
}
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from omp_get_max_threads: %d, ompthreads);
        }
    }
    else
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from omp_get_num_threads);
#pragma omp parallel
{
        ompthreads = omp_get_max_threads();
}
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting OMP_NUM_THREADS from omp_get_num_threads: %d, ompthreads);
    }
    loop_adapt_parameter_ompnumthreads_threads = ompthreads;
    value->value.ival = ompthreads;
    value->type = LOOP_ADAPT_PARAMETER_TYPE_INT;
    return 0;
}

// This function is called to get the available parameter values for validation and iterating
int loop_adapt_parameter_ompnumthreads_avail(int instance, ParameterValueLimit* limit)
{
    if (instance = 0)
    {
        limit->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE;
        limit->limit.range.start.type = LOOP_ADAPT_PARAMETER_TYPE_INT;
        limit->limit.range.end.type = LOOP_ADAPT_PARAMETER_TYPE_INT;
        limit->limit.range.step.type = LOOP_ADAPT_PARAMETER_TYPE_INT;
        limit->limit.range.start.value.ival = 1;
#pragma omp parallel
{
        limit->limit.range.end.value.ival = omp_get_max_threads();
}
        limit->limit.range.step.value.ival = 1;
        limit->limit.range.current.type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
    }
    return 0;
}

#else



int loop_adapt_parameter_ompnumthreads_set(int instance, ParameterValue value)
{
    return 0;
}

int loop_adapt_parameter_ompnumthreads_get(int instance, ParameterValue* value)
{
    value->type = LOOP_ADAPT_PARAMETER_TYPE_INT;
    value->value.ival = 1;
    return 0;
}

int loop_adapt_parameter_ompnumthreads_avail(int instance, ParameterValueLimit* limit)
{
    return 0;
}

#endif
