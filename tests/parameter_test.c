#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include <loop_adapt_threads.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_parameter_value.h>

int loop_adapt_verbosity = 2;

int main(int argc, char* argv[])
{
    loop_adapt_threads_initialize();
    loop_adapt_threads_register(0);
    ThreadData_t t = loop_adapt_threads_get();


    loop_adapt_parameter_initialize();
    printf("System initialized\n");
    int count = 0;
    char** parameters = NULL;
    int err = loop_adapt_parameter_getnames(&count, &parameters);
    if (err == 0)
    {
        int i = 0;
        printf("Available parameters:\n");
        for (i = 0; i < count; i++)
        {
            printf("%d %s\n", i, parameters[i]);
        }
    }
    else
    {
        printf("loop_adapt_parameter_getnames returned %d\n", err);
    }

    ParameterValue v;
    int i = 0;
    printf("Repeated getting of parameter %s:\n", parameters[4]);
    for (i = 0; i < 4; i++)
    {
        printf("Get %s\n", parameters[4]);
        err = loop_adapt_parameter_get(t, parameters[4], &v);
        if (err == 0)
        {
            loop_adapt_print_param_value(v);
        }
        printf("Getcurrent %s\n", parameters[4]);
        err = loop_adapt_parameter_getcurrent(t, parameters[4], &v);
        if (err == 0)
        {
            loop_adapt_print_param_value(v);
        }
    }
    v.value.ival = 2;
    printf("Setting %s to %d\n", parameters[4], v.value.ival);
    loop_adapt_print_param_value(v);
    err = loop_adapt_parameter_set(t, parameters[4], v);
    if (err == 0)
    {
        loop_adapt_print_param_value(v);
    }
    printf("Get %s\n", parameters[4]);
    err = loop_adapt_parameter_get(t, parameters[4], &v);
    if (err == 0)
    {
        loop_adapt_print_param_value(v);
    }
    printf("Getcurrent %s\n", parameters[4]);
    err = loop_adapt_parameter_getcurrent(t, parameters[4], &v);
    if (err == 0)
    {
        loop_adapt_print_param_value(v);
    }

    printf("Starting finalization\n");
    loop_adapt_parameter_finalize();
    loop_adapt_threads_finalize();
}
