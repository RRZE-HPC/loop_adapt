
#include <stdlib.h>
#include <stdio.h>
#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <error.h>
#include <loop_adapt_internal.h>
#include <loop_adapt_parameter_value_types.h>

#include <loop_adapt_policy_list.h>

#ifdef USE_MPI
#include <mpi.h>
#endif


static PolicyDefinition* loop_adapt_active_policy = NULL;
static int loop_adapt_num_active_policy = 0;

int _loop_adapt_copy_policy(_PolicyDefinition *in, PolicyDefinition* out)
{
    if (in && out)
    {
        out->name = bfromcstr(in->name);
        out->backend = bfromcstr(in->backend);
        out->config = bfromcstr(in->config);
        out->match = bfromcstr(in->match);
        out->eval = in->eval;

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
            bdestroy(pd->backend);
            bdestroy(pd->config);
            bdestroy(pd->match);
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
            return i;
        }
    }
    return -1;
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
        tmp->backend = bfromcstr(backend);
        tmp->config = bfromcstr(config);
        tmp->match = bfromcstr(match);
        tmp->eval = func;

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
        if (ldata->policy >= 0 && ldata->policy < loop_adapt_num_active_policy)
        {
            PolicyDefinition* pd = &loop_adapt_active_policy[ldata->policy];
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Evaluate policy for loop %s using policy %d, loop, bdata(pd->name));
            int err = pd->eval(num_results, inputs, output);
#ifdef MPI
            if (err == 0)
            {
                err = MPI_Reduce(&output, &output, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
            }
#endif
            return err;
        }
        return -ENODEV;
    }
    return err;
}

bstring loop_adapt_policy_get_measurement(int policy)
{
    int i = 0;
    int err = 0;
    bstring out = bfromcstr("");
    if (policy >= 0 && policy < loop_adapt_num_active_policy)
    {
        PolicyDefinition* pd = &loop_adapt_active_policy[policy];
        bconcat(out, pd->backend);
    }
    return out;
}

PolicyDefinition_t loop_adapt_policy_get(int policy)
{
    if (policy >= 0 && policy < loop_adapt_num_active_policy)
    {
        PolicyDefinition_t pd = &loop_adapt_active_policy[policy];
        return pd;
    }
    return NULL;
}

int loop_adapt_policy_getbnames(struct bstrList* policies)
{
    int i = 0;
    for (i = 0; i < loop_adapt_num_active_policy; i++)
    {
        PolicyDefinition_t pd = &loop_adapt_active_policy[i];
        bstrListAdd(policies, pd->name);
    }
    return 0;
}
