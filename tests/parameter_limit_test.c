#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>

#include <loop_adapt_parameter_value.h>
#include <loop_adapt_parameter_limit.h>

void test_limit(ParameterValueLimit l)
{
    printf("Type %s\n", loop_adapt_param_limit_type(l.type));
    char* c = loop_adapt_param_limit_str(l);
    printf("%s\n", c);
    free(c);
}

int main(int argc, char* argv[])
{
    int i = 0;
    ParameterValueLimit ir = DEC_NEW_INTRANGE_PARAM_LIMIT(0, 10, 2);
    test_limit(ir);
    ParameterValueLimit ir_rev = DEC_NEW_INTRANGE_PARAM_LIMIT(10, 0, -2);
    test_limit(ir_rev);
    ParameterValueLimit dr = DEC_NEW_DOUBLERANGE_PARAM_LIMIT(0, 10, 2);
    test_limit(dr);
    ParameterValueLimit il = DEC_NEW_LIST_PARAM_LIMIT();
    test_limit(il);
    ParameterValueLimit dl = DEC_NEW_LIST_PARAM_LIMIT();
    test_limit(dl);
    for (i = 0; i < 5; i++)
    {
        ParameterValue v = DEC_NEW_INT_PARAM_VALUE(i);
        loop_adapt_add_param_limit_list(&il, v);
        test_limit(il);
        loop_adapt_destroy_param_value(v);
    }
    for (i = 0; i < 5; i++)
    {
        ParameterValue v = DEC_NEW_DOUBLE_PARAM_VALUE(i);
        loop_adapt_add_param_limit_list(&dl, v);
        test_limit(dl);
        loop_adapt_destroy_param_value(v);
    }

    ParameterValueLimit tolist;
    loop_adapt_param_limit_tolist(ir, &tolist);
    test_limit(tolist);

    ParameterValue p = DEC_NEW_INT_PARAM_VALUE(4);
    if (loop_adapt_check_param_limit(p, il) == 1)
    {
        printf("In range\n");
    }
    if (loop_adapt_check_param_limit(p, ir) == 1)
    {
        printf("In range\n");
    }
    if (loop_adapt_check_param_limit(p, ir_rev) == 1)
    {
        printf("In range\n");
    }
    loop_adapt_destroy_param_value(p);
    ParameterValue d = DEC_NEW_DOUBLE_PARAM_VALUE(2.0);
    if (loop_adapt_check_param_limit(d, ir) == 1)
    {
        printf("Cast + In range\n");
    }
    loop_adapt_destroy_param_value(d);

    loop_adapt_destroy_param_limit(ir);
    loop_adapt_destroy_param_limit(dr);
    loop_adapt_destroy_param_limit(il);
    loop_adapt_destroy_param_limit(dl);
    loop_adapt_destroy_param_limit(tolist);
    return 0;
}
