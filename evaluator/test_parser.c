#include <stdlib.h>
#include <stdio.h>

#include "interface.h"


int main(int argc, char* argv[])
{
    char cond[] = "1 | 0";
    int ret = parse_condition(cond);
    printf("Res: %d\n", ret);
    return 0;
}
