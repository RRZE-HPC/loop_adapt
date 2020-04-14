#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <dlfcn.h>

#include <syscall.h>
#include <pthread.h>

#include <error.h>
#include <loop_adapt_threads.h>
#include <map.h>
#include <loop_adapt_hwloc_tree.h>

#include <hwloc.h>

#define gettid() syscall(SYS_gettid)

static Map_t loop_adapt_threads = NULL;
static hwloc_topology_t loop_adapt_threads_tree = NULL;
pthread_mutex_t loop_adapt_threads_lock = PTHREAD_MUTEX_INITIALIZER;
/*! \brief  Taskset of the application */
static cpu_set_t loop_adapt_threads_cpuset;
static cpu_set_t loop_adapt_threads_cpuset_inuse;
/*! \brief  Taskset of the master thread of the application */
/*static cpu_set_t loop_adapt_cpuset_master;*/


static ThreadData_t _loop_adapt_new_threaddata()
{
    ThreadData_t tdata = malloc(sizeof(ThreadData));
    if (!tdata)
    {
        fprintf(stderr, "ERROR: Cannot allocate new thread data\n");
        return NULL;
    }
    memset(tdata, 0, sizeof(ThreadData));
    pthread_mutex_init(&tdata->lock, NULL);

    tdata->objidx = -1;
    tdata->cpu = -1;
    return tdata;
}

static void _loop_adapt_destroy_threaddata(void* ptr)
{
    if (ptr)
    {
        //destroy_map(threaddata->threads);
        ThreadData_t threaddata = (ThreadData_t)ptr;
        if (threaddata)
        {
            pthread_mutex_destroy(&threaddata->lock);

            memset(ptr, 0, sizeof(ThreadData));
            free(ptr);
        }
    }
}


int loop_adapt_threads_initialize()
{
    int err = 0;
    /* Initialize cpuset of application */
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Initialize 1st cpuset);
    CPU_ZERO(&loop_adapt_threads_cpuset);
    CPU_ZERO(&loop_adapt_threads_cpuset_inuse);
    err = sched_getaffinity(0, sizeof(cpu_set_t), &loop_adapt_threads_cpuset);
    if (err < 0)
    {
        ERROR_PRINT(Failed to get cpuset);
        return 1;
    }
    if (!loop_adapt_threads)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize thread hash map (pthread_self() to ThreadData_t));
        err = init_imap(&loop_adapt_threads, _loop_adapt_destroy_threaddata);
    }
    if (!loop_adapt_threads_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize thread tree);
        loop_adapt_copy_hwloc_tree(&loop_adapt_threads_tree);
    }
    return 0;
}

int loop_adapt_threads_register(int threadid)
{
    int err = 0;
    int i = 0, j = 0;
    int scope_count = 0;
    int pin_thread = 1;
    int tid = gettid();
    pthread_t pt = pthread_self();
    ThreadData_t tdata = NULL;
    if (!loop_adapt_threads)
    {
        //DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, No thread hash initialized);
        return -EINVAL;
    }
    // Check if thread is already registered in thread hash
    if (get_imap_by_key(loop_adapt_threads, (uint64_t)pt, (void**)&tdata) != 0)
    {
        tdata = _loop_adapt_new_threaddata();
        tdata->pthread = pt;
        tdata->thread = threadid;
        tdata->pid = getpid();
        tdata->tid = tid;
        CPU_ZERO(&tdata->cpuset);
        //tdata->cpu = sched_getcpu();
        for (i = 0; i < CPU_COUNT(&loop_adapt_threads_cpuset); i++)
        {
            if (CPU_ISSET(i, &loop_adapt_threads_cpuset) && (!CPU_ISSET(i, &loop_adapt_threads_cpuset_inuse)))
            {
                CPU_SET(i, &loop_adapt_threads_cpuset_inuse);
                tdata->cpu = i;
                break;
            }
        }

        // Here we pin our threads
        if (get_smap_size(loop_adapt_threads) > 0)
        {
            //TODO_PRINT(Ensure pinning to distinct CPUs);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Pinning thread %d to CPU %d, threadid, i);
            CPU_SET(tdata->cpu, &tdata->cpuset);
            err = sched_setaffinity(tid, sizeof(cpu_set_t), &tdata->cpuset);
            if (err < 0)
            {
                fprintf(stderr, "Failed to set cpuset (pinning) for thread %d\n", threadid);
            }
        }
        err = sched_getaffinity(tid, sizeof(cpu_set_t), &tdata->cpuset);
        if (err < 0)
        {
            fprintf(stderr, "Failed to get cpuset for thread %d\n", threadid);
        }

        // This gets for each scope the related ID in the topology tree, so that
        // we can walk up the tree much faster by not searching for the related
        // parent object in the hwloc siblings list for a type. Although the
        // hwloc tree is a tree, there are object types (like NUMA domain) which
        // are not in the direct path from hardware thread to tree root (machine type).
        for (i = 0; i < LOOP_ADAPT_NUM_SCOPES; i++)
        {
            tdata->scopeOffsets[i] = -1;
        }
        for (i = 0; i < LOOP_ADAPT_NUM_SCOPES; i++)
        {
            LoopAdaptScope_t scope = LoopAdaptScopeList[i];
            //DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Searching for hwloc object with type %s for thread %d, hwloc_obj_type_string(scope), threadid);
            scope_count = hwloc_get_nbobjs_by_type(loop_adapt_threads_tree, scope);
            for (j = 0; j < scope_count; j++)
            {
                hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_threads_tree, scope, j);
                if (obj->type == LOOP_ADAPT_SCOPE_THREAD && obj->os_index == tdata->cpu)
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, PU offset %d for thread %d, j, threadid);
                    tdata->objidx = j;
                }
                if (hwloc_bitmap_isset(obj->cpuset, obj->logical_index))
                {
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Scope %s offset %d/%d for thread %d, hwloc_obj_type_string(scope), j, obj->logical_index, threadid);
                    tdata->scopeOffsets[i] = j;
                    break;
                }
            }
        }
        if (tdata->pid == tid)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Adding master thread %d pt %lu cpu %d obj %d, threadid, (uint64_t)pt, tdata->cpu, tdata->objidx);
        }
        else
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Adding thread %d pt %lu cpu %d obj %d, threadid, (uint64_t)pt, tdata->cpu, tdata->objidx);
        }
        pthread_mutex_lock(&loop_adapt_threads_lock);
        add_imap(loop_adapt_threads, (uint64_t)pt, (void*)tdata);
        pthread_mutex_unlock(&loop_adapt_threads_lock);
    }
    else
    {
        //DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Thread with ID %d already registered, threadid);
    }
    return 0;
}

ThreadData_t loop_adapt_threads_get()
{
    ThreadData_t tdata = NULL;
    if (loop_adapt_threads)
    {
        pthread_t pt = pthread_self();
        //DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_INFO, Getting thread %lu, (uint64_t)pt);
        if (get_imap_by_key(loop_adapt_threads, (uint64_t)pt, (void**)&tdata) != 0)
        {
            ERROR_PRINT(Failed to get data for thread %lu, (uint64_t)pt);
        }
    }
    return tdata;
}

int loop_adapt_threads_finalize()
{
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize application cpusets);
    CPU_ZERO(&loop_adapt_threads_cpuset);
    CPU_ZERO(&loop_adapt_threads_cpuset_inuse);
    if (loop_adapt_threads)
    {
        pthread_mutex_lock(&loop_adapt_threads_lock);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize threads hash map);
        destroy_imap(loop_adapt_threads);
        loop_adapt_threads = NULL;
        pthread_mutex_unlock(&loop_adapt_threads_lock);
    }
    if (loop_adapt_threads_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Finalize threads tree);
        pthread_mutex_lock(&loop_adapt_threads_lock);
        hwloc_topology_destroy(loop_adapt_threads_tree);
        loop_adapt_threads_tree = NULL;
        pthread_mutex_unlock(&loop_adapt_threads_lock);
    }
}

int loop_adapt_threads_get_application_cpus(int** cpus)
{
    if (loop_adapt_threads_tree && CPU_COUNT(&loop_adapt_threads_cpuset) > 0)
    {
        int num_cpus = hwloc_get_nbobjs_by_type(loop_adapt_threads_tree, LOOP_ADAPT_SCOPE_THREAD);
        int* list = malloc(num_cpus * sizeof(int));
        if (!list)
        {
            return -ENOMEM;
        }
        int i = 0;
        int count = 0;
        for (i = 0; i < num_cpus; i++)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_threads_tree, LOOP_ADAPT_SCOPE_THREAD, i);
            if (obj && CPU_ISSET(obj->os_index, &loop_adapt_threads_cpuset))
            {
                list[count] = obj->os_index;
                count++;
            }
        }
        *cpus = list;
        return count;
    }
    return -EFAULT;
}

int loop_adapt_threads_get_cpu(int instance)
{
    int cpu = -1;
    if (loop_adapt_threads_tree && CPU_COUNT(&loop_adapt_threads_cpuset) > 0)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_threads_tree, LOOP_ADAPT_SCOPE_THREAD, instance);
        if (obj && CPU_ISSET(obj->os_index, &loop_adapt_threads_cpuset))
        {
            cpu = obj->os_index;
        }
    }
    return cpu;
}

int loop_adapt_threads_get_socket(int instance)
{
    int socket = -1;
    if (loop_adapt_threads_tree)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_threads_tree, LOOP_ADAPT_SCOPE_SOCKET, instance);
        if (obj)
        {
            socket = obj->os_index;
        }
    }
    return socket;
}

