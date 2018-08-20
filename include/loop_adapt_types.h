#ifndef LOOP_ADAPT_TYPES
#define LOOP_ADAPT_TYPES
#include <pthread.h>
#include <likwid.h>
#include <hwloc.h>
#include <ghash.h>

#include <loop_adapt.h>

/**
 * @file loop_adapt_types.h
 * @author Thomas Roehl
 * @date 19.08.2018
 * @brief File containing the data structures of loop_adapt
 */

#define LOOP_ADAPT_MAX_POLICY_PARAMS 5 /**< \brief Defines the maximal number of policy parameters to avoid dynamic allocations */
#define LOOP_ADAPT_MAX_POLICY_METRICS 30 /**< \brief Defines the maximal number of metrics per policy to avoid dynamic allocations */
#define LOOP_ADAPT_MAX_POLICY_PROFILES 20 /**< \brief Defines the maximal number of profiles per policy to avoid dynamic allocations */

/*! \brief Internal representation of values

This union combines all possible data types that can be used as parameter value.
Currently only ival and dval are used. The decision which union member is used
is based on the Nodeparametertype.
*/
typedef union {
    int ival; /**< \brief Integer */
    double dval; /**< \brief Double */
    char *cval; /**< \brief Char pointer */
    void* pval; /**< \brief Void pointer */
} Value;





/*! \brief Base structure of a search algorithm

This structure contains the information of a search algorithm. Besides the name
it contains the function pointers to the search algorithm functions called
when updating a parameter.
*/
typedef struct {
    char* name; /**< \brief Name of the parameter */
    char* desc; /**< \brief Description of the parameter */
    Nodeparametertype type; /**< \brief Value type of the paramter (int, double, ...) */
    int steps; /**< \brief How many times whill the paramter be changed */
    int change; /**< \brief Current number of changes */
    int has_best; /**< \brief Was there a best value? */
    Value best; /**< \brief Best value */
    Value start; /**< \brief Start value (current at registration) */
    Value cur; /**< \brief Current value (current at registration, this one is returned when getting a paramter back to the application) */
    Value min; /**< \brief Minimal value (minimum at registration) */
    Value max; /**< \brief Maximal value (maximum at registration) */
    int num_old_vals; /**< \brief Number of recorded (tried) parameter values (should be same as change) */
    Value* old_vals; /**< \brief Storage of recorded (tried) parameter values */
} Nodeparameter;
/*! \brief Pointer to a Nodeparameter structure */
typedef Nodeparameter* Nodeparameter_t;


/*! \brief Base structure of a search algorithm

This structure contains the information of a search algorithm. Besides the name
it contains the function pointers to the search algorithm functions called
when updating a parameter.
*/
typedef struct {
    char* name; /**<  \brief Name of the search algorithm */
    int (*init)(Nodeparameter_t np); /**<  \brief Function called to init an parameter (set initial value) */
    int (*next)(Nodeparameter_t np); /**<  \brief Function called to get the next value */
    int (*best)(Nodeparameter_t np); /**<  \brief Function called to mark the best value */
    int (*reset)(Nodeparameter_t np); /**<  \brief Function called to rest the parameter */
} SearchAlgorithm;
/*! \brief Pointer to a SearchAlgorithm structure */
typedef SearchAlgorithm* SearchAlgorithm_t;


/*! \brief Description of a policy parameter

This structure describes a parameter associated to a policy. It contains the
name, description and default mimimum and maximum values if nothing is given
by the user. The eval string is evaluated during the policy's eval function to
see whether the parameter benefits from changing its value. It uses the
variable names defined by PolicyMetric with suffixes '_cur' and '_opt' for
current profile value and optimal (or base) profile value.
*/
typedef struct {
    char* name; /**< \brief Name of the parameter */
    char* desc; /**< \brief Description of the parameter */
    char* def_min; /**< \brief Default mimimum of the parameter */
    char* def_max; /**< \brief Default maximum of the parameter */
    char* eval; /**< \brief Evaluation formula for the parameter */
    int (*pre)(char* location, Nodeparameter_t param); /**< \brief Function called before changing the parameter */
    int (*post)(char* location, Nodeparameter_t param); /**< \brief Function called after changing the parameter */
} PolicyParameter;
/*! \brief Pointer to a PolicyParameter structure */
typedef PolicyParameter* PolicyParameter_t;


/*! \brief Mapping of variable name to metric name

Since working on full metric names is no fun, we establish a mapping from
metric names to variable names. The variables are use in the evaluation string
for a policy parameter.
*/
typedef struct {
    char* var; /**< \brief Variable name */
    char* name; /**< \brief Metric name */
} PolicyMetric;

/*! \brief Base structure of a policy

This structure contains the information of a policy. Besides the name and
description it contains the function pointers to the policy functions called
at loop begin/end and for evaluation. Moreover, it has a list of parameters on
which the policy works on. The parameter name has to match the registered
parameters. Additionally, it allows to select only specific metrics of a LIKWID
group to use default LIKWID groups and don't need to write special groups for
loop_adapt.
*/
typedef struct {
    char* likwid_group; /**< \brief LIKWID group name */
    int likwid_gid; /**< \brief Storage for LIKWID's group id */
    char* name; /**< \brief Human-readble name of the policy */
    char* internalname; /**< \brief Internal name of the policy (has to match structure name when defining)*/
    char* desc; /**< \brief Description of the policy */
    int done; /**< \brief Is the policy done? */
    int finalized; /**< \brief Is the policy finalized? */
    AdaptScope scope; /**< \brief Scope of the policy. At which tree level is it working on */
    int (*loop_adapt_policy_eval)(hwloc_topology_t tree, hwloc_obj_t obj); /**< \brief Function called to determine a new optimal profile */
    int (*loop_adapt_policy_begin)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj); /**< \brief Function called at the beginning of a profile */
    int (*loop_adapt_policy_init)(int num_cpus, int* cpulist, int num_profiles); /**< \brief Function called at the initialization of the policy */
    int (*loop_adapt_policy_end)(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj); /**< \brief Function called at the end of a profile */
    void (*loop_adapt_policy_close)(void); /**< \brief Function called at the finalization of the policy */
    int num_parameters; /**< \brief Number of parameters used by the policy */
    PolicyParameter parameters[LOOP_ADAPT_MAX_POLICY_PARAMS]; /**< \brief Storage for parameters */
    SearchAlgorithm_t search; /**< \brief Search Algorithm */
    int num_metrics; /**< \brief Number of metrics used by the policy */
    PolicyMetric metrics[LOOP_ADAPT_MAX_POLICY_METRICS]; /**< \brief Description of a LIKWID metric */
    int metric_idx[LOOP_ADAPT_MAX_POLICY_METRICS]; /**< \brief ID of the LIKWID metrics in LIKWID's output */
} Policy;
/*! \brief Pointer to a Policy structure */
typedef Policy* Policy_t;


/*! \brief Status of a thread */
typedef enum {
    LOOP_ADAPT_THREAD_PAUSE = 0, /**< \brief Thread is not running */
    LOOP_ADAPT_THREAD_RUN, /**< \brief Thread is running */
} ThreadState;

/*! \brief Base structure describing a thread

This structure contains information about the registered threads. At
registration it determines some values like system PID, current CPU, ...
*/
typedef struct {
    pthread_t pthread; /**< \brief PThreads data structure */
    int thread_id; /**< \brief Thread ID given by loop_adapt */
    int system_tid; /**< \brief System PID (or TID) of the thread */
    int cpuid; /**< \brief Current CPU */
    int objidx; /**< \brief Object index in the topology tree */
    int reg_tid; /**< \brief Thread ID as returned by the registered thread ID function */
    cpu_set_t cpuset; /**< \brief Current CPUset */
    ThreadState state; /**< \brief Status of the thread */
} ThreadData;
/*! \brief Pointer to a ThreadData structure */
typedef ThreadData* ThreadData_t;

/*! \brief Status of a loop */
typedef enum {
    LOOP_STARTED, /**< \brief Loop is running */
    LOOP_STOPPED /**< \brief Loop is stopped */
} LoopRunStatus;

/*! \brief Base loop structure associated with a topology tree

This structure is used as base information about a loop and stored in the 
loop's topology tree. This way we just need to resolve the loop name to the
topology tree and have all required data about the loop.
*/
typedef struct {
    char* filename; /**< \brief Filename where the loop is defined */
    int linenumber; /**< \brief Line number in filename where the loop is defined */
    int num_policies; /**< \brief Number of registered policies */
    LoopRunStatus status; /**< \brief State of the loop */
    int cur_policy_id; /**< \brief ID of currently active policy */
    Policy_t cur_policy; /**< \brief Pointer to the currently active policy */
    Policy_t *policies; /**< \brief List of all policies */
    pthread_mutex_t lock; /**< \brief Lock for tree manipulation */
    GHashTable* threads; /**< \brief Hashtable mapping thread ID to ThreadData */
} Treedata;
/*! \brief Pointer to a Treedata structure */
typedef Treedata* Treedata_t;




// Currently unused but could be used to add events that are updated by the user
// and added to profiles like MLUPs or ProcessedParticles.
//typedef struct {
//    char* name;
//    char* varname;
//    Nodeparametertype type;
//    void* ptr;
//} Nodeevent;

//typedef Nodeevent* Nodeevent_t;


/*! \brief Structure describing a profile

This structure holds the values of a profile including runtime measurements and
all values added by a policy
*/
typedef struct {
    int id; /**< \brief Identifier of the profile */
    TimerData timer; /**< \brief Private timer for the profile */
    double runtime; /**< \brief Runtime of the profile */
    double values[LOOP_ADAPT_MAX_POLICY_METRICS]; /**< \brief Profile values (should be dynamically allocated but currently fixed size) */
    /* TODO: Do statistics for tree nodes */
//    double mins[LOOP_ADAPT_MAX_POLICY_METRICS];
//    double maxs[LOOP_ADAPT_MAX_POLICY_METRICS];
//    double means[LOOP_ADAPT_MAX_POLICY_METRICS];
} Profile;
/*! \brief Pointer to a Profile structure */
typedef Profile* Profile_t;

/*! \brief Structure describing all profiles for a policy

This structure holds the profiles associated with a policy. Besides the base
profile and the measurement profiles it contains the ID of the optimal profile
and the profile iteration counter.
*/
typedef struct {
    int id; /**< \brief Identifier of the policy's profiles. Same as in Treedata->policies */
    int cur_profile; /**< \brief ID of current profile */
    int profile_iters; /**< \brief Number of iterations for each profile */
    int cur_profile_iters; /**< \brief Iteration counter for each profile */
    int opt_profile; /**< \brief ID of optimal profile */
    int num_profiles; /**< \brief Number of profiles */
    int num_values; /**< \brief Number of values per profile */
    Profile_t  profiles[LOOP_ADAPT_MAX_POLICY_PROFILES]; /**< \brief Storage for the measurement profiles */
    Profile_t  base_profile; /**< \brief Storage for the base profile */
} PolicyProfile;
/*! \brief Pointer to a PolicyProfile structure */
typedef PolicyProfile* PolicyProfile_t;

/*! \brief Structure associated with a node in the topology tree

This structure holds data for a tree node. Each tree node has such a structure.
It contains the profile data as well as the parameters added to the node.
*/
typedef struct {
    int cur_policy; /**< \brief Currently active policy */
    int num_policies; /**< \brief Number of policies (length of policies) */
    int count; /**< \brief How many processing units have processed this node */
    int num_pes; /**< \brief How many processing units are below this node */
    int done; /**< \brief Are we done with the profiles? */
    pthread_mutex_t lock; /**< \brief Safety lock */
    pthread_cond_t cond; /**< \brief Safety condition */
    PolicyProfile_t* policies; /**< \brief Storage for policy profiles */
    GHashTable* param_hash; /**< \brief Hash table paramname -> Nodeparameter_t */
// Currently unused
//    int num_events;
//    Nodeevent_t *events;
} Nodevalues;
/*! \brief Pointer to a Nodevalues structure */
typedef Nodevalues* Nodevalues_t;



#endif
