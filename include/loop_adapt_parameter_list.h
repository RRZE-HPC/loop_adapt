/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_list.h
 *
 *      Description:  List of included parameters for runtime manipulation
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

//#include <loop_adapt_parameter_template.h>
#include <loop_adapt_parameter_prefetcher.h>
#include <loop_adapt_parameter_ompnumthreads.h>
#include <loop_adapt_parameter_cpufrequency.h>
#include <loop_adapt_parameter_uncorefrequency.h>

ParameterDefinition loop_adapt_parameter_list[] = {
// This adds the parameter value to the list of provided parameters. This list is used to populate the parameter tree at runtime
//    {"TEMPLATE", LOOP_ADAPT_SCOPE_X, LOOP_ADAPT_PARAMETER_TYPE_X, X, loop_adapt_parameter_template_set, loop_adapt_parameter_template_get, loop_adapt_parameter_template_avail},
    {.name = "HW_PREFETCHER",
     .description = "Hardware prefetcher",
     .scope = LOOP_ADAPT_SCOPE_THREAD,
     .value = DEC_NEW_BOOL_PARAM_VALUE(TRUE),
     .init = loop_adapt_parameter_prefetcher_init,
     .finalize = loop_adapt_parameter_prefetcher_finalize,
     .set = loop_adapt_parameter_prefetcher_hwpf_set,
     .get = loop_adapt_parameter_prefetcher_hwpf_get,
     .avail = loop_adapt_parameter_prefetcher_avail,
    },
    {.name = "CL_PREFETCHER",
     .description = "Hardware prefetcher",
     .scope = LOOP_ADAPT_SCOPE_THREAD,
     .value = DEC_NEW_BOOL_PARAM_VALUE(TRUE),
     .init = loop_adapt_parameter_prefetcher_init,
     .finalize = loop_adapt_parameter_prefetcher_finalize,
     .set = loop_adapt_parameter_prefetcher_clpf_set,
     .get = loop_adapt_parameter_prefetcher_clpf_get,
     .avail = loop_adapt_parameter_prefetcher_avail,
    },
    {.name = "DCU_PREFETCHER",
     .description = "Hardware prefetcher",
     .scope = LOOP_ADAPT_SCOPE_THREAD,
     .value = DEC_NEW_BOOL_PARAM_VALUE(TRUE),
     .init = loop_adapt_parameter_prefetcher_init,
     .finalize = loop_adapt_parameter_prefetcher_finalize,
     .set = loop_adapt_parameter_prefetcher_dcupf_set,
     .get = loop_adapt_parameter_prefetcher_dcupf_get,
     .avail = loop_adapt_parameter_prefetcher_avail,
    },
    {.name = "IP_PREFETCHER",
     .description = "Hardware prefetcher",
     .scope = LOOP_ADAPT_SCOPE_THREAD,
     .value = DEC_NEW_BOOL_PARAM_VALUE(TRUE),
     .init = loop_adapt_parameter_prefetcher_init,
     .finalize = loop_adapt_parameter_prefetcher_finalize,
     .set = loop_adapt_parameter_prefetcher_ippf_set,
     .get = loop_adapt_parameter_prefetcher_ippf_get,
     .avail = loop_adapt_parameter_prefetcher_avail,
    },
#ifdef _OPENMP
    {.name = "OMP_NUM_THREADS",
     .description = "Number of OpenMP threads",
     .scope = LOOP_ADAPT_SCOPE_SYSTEM,
     .value = DEC_NEW_INT_PARAM_VALUE(1),
     .set = loop_adapt_parameter_ompnumthreads_set,
     .get = loop_adapt_parameter_ompnumthreads_get,
     .avail = loop_adapt_parameter_ompnumthreads_avail,
    },
#endif
    {.name = "CPU_FREQUENCY",
     .description = "CPU frequency",
     .scope = LOOP_ADAPT_SCOPE_THREAD,
     .value = DEC_NEW_INT_PARAM_VALUE(1),
     .init = loop_adapt_parameter_cpufrequency_init,
     .set = loop_adapt_parameter_cpufrequency_set,
     .get = loop_adapt_parameter_cpufrequency_get,
     .avail = loop_adapt_parameter_cpufrequency_avail,
     .finalize = loop_adapt_parameter_cpufrequency_finalize,
    },
//    {.name = "UNCORE_FREQUENCY",
//     .description = "Uncore frequency",
//     .scope = LOOP_ADAPT_SCOPE_SOCKET,
//     .value = DEC_NEW_UINT_PARAM_VALUE(100000U),
//     .init = loop_adapt_parameter_uncorefrequency_init,
//     .set = loop_adapt_parameter_uncorefrequency_set,
//     .get = loop_adapt_parameter_uncorefrequency_get,
//     .avail = loop_adapt_parameter_uncorefrequency_avail,
//     .finalize = loop_adapt_parameter_uncorefrequency_finalize,
//    },
    {.name = NULL}
};
