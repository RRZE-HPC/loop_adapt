#ifndef LOOP_ADAPT_THREADS_H
#define LOOP_ADAPT_THREADS_H

#include <loop_adapt_threads_types.h>

int loop_adapt_threads_initialize();

int loop_adapt_threads_register(int threadid);
ThreadData_t loop_adapt_threads_get();
ThreadData_t loop_adapt_threads_getthread(int id);

int loop_adapt_threads_get_application_cpus(int** cpus);
int loop_adapt_threads_get_cpu(int instance);
int loop_adapt_threads_get_socket(int instance);

int loop_adapt_threads_finalize();


int loop_adapt_threads_register_inparallel_func(int(*pf)(void));
int loop_adapt_threads_in_parallel();
int loop_adapt_threads_get_count();

#endif /* LOOP_ADAPT_THREADS_H */
