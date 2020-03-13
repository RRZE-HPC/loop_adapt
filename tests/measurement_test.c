#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <loop_adapt_measurement.h>
#include <loop_adapt_threads.h>
#include <loop_adapt_parameter_value.h>
#include <bstrlib.h>

int loop_adapt_verbosity = 2;

int main(int argc, char* argv[])
{
    loop_adapt_threads_initialize();
    loop_adapt_threads_register(0);
    ThreadData_t t = loop_adapt_threads_get();


    loop_adapt_measurement_initialize();
    bstring conf = bfromcstr("REALTIME");
    bstring metrics = bfromcstr("L3 bandwidth");
    bstring likwid = bfromcstr("L3");

    loop_adapt_measurement_setup(t, "TIMER", conf, metrics);
    loop_adapt_measurement_setup(t, "LIKWID", likwid, metrics);
    printf("Start TIMER for 0\n");
    loop_adapt_measurement_start(t, "LIKWID");
    loop_adapt_measurement_start(t, "TIMER");
    sleep(2);
    printf("Stop TIMER for 0\n");
    loop_adapt_measurement_stop(t, "TIMER");
    loop_adapt_measurement_stop(t, "LIKWID");
/*    loop_adapt_measurement_stop_all(t);*/

    ParameterValue values[4];
    int num_values = 4;
    num_values = loop_adapt_measurement_result(t, "TIMER", num_values, values);
    printf("TIMER num_values %d\n", num_values);
    if (num_values > 0)
    {
        loop_adapt_print_param_value(values[0]);
    }
    num_values = 4;
    num_values = loop_adapt_measurement_result(t, "LIKWID", num_values, values);
    printf("LIKWID num_values %d\n", num_values);
    if (num_values > 0)
    {
        loop_adapt_print_param_value(values[0]);
    }

    loop_adapt_measurement_finalize();

    bdestroy(conf);
    bdestroy(metrics);
    loop_adapt_threads_finalize();
    return 0;
}
