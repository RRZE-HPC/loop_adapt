
#include <stdlib.h>
#include <stdio.h>
#include <bstrlib.h>
#include <error.h>
#include <loop_adapt_internal.h>
#include <loop_adapt_parameter_value_types.h>

#include <loop_adapt_policy_list.h>


static PolicyDefinition* loop_adapt_active_policy = NULL;
static int loop_adapt_num_active_policy = 0;

int _loop_adapt_copy_policy(_PolicyDefinition *in, PolicyDefinition* out)
{
    if (in && out)
    {
        out->name = bfromcstr(in->name);
        out->eval = in->eval;
        if (in->match)
            out->measurement = bformat("%s=%s:%s", in->backend, in->config, in->match);
        else
            out->measurement = bformat("%s=%s", in->backend, in->config);
        return 0;
    }
    return -EINVAL;
}

void loop_adapt_policy_finalize()
{
    int i = 0;
    if (loop_adapt_active_policy)
    {
        for (i = 0; i < loop_adapt_num_active_policy; i++)
        {
            PolicyDefinition* pd = &loop_adapt_active_policy[i];
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Removing runtime policy %s, bdata(pd->name));
            bdestroy(pd->name);
            bdestroy(pd->measurement);
            pd->eval = NULL;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Freeing space for runtime policies);
        free(loop_adapt_active_policy);
        loop_adapt_active_policy = NULL;
        loop_adapt_num_active_policy = 0;
    }
}


int loop_adapt_policy_initialize()
{
    int i = 0;

    if (!loop_adapt_active_policy)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Allocating space for runtime policies);
        loop_adapt_active_policy = malloc(loop_adapt_policy_list_count * sizeof(PolicyDefinition));
        if (!loop_adapt_active_policy)
        {
            return -ENOMEM;
        }
        loop_adapt_num_active_policy = 0;
    }

    for (i = 0; i < loop_adapt_policy_list_count; i++)
    {
        _PolicyDefinition *in = &loop_adapt_policy_list[i];
        PolicyDefinition* out = &loop_adapt_active_policy[i];
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Adding policy %s to runtime policies, in->name);
        _loop_adapt_copy_policy(in, out);
    }
    loop_adapt_num_active_policy = loop_adapt_policy_list_count;
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Added %d runtime policies, loop_adapt_num_active_policy);
}


int loop_adapt_policy_available(char* policy)
{
    int i = 0;
    if (!policy)
        return 0;
    bstring bpol = bfromcstr(policy);
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Check availibility of policy %s, policy);
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

int loop_adapt_register_policy(char* name, char* backend, char* config, char* match, policy_eval_function func)
{
    if (name && backend && config && func)
    {
        PolicyDefinition* tmp = realloc(loop_adapt_active_policy, (loop_adapt_num_active_policy+1)*sizeof(PolicyDefinition));
        if (!tmp)
        {
            return -ENOMEM;
        }
        loop_adapt_active_policy = tmp;
        tmp = NULL;
        tmp = &loop_adapt_active_policy[loop_adapt_num_active_policy];
        tmp->name = bfromcstr(name);
        tmp->eval = func;
        if (match)
        {
            tmp->measurement = bformat("%s=%s:%s", backend, config, match);
        }
        else
        {
            tmp->measurement = bformat("%s=%s", backend, config);
        }
        loop_adapt_num_active_policy++;
        return 0;
    }
    return -EINVAL;
}

int loop_adapt_policy_eval(char* loop, int num_results, ParameterValue* inputs, double* output)
{
    int i = 0;
    int err = 0;
    LoopData_t ldata = NULL;
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Evaluate policy for loop %s with %d values, loop, num_results);
    err = loop_adapt_get_loopdata(loop, &ldata);
    if (err == 0)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Evaluate policy for loop %s using policy %s, loop, bdata(ldata->policy));
        for (i = 0; i < loop_adapt_num_active_policy; i++)
        {
            PolicyDefinition* pd = &loop_adapt_active_policy[i];
            if (bstrncmp(pd->name, ldata->policy, blength(pd->name)) == BSTR_OK)
            {
                return pd->eval(num_results, inputs, output);
            }
        }
        return -ENODEV;
    }
    return err;
}

bstring loop_adapt_policy_get_measurement(bstring policy)
{
    int i = 0;
    int err = 0;
    bstring out = bfromcstr("");
    for (i = 0; i < loop_adapt_num_active_policy; i++)
    {
        PolicyDefinition* pd = &loop_adapt_active_policy[i];
        if (bstrncmp(pd->name, policy, blength(pd->name)) == BSTR_OK)
        {
            bconcat(out, pd->measurement);
        }
    }
    return out;
}