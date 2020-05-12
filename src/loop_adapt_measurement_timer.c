#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <likwid.h>

#include <map.h>
#include <loop_adapt_parameter_value_types.h>

static Map_t timers = NULL;

typedef enum {
    TIMER_MEASUREMENT_LIKWID,
    TIMER_MEASUREMENT_REALTIME,
    TIMER_MEASUREMENT_MONOTONIC,
    TIMER_MEASUREMENT_PROCESS_CPUTIME,
    TIMER_MEASUREMENT_THREAD_CPUTIME,
    TIMER_MEASUREMENT_MAX
} TimerMeasurementStyle;


typedef struct {
    double walltime;
    TimerData timer;
    clockid_t clockid;
    struct timespec clockstart;
    struct timespec clockstop;
    TimerMeasurementStyle style;
} TimerMeasurement;

static void _loop_adapt_destroy_timerdata(void* ptr)
{
    free(ptr);
}

int loop_adapt_measurement_timer_init()
{
    int i = 0;
    timer_init();
    init_imap(&timers, _loop_adapt_destroy_timerdata);
/*    for (i = 0; i < num_instances; i++)*/
/*    {*/
/*        TimerMeasurement* timer = NULL;*/
/*        timer = malloc(sizeof(TimerMeasurement));*/
/*        if (timer)*/
/*        {*/
/*            timer->walltime = 0;*/
/*            timer_reset(&timer->timer);*/
/*            add_imap(timers, instances[i], (void*)timer);*/
/*        }*/
/*    }*/
}

void loop_adapt_measurement_timer_finalize()
{
    destroy_imap(timers);
}


int loop_adapt_measurement_timer_setup(int instance, bstring configuration, bstring metrics)
{
    int newtimer = 0;
    TimerMeasurement* timer = NULL;
    bstring likwid = bfromcstr("LIKWID");
    bstring realtime = bfromcstr("REALTIME");
    bstring monotonic = bfromcstr("MONOTONIC");
    bstring pcpu = bfromcstr("PROCESS_CPUTIME");
    bstring tcpu = bfromcstr("THREAD_CPUTIME");
    TimerMeasurementStyle style = TIMER_MEASUREMENT_MAX;
    clockid_t clockid;
    if (!timers)
    {
        fprintf(stderr, "Timer measurement module not initialized\n");
        return -EINVAL;
    }

    if (bstrncmp(configuration, likwid, blength(likwid)) == BSTR_OK)
    {
        style = TIMER_MEASUREMENT_LIKWID;
    }
    else if (bstrncmp(configuration, realtime, blength(realtime)) == BSTR_OK)
    {
        style = TIMER_MEASUREMENT_REALTIME;
        clockid = CLOCK_REALTIME;
    }
    else if (bstrncmp(configuration, monotonic, blength(monotonic)) == BSTR_OK)
    {
        style = TIMER_MEASUREMENT_MONOTONIC;
        clockid = CLOCK_MONOTONIC;
    }
    else if (bstrncmp(configuration, pcpu, blength(pcpu)) == BSTR_OK)
    {
        style = TIMER_MEASUREMENT_PROCESS_CPUTIME;
        clockid = CLOCK_PROCESS_CPUTIME_ID;
    }
    else if (bstrncmp(configuration, tcpu, blength(tcpu)) == BSTR_OK)
    {
        style = TIMER_MEASUREMENT_THREAD_CPUTIME;
        clockid = CLOCK_THREAD_CPUTIME_ID;
    }
    else
    {
        printf("Unknown configuration %s\n", bdata(configuration));
        return -EINVAL;
    }

    if (get_imap_by_key(timers, instance, (void**)&timer) != 0)
    {
        timer = malloc(sizeof(TimerMeasurement));
        if (!timer)
        {
            return -ENOMEM;
        }
        memset(timer, 0, sizeof(TimerMeasurement));
        newtimer = 1;
    }
    if (timer)
    {
        if (style == TIMER_MEASUREMENT_LIKWID)
        {
            fprintf(stderr, "Setup new LIKWID timer for instance %d (conf: %s)\n", instance, bdata(configuration));
            timer_reset(&timer->timer);
        }
        else
        {
            fprintf(stderr, "Setup new SYSTEM timer for instance %d (conf: %s)\n", instance, bdata(configuration));
            timer->clockid = clockid;
            timer->clockstart.tv_sec = 0;
            timer->clockstart.tv_nsec = 0;
            timer->clockstop.tv_sec = 0;
            timer->clockstop.tv_nsec = 0;
        }
        timer->walltime = 0;
        timer->style = style;

        if (newtimer)
        {
            add_imap(timers, instance, (void*)timer);
        }
    }
    else
    {
        printf("Unknown timer\n");
    }

    bdestroy(likwid);
    bdestroy(realtime);
    bdestroy(monotonic);
    bdestroy(pcpu);
    bdestroy(tcpu);
    return 0;
}

void loop_adapt_measurement_timer_start(int instance)
{
    TimerMeasurement* timer = NULL;
    if (get_imap_by_key(timers, instance, (void**)&timer) == 0)
    {
        switch(timer->style)
        {
            case TIMER_MEASUREMENT_LIKWID:
                fprintf(stderr, "Starting LIKWID timer for instance %d\n", instance);
                timer_start(&timer->timer);
                break;
            case TIMER_MEASUREMENT_REALTIME:
            case TIMER_MEASUREMENT_MONOTONIC:
            case TIMER_MEASUREMENT_PROCESS_CPUTIME:
            case TIMER_MEASUREMENT_THREAD_CPUTIME:
                fprintf(stderr, "Starting SYSTEM timer for instance %d\n", instance);
                clock_gettime(timer->clockid, &timer->clockstart);
                break;
        }

    }

}

void loop_adapt_measurement_timer_startall_cb(mpointer key, mpointer value, mpointer userdata)
{
    int *instance = (int*)key;
    loop_adapt_measurement_timer_start(*instance);
}

void loop_adapt_measurement_timer_startall()
{
    foreach_in_imap(timers, loop_adapt_measurement_timer_startall_cb, NULL);
}
void loop_adapt_measurement_timer_stop(int instance)
{
    TimerMeasurement* timer = NULL;
    if (get_imap_by_key(timers, instance, (void**)&timer) == 0)
    {
        switch(timer->style)
        {
            case TIMER_MEASUREMENT_LIKWID:
                fprintf(stderr, "Stopping LIKWID timer for instance %d\n", instance);
                timer_stop(&timer->timer);
                timer->walltime += timer_print(&timer->timer);
                timer_reset(&timer->timer);
                break;
            case TIMER_MEASUREMENT_REALTIME:
            case TIMER_MEASUREMENT_MONOTONIC:
            case TIMER_MEASUREMENT_PROCESS_CPUTIME:
            case TIMER_MEASUREMENT_THREAD_CPUTIME:
                fprintf(stderr, "Stopping SYSTEM timer for instance %d\n", instance);
                clock_gettime(timer->clockid, &timer->clockstop);
                timer->walltime += (double)(timer->clockstop.tv_sec-timer->clockstart.tv_sec) +
                                   ((double)(timer->clockstop.tv_nsec-timer->clockstart.tv_nsec)*1E-9);
                timer->clockstart.tv_sec = 0;
                timer->clockstart.tv_nsec = 0;
                timer->clockstop.tv_sec = 0;
                timer->clockstop.tv_nsec = 0;
                break;
        }

    }

}

void loop_adapt_measurement_timer_stopall_cb(mpointer key, mpointer value, mpointer userdata)
{
    int *instance = (int*)key;
    loop_adapt_measurement_timer_stop(*instance);
}

void loop_adapt_measurement_timer_stopall()
{
    foreach_in_imap(timers, loop_adapt_measurement_timer_stopall_cb, NULL);
}
int loop_adapt_measurement_timer_result(int instance, int num_values, ParameterValue* values)
{
    TimerMeasurement* timer = NULL;
    if (get_imap_by_key(timers, instance, (void**)&timer) == 0)
    {
        ParameterValue* v = &values[0];
        v->type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE;
        v->value.dval = timer->walltime;
        return 1;
    }
    return 0;
}


int loop_adapt_measurement_timer_configs(struct bstrList* configs)
{
    bstrListAddChar(configs, "REALTIME");
    bstrListAddChar(configs, "MONOTONIC");
    bstrListAddChar(configs, "PROCESS_CPUTIME");
    bstrListAddChar(configs, "THREAD_CPUTIME");
    bstrListAddChar(configs, "LIKWID");
    return 5;
}