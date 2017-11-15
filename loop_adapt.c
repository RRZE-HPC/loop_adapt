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
#include <loop_adapt_helper.h>


static int loop_adapt_active = 1;

static GHashTable* global_hash = NULL;
static hwloc_topology_t topotree = NULL;
static int num_cpus = 0;
static int *cpus_to_objidx = NULL;
static int *cpulist = NULL;

/*static int default_num_profiles = 5;*/
/*static int runtime_num_profiles = 0;*/

static int default_max_num_values = 30;
static int max_num_values = 0;

//static char* likwid_group = NULL;
//static int likwid_gid = -1;
//static int likwid_num_metrics = 0;
#define MIN(x,y) ((x)<(y)?(x):(y))


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
            fprintf(stderr, "Cannot duplicate topology tree\n", string);
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

int loop_adapt_list_policy()
{
    FILE* fp = NULL;
    char* eptr = NULL;
    char buff[256];
    char pname[100];
    int num_policies = 0;
    int ret = 0;

    fp = popen("nm libloop_adapt.so | grep POL_", "r");
    if (fp)
    {
        eptr = fgets(buff, 256, fp);
        while (eptr)
        {
            int len = strlen(buff);
            int i = len-1;
            while (buff[i] != ' ')
                i--;
            i++;
            if (strncmp(buff + i, "POL_", 4) == 0)
            {
                ret = sprintf(pname, "%.*s", len - i - 1, buff + i);
                pname[ret] = '\0';
                Policy_t p = loop_adapt_read_policy(pname);
                if (p)
                {
                    printf("Policy %s:\n", p->name);
                    printf("Identifyer: %s\n", pname);
                    printf("Description: %s\n", p->desc);
                    printf("LIKWID group: %s\n", p->likwid_group);
                    if (p->num_parameters > 0)
                    {
                        printf("Required parameter:\n");
                        for (int j = 0; j < p->num_parameters; j++)
                        {
                            printf("%s : %s\n", p->parameters[j].name,
                                                p->parameters[j].desc);
                        }
                    }
                }
                printf("\n");
                num_policies++;
            }
            eptr = fgets(buff, 256, fp);
        }
        pclose(fp);
    }
    return num_policies;
}

void loop_adapt_register_int_param( char* string,
                                    AdaptScope scope,
                                    int objidx,
                                    char* name,
                                    char* desc,
                                    int cur,
                                    int min,
                                    int max)
{
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (objidx >= 0)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        if (obj)
        {
            //printf("Adding parameter %s to object %d of type %s Min %d Max %d\n", name, objidx, hwloc_obj_type_string(scope), min, max);
            loop_adapt_add_int_parameter(obj, name, desc, cur, min, max, NULL, NULL);
        }
    }
}



int loop_adapt_get_int_param( char* string, AdaptScope scope, int objidx, char* name)
{
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (objidx >= 0)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        while (obj)
        {
            Nodevalues_t v = (Nodevalues_t)obj->userdata;
            if (v)
            {
                for (int i=0; i < v->num_parameters; i++)
                {
                    if (strncmp(name, v->parameters[i]->name, strlen(name)) == 0 &&
                        v->parameters[i]->type == NODEPARAMETER_INT)
                    {
                        return v->parameters[i]->icur;
                    }
                }
            }
            obj = obj->parent;
        }
    }
    return 0;
}

void loop_adapt_register_double_param( char* string,
                                       AdaptScope scope,
                                       int objidx,
                                       char* name,
                                       char* desc,
                                       double cur,
                                       double min,
                                       double max)
{
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (objidx >= 0)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        if (obj)
        {
            //printf("Adding parameter %s to object %d of type %s Min %d Max %d\n", name, objidx, hwloc_obj_type_string(scope), min, max);
            loop_adapt_add_double_parameter(obj, name, desc, cur, min, max, NULL, NULL);
        }
    }
}

double loop_adapt_get_double_param( char* string, AdaptScope scope, int objidx, char* name)
{
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    int len = hwloc_get_nbobjs_by_type(tree, scope);
    if (objidx >= 0)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, scope, objidx);
        while (obj)
        {
            Nodevalues_t v = (Nodevalues_t)obj->userdata;
            if (v)
            {
                for (int i=0; i < v->num_parameters; i++)
                {
                    if (strncmp(name, v->parameters[i]->name, strlen(name)) == 0 &&
                        v->parameters[i]->type == NODEPARAMETER_DOUBLE)
                    {
                        return v->parameters[i]->dcur;
                    }
                }
            }
            obj = obj->parent;
        }
    }
    return 0;
}

/*void loop_adapt_register_event(char* string, AdaptScope scope, int objidx, char* name, char* var, Nodeparametertype type, void* ptr)*/
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
                int cpuid = omp_get_thread_num();
                likwid_pinThread(cpuid);
                int objidx = cpus_to_objidx[cpuid];
                int ret = 0;
                Nodevalues_t v = NULL;
                hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);

                v = (Nodevalues_t)obj->userdata;
                if (v->cur_profile < v->num_profiles)
                {
                    //printf("LOOP START CPU %d/%d PROFILE %d/%d\n", cpuid, objidx, v->cur_profile, tdata->num_profiles);
                    if (v->cur_profile == 0)
                    {
                        ret = asprintf(&tdata->filename, "%s", filename);
                        tdata->start_linenumber = linenumber;
                    }
                    

                    if (v->cur_profile > 1)
                    {
                        loop_adapt_exec_policies(tree, obj);
                    }
                    loop_adapt_begin_policies(cpuid, tree, obj); 
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



int loop_adapt_end(char* string, char* filename, int linenumber)
{
    if (loop_adapt_active)
    {
        hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
        if (tree)
        {
            Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
            if (tdata->status == LOOP_STARTED)
            {
                int cpuid = sched_getcpu();
                int objidx = cpus_to_objidx[cpuid];
                int ret = 0;
                hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, objidx);
                Nodevalues *v = (Nodevalues *)obj->userdata;
                if (v != NULL && v->cur_profile < v->num_profiles)
                {
                    if (v->cur_profile == 0)
                    {
                        tdata->end_linenumber = linenumber;
                    }
                    loop_adapt_end_policies(cpuid, tree, obj);
                    if (v->cur_profile > 0)
                        update_tree(obj, v->cur_profile-1);
                    v->cur_profile++;
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

void loop_adapt_print(char *string, int profile_num)
{
    hwloc_topology_t tree = g_hash_table_lookup(global_hash, (gpointer) string);
    if (tree)
    {
        Treedata *tdata = (Treedata *)hwloc_topology_get_userdata(tree);
        printf("Loop in file %s line %d to %d\n", tdata->filename, tdata->start_linenumber, tdata->end_linenumber);
        printf("Taking %d profiles totally\n", tdata->num_profiles);
        printf("Printing results for profile %d\n", profile_num);
        printf("\n");
        Policy_t p = tdata->cur_policy;
        for (int i = 0; i < hwloc_get_nbobjs_by_type(tree, HWLOC_OBJ_PU); i++)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(tree, HWLOC_OBJ_PU, i);
            Nodevalues *v = (Nodevalues *)obj->userdata;
            double t = timer_print(&v->runtimes[profile_num]);
            if (t > 0 && profile_num < tdata->num_profiles && profile_num < v->cur_profile)
            {
                printf("Object %d = CPU %d\n", i, obj->os_index);
                printf("Runtime %f seconds\n", t);
                for (int j = 0; j < max_num_values; j++)
                {
                    char *name = perfmon_getMetricName(p->likwid_gid, j);
                    double value = v->profiles[profile_num][j];
                    printf("Metric %s : %f\n", name, value);
                }
                printf("\n");
            }
        }
    }
}



__attribute__((destructor)) void loop_adapt_finalize (void)
{
    printf("Destructor\n");
    if (loop_adapt_active)
    {
        perfmon_stopCounters();
        perfmon_finalize();
    }
    
    GHashTableIter iter;
    gpointer gkey;
    gpointer gvalue;
    char* key;
    hwloc_topology_t tree;
    g_hash_table_iter_init(&iter, global_hash);
    while(g_hash_table_iter_next(&iter, &gkey, &gvalue))
    {
        key = (char*)gkey;
        printf("Cleaning tree for loop %s\n", key);
        tree = (hwloc_topology_t)gvalue;
        printf("Tree %p\n", tree);
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
}

