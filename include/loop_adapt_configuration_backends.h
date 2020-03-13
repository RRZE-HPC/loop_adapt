#ifndef LOOP_ADAPT_CONFIGURATION_BACKENDS_H
#define LOOP_ADAPT_CONFIGURATION_BACKENDS_H

#include <loop_adapt_configuration_types.h>
#include <loop_adapt_configuration_txt.h>
#include <loop_adapt_configuration_cc_client.h>
#include <loop_adapt_configuration_stdout.h>

typedef enum {
    LA_CONFIG_IN_TXT = 0,
    LA_CONFIG_IN_CC_CLIENT,
//    LA_CONFIG_IN_SQL,
    LA_CONFIG_IN_MAX,
} ConfigurationInputBackendTypes;
#define LA_CONFIG_IN_MIN LA_CONFIG_IN_TXT

typedef enum {
    LA_CONFIG_OUT_TXT = 0,
    LA_CONFIG_OUT_STDOUT,
    LA_CONFIG_OUT_CC_CLIENT,
//    LA_CONFIG_OUT_SQL,
    LA_CONFIG_OUT_MAX,
} ConfigurationOutputBackendTypes;
#define LA_CONFIG_OUT_MIN LA_CONFIG_OUT_TXT

static LoopAdaptInputConfigurationFunctions loop_adapt_configuration_input_list[LA_CONFIG_IN_MAX] = {
    [LA_CONFIG_IN_TXT] = {.init = loop_adapt_config_txt_input_init,
                          .finalize = loop_adapt_config_txt_finalize,
                          .getnew = loop_adapt_get_new_config_txt,
                          .getcurrent = loop_adapt_get_current_config_txt,
                         },
    [LA_CONFIG_IN_CC_CLIENT] = {.init = loop_adapt_config_cc_client_init,
                                .finalize = loop_adapt_config_cc_client_finalize,
                                .getnew = loop_adapt_get_new_config_cc_client,
                                .getcurrent = loop_adapt_get_current_config_cc_client,
                               }
};

static LoopAdaptOutputConfigurationFunctions loop_adapt_configuration_output_list[LA_CONFIG_OUT_MAX] = {
    [LA_CONFIG_OUT_TXT] = {.init = loop_adapt_config_txt_output_init,
                           .write = loop_adapt_config_txt_write,
                           .finalize = NULL,
                          },
    [LA_CONFIG_OUT_CC_CLIENT] = {.init = NULL,
                                 .write = loop_adapt_config_cc_client_write,
                                 .finalize = NULL,
                                },
    [LA_CONFIG_OUT_STDOUT] = {.init = loop_adapt_config_stdout_init,
                              .write = loop_adapt_config_stdout_write,
                              .finalize = NULL,
                             },
};



#endif /* LOOP_ADAPT_CONFIGURATION_BACKENDS_H */
