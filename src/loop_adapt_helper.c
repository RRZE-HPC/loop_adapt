
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
    if (ptr && !ptrmem)
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

int get_pes_below_obj(hwloc_topology_t tree, AdaptScope scope, int idx)
{
    hwloc_obj_t root = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)scope, idx);
    if (root)
    {
        hwloc_cpuset_t cpuset = root->cpuset;
        return hwloc_bitmap_weight(cpuset);
    }
    return 0;
}

int loop_adapt_location(hwloc_obj_t obj, char* array, int len)
{
    int ret = 0;
    switch (obj->type)
    {
        case HWLOC_OBJ_MACHINE:
            ret = snprintf(array, len-1, "N");
            break;
        case HWLOC_OBJ_NUMANODE:
            ret = snprintf(array, len-1, "M%d", obj->os_index);
            break;
        case HWLOC_OBJ_PACKAGE:
            ret = snprintf(array, len-1, "S%d", obj->os_index);
            break;
        case HWLOC_OBJ_PU:
            ret = snprintf(array, len-1, "T%d", obj->os_index);
            break;
    }
    return ret;
}


void update_best(PolicyParameter_t pp, hwloc_obj_t baseobj, hwloc_obj_t setobj)
{
    Nodevalues_t basev = (Nodevalues_t)baseobj->userdata;
    Nodevalues_t setv = (Nodevalues_t)setobj->userdata;
    char* pname = pp->name;
    Nodeparameter_t setp = g_hash_table_lookup(setv->param_hash, (gpointer) pname);
    Nodeparameter_t basep = g_hash_table_lookup(basev->param_hash, (gpointer) pname);
    if (setp != NULL && basep != NULL )
    {
        if (loop_adapt_debug)
            printf("Setting best for param %s to ", pname);
        if (setp->type == NODEPARAMETER_INT)
        {
            if (loop_adapt_debug) printf("%d", basep->cur.ival);
            setp->best.ival = basep->cur.ival;
        }
        else if (setp->type == NODEPARAMETER_DOUBLE)
        {
            if (loop_adapt_debug) printf("%f", basep->cur.dval);
            setp->best.dval = basep->cur.dval;
        }
        if (loop_adapt_debug)
            printf(" in node %d type %s\n", setobj->os_index, loop_adapt_type_name(setobj->type));
    }
}



static Profile_t new_profile()
{
    Profile_t p = malloc(sizeof(Profile));
    if (!p)
    {
        printf("Cannot allocate new profile\n");
    }
    else
    {
        memset(p, 0, sizeof(Profile));
    }
    return p;
}

static int combine_profiles(Profile_t in, Profile_t inout, int num_values)
{
    if (in && inout )
    {
        if (in->runtime == 0)
        {
            in->runtime = timer_print(&in->timer);
        }
        inout->runtime += in->runtime;
        for (int i = 0; i < num_values; i++)
        {
            if (loop_adapt_debug > 1)
            {
                printf("Combine %f into %f\n", in->values[i], inout->values[i]);
            }
            inout->values[i] += in->values[i];
        }
        return 0;
    }
    return -EINVAL;
}

static PolicyProfile_t new_pol_profile()
{
    PolicyProfile_t pp = malloc(sizeof(PolicyProfile));
    if (!pp)
    {
        printf("Cannot allocate new policy profile\n");
    }
    else
    {
        memset(pp, 0, sizeof(PolicyProfile));
    }
    return pp;
}

static int combine_pol_profiles(PolicyProfile_t in, PolicyProfile_t inout, int profile)
{
    Profile_t inprofile = NULL;
    Profile_t outprofile = NULL;
    

    if (profile >= 0)
    {
        inprofile = in->profiles[profile];
        outprofile = inout->profiles[profile];
    }
    else
    {
        inprofile = in->base_profile;
        outprofile = inout->base_profile;
    }
    inout->cur_profile_iters = in->cur_profile_iters;
    inout->num_profiles = MAX(inout->num_profiles, in->num_profiles);
    inout->profile_iters = MAX(inout->profile_iters, in->profile_iters);
    inout->cur_profile = MAX(inout->cur_profile, in->cur_profile);
    return combine_profiles(inprofile, outprofile, MIN(in->num_values, inout->num_values));
}

static int
_allocate_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type, int polidx, int num_profiles, int profile_iterations, int num_values)
{
    int len = hwloc_get_nbobjs_by_type(tree, type);
    //Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
    for (int i = 0; i < len; i++)
    {
        // Get the object
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        // Allocate the memory for the profiles and profiles' values or take the existing one
        Nodevalues_t v = NULL;
        if (obj->userdata == NULL)
        {
            v = malloc(sizeof(Nodevalues));
            memset(v, 0, sizeof(Nodevalues));
            v->policies = NULL;
        }
        else
        {
            v = (Nodevalues_t)obj->userdata;
        }
        // If the profile array is empty or we add one policy, enlarge the arrays in the node
        if (!v->policies || v->num_policies < polidx)
        {
            if (loop_adapt_debug == 2)
                fprintf(stderr, "DEBUG: Enlarge policy arrays of %s %d to %d\n", loop_adapt_type_name((AdaptScope)obj->type), i, polidx+1);
            v->policies = (PolicyProfile_t*)realloc_buffer(v->policies, MAX(1, polidx+1)*sizeof(PolicyProfile_t));
            
        }
        PolicyProfile_t pp = new_pol_profile();
        pp->num_profiles = num_profiles;
        pp->num_values = num_values;
        pp->profile_iters = profile_iterations;
        // cur_profile == -1 means base_profile, >= 0 is normal profile
        pp->cur_profile = -1;
        pp->opt_profile = -1;
        pp->id = polidx;
        for (int i = 0; i < pp->num_profiles; i++)
        {
            pp->profiles[i] = new_profile();
            pp->profiles[i]->id = i;
        }
        pp->base_profile = new_profile();
        pp->base_profile->id = -1;

        v->policies[polidx] = pp;
        v->num_policies = polidx+1;

        v->cur_policy = 0;

/*        v->num_events = 0;*/
        v->num_pes = get_pes_below_obj(tree, type, i);
        v->count = 0;
/*        v->events = NULL;*/
        // The param hash contains the parameters registered at the current node
        //v->param_hash = g_hash_table_new(g_str_hash, g_str_equal);
        pthread_mutex_init(&v->lock, NULL);
        pthread_cond_init(&v->cond, NULL);

        // save the profile struct in the object
        obj->userdata = (void*)v;
    }
    return 0;
}



void free_hashData(gpointer key, gpointer value, gpointer user_data)
{
    Nodeparameter_t np = (Nodeparameter_t)value;
    if (np->name)
    {
        free(np->name);
    }
    if (np->desc)
    {
        free(np->desc);
    }
    free(np->old_vals);
    memset(np, 0, sizeof(Nodeparameter));
    free(np);
}


static void _free_nodevalues(hwloc_topology_t tree, hwloc_obj_type_t type)
{
    int len = hwloc_get_nbobjs_by_type(tree, type);
    for (int i = 0; i < len; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;

        for (int j = 0; j < v->num_policies; j++)
        {
            PolicyProfile_t pp = v->policies[j];
            for (int k = 0; k < pp->num_profiles; k++)
            {
                free(pp->profiles[k]);
                pp->profiles[k] = NULL;
            }
            free(pp->base_profile);
            pp->base_profile = NULL;
        }
        free(v->policies);
        v->policies = NULL;
        pthread_mutex_destroy(&v->lock);
        pthread_cond_destroy(&v->cond);
        if (v->param_hash)
        {
            if (g_hash_table_size(v->param_hash) > 0)
            {
                g_hash_table_foreach(v->param_hash, free_hashData, NULL);
            }
            //g_hash_table_destroy(v->param_hash);
            v->param_hash = NULL;
        }
        free(v);
        obj->userdata = NULL;
    }
    return;
}

int populate_tree(hwloc_topology_t tree, int polidx, int num_profiles, int profile_iterations)
{
    int max_num_values = LOOP_ADAPT_MAX_POLICY_METRICS;
    Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
    max_num_values = MIN(tdata->policies[polidx]->num_metrics, max_num_values);
    int ret = _allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_THREAD, polidx, num_profiles, profile_iterations, max_num_values);
    if (ret)
        return ret;
    ret = _allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_SOCKET, polidx, num_profiles, profile_iterations, max_num_values);
    if (ret)
        return ret;
    ret = _allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_NUMA, polidx, num_profiles, profile_iterations, max_num_values);
    if (ret)
        return ret;
    ret = _allocate_nodevalues(tree, LOOP_ADAPT_SCOPE_MACHINE, polidx, num_profiles, profile_iterations, max_num_values);
    if (ret)
        return ret;
    return 0;
}

void tidy_tree(hwloc_topology_t tree)
{
    _free_nodevalues(tree, LOOP_ADAPT_SCOPE_THREAD);
    _free_nodevalues(tree, LOOP_ADAPT_SCOPE_SOCKET);
    _free_nodevalues(tree, LOOP_ADAPT_SCOPE_NUMA);
    _free_nodevalues(tree, LOOP_ADAPT_SCOPE_MACHINE);
}

AdaptScope getLoopAdaptType(hwloc_obj_type_t t)
{
    return (AdaptScope)t;
}

void update_tree(hwloc_obj_t obj, int profile)
{
    if (!obj)
    {
        return;
    }
    Nodevalues_t orig = (Nodevalues_t)obj->userdata;
    PolicyProfile_t pp_orig = orig->policies[orig->cur_policy];
    hwloc_obj_t walker = obj->parent;

    while (walker)
    {
        AdaptScope t = getLoopAdaptType(walker->type);
        if (t == LOOP_ADAPT_SCOPE_MACHINE ||
            t == LOOP_ADAPT_SCOPE_NUMA ||
            t == LOOP_ADAPT_SCOPE_SOCKET)
        {
            Nodevalues_t dest = (Nodevalues_t)walker->userdata;
            dest->cur_policy = orig->cur_policy;
            PolicyProfile_t pp_dest = dest->policies[dest->cur_policy];
            if (loop_adapt_debug > 1)
            {
                printf("Combining %s %d into %s %d\n", loop_adapt_type_name(obj->type), obj->os_index, loop_adapt_type_name(walker->type), walker->os_index);
            }
            pthread_mutex_lock(&dest->lock);
            combine_pol_profiles(pp_orig, pp_dest, profile);
            pthread_mutex_unlock(&dest->lock);
        }
        walker = walker->parent;
    }
}

static int _update_cur_policy_in_tree_types(hwloc_topology_t tree, hwloc_obj_type_t type, int polidx)
{
    hwloc_obj_t obj = NULL;
    int len = hwloc_get_nbobjs_by_type(tree, type);
    for (int i = 0; i < len; i++)
    {
        obj = hwloc_get_obj_by_type(tree, type, i);
        Nodevalues_t nv = (Nodevalues_t)obj->userdata;
        if (nv)
        {
            //nv->cur_profile = -1;
            //printf("Setting cur_profile to 0 for %s %d\n", loop_adapt_type_name(type), obj->os_index);
            nv->cur_policy = polidx;
            nv->done = 0;
        }
    }
}

void update_cur_policy_in_tree(hwloc_topology_t tree, int polidx)
{
    _update_cur_policy_in_tree_types(tree, HWLOC_OBJ_PU, polidx);
    _update_cur_policy_in_tree_types(tree, HWLOC_OBJ_PACKAGE, polidx);
    _update_cur_policy_in_tree_types(tree, HWLOC_OBJ_NUMANODE, polidx);
    _update_cur_policy_in_tree_types(tree, HWLOC_OBJ_MACHINE, polidx);
}
















