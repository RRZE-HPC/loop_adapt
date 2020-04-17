#ifndef LOOP_ADAPT_INTERNAL_H
#define LOOP_ADAPT_INTERNAL_H

#include <loop_adapt_types.h>

int loop_adapt_get_loop_parameter(char* string, struct bstrList* parameters);
int loop_adapt_get_loopdata(char* string, LoopData_t *loopdata);

#endif