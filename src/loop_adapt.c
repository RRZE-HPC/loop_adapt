#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <dlfcn.h>

#include <hwloc.h>
#include <ghash.h>
#include <likwid.h>

#include <loop_adapt.h>
#include <loop_adapt_eval.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>


static int loop_adapt_active = 1;

static GHashTable* global_hash = NULL;
static hwloc_topology_t topotree = NULL;
static int num_cpus = 0;
static int *cpus_to_objidx = NULL;
static int *cpulist = NULL;

/*static int default_num_profiles = 5;*/
/*static int runtime_num_profiles = 0;*/

/*static int default_max_num_values = 30;*/
static int max_num_values = 0;

unsigned int (*tcount_func)() = NULL;
unsigned int (*tid_func)() = NULL;

//static char* likwid_group = NULL;
//static int likwid_gid = -1;
//static int likwid_num_metrics = 0;
//#define MIN(x,y) ((x)<(y)?(x):(y))


__attribute__((constructor)) void loop_adapt_init (void)
{
    global_hash = g_hash_table_new(g_str_hash, g_str_equal);
    if (!global_hash)
    {
        loop_adapt_active = 0;
        return;
    }

    int ret = hwloc_topology_init(&topotree);
    if (ret < 0)
    {
        loop_adapt_active = 0;
        return;
    }
    hwloc_topology_load(topotree);

    num_cpus = hwloc_get_nbobjs_by_type(topotree, HWLOC_OBJ_PU);
    cpus_to_objidx = malloc(num_cpus * sizeof(int));
    cpulist = malloc(num_cpus * sizeof(int));
    for (int i = 0; i < num_cpus; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(topotree, HWLOC_OBJ_PU, i);
        cpus_to_objidx[obj->os_index] = i;
        cpulist[i] = obj->os_index;
    }
    setenv("LIKWID_FORCE", "1", 1);

/*    if (getenv("LOOP_ADAPT_NUM_PROFILES") == NULL)*/
/*    {*/
/*        runtime_num_profiles = default_num_profiles;*/
/*    }*/
/*    else*/
/*    {*/
/*        runtime_num_profiles = atoi(getenv("LOOP_ADAPT_NUM_PROFILES"));*/
/*    }*/
/*    if (getenv("LOOP_ADAPT_MAX_VALUES_PER_PROFILE") == NULL)*/
/*    {*/
/*        max_num_values = default_max_num_values;*/
/*    }*/
/*    else*/
/*    {*/
/*        max_num_values = atoi(getenv("LOOP_ADAPT_MAX_VALUES_PER_PROFILE"));*/
/*    }*/
}

void loop_adapt_register_tcount_func(unsigned int (*handle)())
{
    tcount_func = handle;
}
void loop_adapt_register_tid_func(unsigned int (*handle)())
{
    tid_func = handle;
}

void loop_adapt_get_tcount_func(unsigned int (**handle)())
{
    *handle = tcount_func;
}
void loop_adapt_get_tid_func(unsigned int (**handle)())
{
    *handle = tid_func;
}


void loop_adapt_register(char* string)
{
    int err = 0;
    Treedata *tdata = NULL;
    hwloc_topology_t dup_tree = NULL;
    // Check whether the string is already present in the hash table
    // If yes, warn and return
    if (loop_adapt_active)
    {
        dup_tree = g_hash_table_lookup(global_hash, (gpointer) string);
        if (dup_tree)
        {
            fprintf(stderr, "Loop string %s already registered\n", string);
            return;
        }

        // Duplicate the topology tree, each loop gets its own tree
        //hwloc_topology_init(&dup_tree);
        err = hwloc_topology_dup(&dup_tree, topotree);
        if (err)
        {
            fprintf(stderr, "Cannot duplicate topology tree %s\n", string);
            free(tdata);
            return;
        }

        // Allocate the global data struct describing a loop
        tdata = malloc(sizeof(Treedata));
        if (!tdata)
        {
            fprintf(stderr, "Cannot allocate loop data for string %s\n", string);
            return;
        }
        memset(tdata, 0, sizeof(Treedata));
        tdata->status = LOOP_STOPPED;
        // Safe the global data struct in the tree
        hwloc_topology_set_userdata(dup_tree, tdata);

        // Moreover, each HW thread, package, NUMA node and the machine object
        // get a values struct
        // The num_profiles in Treedata must be set when calling these functions
        //allocate_nodevalues(dup_tree, LOOP_ADAPT_SCOPE_THREAD, max_num_values);
        //allocate_nodevalues(dup_tree, LOOP_ADAPT_SCOPE_SOCKET, max_num_values);
        //allocate_nodevalues(dup_tree, LOOP_ADAPT_SCOPE_NUMA, max_num_values);
        //allocate_nodevalues(dup_tree, LOOP_ADAPT_SCOPE_MACHINE, max_num_values);

        // insert the loop-specific tree into the hash table using the string as key
        g_hash_table_insert(global_hash, (gpointer) g_strdup(string), (gpointer) dup_tree);
    }
}

static Policy_t loop_adapt_read_policy( char* polname)
{
    char *error = NULL;
    void* handle = dlopen(NULL, RTLD_LAZY);
    if (!handle) {
       fprintf(stderr, "%s\n", dlerror());
       return NULL;
    }
    dlerror();
    Policy_t p = dlsym(handle, polname);
    error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Cannot find policy %s\n", polname);
        return NULL;
    }
    dlclose(handle);
    return p;
}

void loop_adapt_register_policy( char* string, char* polname, int num_profiles)
{
    Policy_t p = loop_adapt_read_policy(polname);
    if (p)
    {
        hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
        if (tree)
        {
            printf("Add policy %s to tree of loop region %s with %d profiles\n", p->name, string, num_profiles);
            loop_adapt_add_policy(tree, p, num_cpus, cpulist, num_profiles);
        }
    }
}



void loop_adapt_register_int_param( char* string,
                                    AdaptScope scope,
                                    int cpu,
                                    char* name,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max)
{
    int objidx;
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        if (objidx >= 0 && objidx < len)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
            if (obj)
            {
                //printf("Adding parameter %s to object %d of type %s Min %d Max %d\n", name, objidx, hwloc_obj_type_string(scope), min, max);
                if (min == 0)
                {
                    min = (int)get_param_def_min(tree, name);
                    if (loop_adapt_debug)
                        fprintf(stderr, "DEBUG: Calculating minimum for %s: %d\n", name, min);
                }
                if (max == 0)
                {
                    max = (int)get_param_def_max(tree, name);
                    if (loop_adapt_debug)
                        fprintf(stderr, "DEBUG: Calculating maximum for %s: %d\n", name, max);
                }
                loop_adapt_add_int_parameter(obj, name, desc, cur, min, max, NULL, NULL);
            }
        }
    }
}



int loop_adapt_get_int_param( char* string, AdaptScope scope, int cpu, char* name)
{
    int objidx;
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
        {
            objidx = cpus_to_objidx[cpu];
            if (loop_adapt_debug)
                fprintf(stderr, "DEBUG: Get param %s for CPU %d (Idx %d)\n",name, cpu, objidx);
        }
        else
        {
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
            if (loop_adapt_debug)
                fprintf(stderr, "DEBUG: Get param %s for %s above CPU %d (Idx %d)\n",name, loop_adapt_type_name(scope), cpu, objidx);
        }
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        if (v)
        {
            Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer) name);
            if (np)
            {
                if (np->best.ibest < 0 || v->cur_profile < v->num_profiles[v->cur_policy])
                    return np->cur.icur;
                else
                    return np->best.ibest;
            }
        }
    }
    return 0;
}

void loop_adapt_register_double_param( char* string,
                                       AdaptScope scope,
                                       int cpu,
                                       char* name,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max)
{
    int objidx;
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        if (objidx >= 0 && objidx < len)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
            if (obj)
            {
                if (min == 0)
                {
                    min = get_param_def_min(tree, name);
                    if (loop_adapt_debug)
                        fprintf(stderr, "DEBUG: Calculating minimum for %s: %f\n", name, min);
                }
                if (max == 0)
                {
                    max = get_param_def_max(tree, name);
                    if (loop_adapt_debug)
                        fprintf(stderr, "DEBUG: Calculating maximum for %s: %f\n", name, max);
                }
                //printf("Add parameter %s with min %f and max %f\n", name, min, max);
                loop_adapt_add_double_parameter(obj, name, desc, cur, min, max, NULL, NULL);
            }
        }
    }
}

int loop_adapt_get_double_param_param( char* string, AdaptScope scope, int cpu, char* name)
{
    int objidx;
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        if (v)
        {
            Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer) name);
            if (np)
            {
                if (np->best.dbest < 0 || v->cur_profile < v->num_profiles[v->cur_policy])
                    return np->cur.dcur;
                else
                    return np->best.dbest;
            }
        }
    }
    return 0;
}

/*void loop_adapt_register_event(char* string, AdaptScope scope, int cpu, char* name, char* var, Nodeparametertype type, void* ptr)*/
/*{*/
/*    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);*/
/*    int len = hwloc_get_nbobjs_by_type(tree, type);*/
/*    if (objidx >= 0)*/
/*    {*/
/*        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);*/
/*        if (obj)*/
/*        {*/
/*            printf("Register event %s\n", name);*/
/*            loop_adapt_add_event(obj, name, var, ptr);*/
/*        }*/
/*    }*/
/*}*/

int loop_adapt_begin(char* string, char* filename, int linenumber)
{

    if (loop_adapt_active)
    {
        hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
        if (tree)
        {
            Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
            if (tdata->status == LOOP_STOPPED)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Starting loop %s\n", string);
                int cpuid = 0;
                if (tid_func)
                    cpuid = tid_func();
#ifdef _OPENMP
                else
                    cpuid = omp_get_thread_num();
#endif
                //likwid_pinThread(cpuid);
                if (tcount_func && tcount_func() > 1)
                {
                    int objidx = cpus_to_objidx[cpuid];
                    Nodevalues_t v = NULL;
                    hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);

                    v = (Nodevalues_t)obj->userdata;
                    if (v->cur_profile < v->num_profiles[v->cur_policy])
                    {
                        //printf("LOOP START CPU %d/%d PROFILE %d/%d\n", cpuid, objidx, v->cur_profile, tdata->num_profiles);
                        if (v->cur_profile == 0)
                        {
                            int ret = asprintf(&tdata->filename, "%s", filename);
                            ret++;
                            tdata->linenumber = linenumber;
                        }
                        

                        if (v->cur_profile > 1)
                        {
                            loop_adapt_exec_policies(tree, obj);
                        }
                        loop_adapt_begin_policies(cpuid, tree, obj); 
                    }
                    tdata->status = LOOP_STARTED;
                }
                else if (tcount_func && tcount_func() == 1)
                {
                    for (int c = 0; c < num_cpus; c++)
                    {
                        int objidx = cpus_to_objidx[cpulist[c]];
                        Nodevalues_t v = NULL;
                        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);

                        v = (Nodevalues_t)obj->userdata;
                        if (v->cur_profile < v->num_profiles[v->cur_policy])
                        {
                            //printf("LOOP START CPU %d/%d PROFILE %d/%d\n", cpuid, objidx, v->cur_profile, tdata->num_profiles);
                            if (v->cur_profile == 0)
                            {
                                int ret = asprintf(&tdata->filename, "%s", filename);
                                ret++;
                                tdata->linenumber = linenumber;
                            }
                            

                            if (v->cur_profile > 1)
                            {
                                loop_adapt_exec_policies(tree, obj);
                            }
                            loop_adapt_begin_policies(cpuid, tree, obj); 
                        }
                    }
                    tdata->status = LOOP_STARTED;
                }
            }
        }
        else
        {
            fprintf(stderr, "Loop with string %s unknown. Needs to be registered first\n", string);
        }
    }
    return 1;
}



int loop_adapt_end(char* string)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
        if (tree)
        {
            Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
            if (tdata->status == LOOP_STARTED)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Ending loop %s\n", string);
                int cpuid = 0;
                if (tid_func)
                    cpuid = tid_func();
#ifdef _OPENMP
                else
                    cpuid = omp_get_thread_num();
#endif

                int objidx = cpus_to_objidx[cpuid];

                hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                Nodevalues *v = (Nodevalues *)obj->userdata;
                if (v != NULL && v->cur_profile < v->num_profiles[v->cur_policy])
                {
                    loop_adapt_end_policies(cpuid, tree, obj);
                    if (v->cur_profile > 0)
                        update_tree(obj, v->cur_profile-1);
                    v->cur_profile++;
                }

                if (tcount_func && tcount_func() == 1)
                {
                    for (int c = 0; c < num_cpus; c++)
                    {
                        objidx = cpus_to_objidx[cpulist[c]];

                        hwloc_obj_t new = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                        if (obj == new)
                            continue;
                        Nodevalues *newv = (Nodevalues *)new->userdata;
                        if (newv != NULL && newv->cur_profile < newv->num_profiles[newv->cur_policy])
                        {
                            loop_adapt_end_policies(cpuid, tree, new);
                            if (newv->cur_profile > 0)
                                update_tree(new, newv->cur_profile-1);
                            newv->cur_profile++;
                        }
                    }
                }
                if (v->cur_profile == v->num_profiles[v->cur_policy])
                {
                    printf("Set optimal parameters\n");
                    tdata->cur_policy->done = 1;
                    tdata->cur_policy->loop_adapt_eval_close();
                    tdata->cur_policy = NULL;
                    for (int i=0; i < tdata->num_policies; i++)
                    {
                        if (!tdata->policies[i]->done)
                        {
                            printf("Switch to policy %s\n", tdata->policies[i]->name);
                            tdata->cur_policy = tdata->policies[i];
                            tdata->cur_policy_id = i;
                            update_cur_policy_in_tree(tree, tdata->cur_policy_id);
                        }
                    }
                    if (!tdata->cur_policy)
                    {
                        printf("Disable loop_adapt\n");
                        loop_adapt_active = 0;
                    }
                }
                tdata->status = LOOP_STOPPED;
            }
        }
        else
        {
            fprintf(stderr, "Loop with string %s unknown. Needs to be registered first\n", string);
        }
    }
    return 1;
}





__attribute__((destructor)) void loop_adapt_finalize (void)
{
    GHashTableIter iter;
    gpointer gkey;
    gpointer gvalue;
    char* key;
    hwloc_topology_t tree;
    g_hash_table_iter_init(&iter, global_hash);
    while(g_hash_table_iter_next(&iter, &gkey, &gvalue))
    {
        key = (char*)gkey;
        tree = (hwloc_topology_t)gvalue;
/*        free_nodevalues(tree, HWLOC_OBJ_PU);*/
/*        free_nodevalues(tree, HWLOC_OBJ_PACKAGE);*/
/*        free_nodevalues(tree, HWLOC_OBJ_NUMANODE);*/
/*        free_nodevalues(tree, HWLOC_OBJ_MACHINE);*/

        tidy_tree(tree);

        Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
        free(tdata->filename);
        if (tdata->num_policies > 0)
        {
            free(tdata->policies);
            tdata->num_policies = 0;
            tdata->cur_policy = NULL;
        }
        free(tdata);
        hwloc_topology_set_userdata(tree, NULL);
        hwloc_topology_destroy(tree);
    }
    g_hash_table_destroy(global_hash);
}

