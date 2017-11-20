
#include <hwloc.h>
#include <pthread.h>
#include <loop_adapt_types.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

void *realloc_buffer(void *ptrmem, size_t size) {
    void *ptr = realloc(ptrmem, size);
    if (!ptr)  {
        fprintf(stderr, "realloc(%p, %lu): errno=%d\n", ptrmem, size, errno);
        free (ptrmem);
    }
    if (!ptrmem)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

double get_param_def_min(hwloc_topology_t tree, char* name)
{
    double min = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    for (int i = 0; i < tdata->num_policies; i++)
    {
        Policy_t p = tdata->policies[i];
        for (int j = 0; j < p->num_parameters; j++)
        {
            PolicyParameter_t pp = &p->parameters[j];
            if (strncmp(name, pp->name, strlen(name)) == 0)
            {
                //printf("Found parameter %s in policy %s with f = %s\n", name, p->name, pp->def_min);
                min = la_calc_calculate(pp->def_min);
                return min;
            }
        }
    }
    return min;
}

double get_param_def_max(hwloc_topology_t tree, char* name)
{
    double max = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    for (int i= 0; i < tdata->num_policies; i++)
    {
        Policy_t p = tdata->policies[i];
        for (int j = 0; j < p->num_parameters; j++)
        {
            PolicyParameter_t pp = &p->parameters[j];
            if (strncmp(name, pp->name, strlen(name)) == 0)
            {
                //printf("Found parameter %s in policy %s with f = %s\n", name, p->name, pp->def_max);
                max = la_calc_calculate(pp->def_max);
                return max;
            }
        }
    }
    return max;
}

int get_objidx_above_cpuidx(hwloc_topology_t tree, AdaptScope scope, int cpuidx)
{
    hwloc_obj_t cpuobj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, cpuidx);
    hwloc_obj_t walker = NULL;
    if (cpuobj)
    {
        walker = cpuobj->parent;
        while(walker && walker->type != (hwloc_obj_type_t)scope)
        {
            walker = walker->parent;
        }
        if (walker)
            return walker->logical_index;
    }
    return -1;
}


void update_best(PolicyParameter_t pp, hwloc_obj_t baseobj, hwloc_obj_t setobj)
{
    Nodevalues_t basev = (Nodevalues_t)baseobj->userdata;
    Nodevalues_t setv = (Nodevalues_t)setobj->userdata;

    char* pname = pp->name;
    Nodeparameter_t setp = g_hash_table_lookup(setv->param_hash, (gpointer) pname);
    Nodeparameter_t basep = g_hash_table_lookup(basev->param_hash, (gpointer) pname);
    if (setp && basep)
    {
        if (loop_adapt_debug)
            printf("Setting best for param %s to ", pname);
        if (setp->type == NODEPARAMETER_INT)
        {
            if (loop_adapt_debug) printf("%d", basep->cur.icur);
            setp->best.ibest = basep->cur.icur;
        }
        else if (setp->type == NODEPARAMETER_DOUBLE)
        {
            if (loop_adapt_debug) printf("%f", basep->cur.dcur);
            setp->best.dbest = basep->cur.dcur;
        }
        if (loop_adapt_debug)
            printf(" in node %d type %s\n", setobj->os_index, loop_adapt_type_name(setobj->type));
    }
}

void set_param_step(char* param, Nodevalues_t vals, int step)
{
    Nodeparameter_t np = g_hash_table_lookup(vals->param_hash, (gpointer)param);
    if (np)
    {
        if (np->type == NODEPARAMETER_INT)
        {
            int min = np->min.imin;
            int max = np->max.imax;
            int s = (max-min) / np->inter;
            np->cur.icur = min + (step * s);
        }
        else if (np->type == NODEPARAMETER_DOUBLE)
        {
            double min = np->min.dmin;
            double max = np->max.imax;
            double s = (max-min) / np->inter;
            np->cur.dcur = min + (step * s);
        }
    }
}

int allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int polidx, int num_profiles, int num_values)
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
        
        if (!v->profiles || v->num_policies < polidx)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Enlarge arrays of node %d\n", i);
            v->profiles = (double***)realloc_buffer(v->profiles, MIN(1, polidx+1)*sizeof(double**));
            v->num_profiles = (int*)realloc_buffer(v->num_profiles, MIN(1, polidx+1)*sizeof(int));
            v->runtimes = (TimerData**)realloc_buffer(v->runtimes,  MIN(1, polidx+1)*sizeof(TimerData*));
            v->num_values = realloc_buffer(v->num_values, MIN(1, polidx+1)*sizeof(int));
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Number of policies %d\n", polidx+1);
            v->opt_profiles = (int*)realloc_buffer(v->opt_profiles, MIN(1, polidx+1)*sizeof(int));
            v->num_policies = polidx+1;
        }
        v->profiles[polidx] = malloc(num_profiles*sizeof(double*));
        v->runtimes[polidx] = malloc((num_profiles)*sizeof(TimerData));
        v->num_profiles[polidx] = num_profiles;
        v->num_values[polidx] = num_values;
        v->cur_policy = 0;
        v->cur_profile = 0;
        //v->num_parameters = 0;
        v->param_hash = g_hash_table_new(g_str_hash, g_str_equal);
        pthread_mutex_init(&v->lock, NULL);
        pthread_cond_init(&v->cond, NULL);
        // Allocate space for metrics
        for (int j = 0; j < v->num_profiles[polidx]; j++)
        {
            
            v->profiles[polidx][j] = malloc(v->num_values[polidx] * sizeof(double));
            memset(v->profiles[polidx][j], 0, v->num_values[polidx] * sizeof(double));
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
        for (int j = 0; j < v->num_policies; j++)
        {
            for (int k = 0; k < v->num_profiles[j]; k++)
                free(v->profiles[j][k]);
            free(v->profiles[j]);
            free(v->runtimes[j]);
        }
/*        for (int j = 0; j < v->num_parameters; j++)*/
/*        {*/
/*            free(v->parameters[j]);*/
/*        }*/
        pthread_mutex_destroy(&v->lock);
        pthread_cond_destroy(&v->cond);
        g_hash_table_destroy(v->param_hash);
        free(v->opt_profiles);
        free(v->profiles);
        free(v->runtimes);
/*        free(v->parameters);*/
        free(v->num_profiles);
        free(v->num_values);
        free(v);
        obj->userdata = NULL;
    }
    return;
}

int populate_tree(hwloc_topology_t tree, int polidx, int num_profiles)
{
    int max_num_values = LOOP_ADAPT_MAX_POLICY_METRICS;
    int ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_THREAD, polidx, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_SOCKET, polidx, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_NUMA, polidx, num_profiles, max_num_values);
    if (ret)
        return ret;
    ret = allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_MACHINE, polidx, num_profiles, max_num_values);
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

    for (int m = 0; m < bvals->num_values[bvals->cur_policy]; m++)
    {
        ovals->profiles[bvals->cur_policy][profile][m] += bvals->profiles[bvals->cur_policy][profile][m];
        if (ovals->runtimes[bvals->cur_policy][profile].start.int64 < bvals->runtimes[bvals->cur_policy][profile].start.int64)
        {
            bvals->runtimes[bvals->cur_policy][profile].start.int64 = ovals->runtimes[bvals->cur_policy][profile].start.int64;
        }
        if (ovals->runtimes[bvals->cur_policy][profile].stop.int64 > bvals->runtimes[bvals->cur_policy][profile].stop.int64)
        {
            bvals->runtimes[bvals->cur_policy][profile].stop.int64 = ovals->runtimes[bvals->cur_policy][profile].stop.int64;
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

static inline int update_cur_policy_in_tree_types(hwloc_topology_t tree, hwloc_obj_type_t type, int polidx)
{
    hwloc_obj_t obj = NULL;
    int len = hwloc_get_nbobjs_by_type(tree, type);
    for (int i = 0; i < len; i++)
    {
        obj = hwloc_get_obj_by_type(tree, type, i);
        Nodevalues_t nv = (Nodevalues_t)obj->userdata;
        if (nv)
        {
            nv->cur_profile = 0;
            nv->cur_policy = polidx;
        }
    }
}

void update_cur_policy_in_tree(hwloc_topology_t tree, int polidx)
{
    update_cur_policy_in_tree_types(tree, HWLOC_OBJ_PU, polidx);
    update_cur_policy_in_tree_types(tree, HWLOC_OBJ_PACKAGE, polidx);
    update_cur_policy_in_tree_types(tree, HWLOC_OBJ_NUMANODE, polidx);
    update_cur_policy_in_tree_types(tree, HWLOC_OBJ_MACHINE, polidx);
}
















