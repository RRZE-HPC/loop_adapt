#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <bstrlib.h>

#include <error.h>
#include <loop_adapt_threads.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_policy.h>

LoopAdaptDebugLevel loop_adapt_verbosity = 0;

int main(int argc, char* argv[])
{
    int i = 0;
    struct bstrList* parameters = bstrListCreate();
    struct bstrList* policies = bstrListCreate();

    loop_adapt_threads_initialize();
    loop_adapt_threads_register(0);
    ThreadData_t t = loop_adapt_threads_get();


    loop_adapt_parameter_initialize();

    loop_adapt_parameter_getbnames(parameters);

    printf("Available parameters:\n");
    for (i = 0; i < parameters->qty; i++)
    {
        printf("- %s\n", bdata(parameters->entry[i]));
    }
    bstrListDestroy(parameters);

    loop_adapt_parameter_finalize();

    loop_adapt_policy_initialize();

    loop_adapt_policy_getbnames(policies);
    printf("Available policies:\n");
    for (i = 0; i < policies->qty; i++)
    {
        printf("- %s\n", bdata(policies->entry[i]));
    }
    bstrListDestroy(policies);

    loop_adapt_policy_finalize();

    loop_adapt_threads_finalize();
    return 0;
}
