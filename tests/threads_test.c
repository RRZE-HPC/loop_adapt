#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>

int loop_adapt_verbosity = 0;
#include <loop_adapt_threads.h>



int main(int argc, char* argv[])
{
    loop_adapt_threads_initialize();

    loop_adapt_threads_finalize();

    loop_adapt_threads_initialize();
    loop_adapt_threads_register(0);
    ThreadData_t t = loop_adapt_threads_get();
    if (!t)
    {
        printf("Failed to get thread data\n");
    }
    loop_adapt_threads_register(0);
    loop_adapt_threads_finalize();
    return 0;
}
