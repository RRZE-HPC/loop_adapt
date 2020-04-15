#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <errno.h>

using namespace std;

extern "C"
{
// A parameter value is a container for different types like integer, double, string, boolean, ...
#include <loop_adapt_parameter_value.h>
// A configuration is a runtime configuration that tells loop_adapt to run the loop with these parameter settings
// and measure the desired metrics
#include <loop_adapt_configuration.h>

#include <loop_adapt_configuration_cc_client.h>
}

static int initialized = 0;

class Client {
    public:

        std::string user;
        std::string pass;
        std::string random_name;
        std::string random_name2;
        std::string data;

        std::string parameter;
        std::string argument;
        std::string objective;

        LoopAdaptConfiguration_t current;

        int server_alive() {
            return 1;
        }
        void add_parameter(std::string parameter) {
            std::cout << "LoopAdapt CC Client: add_parameter: " << parameter;
        }
        void add_argument(std::string argument) {
            std::cout << "LoopAdapt CC Client: add_argument: " << argument;
        }
        void add_objective(std::string objective) {
            std::cout << "LoopAdapt CC Client: add_objective: " << objective;
        }
        int report_config(double result) {
            std::cout << "LoopAdapt CC Client: report_config: " << result;
        }
        void setup() {
            std::cout << "LoopAdapt CC Client: setup";
        }
        std::string set_config() {
            std::cout << "LoopAdapt CC Client: set_config";
            return "bla";
        }
        double eval_config(int num_values, ParameterValue* values) {
            std::cout << "LoopAdapt CC Client: eval_config";
        }

        void close() {
            std::cout << "LoopAdapt CC Client: close";
        }
        Client(std::string data, std::string user, std::string pass, std::string random_name, std::string random_name2)
        {
            std::cout << "LoopAdapt CC Client: constructor";
            initialized = 1;
        }
        Client()
        {
//            std::cout << "LoopAdapt CC Client: constructor without args";
            return;
        }
};

Client client;

// Currently the init function does not get any information from loop_adapt but here we need
// username/password etc. Reading it from enviroment is currently only used for
// demonstration.
extern "C" int loop_adapt_config_cc_client_init()
{
    std::string data = getenv("LA_CONFIG_CC_DATA");
    std::string user = getenv("LA_CONFIG_CC_USER");
    std::string pass = getenv("LA_CONFIG_CC_PASS");
    std::string random_name = getenv("LA_CONFIG_CC_RANDOM_NAME");
    std::string random_name2 = getenv("LA_CONFIG_CC_RANDOM_NAME2");

    std::string x = "EXAMPLE_PARAMETER";
    std::string pp = "EXAMPLE_ARGUMENT";
    std::string obj = "EXAMPLE_OBJECTIVE";

    if (!initialized)
    {
        client = Client(data, user, pass, random_name, random_name2);
        if (client.server_alive())
        {
            client.add_parameter(x);
            client.add_argument(pp);
//            client.add_argument(machine_class_name);
//            client.add_argument(teq);
//            client.add_argument(test_limit);
            client.add_objective(obj);
            // Alternative:
            // client.start();
            client.setup();
            initialized = 1;
        }
        else
        {
            -ENODEV;
        }
    }
}

extern "C" void loop_adapt_config_cc_client_finalize()
{
    if (initialized)
    {
        client.close();
    }
}


// We get a string identifying the loop. If we should be able to handle multiple
// loops simultaneously, we should hand it over to client.set_config() which
// returns the per-loop config. Moreover, the client.current variable should be
// a hash map string -> current config
extern "C" LoopAdaptConfiguration_t loop_adapt_get_new_config_cc_client(char* string)
{
    LoopAdaptConfiguration_t config = NULL;
    if (initialized)
    {
        std::string config_data = client.set_config();
        // config_data string to LoopAdaptConfiguration_t config (list of parameter settings and measurement instructions)
        if (client.current)
        {
            // We don't need the last configuration anymore, so destroy it.
            loop_adapt_configuration_destroy_config(client.current);
        }
        // Update current configuration
        client.current = config;
    }
    return config;
}

// Return the current configuration. See comment at loop_adapt_get_new_config_cc_client
// for string function argument for per-loop configurations
extern "C" LoopAdaptConfiguration_t loop_adapt_get_current_config_cc_client(char* string)
{
    if (initialized)
    {
        return client.current;
    }
    return NULL;
}

// Write out measurement results
extern "C" int loop_adapt_config_cc_client_write(ThreadData_t thread, char* loopname, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
    if (initialized)
    {
        // Only one result is bad. We should be able to report multiple values
        double rtime = client.eval_config(num_results, results);;
        // LoopAdaptConfiguration_t to report string/value/command
        return client.report_config(rtime);
    }
    return -ENODEV;
}
