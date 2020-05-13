#ifndef LOOP_ADAPT_MEASUREMENT_LIST_H
#define LOOP_ADAPT_MEASUREMENT_LIST_H

#include <loop_adapt_measurement_likwid_nvmon.h>
#include <loop_adapt_measurement_likwid.h>
#include <loop_adapt_measurement_timer.h>

#ifdef LIKWID_NVMON
#define NUM_LOOP_ADAPT_MEASUREMENTS 3
#else
#define NUM_LOOP_ADAPT_MEASUREMENTS 2
#endif

int loop_adapt_measurement_list_count = NUM_LOOP_ADAPT_MEASUREMENTS;

MeasurementDefinition loop_adapt_measurement_list[NUM_LOOP_ADAPT_MEASUREMENTS] = {
    {.name = "LIKWID",
     .scope = LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD,
     .init = loop_adapt_measurement_likwid_init,
     .setup = loop_adapt_measurement_likwid_setup,
     .start = loop_adapt_measurement_likwid_start,
     .startall = loop_adapt_measurement_likwid_startall,
     .stop = loop_adapt_measurement_likwid_stop,
     .stopall = loop_adapt_measurement_likwid_stopall,
     .result = loop_adapt_measurement_likwid_result,
     .configs = loop_adapt_measurement_likwid_configs,
     .finalize = loop_adapt_measurement_likwid_finalize
    },
    {.name = "TIMER",
     .scope = LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD,
     .init = loop_adapt_measurement_timer_init,
     .setup = loop_adapt_measurement_timer_setup,
     .start = loop_adapt_measurement_timer_start,
     .startall = loop_adapt_measurement_timer_startall,
     .stop = loop_adapt_measurement_timer_stop,
     .stopall = loop_adapt_measurement_timer_stopall,
     .result = loop_adapt_measurement_timer_result,
     .configs = loop_adapt_measurement_timer_configs,
     .finalize = loop_adapt_measurement_timer_finalize
    },
#ifdef LIKWID_NVMON
    {.name = "LIKWID_NVMON",
     .scope = LOOP_ADAPT_MEASUREMENT_SCOPE_GPU,
     .init = loop_adapt_measurement_likwid_nvmon_init,
     .setup = loop_adapt_measurement_likwid_nvmon_setup,
     .start = loop_adapt_measurement_likwid_nvmon_start,
     .startall = NULL,
     .stop = loop_adapt_measurement_likwid_nvmon_stop,
     .stopall = NULL,
     .result = loop_adapt_measurement_likwid_nvmon_result,
     .configs = loop_adapt_measurement_likwid_nvmon_configs,
     .finalize = loop_adapt_measurement_likwid_nvmon_finalize
    },
#endif
};

#endif /* LOOP_ADAPT_MEASUREMENT_LIST_H */
