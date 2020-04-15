#ifndef LOOP_ADAPT_TYPES_H
#define LOOP_ADAPT_TYPES_H

#include <pthread.h>
#include <likwid.h>
#include <map.h>

#include <loop_adapt.h>


typedef unsigned int boolean;


/*! \brief Status of a thread */
typedef enum {
    LOOP_ADAPT_THREAD_PAUSE = 0, /**< \brief Thread is not running */
    LOOP_ADAPT_THREAD_RUN, /**< \brief Thread is running */
} LoopThreadState;

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
    int current_config_id; 
    cpu_set_t cpuset; /**< \brief Current CPUset */
    LoopThreadState state; /**< \brief Status of the thread */
} LoopThreadData;
/*! \brief Pointer to a ThreadData structure */
typedef LoopThreadData* LoopThreadData_t;

/*! \brief Status of a loop */
typedef enum {
    LOOP_STARTED = 0, /**< \brief Loop is running */
    LOOP_STOPPED = 1, /**< \brief Loop is stopped */
} LoopRunStatus;

/*! \brief Base loop structure associated with a topology tree

This structure is used as base information about a loop and stored in the
loop's topology tree. This way we just need to resolve the loop name to the
topology tree and have all required data about the loop.
*/
typedef struct {
    char* filename; /**< \brief Filename where the loop is defined */
    int linenumber; /**< \brief Line number in filename where the loop is defined */
    int num_iterations;
    int max_iterations;
//    int num_policies; /**< \brief Number of registered policies */
    LoopRunStatus status; /**< \brief State of the loop */
//   int cur_policy_id; /**< \brief ID of currently active policy */
//    Policy_t cur_policy; /**< \brief Pointer to the currently active policy */
//    Policy_t *policies; /**< \brief List of all policies */
    pthread_mutex_t lock; /**< \brief Lock for tree manipulation */
//    Map_t threads; /**< \brief Hashtable mapping thread ID to ThreadData */
    int current_config_id;
    int announced;
} LoopData;
/*! \brief Pointer to a Treedata structure */
typedef LoopData* LoopData_t;


#endif /* LOOP_ADAPT_TYPES_H */
