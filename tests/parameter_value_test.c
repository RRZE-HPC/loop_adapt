#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>

#include <loop_adapt_parameter_value.h>

void test_param(ParameterValue p)
{
    loop_adapt_print_param_value(p);
    char* pstr = loop_adapt_param_value_str(p);

    printf("'%s' Type '%s'\n", pstr, loop_adapt_print_param_valuetype(p.type));
    free(pstr);
    loop_adapt_destroy_param_value(p);
}


void dummy() { ;; }

int main(int argc, char* argv[])
{
    ParameterValue p = DEC_NEW_INT_PARAM_VALUE(4);
    ParameterValue s = DEC_NEW_STR_PARAM_VALUE("performance");
    ParameterValue d = DEC_NEW_DOUBLE_PARAM_VALUE(3.11);
    ParameterValue f = DEC_NEW_FLOAT_PARAM_VALUE(42.0);
    ParameterValue c = DEC_NEW_CHAR_PARAM_VALUE('v');
    ParameterValue b = DEC_NEW_BOOL_PARAM_VALUE(TRUE);
    ParameterValue ptr = DEC_NEW_PTR_PARAM_VALUE(&dummy);

    test_param(p);
    test_param(s);
    test_param(d);
    test_param(f);
    test_param(c);
    test_param(b);
    test_param(ptr);

    ParameterValue copy;
    ParameterValue compare = DEC_NEW_STR_PARAM_VALUE("performance");
    loop_adapt_copy_param_value(s, &copy);
    if (!loop_adapt_equal_param_value(copy, compare))
    {
        printf("Compare failed\n");
    }
    test_param(copy);


    ParameterValue t = loop_adapt_new_param_value(LOOP_ADAPT_PARAMETER_TYPE_DOUBLE);
    test_param(t);

    return 0;
}
