#ifndef LOOP_ADAPT_THREADS_TYPES_H
#define LOOP_ADAPT_THREADS_TYPES_H

#include <sys/types.h>
#include <pthread.h>
#include <loop_adapt_scopes.h>



typedef struct {
    int objidx;
    int cpu;
    int thread;
    pid_t pid;
    pid_t tid;
    cpu_set_t cpuset;
    pthread_t pthread;
    pthread_mutex_t lock;
    int scopeOffsets[LOOP_ADAPT_NUM_SCOPES];
} ThreadData;
typedef ThreadData* ThreadData_t;



#endif /* LOOP_ADAPT_THREADS_TYPES_H */
