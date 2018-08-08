#define _GNU_SOURCE
#include<omp.h>
#include <stdlib.h>
#include <stdio.h>
#include<sys/time.h>

#include <sched.h>

#include <loop_adapt.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define NUM_PROFILES 7
#define NUM_ITERATIONS 20
#define NUM_THREADS 1

int main()
{
    /*************** Configurations *************************/
    int niter = 100; //min. iter count

    int startSize = 1000;
    int endSize = 20000;
    int incSize = 1000;
    int cacheSize = 262144; //cache size to be blocked for
    /********************************************************/

   printf("%5s\t%5s\t%10s\t%10s\t\t%10s\n","Iter", "Thread", "Size", "BlockSize", "MLUPS");

    int thread_num = 1;

#pragma omp parallel
    {
#pragma omp single
        {
            thread_num = omp_get_num_threads();
        }
    }

    int sf = 2;

    //int blockSize = (cacheSize/(3.0*8*sf));
    //int ablock[NUM_THREADS] = {blockSize};
    REGISTER_THREAD_COUNT_FUNC(omp_get_num_threads);
    REGISTER_THREAD_ID_FUNC(sched_getcpu);
    REGISTER_LOOP("SWEEP");
    REGISTER_POLICY("SWEEP", "POL_BLOCKSIZE", NUM_PROFILES, 10);
    //REGISTER_POLICY("SWEEP", "POL_DVFS", NUM_PROFILES, 1);


    int Size = 10000;

/*    for(int Size= startSize; Size<endSize; Size+=incSize)*/
/*    {*/
        
        double *phi = (double*) malloc(Size*Size*sizeof(double));
        double *phi_new = (double*) malloc(Size*Size*sizeof(double));
        memset(phi, 0, Size*Size*sizeof(double));

    //for (int iteration = 0; iteration < NUM_ITERATIONS ; iteration++)
/*    int iteration;*/
/*    LA_FOR("SWEEP", iteration=0, iteration < NUM_ITERATIONS, iteration++)*/
/*    {*/

#pragma omp parallel
{
    int cpu = sched_getcpu();
    REGISTER_PARAMETER("SWEEP", LOOP_ADAPT_SCOPE_THREAD, "blksize", cpu, NODEPARAMETER_INT, (cacheSize/(3.0*8*2)), (cacheSize/(3.0*8*3)), (cacheSize/(3.0*8)));
/*    REGISTER_PARAMETER("SWEEP", LOOP_ADAPT_SCOPE_SOCKET, "cfreq", cpu, NODEPARAMETER_DOUBLE, 0, 0, 0);*/
/*    REGISTER_PARAMETER("SWEEP", LOOP_ADAPT_SCOPE_SOCKET, "ufreq", cpu, NODEPARAMETER_DOUBLE, 0, 0, 0);*/



#pragma omp for schedule(runtime)
        for(int i=1;i<Size-1;++i)
        {
            for(int j=1;j<Size-1;++j)
            {
                phi[i*Size+j]=1;
                phi_new[i*Size+j]=1;
            }
        }
}

        int ctr = 0;

        struct timeval tym;
        gettimeofday(&tym,NULL);
        double wcs=tym.tv_sec+(tym.tv_usec*1e-6);
        double wce=wcs;

        //while((wce-wcs) < 0.1)
        {
            int iter;
            LA_FOR("SWEEP", iter=1, iter<=niter, ++iter)
            //for(int iter=1;iter<=niter;++iter)
            {
                printf("Iter %d\n", iter);
#pragma omp parallel
{
                int blockSize;
                int cpu = sched_getcpu();
                GET_INT_PARAMETER("SWEEP", blockSize, LOOP_ADAPT_SCOPE_THREAD, "blksize", cpu);
                int nBlocks = (int) (Size/((double)blockSize));
                for(int bs=0; bs<(nBlocks+1); ++bs)
                {

#pragma omp for schedule(runtime)
                    for(int i=1;i<Size-1;++i)
                    {
#pragma simd
                        for(int j=MAX(1,(bs)*blockSize); j<MIN((bs+1)*blockSize,Size-1); ++j)
                        {

                            phi_new[i*Size+j]= (-4)*phi[i*Size+j] + phi[(i+1)*Size+j]+phi[(i-1)*Size+j]+phi[i*Size+j+1] +phi[i*Size+j-1];
                        }
                    }
                }
}
                //swap arrays
                double* temp = phi_new;
                phi_new = phi;
                phi = temp;
            }
            ++ctr;
            gettimeofday(&tym,NULL);
            wce=tym.tv_sec+(tym.tv_usec*1e-6);
        }

        double size_d = Size;
        double mlups = (size_d*size_d*niter*ctr*1.0e-6)/(wce-wcs);

        char thread_num_str[6];
        sprintf(thread_num_str, "%d", thread_num);
        char Size_str[11];
        sprintf(Size_str, "%d", Size);
        char blockSize_str[11];
        //sprintf(blockSize_str, "%d", blockSize);
        char mlup_str[11];
        sprintf(mlup_str, "%6.4f", mlups);
        char time_str[11];
        sprintf(time_str, "%6.4f", (wce-wcs));
        printf("%5s\t%10s\t%10s\t\t%10s\t%10s\n",thread_num_str, Size_str, blockSize_str, mlup_str, time_str);

/*    }*/
    printf("App ends\n");
/*    for (int i = 0; i < Size ; i++)*/
/*    {*/
/*        for (int j= 0; j < Size; j++)*/
/*        {*/
/*            if (phi[i*Size+j] != 0)*/
/*                printf("%.1f ", phi[i*Size+j]);*/
/*        }*/
/*        printf("\n");*/
/*    }*/
    
    free(phi);
    free(phi_new);

    return 0;
}
