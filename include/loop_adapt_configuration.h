#ifndef LOOP_ADAPT_CONFIGURATION_H
#define LOOP_ADAPT_CONFIGURATION_H

#include <loop_adapt_policy_types.h>
#include <loop_adapt_configuration_types.h>

int loop_adapt_configuration_initialize();

int loop_adapt_get_new_configuration(char* string, int config_id, LoopAdaptConfiguration_t *config);


int loop_adapt_write_configuration_results(ThreadData_t thread, char* loopname, PolicyDefinition_t policy, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results);

void loop_adapt_configuration_destroy_config(LoopAdaptConfiguration_t config);
int loop_adapt_configuration_resize_config(LoopAdaptConfiguration_t *config, int num_parameters);

void loop_adapt_configuration_finalize();



// int loop_adapt_configuration_announce(LoopAdaptAnnounce_t announe);


/* Helper functions and macros*/
int loop_adapt_config_parse_default_entry(bstring b, bstring* first, bstring* second, bstring* third);
#define loop_adapt_config_parse_default_entry_bbb(f, s, t) \
    bformat("%s=%s:%s", bdata(f), bdata(s), bdata(t))

#define loop_adapt_config_parse_default_entry_bbc(f, s, t) \
    bformat("%s=%s:%s", bdata(f), bdata(s), t)

#define loop_adapt_config_parse_default_entry_bcc(f, s, t) \
    bformat("%s=%s:%s", bdata(f), s, t)

#define loop_adapt_config_parse_default_entry_bdc(f, s, t) \
    bformat("%s=%d:%s", bdata(f), s, t)
#endif
