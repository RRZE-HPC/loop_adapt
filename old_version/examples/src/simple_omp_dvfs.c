#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <omp.h>

#include <loop_adapt.h>

#include <likwid.h>

#define ARRAY_SIZE 80000000
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
    REGISTER_LOOP("DVFS");
    REGISTER_POLICY("DVFS", "POL_DVFS", 5, 2);
    REGISTER_SEARCHALGO("DVFS", "POL_DVFS", "SEARCH_BASIC");

#pragma omp parallel
{
    max_threads = omp_get_num_threads();
    double tmp[8] = { 0.8, 1.2, 1.5, 1.9, 2.3, 2.7, 3, 3.4};
    REGISTER_PARAMETER_LIST("DVFS", "cfreq", LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu(), NODEPARAMETER_DOUBLE, 8, tmp);
}


    timer_start(&t);
    LA_FOR("DVFS", j = 0, j < ITERATIONS, j++)
    {
        printf("Loop Body Begin\n");
        GET_INT_PARAMETER("DVFS", "cfreq", blockSize, LOOP_ADAPT_SCOPE_MACHINE, sched_getcpu());
#pragma omp parallel private(i) num_threads(max_threads)
{
        //int cpu = sched_getcpu();

#pragma omp for 
        for (i = 0; i < ARRAY_SIZE; i++)
        {
            a[i] = (b[i]*b[i])+(0.5*b[i])+(c[i]*c[i])+(0.5*c[i]);
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

