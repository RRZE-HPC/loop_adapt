#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <omp.h>

#include <loop_adapt.h>

#include <likwid.h>

#define ARRAY_SIZE 80000000
#define ITERATIONS 15

#define TEST_LOOP(s, var, start, cond, inc) \
    for (var = (start); cond; var += inc)

void dummy(void)
{
    return;
}

int main(int argc, char* argv[])
{
    int i, j, k;
    double *a, *b, *c;
    double s = 0;
    int blockSize = 0;
    int edata = 1;
    int max_threads = 1;
    int nr_threads = 1;
    TimerData t;
    a = malloc(ARRAY_SIZE * sizeof(double));
    b = malloc(ARRAY_SIZE * sizeof(double));
    c = malloc(ARRAY_SIZE * sizeof(double));
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = 0;
        b[i] = 2.0;
        c[i] = 2.0;
    }
    timer_init();


    REGISTER_THREAD_COUNT_FUNC(omp_get_num_threads);
    REGISTER_THREAD_ID_FUNC(omp_get_thread_num);
    REGISTER_CPU_ID_FUNC(sched_getcpu);
    REGISTER_LOOP("LOOP");
    REGISTER_POLICY("LOOP", "POL_BLOCKSIZE", 9, 1);
    REGISTER_SEARCHALGO("LOOP", "POL_BLOCKSIZE", "SEARCH_BASIC");
    
    //REGISTER_NEW("LOOP", "POL_BLOCKSIZE", "SEARCH_BASIC", 1)
    //REGISTER_NEW("LOOP", "POL_OMPTHREADS", "SEARCH_BASIC", 2)
    //REGISTER_POLICY("LOOP", "POL_OMPTHREADS", 7, 1);
    //REGISTER_EVENT("LOOP", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), "edata", "edata", NODEPARAMETER_INT, &edata);

#pragma omp parallel
{
    max_threads = omp_get_num_threads();
    //REGISTER_PARAMETER("LOOP", "blksize", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), NODEPARAMETER_INT, 5, 1, 20);
    int tmp[10] = {1,4,6,10,50,100,200,300,500,1000};
    REGISTER_PARAMETER_LIST("LOOP", "blksize", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), NODEPARAMETER_INT, 10, tmp);
}
    //REGISTER_PARAMETER("LOOP", LOOP_ADAPT_SCOPE_MACHINE, "nthreads", 0, NODEPARAMETER_INT, 1, 1, max_threads);
    //nr_threads = max_threads;

    timer_start(&t);
    LA_FOR("LOOP", j = 0, j < ITERATIONS, j++)
    {
        printf("Loop Body Begin\n");
        //GET_INT_PARAMETER("LOOP", nr_threads, LOOP_ADAPT_SCOPE_MACHINE, "nthreads", 0);
        GET_INT_PARAMETER("LOOP", "blksize", blockSize, LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu());
#pragma omp parallel private(i) num_threads(max_threads)
{

#pragma omp for
        for (i = 0; i < ARRAY_SIZE; i++)
        {
            for (k = 0; k < blockSize; k++)
                a[i] =+ (b[i]*b[i])+(0.5*b[i])+(c[i]*c[i])+(0.5*c[i]);
            if (a[i] < 0)
                dummy();
        }
}
        printf("Loop Body End\n");
    }
    timer_stop(&t);
    free(a);
    free(b);
    free(c);
    return 0;

}

