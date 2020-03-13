#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <omp.h>

#include <loop_adapt.h>

#define ARRAY_SIZE 80000000
#define ITERATIONS 30

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
    a = (double *)malloc(ARRAY_SIZE * sizeof(double));
    b = (double *)malloc(ARRAY_SIZE * sizeof(double));
    c = (double *)malloc(ARRAY_SIZE * sizeof(double));
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = 0;
        b[i] = 2.0;
        c[i] = 2.0;
    }
    //timer_init();

    setenv("LA_CONFIG_TXT_INPUT", "../config/LOOP.txt", 1);
    setenv("LA_CONFIG_INPUT_TYPE", "0", 1);
    setenv("LA_CONFIG_OUTPUT_TYPE", "1", 1);

    loop_adapt_debug_level(2);

    LA_INIT;

    LA_REGISTER("LOOP", 5);
    LA_NEW_INT_PARAMETER("blksize", LOOP_ADAPT_SCOPE_SYSTEM, 1);

#pragma omp parallel
{
    max_threads = omp_get_num_threads();
    //LA_REGISTER_THREAD(omp_get_thread_num());
    //REGISTER_PARAMETER("LOOP", "blksize", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), NODEPARAMETER_INT, 5, 1, 20);
/*    int tmp[10] = {1,4,6,10,50,100,200,300,500,1000};*/
/*    REGISTER_PARAMETER_LIST("LOOP", "blksize", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), NODEPARAMETER_INT, 10, tmp);*/
}
    //REGISTER_PARAMETER("LOOP", LOOP_ADAPT_SCOPE_MACHINE, "nthreads", 0, NODEPARAMETER_INT, 1, 1, max_threads);
    //nr_threads = max_threads;

    LA_GET_INT_PARAMETER("blksize", blockSize);

    printf("Blocksize (before) %d\n", blockSize);

    printf("Set Blocksize to 2\n");
    LA_SET_INT_PARAMETER("blksize", 2);
    LA_GET_INT_PARAMETER("blksize", blockSize);
    printf("Blocksize (after) %d\n", blockSize);


    //timer_start(&t);
    LA_FOR("LOOP", j = 0, j < ITERATIONS, j++)
    {
        printf("Loop Body Begin %d\n", max_threads);

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
    //timer_stop(&t);
    LA_FINALIZE;
    free(a);
    free(b);
    free(c);
    return 0;

}
