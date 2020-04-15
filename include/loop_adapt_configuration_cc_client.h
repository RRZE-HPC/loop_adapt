#ifndef LOOP_ADAPT_CONFIGURATION_CC_CLIENT_H
#define LOOP_ADAPT_CONFIGURATION_CC_CLIENT_H

#include <loop_adapt_configuration_types.h>

int loop_adapt_config_cc_client_init();

LoopAdaptConfiguration_t loop_adapt_get_new_config_cc_client(char* string);

LoopAdaptConfiguration_t loop_adapt_get_current_config_cc_client(char* string);

int loop_adapt_config_cc_client_write(ThreadData_t thread, char* loopname, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results);

void loop_adapt_config_cc_client_finalize();

#endif /* LOOP_ADAPT_CONFIGURATION_CC_CLIENT_H */
