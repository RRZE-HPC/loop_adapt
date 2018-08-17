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
#include <loop_adapt_policy.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>
#include <loop_adapt_config.h>
#include <loop_adapt_search.h>

/*! \brief  This is the base hwloc topology tree used later to create copies for each registered loop */
static hwloc_topology_t topotree = NULL;
/*! \brief  Number of CPUs in the system */
static int num_cpus = 0;
/*! \brief  Number of CPUs in the taskset for the application */
static int num_active_cpus = 0;
/*! \brief  Loopup table from CPU id to hwloc PU (=HW thread) index */
static int *cpus_to_objidx = NULL;
/*! \brief  Loopup table for hwloc PU (=HW thread) index to CPU id */
static int *cpulist = NULL;

/*! \brief  This is the global hash table, the main entry point to the loop_adapt data */
GHashTable* loop_adapt_global_hash = NULL;
/*! \brief  This is the lock for the global hash table */
pthread_mutex_t loop_adapt_global_hash_lock = PTHREAD_MUTEX_INITIALIZER;

static int max_num_values = 0;

/*! \brief Function pointer for a function returning the maximum number of threads */
int (*_tcount_func)() = NULL;
/*! \brief Function pointer for a function returning the current thread ID */
int (*_tid_func)() = NULL;

/*! \brief  Taskset of the application */
cpu_set_t loop_adapt_cpuset;
/*! \brief  Taskset of the master thread of the application */
cpu_set_t loop_adapt_cpuset_master;

/*! \brief Basic lock securing the loop_adapt data structures */
static pthread_mutex_t loop_adapt_lock = PTHREAD_MUTEX_INITIALIZER;


/*! \brief  The constructor loop_adapt_init()

It initializes some basic data structures like the hwloc topology tree used as base for later copies, the amount of CPUs in the taskset, and some loopup tables.
*/
__attribute__((constructor)) void loop_adapt_init (void)
{
    char spid[30];
    // Create new global hash table
    loop_adapt_global_hash = g_hash_table_new(g_str_hash, g_str_equal);
    if (!loop_adapt_global_hash)
    {
        loop_adapt_active = 0;
        return;
    }

    // Initialize hwloc topology tree
    int ret = hwloc_topology_init(&topotree);
    if (ret < 0)
    {
        loop_adapt_active = 0;
        return;
    }
    hwloc_topology_load(topotree);

    // Get number of CPUs in the system
    num_cpus = hwloc_get_nbobjs_by_type(topotree, HWLOC_OBJ_PU);
    // Allocate and fill lookup tables
    cpus_to_objidx = malloc(num_cpus * sizeof(int));
    cpulist = malloc(num_cpus * sizeof(int));
    for (int i = 0; i < num_cpus; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(topotree, HWLOC_OBJ_PU, i);
        cpus_to_objidx[obj->os_index] = i;
        cpulist[i] = obj->os_index;
    }

    // Set LIKWID_FORCE to avoid complications at first loop iteration
    setenv("LIKWID_FORCE", "1", 1);
    ret = snprintf(spid, 29, "%d", getpid());
    spid[ret] = '\0';
    ret = 0;
    // Set LIKWID_PERF_PID if LIKWID was compiled with perf_event backend
    // to work with perf_event_paranoid == 1
    setenv("LIKWID_PERF_PID", spid, 1);

    // Get the taskset for the application
    CPU_ZERO(&loop_adapt_cpuset);
    ret = sched_getaffinity(0, sizeof(cpu_set_t), &loop_adapt_cpuset);
    num_active_cpus = CPU_COUNT(&loop_adapt_cpuset);

    // Initialize timer
    timer_init();
}

int _loop_adapt_get_cpucount()
{
    return num_active_cpus;
}

/* Register the function to retrieve the number of threads/tasks/... like
 * omp_get_num_threads() or similar
 */
void loop_adapt_register_cpucount_func(int (*handle)())
{
    if (handle)
    {
        _tcount_func = handle;
    }
}

/* Register the function to retrieve the identifier of a thread/task/... like
 * omp_get_thread_num() or similar.
 */
void loop_adapt_register_getcpu_func(int (*handle)())
{
    if (handle)
    {
        _tid_func = handle;
    }
}

/* Get the function pointer to retrieve the number of threads/tasks */
void loop_adapt_get_cpucount_func(int (**handle)())
{
    *handle = NULL;
    int val = 0;
    if (_tcount_func && _tcount_func() <= _loop_adapt_get_cpucount())
    {
        *handle = _tcount_func;
    }
    else
    {
        *handle = _loop_adapt_get_cpucount;
    }
    return;
}
/* Get the function pointer to retrieve the identifier of a thread/task */
void loop_adapt_get_getcpu_func(int (**handle)())
{
    *handle = NULL;
    if (_tid_func)
    {
        *handle = _tid_func;
    }
}

/* Register a loop in loop_adapt */
void loop_adapt_register(char* string)
{
    int err = 0;
    Treedata *tdata = NULL;
    hwloc_topology_t dup_tree = NULL;
    // Check whether the string is already present in the hash table
    // If yes, warn and return
    if (loop_adapt_active)
    {
        pthread_mutex_lock(&loop_adapt_global_hash_lock);
        dup_tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
        if (dup_tree)
        {
            fprintf(stderr, "ERROR: Loop string %s already registered\n", string);
            return;
        }

        // Duplicate the topology tree, each loop gets its own tree
        //hwloc_topology_init(&dup_tree);
        err = hwloc_topology_dup(&dup_tree, topotree);
        if (err)
        {
            fprintf(stderr, "ERROR: Cannot duplicate topology tree %s\n", string);
            free(tdata);
            return;
        }

        // Allocate the global data struct describing a loop
        tdata = malloc(sizeof(Treedata));
        if (!tdata)
        {
            fprintf(stderr, "ERROR: Cannot allocate loop data for string %s\n", string);
            return;
        }
        /* Initialize data struct representing a single loop */
        memset(tdata, 0, sizeof(Treedata));
        tdata->status = LOOP_STOPPED;
        tdata->cur_policy_id = -1;
        tdata->cur_policy = NULL;
        tdata->filename = NULL;
        tdata->policies = NULL;
        tdata->linenumber = -1;
        tdata->num_policies = 0;
        tdata->threads = g_hash_table_new(g_direct_hash, g_direct_equal);
        pthread_mutex_init(&tdata->lock, NULL);

        // Safe the global data struct in the tree
        hwloc_topology_set_userdata(dup_tree, tdata);

        // insert the loop-specific tree into the hash table using the string as key
        pthread_mutex_lock(&loop_adapt_global_hash_lock);
        g_hash_table_insert(loop_adapt_global_hash, (gpointer) g_strdup(string), (gpointer) dup_tree);
        pthread_mutex_unlock(&loop_adapt_global_hash_lock);
    }
}

static Policy_t loop_adapt_read_policy( char* polname)
{
    Policy_t out = NULL;
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
        
        return NULL;
    }
    dlclose(handle);
    out = malloc(sizeof(Policy));
    if (out)
    {
        memcpy(out, p, sizeof(Policy));
    }
    return out;
}

/* Register a policy for a loop */
void loop_adapt_register_policy( char* string, char* polname, int num_profiles, int iters_per_profile)
{
    hwloc_topology_t tree = NULL;
    /* Check whether this loop name is already registered */
    pthread_mutex_lock(&loop_adapt_global_hash_lock);
    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
    pthread_mutex_unlock(&loop_adapt_global_hash_lock);
    if (tree)
    {
        /* Read the policy. This uses dlsym to search for the struct in the current executable */
        Policy_t p = loop_adapt_read_policy(polname);
        if (p)
        {
            if (loop_adapt_debug)
                printf("Add policy %s to tree of loop region %s with %d profiles and %d iterations per profile\n", p->name, string, num_profiles, iters_per_profile);
            // We register num_profiles+1 because one profile is used as base for the further evaluation
            loop_adapt_add_policy(tree, p, num_cpus, cpulist, num_profiles, iters_per_profile);
        }
        else
        {
            fprintf(stderr, "ERROR: Cannot find policy %s\n", polname);
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Loop region %s not registered\n", string);
    }
}

/* Register a policy for a loop */
void loop_adapt_register_searchalgo( char* string, char* polname, char *searchname)
{
    hwloc_topology_t tree = NULL;
    /* Check whether this loop name is already registered */
    pthread_mutex_lock(&loop_adapt_global_hash_lock);
    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
    pthread_mutex_unlock(&loop_adapt_global_hash_lock);
    if (tree)
    {
        /* Read the policy from already registered ones */
        Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        Policy_t p = NULL;
        for (int i = 0; i < tdata->num_policies; i++)
        {
            if (strncmp(tdata->policies[i]->internalname, polname, strlen(tdata->policies[i]->internalname)) == 0)
            {
                p = tdata->policies[i];
                break;
            }
        }
        if (p)
        {
            /* Get the search algorithm */
            SearchAlgorithm_t s = loop_adapt_search_select(searchname);
            if (s)
            {
                if (loop_adapt_debug)
                    printf("Add search algorithm %s to policy %s of loop region %s\n", s->name, p->internalname, string);

                p->search = s;
            }
        }
        else
        {
            fprintf(stderr, "ERROR: Cannot find policy %s\n", polname);
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Loop region %s not registered\n", string);
    }
}

/* Register an integer parameter for later manipulation through a policy. It is
 * save to register also unused parameters. It is assumed that each thread is
 * registering the parameter for itself. If the scope is not thread, it walks
 * up the tree until it found the parent node of the CPU with the proper scope.
 */
void loop_adapt_register_int_param( char* string,
                                    char* name,
                                    AdaptScope scope,
                                    int cpu,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max)
{
    int objidx;
    hwloc_topology_t tree = NULL;
    Treedata_t tdata = NULL;
    if (!string || cpu < 0 || !name || scope <= LOOP_ADAPT_SCOPE_NONE || scope >= LOOP_ADAPT_SCOPE_MAX)
    {
        fprintf(stderr, "ERROR: Invalid argument to loop_adapt_register_int_param\n");
        return;
    }
    // Check whether the loop was registered
    pthread_mutex_lock(&loop_adapt_global_hash_lock);
    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
    pthread_mutex_unlock(&loop_adapt_global_hash_lock);

    if (tree && cpu >= 0 && cpu < num_cpus)
    {
        tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        int len = hwloc_get_nbobjs_by_type(tree, scope);
        // Resolve object index based on the scope
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
        {
            objidx = cpus_to_objidx[cpu];
        }
        else
        {
            // Walk up the tree starting with own CPU until we find an object
            // with type == scope
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        }
        if (objidx >= 0 && objidx < len)
        {
            // We found an object index, so get the object and add the parameter
            // for the scope
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
            Nodevalues_t vals = (Nodevalues_t)obj->userdata;
            if (obj)
            {
                // If user does not give min and max, use the defined min or max
                // The min or max is policy specific, but if multiple policies
                // work on the same parameter, the min and max of the first
                // policy are chosen.
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
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Parameter %s: %d/%d/%d at %s %d\n", name, min, cur, max, loop_adapt_type_name(obj->type), obj->logical_index);
                // Add the parameter to the object's parameter hash
                loop_adapt_add_int_parameter(obj, name, desc, cur, min, max, NULL, NULL);
                
/*                for (int i=0; i < tdata->num_policies; i++)*/
/*                {*/
/*                    Policy_t p = tdata->policies[i];*/
/*                    PolicyProfile_t pp = vals->policies[i];*/
/*                    for (int j = 0; j < p->num_parameters; j++)*/
/*                    {*/
/*                        if (strncmp(p->parameters[j].name, name, strlen(name)) == 0)*/
/*                        {*/
/*                            loop_adapt_search_add_int_param(p->search, name, cur, min, max, pp->num_profiles);*/
/*                            break;*/
/*                        }*/
/*                    }*/
/*                }*/
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
    // Check whether the loop was registered
    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

    if (tree && cpu >= 0 && cpu < num_cpus)
    {
        int len = hwloc_get_nbobjs_by_type(tree, scope);
        Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
        {
            objidx = cpus_to_objidx[cpu];
            if (loop_adapt_debug)
            {
                fprintf(stderr, "DEBUG: Get param %s for CPU %d (Idx %d): ",name, cpu, objidx);
            }
        }
        else
        {
            // Walk up the tree starting with own CPU until we find an object
            // with type == scope
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
            if (loop_adapt_debug)
            {
                fprintf(stderr, "DEBUG: Get param %s for %s above CPU %d (Idx %d): ",name, loop_adapt_type_name(scope), cpu, objidx);
            }
        }
        

        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        PolicyProfile_t p = v->policies[v->cur_policy];

        if (v && v->policies && v->param_hash)
        {
            // Get the parameter data from object's parameter hash table
            Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer) name);
            if (np)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "%s = %d\n", name, np->cur.ival);
                ret = np->cur.ival;
            }
        }
        if (loop_adapt_debug)
            fflush(stderr);
    }
    return ret;
}

// See loop_adapt_register_int_param
void loop_adapt_register_double_param( char* string,
                                       char* name,
                                       AdaptScope scope,
                                       int cpu,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max)
{
    int objidx;
    hwloc_topology_t tree = NULL;
    Treedata_t tdata = NULL;
    if (!string || cpu < 0 || !name || scope <= LOOP_ADAPT_SCOPE_NONE || scope >= LOOP_ADAPT_SCOPE_MAX)
    {
        fprintf(stderr, "ERROR: Invalid argument to loop_adapt_register_double_param\n");
        return;
    }
    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

    
    if (cpu >= 0 && cpu < num_cpus)
    {
        tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        int len = hwloc_get_nbobjs_by_type(tree, scope);
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        if (objidx >= 0 && objidx < len)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
            Nodevalues_t vals = (Nodevalues_t)obj->userdata;
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
                
/*                for (int i=0; i < tdata->num_policies; i++)*/
/*                {*/
/*                    Policy_t p = tdata->policies[i];*/
/*                    PolicyProfile_t pp = vals->policies[i];*/
/*                    for (int j = 0; j < p->num_parameters; j++)*/
/*                    {*/
/*                        if (strncmp(p->parameters[j].name, name, strlen(name)) == 0)*/
/*                        {*/
/*                            loop_adapt_search_add_dbl_param(p->search, name, cur, min, max, pp->num_profiles);*/
/*                            break;*/
/*                        }*/
/*                    }*/
/*                }*/
            }
        }
    }
}

// See loop_adapt_get_int_param
double loop_adapt_get_double_param_param( char* string, AdaptScope scope, int cpu, char* name)
{
    int objidx = 0;
    double ret = 0;
    hwloc_topology_t tree = NULL;

    tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);

    if (cpu >= 0 && cpu < num_cpus)
    {
        int len = hwloc_get_nbobjs_by_type(tree, scope);
        Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        if (scope == LOOP_ADAPT_SCOPE_THREAD)
            objidx = cpus_to_objidx[cpu];
        else
            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        Nodevalues_t v = (Nodevalues_t)obj->userdata;
        PolicyProfile_t p = v->policies[v->cur_policy];
        if (v)
        {
            Nodeparameter_t np = g_hash_table_lookup(v->param_hash, (gpointer) name);
            if (np)
            {
                if (loop_adapt_debug)
                    fprintf(stderr, "%s = %d\n", name, np->cur.ival);
                ret = np->cur.dval;
            }
        }
    }
    return 0;
}

/*void loop_adapt_register_event(char* string, AdaptScope scope, int cpu, char* name, char* var, Nodeparametertype type, void* ptr)*/
/*{*/
/*    int objidx;*/
/*    hwloc_topology_t tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);*/
/*    int len = hwloc_get_nbobjs_by_type(tree, type);*/
/*    if (cpu >= 0 && cpu < num_cpus)*/
/*    {*/
/*        if (scope == LOOP_ADAPT_SCOPE_THREAD)*/
/*            objidx = cpus_to_objidx[cpu];*/
/*        else*/
/*            objidx = get_objidx_above_cpuidx(tree, scope, cpus_to_objidx[cpu]);*/
/*        if (objidx >= 0)*/
/*        {*/
/*            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);*/
/*            if (obj)*/
/*            {*/
/*                printf("Register event %s\n", name);*/
/*                loop_adapt_add_event(obj, name, var, type, ptr);*/
/*            }*/
/*        }*/
/*    }*/
/*}*/

/* This is just a helper function to save the filename and linenumber where
 * the loop was defined
 */
static int _loop_adapt_fill_tdata(Treedata_t tdata, char* filename, int linenumber)
{
    int ret = 0;
    pthread_mutex_lock(&tdata->lock);
    if (tdata->filename == NULL)
    {
        int ret = asprintf(&tdata->filename, "%s", filename);
        if (ret < 0)
        {
            ret = -ENOMEM;
        }
    }
    if (tdata->linenumber < 0)
    {
        tdata->linenumber = linenumber;
    }
    pthread_mutex_unlock(&tdata->lock);
    return ret;
}

/* Duplicate an int64 value, required for storing in a hash table */
static int64_t* intdup(int64_t i)
{
    int64_t* m = malloc(sizeof(int64_t));
    if (m)
    {
        *m = i;
    }
    return m;
}

/* Get thread information. If information is not available for the current
 * thread, create it and add the thread to the thread hash table
 */
static ThreadData_t _loop_adapt_get_thread(Treedata_t tdata)
{

    int64_t system_tid = gettid();
    int (*tid_func)() = NULL;
    // Check whether the thread was already registered. Key is the system TID
    ThreadData_t t = g_hash_table_lookup(tdata->threads, (gpointer)system_tid);
    if (!t)
    {
        // Thread does not exist, so create a new one
        t = malloc(sizeof(ThreadData));
        if (!t)
        {
            return NULL;
        }
        t->system_tid = system_tid;
        if (loop_adapt_debug)
            printf("NEW THREAD %d\n", system_tid);
        // We store some additional information about the thread besides TID
        t->pthread = pthread_self();
        t->cpuid = sched_getcpu();
        t->objidx = cpus_to_objidx[t->cpuid];
        t->state = LOOP_ADAPT_THREAD_RUN;
        // Read current CPU affinity of thread
        int ret = sched_getaffinity(system_tid, sizeof(cpu_set_t), &t->cpuset);
        loop_adapt_get_getcpu_func(&tid_func);
        if (tid_func)
        {
            // If a function to get a thread ID was registered, store that ID
            t->reg_tid = tid_func();
        }
        // Add the new thread to the hash table
        g_hash_table_insert(tdata->threads, (gpointer)system_tid, (gpointer)t);
    }
    return t;
}

static int _loop_adapt_begin_cpu(hwloc_topology_t tree, int cpuid)
{
    int objidx = cpus_to_objidx[cpuid];
    hwloc_obj_t cpu_obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
    if (cpu_obj)
    {
        Nodevalues_t cpu_vals = (Nodevalues_t)cpu_obj->userdata;
        if (cpu_vals && cpu_vals->policies)
        {
            PolicyProfile_t p = cpu_vals->policies[cpu_vals->cur_policy];
            if (p->cur_profile_iters == 0 && p->cur_profile < p->num_profiles)
            {
                /* Start the loop measurements */
                loop_adapt_begin_policies(cpuid, tree, cpu_obj);
            }
        }
    }
    return 0;
}

/* This function runs through the tree and executes the current policy on all
 * nodes of the policy scope. If the evaluation of the policy is positive,
 * update parameters.
 */
static int _loop_adapt_policy_tree(char* string, hwloc_topology_t tree)
{
    if (loop_adapt_active)
    {
        int done = 0;
        Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
        Policy_t p = tdata->cur_policy;
        // Iterate over all objects of policy's scope
        for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, p->scope); i++)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree,  p->scope, i);
            if (obj)
            {
                Nodevalues_t vals = (Nodevalues_t)obj->userdata;
                if (vals)
                {
                    PolicyProfile_t pp = vals->policies[vals->cur_policy];
                    if (pp->cur_profile == pp->num_profiles && !vals->done && pp->cur_profile_iters == 0)
                    {
                        done = 1;
                        if (loop_adapt_debug)
                            printf("Analyse last %s %d\n", loop_adapt_type_name( p->scope), i);
                        loop_adapt_exec_policies(tree, obj);
                    }
                    else if (pp->cur_profile > 0 && !vals->done && pp->cur_profile_iters == 0)
                    {
                        if (loop_adapt_debug)
                            printf("Analyse %s %d\n", loop_adapt_type_name( p->scope), i);
                        loop_adapt_exec_policies(tree, obj);
                    }
                    else if (pp->cur_profile == 0)
                    {
                        loop_adapt_init_parameter(obj, p);
                    }
                    if (pp->cur_profile == pp->num_profiles)
                    {
                        p->done = 1;
                    }
                }
            }
        }
        if (p->done && !p->finalized)
        {
            if (loop_adapt_debug)
                printf("Finalize %s\n", p->name);
            for (AdaptScope s = LOOP_ADAPT_SCOPE_MACHINE; s < LOOP_ADAPT_SCOPE_MAX; s++)
            {
                for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, (hwloc_obj_type_t)s); i++)
                {
                    hwloc_obj_t obj = hwloc_get_obj_by_type(tree, (hwloc_obj_type_t)s, i);
                    if (obj)
                    {
                        Nodevalues_t vals = (Nodevalues_t)obj->userdata;
                        if (vals)
                        {
                            PolicyProfile_t pp = vals->policies[vals->cur_policy];
                            vals->done = 1;
                            pp->cur_profile++;
                        }
                    }
                }
            }
            p->finalized = 1;
            loop_adapt_write_profiles(string, tree);
            loop_adapt_write_parameters(string, tree);
            loop_adapt_active = 0;
        }
    }
    return 0;
}

static int _loop_adapt_end_cpu(hwloc_topology_t tree, int cpuid)
{
    int objidx = cpus_to_objidx[cpuid];
    hwloc_obj_t cpu_obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Policy_t pol = tdata->cur_policy;
    if (cpu_obj)
    {
        Nodevalues_t cpu_vals = (Nodevalues_t)cpu_obj->userdata;
        if (cpu_vals)
        {
            PolicyProfile_t p = cpu_vals->policies[cpu_vals->cur_policy];

            if (p->cur_profile_iters < p->profile_iters-1 && p->cur_profile < p->num_profiles)
            {
                /* More iterations are required for the profile */
                p->cur_profile_iters++;
            }
            else if (p->cur_profile_iters == p->profile_iters-1 && p->cur_profile < p->num_profiles)
            {
                /* Stop the loop measurements */
                loop_adapt_end_policies(cpuid, tree, cpu_obj);
                int update_profile = p->cur_profile;
                p->cur_profile++;
                p->cur_profile_iters = 0;
                /* Update results in the whole tree */
                update_tree(cpu_obj, update_profile);
            }
            if (p->cur_profile == p->num_profiles)
            {
                // Mark the policy as done if we measured num_profiles profiles
                pol->done = 1;
            }
        }
    }
    return 0;
}

/* This function is called at the beginning of each loop iteration */
int loop_adapt_begin(char* string, char* filename, int linenumber)
{
    /* Just do something if loop_adapt is active */
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;
        hwloc_obj_t cpu_obj = NULL, new_obj = NULL;
        Nodevalues_t cpu_vals = NULL, new_vals = NULL;
        int (*tid_func)() = NULL;
        int (*tcount_func)() = NULL;
        /* Get the topology tree associated with the loop string
         * With this we can have different settings for different loops
         */
        tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
        if (tree)
        {
            /* Each tree contains some data describing the loop, e.g filename
             * and line number of loop and how many policies are registered
             * for this loop
             */
            Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
            /* Set some basic information about the loop */
            if (tdata->filename == NULL && tdata->linenumber < 0)
                _loop_adapt_fill_tdata(tdata, filename, linenumber);
            ThreadData_t thread = _loop_adapt_get_thread(tdata);
            /* Only perform the start routine if loop is currently stopped. */
            if (tdata->status == LOOP_STOPPED)
            {
                int cpuid = 0;
                int gl_cpucount = CPU_COUNT(&loop_adapt_cpuset);
                
                /* Get the function pointers to the functions for number of
                 * threads/tasks/... and the ID of a thread/task/...
                 */
                int cpucount = 0;
                int (*tcount_func)() = NULL;
                loop_adapt_get_cpucount_func(&tcount_func);
                if (tcount_func)
                {
                    cpucount = tcount_func();
                }
                if (loop_adapt_debug)
                    fprintf(stderr, "DEBUG: Starting loop %s for %d CPUs Total %d\n", string, cpucount, gl_cpucount);

                _loop_adapt_policy_tree(string, tree);

                // This is the case when we are already in a parallel region or completely serial (no cpucount function registered)
                if (cpucount > 1)
                {
                    loop_adapt_get_getcpu_func(&tid_func);
                    cpuid = (tid_func != NULL ? tid_func() : sched_getcpu());

                    _loop_adapt_begin_cpu(tree, cpuid);
                }
                // This is the case when we are in a serial region but threading is generally activated
                else if (cpucount == 1)
                {
                    /* This block is used if loop_adapt_begin is called from a
                     * serial region, so we have to iterate over all CPUs
                     * and evaluate/start the measurements
                     */
                    
                    for (int c = 0; c < num_cpus; c++)
                    {
                        if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
                        {
                            _loop_adapt_begin_cpu(tree, cpulist[c]);
                        }
                    }
                    
                }
                // Mark the loop as started
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


/*static int _loop_adapt_final_policy_cpu(hwloc_topology_t tree, int cpuid)*/
/*{*/
/*    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);*/
/*    */
/*    if (tdata->cur_policy)*/
/*    {*/
/*        int objidx = cpus_to_objidx[cpuid];*/
/*        hwloc_obj_t cpu_obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);*/
/*        if (cpu_obj)*/
/*        {*/
/*            Nodevalues_t cpu_vals = (Nodevalues_t)cpu_obj->userdata;*/
/*             If the optimal profile is -1, this means that none of*/
/*             * the tried parameter settings is better than the original*/
/*             * ones. Otherwise set the best-performing parameter.*/
/*             */
/*            if (cpu_vals)*/
/*            {*/
/*                PolicyProfile_t p = cpu_vals->policies[cpu_vals->cur_policy];*/
/*                if (p->opt_profile < 0)*/
/*                {*/
/*                    printf("Set start value as best value in all nodes\n");*/
/*                    loop_adapt_reset_parameter(tree, tdata->cur_policy);*/
/*                }*/
/*                else*/
/*                {*/
/*                    loop_adapt_best_parameter(tree, tdata->cur_policy);*/
/*                }*/
/*                cpu_vals->done = 1;*/
/*            }*/
/*        }*/
/*    }*/
/*    return 0;*/
/*}*/

/*static int _loop_adapt_next_undone_policy(Treedata_t tdata)*/
/*{*/
/*    int idx = -1;*/
    /* Search for undone policy */
/*    for (int i=0; i < tdata->num_policies; i++)*/
/*    {*/
/*        if (!tdata->policies[i]->done)*/
/*        {*/
/*            idx = i;*/
/*            break;*/
/*        }*/
/*    }*/
/*    return idx;*/
/*}*/

static int _loop_adapt_disable(char* string, hwloc_topology_t tree)
{
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    /* Update the tree one last time for all CPUs */
    for (int c = 0; c < num_cpus; c++)
    {
        if (CPU_ISSET(cpulist[c], &loop_adapt_cpuset))
        {
            int objidx = cpus_to_objidx[cpulist[c]];
            hwloc_obj_t cpu_obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
            if (cpu_obj != NULL)
            {
                Nodevalues_t cpu_vals = (Nodevalues_t)cpu_obj->userdata;
                if (cpu_vals)
                {
                    PolicyProfile_t p = cpu_vals->policies[cpu_vals->cur_policy];
                    update_tree(cpu_obj, p->cur_profile);
                }
            }
         }
    }
    /* Write out the measured profiles */
    loop_adapt_write_profiles(string, tree);
    /* Write out the parameter settings */
    loop_adapt_write_parameters(string, tree);
    return 0;
}

/* This function is called at the end of each loop iteration */
int loop_adapt_end(char* string)
{
    /* Just do something if loop_adapt is active */
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = NULL;

        /* Get the topology tree associated with the loop string
         * With this we can have different settings for different loops
         */
        tree = g_hash_table_lookup(loop_adapt_global_hash, (gpointer) string);
        if (tree)
        {
            /* Each tree contains some data describing the loop, e.g filename
             * and line number of loop and how many policies are registered
             * for this loop
             */
            Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
            ThreadData_t thread = _loop_adapt_get_thread(tdata);
            /* Only perform the end routine if loop is currently running. */
            if (tdata->status == LOOP_STARTED)
            {
                /* Get the function pointers to the functions for number of
                 * threads/tasks/... and the ID of a thread/task/...
                 */
                int cpucount = 0;
                int (*tcount_func)() = NULL;
                loop_adapt_get_cpucount_func(&tcount_func);
                if (tcount_func)
                {
                    cpucount = tcount_func();
                }
                uint64_t cmask = 0x0ULL;
                // This is the case when we are already in a parallel region or completely serial
                if (cpucount > 1)
                {
                    int (*tid_func)() = NULL;
                    loop_adapt_get_getcpu_func(&tid_func);
                    int cpuid = (tid_func != NULL ? tid_func() : sched_getcpu());
                    /* Get the current object ID. This is the ID in the hwloc
                     * topology tree.
                     */
                    int finalize = _loop_adapt_end_cpu(tree, cpuid);
                    if (finalize > 0)
                    {
                        //_loop_adapt_final_policy_cpu(tree, cpuid);
                        cmask |= (1ULL<<cpuid);
                    }
                }
                // This is the case when we are in a serial region but threading is generally activated
                else if (cpucount == 1)
                {
                    
                    for (int c = 0; c < num_cpus; c++)
                    {
                        int cpuid = cpulist[c];
                        if (CPU_ISSET(cpuid, &loop_adapt_cpuset))
                        {
                            int finalize = _loop_adapt_end_cpu(tree, cpuid);
                            if (finalize > 0)
                            {
                                //_loop_adapt_final_policy_cpu(tree, cpuid);
                                cmask |= (1ULL<<c);
                            }
                            else if (finalize < 0)
                            {
                                break;
                            }
                        }
                    }
                }
                

                /* Mark loop as stopped */
                tdata->status = LOOP_STOPPED;
            }
        }
        else
        {
            fprintf(stderr, "Loop with string %s unknown. Needs to be registered first\n", string);
        }
    }
    if (loop_adapt_debug)
        fprintf(stderr, "DEBUG: Ending loop %s\n\n", string);

    return 1;
}





__attribute__((destructor)) void loop_adapt_finalize (void)
{
    GHashTableIter iter;
    gpointer gkey;
    gpointer gvalue;
    char* key;
    hwloc_topology_t tree;

    // Grab the lock to be able to manipulate the global hash table
    pthread_mutex_lock(&loop_adapt_global_hash_lock);
    if (loop_adapt_global_hash)
    {
        // Iterate over all registered loops
        g_hash_table_iter_init(&iter, loop_adapt_global_hash);
        while(g_hash_table_iter_next(&iter, &gkey, &gvalue))
        {
            key = (char*)gkey;
            tree = (hwloc_topology_t)gvalue;
            // Cleanup tree with profiles
            tidy_tree(tree);

            // Cleanup tree data
            Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
            free(tdata->filename);
            if (tdata->num_policies > 0)
            {
                for (int i = 0; i < tdata->num_policies; i++)
                {
                    free(tdata->policies[i]);
                }
                free(tdata->policies);
                tdata->num_policies = 0;
                tdata->cur_policy = NULL;
                pthread_mutex_destroy(&tdata->lock);
            }
            // Cleanup thread data
            GHashTableIter thread_iter;
            int64_t tkey = 0;
            ThreadData_t tvalue = NULL;
            g_hash_table_iter_init(&iter, tdata->threads);
            while(g_hash_table_iter_next(&iter, (gpointer)tkey, (gpointer)&tvalue))
            {
                free(tvalue);
            }
            free(tdata);
            // Destroy hwloc tree
            hwloc_topology_set_userdata(tree, NULL);
            hwloc_topology_destroy(tree);
        }
        g_hash_table_destroy(loop_adapt_global_hash);
    }
    // Clean the base hwloc tree (parent of all loop-specific trees)
    if (topotree)
    {
        hwloc_topology_destroy(topotree);
        topotree = NULL;
    }
    pthread_mutex_unlock(&loop_adapt_global_hash_lock);
}

