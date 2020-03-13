#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include <loop_adapt_threads.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_parameter_value.h>
#include <loop_adapt_measurement.h>
#include <loop_adapt_configuration.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>

int loop_adapt_verbosity = 2;


void print_config(LoopAdaptConfiguration_t config)
{
    int i = 0;
    int j = 0;
    bstring sep = bfromcstr(";");
    struct bstrList* plist = bstrListCreate();
    struct bstrList* mlist = bstrListCreate();
    for (i = 0; i < config->num_parameters; i++)
    {
        for (j = 0; j < config->parameters[i].num_values; j++)
        {
            if (config->parameters[i].type != LOOP_ADAPT_PARAMETER_TYPE_INVALID)
            {
                char* pstr = loop_adapt_param_value_str(config->parameters[i].values[j]);
                bstring x = bformat("%s=%d:%s", bdata(config->parameters[i].parameter), j, pstr );
                bstrListAdd(plist, x);
                bdestroy(x);
                free(pstr);
            }
        }
    }
    for (i = 0; i < config->num_measurements; i++)
    {
        LoopAdaptConfigurationMeasurement* m = &config->measurements[i];
        bstring x = bformat("%s=%s:%s", bdata(m->measurement), bdata(m->config), bdata(m->metric));
        bstrListAdd(mlist, x);
        bdestroy(x);
    }
    bstring m = bjoin(mlist, sep);
    bstring p = bjoin(plist, sep);
    printf("%s|%s\n", bdata(p), bdata(m));
    bdestroy(m);
    bdestroy(p);
    bstrListDestroy(mlist);
    bstrListDestroy(plist);
    bdestroy(sep);
}

int main(int argc, char* argv[])
{
    int err = 0;
    loop_adapt_threads_initialize();
    ThreadData_t t = loop_adapt_threads_get();

    loop_adapt_parameter_initialize();
    loop_adapt_measurement_initialize();

    setenv("LA_CONFIG_TXT_INPUT", "../config/LOOP.txt", 1);
    setenv("LA_CONFIG_INPUT_TYPE", "0", 1);
    setenv("LA_CONFIG_OUTPUT_TYPE", "1", 1);

    err = loop_adapt_configuration_initialize();

    if (err < 0)
    {
        loop_adapt_measurement_finalize();
        loop_adapt_parameter_finalize();
        loop_adapt_threads_finalize();
        return 1;
    }
    printf("Initialization done\n");

    LoopAdaptConfiguration_t config = loop_adapt_get_new_configuration("LOOP");
    print_config(config);

    LoopAdaptConfiguration_t cur = loop_adapt_get_current_configuration("LOOP");
    print_config(cur);
/*    loop_adapt_configuration_destroy_config(config);*/
/*    config = NULL;*/
/*    cur = NULL;*/

    config = loop_adapt_get_new_configuration("LOOP");
    print_config(config);

    ParameterValue values[1];
    values[0].type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE;
/*    values[1].type = LOOP_ADAPT_PARAMETER_TYPE_DOUBLE;*/
    values[0].value.dval = 24.0;
/*    values[1].value.dval = 42.0;*/
    printf("Write config\n");
    loop_adapt_write_configuration_results(config, 1, values);

    printf("Start finalization\n");
    loop_adapt_configuration_finalize();
    loop_adapt_measurement_finalize();
    loop_adapt_parameter_finalize();
    loop_adapt_threads_finalize();
    return 0;
}
