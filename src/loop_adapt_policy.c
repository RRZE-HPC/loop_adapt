
#include <stdlib.h>
#include <stdio.h>
#include <bstrlib.h>
#include <loop_adapt_parameter_value_types.h>

#include <loop_adapt_policy_list.h>


static PolicyDefinition* loop_adapt_active_policy = NULL;
static int loop_adapt_num_active_policy = 0;


int loop_adapt_policy_initialize()
{
    int i = 0;

    for (i = 0; i < loop_adapt_policy_list_count; i++)
    {
        
    }

}


int loop_adapt_policy_available(char* policy)
{
    int i = 0;
    if (!policy)
        return 0;
    bstring bpol = bfromcstr(policy);

    for (int i = 0; i < loop_adapt_num_active_policy; i++)
    {
        PolicyDefinition* pd = &loop_adapt_active_policy[i];
        if (bstrncmp(pd->name, bpol, blength(pd->name)) == BSTR_OK)
        {
            return 1;
        }
    }
    return 0;
}

int loop_adapt_policy_eval(char* loop, int num_results, ParameterValue* inputs, ParameterValue* output);

bstring loop_adapt_policy_get_measurement(bstring policy);