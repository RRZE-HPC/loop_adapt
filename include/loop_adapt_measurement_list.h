#ifndef LOOP_ADAPT_MEASUREMENT_LIST_H
#define LOOP_ADAPT_MEASUREMENT_LIST_H

#include <loop_adapt_measurement_likwid.h>
#include <loop_adapt_measurement_timer.h>

#define NUM_LOOP_ADAPT_MEASUREMENTS 2

int loop_adapt_measurement_list_count = NUM_LOOP_ADAPT_MEASUREMENTS;

MeasurementDefinition loop_adapt_measurement_list[NUM_LOOP_ADAPT_MEASUREMENTS] = {
    {"LIKWID", LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD, loop_adapt_measurement_likwid_init, loop_adapt_measurement_likwid_setup, loop_adapt_measurement_likwid_start, loop_adapt_measurement_likwid_stop, loop_adapt_measurement_likwid_result, loop_adapt_measurement_likwid_finalize},
    {"TIMER", LOOP_ADAPT_MEASUREMENT_SCOPE_THREAD, loop_adapt_measurement_timer_init, loop_adapt_measurement_timer_setup, loop_adapt_measurement_timer_start, loop_adapt_measurement_timer_stop, loop_adapt_measurement_timer_result, loop_adapt_measurement_timer_finalize},
};

#endif /* LOOP_ADAPT_MEASUREMENT_LIST_H */
