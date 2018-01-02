#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <dlfcn.h>
#include <sched.h>

#include <hwloc.h>
#include <ghash.h>
#include <likwid.h>

#include <loop_adapt.h>
#include <loop_adapt_eval.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>
#include <loop_adapt_config.h>

//int loop_adapt_active = 1;

static hwloc_topology_t topotree = NULL;
static int num_cpus = 0;
static int *cpus_to_objidx = NULL;
static int *cpulist = NULL;

GHashTable* loop_adapt_global_hash = NULL;
pthread_mutex_t loop_adapt_global_hash_lock = PTHREAD_MUTEX_INITIALIZER;

static int max_num_values = 0;

int (*tcount_func)() = NULL;
int (*tid_func)() = NULL;

cpu_set_t loop_adapt_cpuset;

static pthread_mutex_t loop_adapt_lock = PTHREAD_MUTEX_INITIALIZER;



__attribute__((constructor)) void loop_adapt_init (void)
{
    loop_adapt_global_hash = g_hash_table_new(g_str_hash, g_str_equal);
    if (!loop_adapt_global_hash)
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
    CPU_ZERO(&loop_adapt_cpuset);
    ret = sched_getaffinity(0, sizeof(loop_adapt_cpuset), &loop_adapt_cpuset);
    timer_init();
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

void loop_adapt_register_tcount_func(int (*handle)())
{
    tcount_func = handle;
}
void loop_adapt_register_tid_func(int (*handle)())
{
    tid_func = handle;
}

void loop_adapt_get_tcount_func(int (**handle)())
{
    *handle = tcount_func;
}
void loop_adapt_get_tid_func(int (**handle)())
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
//        pthread_mutex_lock(&loop_adapt_global_hash_lock);
#pragma omp critical (lookup)
{
        dup_tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
}
//        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
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
        tdata->cur_policy_id = -1;
        // Safe the global data struct in the tree
        hwloc_topology_set_userdata(dup_tree, tdata);

        // insert the loop-specific tree into the hash table using the string as key
        pthread_mutex_lock(&loop_adapt_global_hash_lock);
#pragma omp critical (lookup)
{
        g_hash_table_insert(loop_adapt_global_hash, (gpointer) g_strdup(string), (gpointer) dup_tree);
}
        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
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

void loop_adapt_register_policy( char* string, char* polname, int num_profiles, int iters_per_profile)
{
    hwloc_topology_t tree = NULL;
    Policy_t p = loop_adapt_read_policy(polname);
    if (p)
    {
//        pthread_mutex_lock(&loop_adapt_global_hash_lock);
#pragma omp critical (lookup)
{
        tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
}
//        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
        if (tree)
        {
            printf("Add policy %s to tree of loop region %s with %d profiles and %d iterations per profile\n", p->name, string, num_profiles, iters_per_profile);
            // We register num_profiles+1 because one profile is used as base for the further evaluation
            loop_adapt_add_policy(tree, p, num_cpus, cpulist, num_profiles+1, iters_per_profile);
        }
    }
}

int loop_adapt_for_type(AdaptScope scope)
{
    return hwloc_get_nbobjs_by_type(topotree, scope);
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
    hwloc_topology_t tree = NULL;

    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (tree && cpu >= 0 && cpu < num_cpus)
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
                //if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Parameter %s: %d/%d/%d\n", name, min, cur, max);
                loop_adapt_add_int_parameter(obj, name, desc, cur, min, max, NULL, NULL);
            }
        }
    }
}



int loop_adapt_get_int_param( char* string, AdaptScope scope, int cpu, char* name)
{
    int ret = 0;
    int objidx;
    TimerData t;
    hwloc_topology_t tree = NULL;

    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

    //int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (tree && cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
        {
            objidx = cpus_to_objidx[cpu];
            //if (loop_adapt_debug)
                //fprintf(stderr, "DEBUG: Get param %s for CPU %d (Idx %d):",name, cpu, objidx);
        }
        else
        {
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
            //if (loop_adapt_debug)
                //fprintf(stderr, "DEBUG: Get param %s for %s above CPU %d (Idx %d):",name, loop_adapt_type_name(scope), cpu, objidx);
        }
        

        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        if (v)
        {
            Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer) name);
            if (np)
            {
                if (np->best.ibest < 0 || v->cur_profile < v->num_profiles[v->cur_policy])
                {
                    //if (loop_adapt_debug)
                        //fprintf(stderr, " %d\n", np->cur.icur);
                    ret = np->cur.icur;
                }
                else
                {
                    //if (loop_adapt_debug)
                        //fprintf(stderr, " %d\n", np->best.ibest);
                    ret = np->best.ibest;
                }
            }
            if (loop_adapt_debug)
                fprintf(stderr, "DEBUG: Get param %s for %s %d: %d\n",name, loop_adapt_type_name(scope), objidx, ret);
        }
    }
    return ret;
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
    hwloc_topology_t tree = NULL;

    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

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
    hwloc_topology_t tree = NULL;

    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

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

void loop_adapt_register_event(char* string, AdaptScope scope, int cpu, char* name, char* var, Nodeparametertype type, void* ptr)
{
    int objidx;
    hwloc_topology_t tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, type);
    if (cpu >= 0 && cpu < num_cpus)
    {
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        if (objidx >= 0)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
            if (obj)
            {
                printf("Register event %s\n", name);
                loop_adapt_add_event(obj, name, var, type, ptr);
            }
        }
    }
}

int loop_adapt_begin(char* string, char* filename, int linenumber)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;
        hwloc_obj_t obj = NULL, new = NULL;
        Nodevalues_t v = NULL, newv = NULL;
        tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
        if (tree)
        {
            Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
            if (tdata->status == LOOP_STOPPED)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Starting loop %s\n", string);
                int cpuid = 0;
                int objidx = 0;
                if (tid_func)
                    cpuid = tid_func();
                else
                    cpuid = sched_getcpu();

                // This is the case when we are already in a parallel region
                if (tcount_func && tcount_func() > 1)
                {
                    objidx = cpus_to_objidx[cpuid];
                    obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                    v = (Nodevalues_t)obj->userdata;
                    //printf("Begin Cur iters %d eval iters %d\n", v->cur_profile_iters[v->cur_policy], v->profile_iters[v->cur_policy]);
                    if (v != NULL &&
                        v->cur_profile_iters[v->cur_policy] == 0 &&
                        v->cur_profile < v->num_profiles[v->cur_policy])
                    {
                        if (v->cur_profile == 0)
                        {
                            int ret = 0;
                            if (!tdata->filename)
                                ret = asprintf(&tdata->filename, "%s", filename);
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
                // This is the case when we are in a serial region but threading is generally activated
                else if (tcount_func && tcount_func() == 1)
                {
                    for (int c = 0; c < num_cpus; c++)
                    {
                        if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
                        {
                            objidx = cpus_to_objidx[cpulist[c]];
                            new = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);

                            newv = (Nodevalues_t)new->userdata;
                            //printf("Begin CPU %d Cur iters %d eval iters %d\n", cpulist[c], v->cur_profile_iters[v->cur_policy], v->profile_iters[v->cur_policy]);
                            if (newv != NULL &&
                                newv->cur_profile_iters[newv->cur_policy] == 0 && 
                                newv->cur_profile < newv->num_profiles[newv->cur_policy])
                            {
                                //printf("LOOP START CPU %d/%d PROFILE %d/%d\n", cpuid, objidx, v->cur_profile, tdata->num_profiles);
                                if (newv->cur_profile == 0)
                                {
                                    int ret = 0;
                                    if (!tdata->filename)
                                        ret = asprintf(&tdata->filename, "%s", filename);
                                    ret++;
                                    tdata->linenumber = linenumber;
                                }
                                

                                if (newv->cur_profile > 1)
                                {
                                    loop_adapt_exec_policies(tree, new);
                                }
                                
                                //v->cur_profile_iters[v->cur_policy] = 0;
                            }
                        }
                    }
                    for (int c = 0; c < num_cpus; c++)
                    {
                        if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
                        {
                            objidx = cpus_to_objidx[cpulist[c]];
                            new = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                            newv = (Nodevalues_t)new->userdata;
                            if (newv != NULL &&
                                newv->cur_profile_iters[newv->cur_policy] == 0 && 
                                newv->cur_profile < newv->num_profiles[newv->cur_policy])
                            {
                                loop_adapt_begin_policies(cpulist[c], tree, new);
                            }
                        }
                    }
                }
                tdata->status = LOOP_STARTED;
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
        hwloc_topology_t tree = NULL;
        hwloc_obj_t obj = NULL, new = NULL;
        Nodevalues_t v = NULL, newv = NULL;
        tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
        if (tree)
        {
            Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
            if (tdata->status == LOOP_STARTED)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Ending loop %s\n", string);
                int cpuid = 0;
                int objidx = 0;
                if (tid_func)
                    cpuid = tid_func();
                else
                    cpuid = sched_getcpu();
                // This is the case when we are already in a parallel region
                if (tcount_func && tcount_func() > 1)
                {
                    objidx = cpus_to_objidx[cpuid];

                    obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                    v = (Nodevalues_t)obj->userdata;
                    //printf("End Cur iters %d eval iters %d\n", v->cur_profile_iters[v->cur_policy], v->profile_iters[v->cur_policy]);
                    if (v != NULL &&
                        v->cur_profile_iters[v->cur_policy] == v->profile_iters[v->cur_policy]-1 &&
                        v->cur_profile < v->num_profiles[v->cur_policy])
                    {
                        //printf("Call single loop_adapt_end_policies %d\n", cpuid);
                        loop_adapt_end_policies(cpuid, tree, obj);
                        if (v->cur_profile > 0)
                        {
                            update_tree(obj, v->cur_profile-1);
                        }
                        v->cur_profile++;
                        v->cur_profile_iters[v->cur_policy] = 0;
                    }
                    else if (v != NULL)
                    {
                        v->cur_profile_iters[v->cur_policy]++;
                    }
                }
                // This is the case when we are in a serial region but threading is generally activated
                else if (tcount_func && tcount_func() == 1)
                {
                    objidx = cpus_to_objidx[cpuid];
                    obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                    v = (Nodevalues_t)obj->userdata;
                    for (int c = 0; c < num_cpus; c++)
                    {
                        if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
                        {
                            objidx = cpus_to_objidx[cpulist[c]];
        
                            new = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                            newv = (Nodevalues_t)new->userdata;
                            //printf("End CPU %d Cur iters %d eval iters %d\n", cpulist[c], v->cur_profile_iters[v->cur_policy], v->profile_iters[v->cur_policy]);
                            if (newv != NULL &&
                                newv->cur_profile_iters[newv->cur_policy] == newv->profile_iters[newv->cur_policy]-1 &&
                                newv->cur_profile < newv->num_profiles[newv->cur_policy])
                            {
                                //printf("Call multi loop_adapt_end_policies %d\n", cpulist[c]);
                                loop_adapt_end_policies(cpulist[c], tree, new);
                                if (newv->cur_profile > 0)
                                {
                                    update_tree(new, newv->cur_profile-1);
                                }
                                newv->cur_profile++;
                                newv->cur_profile_iters[newv->cur_policy] = 0;
                            }
                            else if (newv != NULL)
                            {
                                newv->cur_profile_iters[newv->cur_policy]++;
                            }
                            if (newv->cur_profile == newv->num_profiles[newv->cur_policy])
                            {
                                loop_adapt_exec_policies(tree, new);
                            }
                        }
                    }
                }
                if (v->cur_profile == v->num_profiles[v->cur_policy])
                {
                    printf("Set optimal parameters\n");
#pragma omp barrier
                    pthread_mutex_lock(&loop_adapt_lock);
                    if (tdata->cur_policy)
                    {
                        if (v->opt_profiles[v->cur_policy] == 0)
                        {
                            printf("Set start value as best value in all nodes\n");
                            loop_adapt_reset_parameter(tree, tdata->cur_policy);
                        }
                        else
                        {
                            loop_adapt_best_parameter(tree, tdata->cur_policy);
                        }
                        tdata->cur_policy->done = 1;
                        tdata->cur_policy->loop_adapt_eval_close();
                        if (tdata->num_policies > 1)
                        {
                            int idx = -1;
                            for (int i=0; i < tdata->num_policies; i++)
                            {
                                
                                if (!tdata->policies[i]->done)
                                {
                                    idx = i;
                                    break;
                                }
                            }
                            if (idx >= 0)
                            {
                                printf("Switch to policy %s (%d)\n", tdata->policies[idx]->name, idx);
                                tdata->cur_policy = tdata->policies[idx];
                                tdata->cur_policy_id = idx;
                                update_cur_policy_in_tree(tree, idx);
                            }
                            else
                            {
                            tdata->cur_policy = NULL;
                            tdata->cur_policy_id = -1;
                            }
                         }
                         else
                         {
                            tdata->cur_policy = NULL;
                            tdata->cur_policy_id = -1;
                         }
                    }
                    pthread_mutex_unlock(&loop_adapt_lock);
                    if (!tdata->cur_policy && tdata->cur_policy_id < 0)
                    {
                        printf("Disable loop_adapt\n");
                        for (int c = 0; c < num_cpus; c++)
                        {
                            if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
                            {
                                objidx = cpus_to_objidx[cpulist[c]];
            
                                new = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                                newv = (Nodevalues_t)new->userdata;
                                update_tree(new, newv->cur_profile-1);
                             }
                        }
                        loop_adapt_write_profiles(string, tree);
                        loop_adapt_write_parameters(string, tree);
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

    pthread_mutex_lock(&loop_adapt_global_hash_lock);
    if (loop_adapt_global_hash)
    {
        
        g_hash_table_iter_init(&iter, loop_adapt_global_hash);
        while(g_hash_table_iter_next(&iter, &gkey, &gvalue))
        {
            key = (char*)gkey;
            tree = (hwloc_topology_t)gvalue;

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
        g_hash_table_destroy(loop_adapt_global_hash);
        loop_adapt_global_hash = NULL;
        
    }
    pthread_mutex_unlock(&loop_adapt_global_hash_lock);
}

