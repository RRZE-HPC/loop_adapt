#include "Array.h"
#include <math.h>
#include <fstream>

Array::Array(int xSize_, int ySize_):xSize(xSize_),ySize(ySize_)
{

    val=new double[xSize*ySize];
    fill(0);
}

Array::~Array()
{
    delete[] val;
}

void Array::fill(double value, bool onlyInner)
{
    int shift=0;

    if(onlyInner)
    {
        shift=1;
    }

#pragma omp parallel for
    for(int i=shift; i<xSize-shift; ++i)
    {
        for(int j=shift; j<ySize-shift; ++j)
        {
            val[i*ySize+j] = value;
        }
    }
}

void Array::fill(funSig func, bool onlyInner)
{
    int shift=0;

    if(onlyInner)
    {
        shift=1;
    }

#pragma omp parallel for
    for(int i=shift; i<xSize-shift; ++i)
    {
        for(int j=shift; j<ySize-shift; ++j)
        {
            val[i*ySize+j] = func(j,i);
        }
    }
}

void Array::fill_left_boundary(funSig func)
{
#pragma omp parallel for
    for(int i=0; i<xSize; ++i)
    {
        val[i*ySize] = func(0,i);
    }
}

void Array::fill_right_boundary(funSig func)
{
#pragma omp parallel for
    for(int i=0; i<xSize; ++i)
    {
        val[i*ySize+(ySize-1)] = func((ySize-1),i);
    }
}

void Array::fill_top_boundary(funSig func)
{
#pragma omp parallel for
    for(int i=0; i<ySize; ++i)
    {
        val[i+(xSize-1)*ySize] = func(i,(xSize-1));
    }
}

void Array::fill_bottom_boundary(funSig func)
{
#pragma omp parallel for
    for(int i=0; i<ySize; ++i)
    {
        val[i] = func(i,0);
    }
}

double Array::l2Error(funSig func)
{
    double err=0;

    for(int i=0; i<xSize; ++i)
    {
        for(int j=0; j<ySize; ++j)
        {
            double curr_val=(val[i*ySize+j]-func(j,i));
            err+=curr_val*curr_val;
        }
    }

    //TODO: take sqrt
    return sqrt(err)/(xSize*ySize);
}

void Array::writeToFile(const char* filename)
{
    std::fstream file;
    file.open(filename, std::fstream::out);

    for(int i=0; i<xSize; ++i)
    {
        for(int j=0; j<ySize; ++j)
        {
            file<<val[i*ySize+j]<<"\n";
        }
        file<<"\n";
    }

    file.close();
}

