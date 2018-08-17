#ifndef LOOP_ADAPT_TYPES
#define LOOP_ADAPT_TYPES
#include <pthread.h>
#include <likwid.h>
#include <hwloc.h>
#include <ghash.h>

#include <loop_adapt.h>

#define LOOP_ADAPT_MAX_POLICY_PARAMS 5
#define LOOP_ADAPT_MAX_POLICY_METRICS 30
#define LOOP_ADAPT_MAX_POLICY_PROFILES 20

//typedef enum {
//    LOOP_ADAPT_SCOPE_NONE = -1,
//    LOOP_ADAPT_SCOPE_MACHINE = HWLOC_OBJ_MACHINE,
//    LOOP_ADAPT_SCOPE_NUMA = HWLOC_OBJ_NUMANODE,
//    LOOP_ADAPT_SCOPE_SOCKET = HWLOC_OBJ_PACKAGE,
//    LOOP_ADAPT_SCOPE_THREAD = HWLOC_OBJ_PU,
//    LOOP_ADAPT_SCOPE_MAX,
//} AdaptScope;


typedef union {
    int ival;
    double dval;
    char *cval;
} Value;


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


//typedef struct {
//    char* name;
//    int steps;
//    int change;
//    int max_change;
//    int has_best;
//    Nodeparametertype type;
//    Value best;
//    Value start;
//    Value cur;
//    Value min;
//    Value max;
//    int num_old_vals;
//    Value* old_vals;
//} SearchAlgorithmParam;
//typedef SearchAlgorithmParam* SearchAlgorithmParam_t;



typedef struct {
    char* name;
    char* desc;
    Nodeparametertype type;
    int steps;
    int change;
    Value best;
    Value start;
    Value cur;
    Value min;
    Value max;
    int has_best;
    int num_old_vals;
    Value* old_vals;
    void (*pre)(char* fmt, ...);
    void (*post)(char* fmt, ...);
} Nodeparameter;

typedef Nodeparameter* Nodeparameter_t;

//typedef PolicyParameter* PolicyParameter_t;

typedef struct {
    char* name;
    int (*init)(Nodeparameter_t np);
    int (*next)(Nodeparameter_t np);
    int (*best)(Nodeparameter_t np);
    int (*reset)(Nodeparameter_t np);
} SearchAlgorithm;

typedef SearchAlgorithm* SearchAlgorithm_t;


typedef struct {
    char* likwid_group;
    int likwid_gid;
    char* name;
    char* internalname;
    char* desc;
    int done;
    int finalized;
    //int optimal_profile;
    AdaptScope scope;
    int (*loop_adapt_policy_eval)(hwloc_topology_t tree, hwloc_obj_t obj);
    int (*loop_adapt_policy_begin)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
    int (*loop_adapt_policy_init)(int num_cpus, int* cpulist, int num_profiles);
    int (*loop_adapt_policy_end)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj);
    void (*loop_adapt_policy_close)(void);
    //double tolerance;
    int num_parameters;
    PolicyParameter parameters[LOOP_ADAPT_MAX_POLICY_PARAMS];
    SearchAlgorithm_t search;
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
    char* filename; /*! \brief Filename where the loop is defined */
    int linenumber; /*! \brief Line number in filename where the loop is defined */
    int num_policies; /*! \brief Number of registered policies */
    LoopRunStatus status; /*! \brief State of the loop */
    int cur_policy_id; /*! \brief ID of currently active policy */
    Policy_t cur_policy; /*! \brief Pointer to the currently active policy */
    Policy_t *policies; /*! \brief List of all policies */
    pthread_mutex_t lock; /*! \brief Lock for tree manipulation */
    GHashTable* threads; /*! \brief Hashtable mapping thread ID to ThreadData */
} Treedata;
typedef Treedata* Treedata_t;





typedef struct {
    char* name;
    char* varname;
    Nodeparametertype type;
    void* ptr;
} Nodeevent;

typedef Nodeevent* Nodeevent_t;

typedef struct {
    int id;
    TimerData timer;
    double runtime;
    double values[LOOP_ADAPT_MAX_POLICY_METRICS];
    /* TODO: Do statistics for tree nodes */
//    double mins[LOOP_ADAPT_MAX_POLICY_METRICS];
//    double maxs[LOOP_ADAPT_MAX_POLICY_METRICS];
//    double means[LOOP_ADAPT_MAX_POLICY_METRICS];
} Profile;
typedef Profile* Profile_t;

typedef struct {
    int id;
    int cur_profile;
    int profile_iters;
    int cur_profile_iters;
    int opt_profile;
    int num_profiles;
    int num_values;
    Profile_t  profiles[LOOP_ADAPT_MAX_POLICY_PROFILES];
    Profile_t  base_profile;
} PolicyProfile;
typedef PolicyProfile* PolicyProfile_t;

typedef struct {
    int cur_policy;
    int num_policies;
//    int cur_profile;
    // One entry per policy
//    int* num_profiles;
//    int* profile_iters;
//    int* cur_profile_iters;
//    int* num_values;
//    int* opt_profiles;
    int count;
    int num_pes;
    int done;
    // Pthread fun
    pthread_mutex_t lock;
    pthread_cond_t cond;
    // Array with policy data
    PolicyProfile_t* policies;
    /* two dimensional array, first dim is the policyID and second dim
     * is the profileID.
     */
//    TimerData **timers;
//    double** runtimes;
    /* three dimensional array, first dim is the policyID, second profileID and
     * third dim are the values of the profile.
     */
//    double ***profiles;
    /* two dimensional array, first dim is the policyID and second dim is the
     * values of the profile.
     */
//    double **base_profiles;
//    TimerData* base_timers;
//    double *base_runtimes;
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
