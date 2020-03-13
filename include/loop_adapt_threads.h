#ifndef LOOP_ADAPT_THREADS_H
#define LOOP_ADAPT_THREADS_H

#include <loop_adapt_threads_types.h>

int loop_adapt_threads_initialize();

int loop_adapt_threads_register(int threadid);
ThreadData_t loop_adapt_threads_get();

int loop_adapt_threads_get_application_cpus(int** cpus);
int loop_adapt_threads_get_cpu(int instance);
int loop_adapt_threads_get_socket(int instance);

int loop_adapt_threads_finalize();


#endif /* LOOP_ADAPT_THREADS_H */
