#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <omp.h>

#include <loop_adapt.h>

#include <likwid.h>

#define ARRAY_SIZE 60000000
#define ITERATIONS 20

#define TEST_LOOP(s, var, start, cond, inc) \
    for (var = (start); cond; var += inc)

void dummy(void)
{
    return;
}

int main(int argc, char* argv[])
{
    int i, j;
    double *a, *b;
    double s = 0;
    int blockSize = 0;
    int edata = 1;
    int max_threads = 1;
    int nr_threads = 1;
    TimerData t;
    a = malloc(ARRAY_SIZE * sizeof(double));
    b = malloc(ARRAY_SIZE * sizeof(double));
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = 0;
        b[i] = 2.0;
    }
    timer_init();


    REGISTER_THREAD_COUNT_FUNC(omp_get_num_threads);
    REGISTER_THREAD_ID_FUNC(sched_getcpu);
    REGISTER_LOOP("LOOP");
    REGISTER_POLICY("LOOP", "POL_BLOCKSIZE", 5, 2);
    //REGISTER_POLICY("LOOP", "POL_OMPTHREADS", 3, 2);
    //REGISTER_EVENT("LOOP", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), "edata", "edata", NODEPARAMETER_INT, &edata);
    
#pragma omp parallel
{
    max_threads = omp_get_num_threads();
    REGISTER_PARAMETER("LOOP", LOOP_ADAPT_SCOPE_THREAD, "blksize", sched_getcpu(), NODEPARAMETER_INT, 5, 1, 10);
}
    REGISTER_PARAMETER("LOOP", LOOP_ADAPT_SCOPE_MACHINE, "nthreads", 0, NODEPARAMETER_INT, max_threads, 1, max_threads);
    nr_threads = max_threads;

    timer_start(&t);
    LA_FOR("LOOP", j = 0, j < ITERATIONS, j++)
    {
        GET_INT_PARAMETER("LOOP", nr_threads, LOOP_ADAPT_SCOPE_MACHINE, "nthreads", 0);
        //printf("Running with %d threads\n", nr_threads);
#pragma omp parallel private(i) num_threads(nr_threads)
{
        int cpu = sched_getcpu();
    
        GET_INT_PARAMETER("LOOP", blockSize, LOOP_ADAPT_SCOPE_THREAD, "blksize", cpu);
        
#pragma omp for 
        for (i = 0; i < ARRAY_SIZE; i++)
        {
            
            a[i] = (b[i]*b[i])+(0.5*b[i]);
            if (a[i] < 0)
                dummy();
        }
}
    }
    timer_stop(&t);
    printf("Loop time %f\n", timer_print(&t));
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        s += a[i] + 2;
    }

    printf("sum %f\n", s);
    return 0;

}

