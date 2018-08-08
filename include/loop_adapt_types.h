#ifndef LOOP_ADAPT_TYPES
#define LOOP_ADAPT_TYPES
#include <pthread.h>
#include <likwid.h>
#include <hwloc.h>
#include <ghash.h>

#include <loop_adapt.h>

#define LOOP_ADAPT_MAX_POLICY_PARAMS 5
#define LOOP_ADAPT_MAX_POLICY_METRICS 30

//typedef enum {
//    LOOP_ADAPT_SCOPE_NONE = -1,
//    LOOP_ADAPT_SCOPE_MACHINE = HWLOC_OBJ_MACHINE,
//    LOOP_ADAPT_SCOPE_NUMA = HWLOC_OBJ_NUMANODE,
//    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
//    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU,
//    LOOP_ADAPT_SCOPE_MAX,
//} AdaptScope;


typedef struct {
    char* name;
    char* desc;
    char* def_min;
    char* def_max;
    char* eval;
} PolicyParameter;

typedef PolicyParameter* PolicyParameter_t;

typedef enum {
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL_THEN,
    GREATER_EQUAL_THEN,
} PolicyMetricCompare;

typedef enum {
    NONE,
    UP,
    DOWN,
} PolicyMetricDestination;

typedef enum {
    LOOP_STARTED,
    LOOP_STOPPED
} LoopRunStatus;

typedef struct {
    char* var;
    char* name;
} PolicyMetric;

//typedef PolicyParameter* PolicyParameter_t;

typedef struct {
    char* likwid_group;
    int likwid_gid;
    char* name;
    char* desc;
    int done;
    int finalized;
    //int optimal_profile;
    AdaptScope scope;
    int (*loop_adapt_eval)(hwloc_topology_t tree, hwloc_obj_t obj);
    int (*loop_adapt_eval_begin)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
    int (*loop_adapt_eval_init)(int num_cpus, int* cpulist, int num_profiles);
    int (*loop_adapt_eval_end)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
    void (*loop_adapt_eval_close)(void);
    //double tolerance;
    int num_parameters;
    PolicyParameter parameters[LOOP_ADAPT_MAX_POLICY_PARAMS];
    
    //char* eval;
    int num_metrics;
    PolicyMetric metrics[LOOP_ADAPT_MAX_POLICY_METRICS];
    int metric_idx[LOOP_ADAPT_MAX_POLICY_METRICS];
} Policy;

typedef Policy* Policy_t;

typedef enum {
    LOOP_ADAPT_THREAD_PAUSE = 0,
    LOOP_ADAPT_THREAD_RUN,
} ThreadState;

typedef struct {
    pthread_t pthread;
    int thread_id;
    int system_tid;
    int cpuid;
    int objidx;
    int reg_tid;
    cpu_set_t cpuset;
    ThreadState state;
} ThreadData;

typedef ThreadData* ThreadData_t;

typedef struct {
    char* filename;
    int linenumber;
    int num_policies;
    LoopRunStatus status;
    int cur_policy_id;
    Policy_t cur_policy;
    Policy_t *policies;
    pthread_mutex_t lock;
    GHashTable* threads;
} Treedata;

typedef Treedata* Treedata_t;

typedef union {
    int ival;
    double dval;
    char *cval;
} Value;


typedef struct {
    char* name;
    char* desc;
    Nodeparametertype type;
    int inter;
    Value best;
    Value start;
    Value cur;
    Value min;
    Value max;
    int num_old_vals;
    Value* old_vals;
    void (*pre)(char* fmt, ...);
    void (*post)(char* fmt, ...);
} Nodeparameter;

typedef Nodeparameter* Nodeparameter_t;

typedef struct {
    char* name;
    char* varname;
    Nodeparametertype type;
    void* ptr;
} Nodeevent;

typedef Nodeevent* Nodeevent_t;

typedef struct {
    int cur_policy;
    int num_policies;
    int cur_profile;
    // One entry per policy
    int* num_profiles;
    int* profile_iters;
    int* cur_profile_iters;
    int* num_values;
    int* opt_profiles;
    int count;
    int num_pes;
    int done;
    // Pthread fun
    pthread_mutex_t lock;
    pthread_cond_t cond;
    /* two dimensional array, first dim is the policyID and second dim
     * is the profileID.
     */
    TimerData **timers;
    double** runtimes;
    /* three dimensional array, first dim is the policyID, second profileID and
     * third dim are the values of the profile.
     */
    double ***profiles;
    // String to obj hashtable for the node-level parameters
    GHashTable* param_hash;
    // String to obj hashtable for the node-level events
    int num_events;
    Nodeevent_t *events;
} Nodevalues;

typedef Nodevalues* Nodevalues_t;


//typedef struct {
//    char* name;
//    char* desc;
//    Nodeparametertype type;
//    union {
//        int ibest;
//        double dbest;
//    } best;
//} PolicyResultParameter;

//typedef PolicyResultParameter* PolicyResultParameter_t;

//typedef struct {
//    char* name;
//    char* desc;
//    int num_values;
//    double* values;
//    int num_parameters;
//    PolicyResultParameter_t parameters;
//} PolicyResult;

//typedef PolicyResult* PolicyResult_t;


#endif
