
#include <hwloc.h>
#include <pthread.h>
#include <loop_adapt_types.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

double get_param_def_min(hwloc_topology_t tree, char* name)
{
    int found = 0;
    double min = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    for (int i= 0; i < tdata->num_policies; i++)
    {
        for (int j = 0; j < tdata->policies[i]->num_parameters; j++)
        {
            if (strncmp(name, tdata->policies[i]->parameters[j].name, strlen(name)) == 0)
            {
                min = calculate(tdata->policies[i]->parameters[j].def_min);
                found = 1;
                break;
            }
        }
        if (found)
            break;
    }
    return min;
}

double get_param_def_max(hwloc_topology_t tree, char* name)
{
    int found = 0;
    double max = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    for (int i= 0; i < tdata->num_policies; i++)
    {
        for (int j = 0; j < tdata->policies[i]->num_parameters; j++)
        {
            if (strncmp(name, tdata->policies[i]->parameters[j].name, strlen(name)) == 0)
            {
                max = calculate(tdata->policies[i]->parameters[j].def_max);
                found = 1;
                break;
            }
        }
        if (found)
            break;
    }
    return max;
}


void update_best(Policy_t p, hwloc_obj_t baseobj, hwloc_obj_t setobj)
{
    Nodevalues_t basev = (Nodevalues_t)baseobj->userdata;
    Nodevalues_t setv = (Nodevalues_t)setobj->userdata;
    for (int i = 0; i < p->num_parameters; i++)
    {
        for (int j = 0; j < setv->num_parameters; j++)
        {
            if (strncmp(p->parameters[i].name,
                        setv->parameters[j]->name,
                        strlen(p->parameters[i].name)) == 0)
            {
                printf("Setting best for param %s to ", setv->parameters[j]->name);
                if (setv->parameters[j]->type == NODEPARAMETER_INT)
                {
                    printf("%d", basev->parameters[j]->cur.icur);
                    setv->parameters[j]->best.ibest = basev->parameters[j]->cur.icur;
                }
                else if (setv->parameters[j]->type == NODEPARAMETER_DOUBLE)
                {
                    printf("%f", basev->parameters[j]->cur.dcur);
                    setv->parameters[j]->best.dbest = basev->parameters[j]->cur.dcur;
                }
                printf(" in node %d type %s\n", setobj->os_index, loop_adapt_type_name(setobj->type));
            }
        }
    }
}

void set_param_min(char* param, Nodevalues_t vals)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strncmp(vals->parameters[j]->name, param, strlen(param)) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                vals->parameters[j]->cur.icur = vals->parameters[j]->min.imin;
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                vals->parameters[j]->cur.dcur = vals->parameters[j]->min.dmin;
            }
            break;
        }
    }
}

void set_param_max(char* param, Nodevalues_t vals)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strcmp(vals->parameters[j]->name, param) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                vals->parameters[j]->cur.icur = vals->parameters[j]->max.imax;
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                vals->parameters[j]->cur.dcur = vals->parameters[j]->max.dmax;
            }
            break;
        }
    }
}

void set_param_step(char* param, Nodevalues_t vals, int step)
{
    for (int j = 0; j < vals->num_parameters; j++)
    {
        if (strcmp(vals->parameters[j]->name, param) == 0)
        {
            if (vals->parameters[j]->type == NODEPARAMETER_INT)
            {
                int min = vals->parameters[j]->min.imin;
                int max = vals->parameters[j]->max.imax;
                int s = (max-min) / vals->parameters[j]->inter;
                vals->parameters[j]->cur.icur = min + (step * s);
            }
            else if (vals->parameters[j]->type == NODEPARAMETER_DOUBLE)
            {
                double min = vals->parameters[j]->min.dmin;
                double max = vals->parameters[j]->max.imax;
                double s = (max-min) / vals->parameters[j]->inter;
                vals->parameters[j]->cur.dcur = min + (step * s);
            }
            break;
        }
    }
}

int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int num_profiles, int num_values)
{
    int len = hwloc_get_nbobjs_by_type(tree, type);
    //Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
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

int populate_tree(hwloc_topology_t tree, int num_profiles)
{
    int max_num_values = LOOP_ADAPT_MAX_POLICY_METRICS;
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

AdaptScope getLoopAdaptType(hwloc_obj_type_t t)
{
    return (AdaptScope)t;
}

void _update_tree(hwloc_obj_t base, hwloc_obj_t obj, int profile)
{
    Nodevalues_t bvals = (Nodevalues_t)base->userdata;
    Nodevalues_t ovals = (Nodevalues_t)obj->userdata;
    pthread_mutex_lock(&ovals->lock);

    for (int m = 0; m < bvals->num_values; m++)
    {
        ovals->profiles[profile][m] += bvals->profiles[profile][m];
        if (ovals->runtimes[profile].start.int64 < bvals->runtimes[profile].start.int64)
        {
            bvals->runtimes[profile].start.int64 = ovals->runtimes[profile].start.int64;
        }
        if (ovals->runtimes[profile].stop.int64 > bvals->runtimes[profile].stop.int64)
        {
            bvals->runtimes[profile].stop.int64 = ovals->runtimes[profile].stop.int64;
        }
    }
    ovals->cur_profile = bvals->cur_profile;
    ovals->num_values = bvals->num_values;
    ovals->num_profiles = bvals->num_profiles;
    
    pthread_mutex_unlock(&ovals->lock);
}

void update_tree(hwloc_obj_t obj, int profile)
{
    hwloc_obj_t walker = obj->parent;

    if (obj && walker)
    {
        while (walker)
        {
            AdaptScope t = getLoopAdaptType(walker->type);
            if (t == LOOP_ADAPT_SCOPE_MACHINE ||
                t == LOOP_ADAPT_SCOPE_NUMA ||
                t == LOOP_ADAPT_SCOPE_SOCKET)
            {
                _update_tree(obj, walker, profile);
            }
            walker = walker->parent;
        }
    }
}
