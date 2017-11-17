#include <omp.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "Array.h"
#include <math.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

//sin(2*pi*x)
double uFn(int i, int j, double dx, double dy)
{
    return sin(2*M_PI*(i*dx));
}

//-4.0*PI*PI*sin(2*PI*x)
double solFn(int i, int j, double dx, double dy)
{
    return -4.0*M_PI*M_PI*sin(2*M_PI*(i*dx));
}


void kernel(Array* phi_new, Array* phi,  double centre_coeff, double dx_coeff, double dy_coeff, int nBlocks, int blockSize)
{
    int xSize = phi->xSize;
    int ySize = phi->ySize;
    for(int bs=0; bs<(nBlocks+1); ++bs)
    {

#pragma omp parallel for schedule(runtime)
        for(int i=1;i<xSize-1;++i)
        {
#pragma simd
            for(int j=MAX(1,(bs)*blockSize); j<MIN((bs+1)*blockSize,ySize-1); ++j)
            {

                phi_new->val[i*ySize+j]= (centre_coeff*phi->val[i*ySize+j] + dy_coeff*(phi->val[(i+1)*ySize+j]+phi->val[(i-1)*ySize+j])+ dx_coeff*(phi->val[i*ySize+j+1] +phi->val[i*ySize+j-1]));
            }
        }
    }
}

int main(const int argc, char* const argv[])
{
    if(argc < 4)
    {
        printf("Usage: %s outerDim innerDim iter blockSize(default:-1) validate(optional)\n", argv[0]);
        return 0;
    }


    int xSize = atoi(argv[1]);
    int ySize = atoi(argv[2]);
    int niter = atoi(argv[3]); //min. iter count
    int blockSize = -1;
    int validate = 0;

    if(argc>4)
    {
        blockSize = atoi(argv[4]);
    }
    if(argc>5)
    {
        if(argv[5])
            validate=1;
    }

    int cacheSize = 262144; //cache size to be blocked for

    int thread_num = 1;

#pragma omp parallel
    {
#pragma omp single
        {
            thread_num = omp_get_num_threads();
        }
    }

    int sf = 2;

    if(blockSize==-1)
    {
        blockSize = cacheSize/(3.0*8*sf);
    }

    int nBlocks = (int) (ySize/((double)blockSize));

    Array *phi = new Array(xSize, ySize);
    Array *phi_new = new Array(xSize, ySize);

    double dx = 1.0/(xSize);
    double dy = 1.0/(ySize-1);
    double x_coeff = (xSize-1)*(xSize-1);
    double y_coeff = (ySize-1)*(ySize-1);
    double centre_coeff = -2.0*(x_coeff + y_coeff);

    phi->fill(std::bind(uFn,std::placeholders::_1, std::placeholders::_2, dx, dy));

    if(validate)
    {
#ifdef WRITE_FILE
        phi->writeToFile("init.txt");
#endif
        kernel(phi_new, phi, centre_coeff, x_coeff, y_coeff, nBlocks, blockSize);
        double error = phi_new->l2Error(std::bind(uFn,std::placeholders::_1, std::placeholders::_2, dx, dy));
#ifdef WRITE_FILE
        phi_new->writeToFile("soln.txt");
#endif
        printf("L2 error = %f\n",error);
    }


    struct timeval tym;
    gettimeofday(&tym,NULL);
    double wcs=tym.tv_sec+(tym.tv_usec*1e-6);
    double wce=wcs;

    for(int iter=1;iter<=niter;++iter)
    {
        kernel(phi_new, phi, centre_coeff, x_coeff, y_coeff, nBlocks, blockSize);
        //swap arrays
        double* temp = phi_new->val;
        phi_new->val = phi->val;
        phi->val = temp;
    }

    gettimeofday(&tym,NULL);
    wce=tym.tv_sec+(tym.tv_usec*1e-6);
    double mlups = (((double)xSize)*((double)ySize)*niter*1.0e-6)/(wce-wcs);

    printf("\n%5s\t%10s\t%10s\t%10s\t\t%10s\n","Thread", "xSize", "ySize", "BlockSize", "MLUPS");

    printf("%4d\t%10d\t%10d\t%10d\t\t%8.2f\n",thread_num, xSize, ySize, blockSize, mlups);

    delete phi;
    delete phi_new;

    return 0;
}
