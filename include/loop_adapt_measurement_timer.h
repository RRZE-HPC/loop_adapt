#ifndef LOOP_ADAPT_MEASUREMENT_TIMER_H
#define LOOP_ADAPT_MEASUREMENT_TIMER_H

int loop_adapt_measurement_timer_init();

int loop_adapt_measurement_timer_setup(int instance, bstring configuration, bstring metrics);
void loop_adapt_measurement_timer_start(int instance);
void loop_adapt_measurement_timer_stop(int instance);
int loop_adapt_measurement_timer_result(int instance, int num_values, ParameterValue* values);

void loop_adapt_measurement_timer_finalize();

#endif /* LOOP_ADAPT_MEASUREMENT_TIMER_H */
