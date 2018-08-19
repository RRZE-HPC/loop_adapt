#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <loop_adapt_policy.h>
#include <loop_adapt_search.h>
#include <loop_adapt_helper.h>

static Policy **eval_policies = NULL;
static int num_eval_policies = 0;

int loop_adapt_add_policy(hwloc_topology_t tree, Policy *p, int num_cpus, int *cpulist, int num_profiles, int iters_per_profile)
{
    int ret = -EINVAL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    if (tdata)
    {
        tdata->policies = realloc_buffer(tdata->policies, (tdata->num_policies+1)*sizeof(Policy_t));
        if (!tdata->policies)
        {
            fprintf(stderr, "Cannot allocate space for policy.\n");
            tdata->num_policies = 0;
            ret = -ENOMEM;
            goto loop_adapt_add_policy_out;
        }
        int polidx = tdata->num_policies;
        tdata->policies[tdata->num_policies] = p;
        tdata->num_policies++;
        if (tdata->cur_policy == NULL)
        {
            tdata->cur_policy = p;
            tdata->cur_policy_id = polidx;
            if (loop_adapt_debug)
                fprintf(stderr, "Starting with policy %s (%d)\n", p->name, polidx);
        }
        ret = populate_tree(tree, polidx, num_profiles, iters_per_profile);
        if (tdata->cur_policy_id < 0)
        {
            printf("Update tree with polidx %d\n", polidx);
            update_cur_policy_in_tree(tree, polidx);
        }
        p->loop_adapt_policy_init(num_cpus, cpulist, num_profiles);
    }
loop_adapt_add_policy_out:
    return ret;
}

static inline int _parameter_exists(Nodevalues_t vals, char* name)
{
    if (vals && name)
    {
        Nodeparameter_t np = g_hash_table_lookup(vals->param_hash, (gpointer) name);
        if (np)
            return 1;
    }
    return 0;
}

static inline Nodeparameter_t _parameter_malloc(char* name, char* desc)
{
    int ret = 0;
    Nodeparameter_t np = NULL;
    np = malloc(sizeof(Nodeparameter));
    if (np)
    {
        memset(np, 0, sizeof(Nodeparameter));
        ret = asprintf(&np->name, "%s", name);
        if (ret < 0)
            printf("Cannot set parameter name %s\n", name);
        ret = asprintf(&np->desc, "%s", desc);
        if (ret < 0)
            printf("Cannot set parameter description %s\n", desc);
    }
    return np;
}


int loop_adapt_add_int_parameter(hwloc_obj_t obj, char* name, char* desc, int cur, int min, int max, void(*pre)(char* fmt, ...), void(*post)(char* fmt, ...))
{
    Nodeparameter_t np = NULL;
    Nodevalues_t nv = NULL;
    Nodeparameter_t *tmp = NULL;
    PolicyProfile_t pp = NULL;

    if (!obj || !name)
    {
        fprintf(stderr, "No obj or no name\n");
        return -EINVAL;
    }
    nv = (Nodevalues_t)obj->userdata;
    if (nv)
    {
        pp = nv->policies[nv->cur_policy];
        if (!nv->param_hash)
        {
            nv->param_hash = g_hash_table_new(g_str_hash, g_str_equal);
        }
        if (_parameter_exists(nv, name))
        {
            fprintf(stderr, "Parameter already exists for %s %d\n", loop_adapt_type_name(obj->type), obj->os_index);
            return 1;
        }
        np = _parameter_malloc(name, desc);
        if (np)
        {
            np->type = NODEPARAMETER_INT;
            np->pre = pre;
            np->post = post;
            np->best.ival = -1;
            np->min.ival = min;
            np->max.ival = max;
            np->cur.ival = cur;
            np->start.ival = cur;
            np->has_best = 0;
            np->steps = pp->num_profiles-1;
            np->old_vals = (Value*)malloc((pp->num_profiles+1) * sizeof(Value));
            np->num_old_vals = 0;
            np->old_vals[0].ival = np->start.ival;
            np->num_old_vals++;
            // Add the parameter to the parameter hash table of the node
            g_hash_table_insert(nv->param_hash, (gpointer) g_strdup(name), (gpointer) np);
        }
    }
    return 0;
}

int loop_adapt_add_double_parameter(hwloc_obj_t obj, char* name, char* desc, double cur, double min, double max, void(*pre)(char* fmt, ...), void(*post)(char* fmt, ...))
{

    Nodeparameter_t np = NULL;
    Nodevalues_t nv = NULL;
    Nodeparameter_t *tmp = NULL;
    PolicyProfile_t pp = NULL;

    if (!obj || !name)
    {
        return -EINVAL;
    }
    nv = (Nodevalues_t)obj->userdata;
    if (nv)
    {
        pp = nv->policies[nv->cur_policy];
        if (!nv->param_hash)
        {
            nv->param_hash = g_hash_table_new(g_str_hash, g_str_equal);
        }
        if (_parameter_exists(nv, name))
        {
            fprintf(stderr, "ERROR: Parameter already exists\n");
            return 1;
        }
        np = _parameter_malloc(name, desc);
        if (np)
        {
            np->type = NODEPARAMETER_DOUBLE;
            np->pre = pre;
            np->post = post;
            np->best.dval = -1.0;
            np->min.dval = min;
            np->max.dval = max;
            np->cur.dval = cur;
            np->has_best = 0;
            np->start.dval = cur;
            np->steps = pp->num_profiles-1;
            np->old_vals = (Value*)malloc((pp->num_profiles+1) * sizeof(Value));
            np->num_old_vals = 0;
            np->old_vals[0].dval = np->cur.dval;
            np->num_old_vals++;
            // Add the parameter to the parameter hash table of the node
            g_hash_table_insert(nv->param_hash, (gpointer) g_strdup(name), (gpointer) np);
        }
    }
    return 0;
}

/*static int _loop_adapt_reset_parameter_in_node(hwloc_obj_t obj, Policy_t p)*/
/*{*/
/*    Nodevalues_t nv = (Nodevalues_t)obj->userdata;*/
/*    */
/*    for (int i = 0; i < p->num_parameters; i++)*/
/*    {*/
/*        if (loop_adapt_debug == 2)*/
/*            fprintf(stderr, "DEBUG: Reset param %s in %s %d\n", p->parameters[i].name, loop_adapt_type_name(obj->type), obj->os_index);*/
/*        Nodeparameter_t np = g_hash_table_lookup(nv->param_hash, (gpointer) p->parameters[i].name);*/
/*        if (np && np->type == NODEPARAMETER_INT)*/
/*        {*/
/*            np->best.ival = np->start.ival;*/
/*            np->cur.ival = np->start.ival;*/
/*        }*/
/*        else if (np && np->type == NODEPARAMETER_DOUBLE)*/
/*        {*/
/*            np->best.dval = np->start.dval;*/
/*            np->cur.dval = np->start.dval;*/
/*        }*/
/*    }*/
/*}*/

/*int loop_adapt_reset_parameter(hwloc_topology_t tree, Policy_t p)*/
/*{*/
/*    for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)p->scope); i++)*/
/*    {*/
/*        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)p->scope, i);*/
/*        _loop_adapt_reset_parameter_in_node(obj, p);*/
/*    }*/
/*    return 0;*/
/*}*/

/*static int _loop_adapt_best_parameter_in_node(hwloc_obj_t obj, Policy_t p)*/
/*{*/
/*    Nodevalues_t nv = (Nodevalues_t)obj->userdata;*/
/*    */
/*    for (int i = 0; i < p->num_parameters; i++)*/
/*    {*/
/*        if (loop_adapt_debug == 2)*/
/*            fprintf(stderr, "DEBUG: Reset param %s in %s %d\n", p->parameters[i].name, loop_adapt_type_name(obj->type), obj->os_index);*/
/*        Nodeparameter_t np = g_hash_table_lookup(nv->param_hash, (gpointer) p->parameters[i].name);*/
/*        if (np && np->type == NODEPARAMETER_INT)*/
/*        {*/
/*            np->cur.ival = np->best.ival;*/
/*            printf("Best %d\n", np->cur.ival);*/
/*        }*/
/*        else if (np && np->type == NODEPARAMETER_DOUBLE)*/
/*        {*/
/*            np->cur.dval = np->best.dval;*/
/*            printf("Best %f\n", np->cur.dval);*/
/*        }*/
/*    }*/
/*}*/

/*int loop_adapt_best_parameter(hwloc_topology_t tree, Policy_t p)*/
/*{*/
/*    for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)p->scope); i++)*/
/*    {*/
/*        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)p->scope, i);*/
/*        _loop_adapt_best_parameter_in_node(obj, p);*/
/*    }*/
/*    return 0;*/
/*}*/

/*static int _loop_adapt_first_parameter_in_node(hwloc_obj_t obj, Policy_t p)*/
/*{*/
/*    Nodevalues_t nv = (Nodevalues_t)obj->userdata;*/
/*    for (int i = 0; i < p->num_parameters; i++)*/
/*    {*/
/*        Nodeparameter_t np = g_hash_table_lookup(nv->param_hash, (gpointer) p->parameters[i].name);*/
/*        if (np && np->type == NODEPARAMETER_INT)*/
/*        {*/
/*            np->cur.ival = np->min.ival;*/
/*            //printf("Initial %d\n", np->min.ival);*/
/*        }*/
/*        else if (np && np->type == NODEPARAMETER_DOUBLE)*/
/*        {*/
/*            np->cur.dval = np->min.dval;*/
/*            //printf("Initial %f\n", np->min.dval);*/
/*        }*/
/*    }*/
/*}*/

/*int loop_adapt_first_parameter(hwloc_topology_t tree, Policy_t p)*/
/*{*/
/*    for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)p->scope); i++)*/
/*    {*/
/*        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)p->scope, i);*/
/*        _loop_adapt_first_parameter_in_node(obj, p);*/
/*    }*/
/*    return 0;*/
/*}*/

/*int loop_adapt_add_event(hwloc_obj_t obj, char* name, char* var, Nodeparametertype type, void* ptr)*/
/*{*/
/*    int ret = 0;*/
/*    Nodevalues_t nv = NULL;*/
/*    Nodeevent_t ne = NULL;*/
/*    */
/*    if (!obj || !name || !var || !ptr)*/
/*    {*/
/*        return -EINVAL;*/
/*    }*/
/*    nv = (Nodevalues_t)obj->userdata;*/
/*    for (int i=0 ; i < nv->num_events; i++)*/
/*    {*/
/*        ne = nv->events[i];*/
/*        if (strncmp(ne->name, name, strlen(ne->name)))*/
/*        {*/
/*            fprintf(stderr, "Event already exists\n");*/
/*            return 1;*/
/*        }*/
/*    }*/
/*    ne = malloc(sizeof(Nodeevent));*/
/*    if (ne)*/
/*    {*/
/*        ret = asprintf(&ne->name, "%s", name);*/
/*        ret = asprintf(&ne->varname, "%s", var);*/
/*        ne->type = type;*/
/*        ne->ptr = ptr;*/

/*        nv->events = realloc_buffer(nv->events, (nv->num_events+1)*sizeof(Nodeevent_t));*/
/*        nv->events[nv->num_events] = ne;*/
/*        nv->num_events++;*/
/*    }*/
/*    return 0;*/
/*}*/

int loop_adapt_begin_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    if (tdata && tdata->num_policies > 0)
    {
        Policy_t p = tdata->cur_policy;
        if (!p)
        {
            p = tdata->policies[0];
            tdata->cur_policy = p;
            tdata->cur_policy_id = 0;
        }
        p->loop_adapt_policy_begin(cpuid, tree, obj);
    }
    return 0;
}

int loop_adapt_end_policies(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    if (tdata && tdata->num_policies > 0)
    {
        Policy_t p = tdata->cur_policy;
        if (p)
        {
            p->loop_adapt_policy_end(cpuid, tree, obj);
        }
    }
    return 0;
}

int loop_adapt_init_parameter(hwloc_obj_t obj, Policy_t policy)
{
    int count = 0;
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (vals)
    {
        for (int i = 0; i < policy->num_parameters; i++)
        {
            Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
            if (param)
            {
                printf("Init param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(obj->type), obj->os_index);
                loop_adapt_search_param_init(policy->search, param);
                count++;
            }
        }
    }
    hwloc_obj_t walker = obj->parent;
    while (walker && !count)
    {
        vals = (Nodevalues_t)walker->userdata;
        if (vals)
        {
            for (int i = 0; i < policy->num_parameters; i++)
            {
                Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
                if (param)
                {
                    printf("Init param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(walker->type), walker->os_index);
                    loop_adapt_search_param_init(policy->search, param);
                    count++;
                }
            }
        }
    }
    return count;
}

static int loop_adapt_best_parameter(hwloc_obj_t obj, Policy_t policy)
{
    int count = 0;
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (vals && vals->param_hash)
    {
        for (int i = 0; i < policy->num_parameters; i++)
        {
            Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
            if (param)
            {
                printf("Mark best param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(obj->type), obj->os_index);
                loop_adapt_search_param_best(policy->search, param);
                count++;
            }
        }
    }
    hwloc_obj_t walker = obj->parent;
    while (walker && !count)
    {
        vals = (Nodevalues_t)walker->userdata;
        if (vals && vals->param_hash)
        {
            for (int i = 0; i < policy->num_parameters; i++)
            {
                Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
                if (param)
                {
                    printf("Mark best param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(walker->type), walker->os_index);
                    loop_adapt_search_param_best(policy->search, param);
                    count++;
                }
            }
        }
    }
    return count;
}

static int loop_adapt_next_parameter(hwloc_obj_t obj, Policy_t policy)
{
    int count = 0;
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (vals)
    {
        for (int i = 0; i < policy->num_parameters; i++)
        {
            Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
            if (param)
            {
                printf("Next param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(obj->type), obj->os_index);
                loop_adapt_search_param_next(policy->search, param);
                count++;
            }
        }
    }
    hwloc_obj_t walker = obj->parent;
    while (walker && !count)
    {
        vals = (Nodevalues_t)walker->userdata;
        if (vals)
        {
            for (int i = 0; i < policy->num_parameters; i++)
            {
                Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
                if (param)
                {
                    printf("Next param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(walker->type), walker->os_index);
                    loop_adapt_search_param_next(policy->search, param);
                    count++;
                }
            }
        }
    }
    return count;
}

static int loop_adapt_reset_parameter(hwloc_obj_t obj, Policy_t policy)
{
    int count = 0;
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (vals)
    {
        for (int i = 0; i < policy->num_parameters; i++)
        {
            Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
            if (param)
            {
                printf("Reset param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(obj->type), obj->os_index);
                loop_adapt_search_param_reset(policy->search, param);
                count++;
            }
        }
    }
    hwloc_obj_t walker = obj->parent;
    while (walker && !count)
    {
        vals = (Nodevalues_t)walker->userdata;
        if (vals)
        {
            for (int i = 0; i < policy->num_parameters; i++)
            {
                Nodeparameter_t param = g_hash_table_lookup(vals->param_hash, policy->parameters[i].name);
                if (param)
                {
                    printf("Reset param %s using search algo %s for %s %d\n", policy->parameters[i].name, policy->search->name, loop_adapt_type_name(walker->type), walker->os_index);
                    loop_adapt_search_param_reset(policy->search, param);
                    count++;
                }
            }
        }
    }
    return count;
}

int loop_adapt_exec_policies(hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0, oldret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && tdata->num_policies > 0)
    {
        Policy_t cur_p = tdata->cur_policy;
        if (cur_p)
        {
            PolicyProfile_t pp = vals->policies[vals->cur_policy];
            if (loop_adapt_debug)
                printf("Executing policy %s for %s %d for profile %d\n", cur_p->name, loop_adapt_type_name(obj->type), obj->logical_index, pp->cur_profile-1);
            ret = cur_p->loop_adapt_policy_eval(tree, obj);
            if (ret)
            {
                pp->opt_profile = pp->cur_profile-1;
                loop_adapt_best_parameter(obj, cur_p);
                loop_adapt_next_parameter(obj, cur_p);
            }
            else if (pp->cur_profile < 0)
            {
                loop_adapt_init_parameter(obj, cur_p);
            }
            else if (pp->cur_profile <= pp->num_profiles-1)
            {
                loop_adapt_next_parameter(obj, cur_p);
            }
            else if (pp->cur_profile = pp->num_profiles)
            {
                loop_adapt_reset_parameter(obj, cur_p);
            }

        }
/*        for (int i = 0; i < tdata->num_policies; i++)*/
/*        {*/
/*            Policy_t p = tdata->policies[i];*/
/*            PolicyProfile_t pp = vals->policies[i];*/
/*            if (p != cur_p && p->likwid_group == cur_p->likwid_group)*/
/*            {*/
/*                if (loop_adapt_debug)*/
/*                    printf("Executing policy %s because it uses the same data\n", p->name);*/
/*                oldret = ret;*/
/*                ret += p->loop_adapt_policy_eval(tree, obj);*/
/*                if (ret != oldret)*/
/*                {*/
/*                    // Should we process these policy parameters as well?*/
/*                    // pp->opt_profile = pp->cur_profile-1;*/
/*                    // loop_adapt_best_parameter(tree, p)*/
/*                }*/
/*            }*/
/*        }*/

    }
    else
    {
        fprintf(stdout, "No policy registered.\n");
    }
    return ret;
}

TimerData* loop_adapt_policy_profile_get_timer(PolicyProfile_t pp)
{
    TimerData *t = NULL;
    if (!pp)
    {
        return NULL;
    }
    return (pp->cur_profile >= 0 ? &pp->profiles[pp->cur_profile]->timer : &pp->base_profile->timer);
}

double* loop_adapt_policy_profile_get_runtime(PolicyProfile_t pp)
{
    TimerData *t = NULL;
    if (!pp)
    {
        return NULL;
    }
    return (pp->cur_profile >= 0 ? &pp->profiles[pp->cur_profile]->runtime : &pp->base_profile->runtime);
}


double* loop_adapt_policy_profile_get_values(PolicyProfile_t pp)
{
    TimerData *t = NULL;
    if (!pp)
    {
        return NULL;
    }
    return (pp->cur_profile >= 0 ? pp->profiles[pp->cur_profile]->values : pp->base_profile->values);
}


__attribute__((destructor)) void loop_adapt_policy_finalize (void)
{
    if (num_eval_policies > 0)
    {
        free(eval_policies);
        num_eval_policies = 0;
    }
}
