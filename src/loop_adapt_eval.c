#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <loop_adapt_eval.h>
#include <loop_adapt_helper.h>

static Policy **eval_policies = NULL;
static int num_eval_policies = 0;

int loop_adapt_add_policy(hwloc_topology_t tree, Policy *p, int num_cpus, int *cpulist, int num_profiles)
{
    int ret = -EINVAL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    if (tdata)
    {
        Policy_t* tmp = realloc(tdata->policies, (tdata->num_policies+1)*sizeof(Policy_t));
        if (!tmp)
        {
            fprintf(stderr, "Cannot allocate space for policy.\n");
            free(tdata->policies);
            tdata->num_policies = 0;
            ret = -ENOMEM;
            goto loop_adapt_add_policy_out;
        }
        tdata->policies = tmp;
        tdata->policies[tdata->num_policies] = p;
        tdata->num_policies++;
        if (tdata->cur_policy == NULL)
        {
            tdata->cur_policy = p;
            int gid = p->loop_adapt_eval_init(num_cpus, cpulist, num_profiles);
            p->likwid_gid = gid;
            ret = populate_tree(tree, num_profiles);
        }
    }
loop_adapt_add_policy_out:
    return ret;
}

static int _parameter_exists(hwloc_obj_t obj, char* name)
{
    Nodevalues_t nv = (Nodevalues_t)obj->userdata;
    for (int i = 0; i < nv->num_parameters; i++)
    {
        if (strncmp(name, nv->parameters[i]->name, strlen(name)) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static Nodeparameter_t _parameter_malloc(char* name, char* desc)
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
    if (!obj || !name)
    {
        return -EINVAL;
    }
    if (_parameter_exists(obj, name))
        return 1;
    

    np = _parameter_malloc(name, desc);
    //printf("Adding parameter %s for obj %s idx %d\n", name, hwloc_obj_type_string(obj->type), obj->os_index);
    nv = (Nodevalues_t)obj->userdata;
    
    np->type = NODEPARAMETER_INT;
    np->pre = pre;
    np->post = post;
    np->best.ibest = -1;
    np->min.imin = min;
    np->max.imax = max;
    np->cur.icur = cur;
    np->inter = nv->num_profiles-2;
    
    tmp = realloc(nv->parameters, (nv->num_parameters+1)*sizeof(Nodevalues_t));
    if (!tmp)
    {
        fprintf(stderr, "Cannot append parameter\n");
        free(nv->parameters);
        nv->num_parameters = 0;
        return -ENOMEM;
    }
    nv->parameters = tmp;
    nv->parameters[nv->num_parameters] = np;
    nv->num_parameters++;
    return 0;
}

int loop_adapt_add_double_parameter(hwloc_obj_t obj, char* name, char* desc, double cur, double min, double max, void(*pre)(char* fmt, ...), void(*post)(char* fmt, ...))
{

    Nodeparameter_t np = NULL;
    Nodevalues_t nv = NULL;
    Nodeparameter_t *tmp = NULL;
    if (!obj || !name)
    {
        return -EINVAL;
    }
    if (_parameter_exists(obj, name))
        return 1;
    

    np = _parameter_malloc(name, desc);
    //printf("Adding parameter %s for obj %s idx %d\n", name, hwloc_obj_type_string(obj->type), obj->os_index);
    nv = (Nodevalues_t)obj->userdata;
    
    np->type = NODEPARAMETER_DOUBLE;
    np->pre = pre;
    np->post = post;
    np->best.dbest = -1.0;
    np->min.dmin = min;
    np->max.dmax = max;
    np->cur.dcur = cur;
    np->inter = nv->num_profiles-2;
    
    tmp = realloc(nv->parameters, (nv->num_parameters+1)*sizeof(Nodevalues_t));
    if (!tmp)
    {
        fprintf(stderr, "Cannot append parameter\n");
        free(nv->parameters);
        nv->num_parameters = 0;
        return -ENOMEM;
    }
    nv->parameters = tmp;
    nv->parameters[nv->num_parameters] = np;
    nv->num_parameters++;
    return 0;
}

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
        }
        p->loop_adapt_eval_begin(cpuid, tree, obj);
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
            p->loop_adapt_eval_end(cpuid, tree, obj);
        }
    }
    return 0;
}

int loop_adapt_exec_policies(hwloc_topology_t tree, hwloc_obj_t obj)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    if (tdata && tdata->num_policies > 0)
    {
        Policy_t cur_p = tdata->cur_policy;
        if (cur_p)
        {
            //printf("Executing policy %s\n", cur_p->name);
            cur_p->loop_adapt_eval(tree, obj);
        }
        for (int i = 0; i < tdata->num_policies; i++)
        {
            Policy_t p = tdata->policies[i];
            if (p != cur_p && p->likwid_group == cur_p->likwid_group)
            {
                printf("Executing policy %s because it uses the same data\n", p->name);
                p->loop_adapt_eval(tree, obj);
            }
        }
    }
    else
    {
        fprintf(stdout, "No policy registered.\n");
    }
    return 0;
}

__attribute__((destructor)) void loop_adapt_eval_finalize (void)
{
    if (num_eval_policies > 0)
    {
        free(eval_policies);
        num_eval_policies = 0;
    }
}
