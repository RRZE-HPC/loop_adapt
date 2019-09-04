#ifndef LA_ARRAY_H
#define LA_ARRAY_H

#include <functional>

struct Array
{
    int xSize;
    int ySize;
    double *val;

    Array(int xSize_, int ySize_);
    ~Array();

    typedef std::function<double(int,int)> funSig;

    void fill(double Val, bool onlyInner=false);
    void fill(funSig func, bool onlyInner=false);
    void fill_left_boundary(funSig func);
    void fill_right_boundary(funSig func);
    void fill_top_boundary(funSig func);
    void fill_bottom_boundary(funSig func);

    //checks error to a function
    double l2Error(funSig func);
    void writeToFile(const char* filename);
};

#endif
