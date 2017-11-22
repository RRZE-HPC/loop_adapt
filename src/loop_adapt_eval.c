#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <loop_adapt_eval.h>
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
            fprintf(stderr, "Starting with policy %s (%d)\n", p->name, polidx);
        }
        ret = populate_tree(tree, polidx, num_profiles, iters_per_profile);
        update_cur_policy_in_tree(tree, polidx);
        p->loop_adapt_eval_init(num_cpus, cpulist, num_profiles);
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
    if (!obj || !name)
    {
        fprintf(stderr, "No obj or no name\n");
        return -EINVAL;
    }
    nv = (Nodevalues_t)obj->userdata;
    if (_parameter_exists(nv, name))
    {
        fprintf(stderr, "Parameter already exists\n");
        return 1;
    }
    np = _parameter_malloc(name, desc);
    if (np)
    {
        np->type = NODEPARAMETER_INT;
        np->pre = pre;
        np->post = post;
        np->best.ibest = -1;
        np->min.imin = min;
        np->max.imax = max;
        np->cur.icur = cur;
        np->inter = nv->num_profiles[nv->cur_policy]-2;
        // Add the parameter to the parameter hash table of the node
        g_hash_table_insert(nv->param_hash, (gpointer) g_strdup(name), (gpointer) np);
    }
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
    nv = (Nodevalues_t)obj->userdata;
    if (_parameter_exists(nv, name))
    {
        fprintf(stderr, "Parameter already exists\n");
        return 1;
    }
    np = _parameter_malloc(name, desc);
    if (np)
    {
        np->type = NODEPARAMETER_DOUBLE;
        np->pre = pre;
        np->post = post;
        np->best.dbest = -1.0;
        np->min.dmin = min;
        np->max.dmax = max;
        np->cur.dcur = cur;
        np->inter = nv->num_profiles[nv->cur_policy]-2;
        // Add the parameter to the parameter hash table of the node
        g_hash_table_insert(nv->param_hash, (gpointer) g_strdup(name), (gpointer) np);
    }
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
