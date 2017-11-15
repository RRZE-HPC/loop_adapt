
#include <stdlib.h>
#include <stdio.h>


#include <interface.h>

int parse_condition(char* cond)
{
    int res = 0;
    char* result;
    Parser prs;
    result = prs.parse(cond);
    res = atoi(result);
    return res;
}
