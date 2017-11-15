
#include <hwloc.h>
#include <pthread.h>
#include <loop_adapt_types.h>
#include <loop_adapt_helper.h>

double loop_adapt_round(double val)
{
    int tmp = 0;
    if (val > 10000)
    {
        tmp = (int) val/1000;
        return (double) tmp * 1000;
    }
    else if (val > 1000)
    {
        tmp = (int) val/100;
        return (double) tmp * 100;
    }
    else if (val > 100)
    {
        tmp = (int) val/10;
        return (double) tmp * 10;
    }
    else if (val > 10)
    {
        tmp = (int) val;
        return (double) tmp;
    }
    else
    {
        tmp = (int) val;
        return (double) tmp;
    }
    return val;
}

int set_param_min(char* param, Nodevalues_t vals)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strncmp(vals->parameters[j]->name, param, strlen(param)) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                vals->parameters[j]->icur = vals->parameters[j]->imin;
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                vals->parameters[j]->dcur = vals->parameters[j]->dmin;
            }
            break;
        }
    }
}

int set_param_max(char* param, Nodevalues_t vals)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strcmp(vals->parameters[j]->name, param) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                vals->parameters[j]->icur = vals->parameters[j]->imax;
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                vals->parameters[j]->dcur = vals->parameters[j]->dmax;
            }
            break;
        }
    }
}

int set_param_step(char* param, Nodevalues_t vals, int step)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strcmp(vals->parameters[j]->name, param) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                int min = vals->parameters[j]->imin;
                int max = vals->parameters[j]->imax;
                int s = (max-min) / vals->parameters[j]->inter;
                vals->parameters[j]->icur = min + (step * s);
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                vals->parameters[j]->dcur = vals->parameters[j]->dmin;
                double min = vals->parameters[j]->dmin;
                double max = vals->parameters[j]->imax;
                double s = (max-min) / vals->parameters[j]->inter;
                vals->parameters[j]->dcur = min + (step * s);
            }
            break;
        }
    }
}

int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int num_profiles, int num_values)
{
    int len = hwloc_get_nbobjs_by_type(tree, type);
    Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
    for (int i = 0; i < len; i++)
    {
        // Get the object
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        // Allocate the memory for the profiles and profiles' values
        Nodevalues_t v = malloc(sizeof(Nodevalues));
        memset(v, 0, sizeof(Nodevalues));
        v->runtimes = malloc((num_profiles)*sizeof(TimerData));
        v->profiles = malloc((num_profiles)*sizeof(double*));
        v->num_profiles = num_profiles;
        v->num_parameters = 0;
        v->num_values = num_values;
        pthread_mutex_init(&v->lock, NULL);
        pthread_cond_init(&v->cond, NULL);
        // Allocate space for metrics
        for (int j = 0; j < v->num_profiles; j++)
        {
            
            v->profiles[j] = malloc(v->num_values * sizeof(double));
            memset(v->profiles[j], 0, v->num_values * sizeof(double));
        }
        // save the profile struct in the object
        obj->userdata = (void*)v;
    }
    return 0;
}

void free_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type)
{
    int len = hwloc_get_nbobjs_by_type(tree, type);
    for (int i = 0; i < len; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        for (int j = 0; j < v->num_profiles; j++)
        {
            free(v->profiles[j]);
        }
        for (int j = 0; j < v->num_parameters; j++)
        {
            free(v->parameters[j]);
        }
        pthread_mutex_destroy(&v->lock);
        pthread_cond_destroy(&v->cond);
        free(v->profiles);
        free(v->runtimes);
        free(v->parameters);
        free(v);
        obj->userdata = NULL;
    }
    return;
}

int populate_tree(hwloc_topology_t tree, Policy_t pol, int num_profiles)
{
    int max_num_values = LOOP_ADAPT_MAX_POLICY_METRICS;
    printf("populate_tree\n");
    int ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_THREAD, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_SOCKET, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_NUMA, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_MACHINE, num_profiles, max_num_values);
    if (ret)
        return ret;
    return 0;
}

void tidy_tree(hwloc_topology_t tree)
{
    free_nodevalues(tree, LOOP_ADAPT_SCOPE_THREAD);
    free_nodevalues(tree, LOOP_ADAPT_SCOPE_SOCKET);
    free_nodevalues(tree, LOOP_ADAPT_SCOPE_NUMA);
    free_nodevalues(tree, LOOP_ADAPT_SCOPE_MACHINE);
}


void _update_tree(hwloc_obj_t base, hwloc_obj_t obj, int profile)
{
    Nodevalues *bvals = (Nodevalues *)base->userdata;
    Nodevalues *ovals = (Nodevalues *)obj->userdata;
    pthread_mutex_lock(&ovals->lock);
    for (int m = 0; m < bvals->num_values; m++)
    {
        ovals->profiles[profile][m] += bvals->profiles[profile][m];
    }
    pthread_mutex_unlock(&ovals->lock);
}

void update_tree(hwloc_obj_t obj, int profile)
{
    hwloc_obj_t walker = obj->parent;

    if (obj && walker)
    {
        while (walker)
        {
            if (walker->type == LOOP_ADAPT_SCOPE_MACHINE ||
                walker->type == LOOP_ADAPT_SCOPE_NUMA ||
                walker->type == LOOP_ADAPT_SCOPE_SOCKET)
            {
                _update_tree(obj, walker, profile);
            }
            walker = walker->parent;
        }
    }
}
