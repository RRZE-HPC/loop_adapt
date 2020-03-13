#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <error.h>

#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_parameter_value.h>

ParameterValue loop_adapt_new_param_value(ParameterValueType_t valuetype)
{
    ParameterValue value;
    switch(valuetype)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
            value.value.ival = 0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
            value.value.uval = 0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
            value.value.lval = 0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
            value.value.ulval = 0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
            value.value.dval = 0.0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
            value.value.fval = 0.0;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
            value.value.bval = FALSE;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
            value.value.dval = '0';
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            value.value.sval = NULL;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            value.value.pval = NULL;
            break;
        default:
            fprintf(stderr, "Invalid parameter value type\n");
            break;
    }
    value.type = valuetype;
    return value;
}

int loop_adapt_copy_param_value(ParameterValue in, ParameterValue *out)
{
    int err = 0;
    int strlength = 0;
    switch(in.type)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
            out->value.ival = in.value.ival;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
            out->value.uval = in.value.uval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
            out->value.lval = in.value.lval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
            out->value.ulval = in.value.ulval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
            out->value.dval = in.value.dval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
            out->value.fval = in.value.fval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
            out->value.bval = in.value.bval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
            out->value.cval = in.value.cval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            out->value.pval = in.value.pval;
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            strlength = strlen(in.value.sval);
            out->value.sval = malloc((strlength+2) * sizeof(char));
            err = snprintf(out->value.sval, strlength+1, "%s", in.value.sval);
            if (err > 0)
            {
                out->value.sval[err] = '\0';
            }
            break;
        default:
            fprintf(stderr, "Invalid parameter value type\n");
            return 1;
            break;
    }
    out->type = in.type;
    return 0;
}

void loop_adapt_print_param_value(ParameterValue v)
{
    switch(v.type)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
            printf("%d (int)\n", v.value.ival);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
            printf("%u (uint)\n", v.value.uval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
            printf("%ld (long)\n", v.value.lval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
            printf("%lu (ulong)\n", v.value.ulval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            printf("%s (string)\n", v.value.sval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
            printf("%f (double)\n", v.value.dval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
            printf("%f (float)\n", v.value.fval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
            printf("%u (bool)\n", v.value.bval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
            printf("%c (char)\n", v.value.cval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            printf("%p (ptr)\n", v.value.pval);
            break;
        default:
            printf("Invalid type\n");
            break;
    }
}

void loop_adapt_destroy_param_value(ParameterValue v)
{
    switch(v.type)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            if (v.value.sval && ((void*)v.value.sval-(void*)loop_adapt_destroy_param_value) > 0x10000) free(v.value.sval);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            break;
        default:
            printf("Invalid type\n");
            break;
    }
}

char* loop_adapt_print_param_valuetype(ParameterValueType_t t)
{
    switch(t)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
            return "INT";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
            return "UINT";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
            return "LONG";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
            return "ULONG";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
            return "DOUBLE";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
            return "FLOAT";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
            return "BOOL";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
            return "CHAR";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            return "STR";
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            return "PTR";
            break;
        default:
            return "INVALID";
            break;
    }
    return NULL;
}

char* loop_adapt_param_value_str(ParameterValue v)
{
    int ret = 0;
    int slen = 51;
    if (v.type == LOOP_ADAPT_PARAMETER_TYPE_STR)
    {
        slen = strlen(v.value.sval)+2;
    }
    char *s = malloc(slen * sizeof(char));
    if (s)
    {
        switch(v.type)
        {
            case LOOP_ADAPT_PARAMETER_TYPE_INT:
                ret = snprintf(s, slen-1, "%d", v.value.ival);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_UINT:
                ret = snprintf(s, slen-1, "%u", v.value.uval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_LONG:
                ret = snprintf(s, slen-1, "%ld", v.value.lval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
                ret = snprintf(s, slen-1, "%lu", v.value.ulval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
                ret = snprintf(s, slen-1, "%f", v.value.dval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
                ret = snprintf(s, slen-1, "%f", v.value.fval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
                ret = snprintf(s, slen-1, "%u", v.value.bval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
                ret = snprintf(s, slen-1, "%c", v.value.cval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_STR:
                ret = snprintf(s, slen, "%s", v.value.sval);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_PTR:
                ret = snprintf(s, slen, "%p", v.value.pval);
                break;
            default:
                ret = snprintf(s, slen, "INVALID");
                break;
        }
        if (ret >= 0)
        {
            s[ret] = '\0';
        }
    }
    return s;
}



int loop_adapt_add_param_value(ParameterValue a, ParameterValue b, ParameterValue *result)
{
    if (a.type == b.type)
    {
        switch(a.type)
        {
            case LOOP_ADAPT_PARAMETER_TYPE_INT:
                result->value.ival = a.value.ival + b.value.ival;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_UINT:
                result->value.uval = a.value.uval + b.value.uval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_LONG:
                result->value.lval = a.value.lval + b.value.lval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
                result->value.ulval = a.value.ulval + b.value.ulval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
                result->value.dval = a.value.dval + b.value.dval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
                result->value.fval = a.value.fval + b.value.fval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
                result->value.bval = (a.value.bval + b.value.bval > 0 ? TRUE : FALSE);
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
                result->value.cval = a.value.cval + b.value.cval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_STR:
                //result->value.ival = a.value.ival + b.value.ival;
                fprintf(stderr, "TYPE ERROR: Summation of strings not possible\n");
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_PTR:
                //result->value.ival = a.value.ival + b.value.ival;
                fprintf(stderr, "TYPE ERROR: Summation of pointers not possible\n");
                break;
            default:
                fprintf(stderr, "TYPE ERROR: Summation of invalid values not possible\n");
                break;
        }
        result->type = a.type;
    }
    return -EINVAL;
}

int loop_adapt_equal_param_value(ParameterValue a, ParameterValue b)
{
    if (a.type == b.type)
    {
        switch(a.type)
        {
            case LOOP_ADAPT_PARAMETER_TYPE_INT:
                return a.value.ival == b.value.ival;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_UINT:
                return a.value.uval == b.value.uval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_LONG:
                return a.value.lval == b.value.lval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
                return a.value.ulval == b.value.ulval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
                return a.value.dval == b.value.dval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
                return a.value.fval == b.value.fval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
                return a.value.bval == b.value.bval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
                return a.value.cval == b.value.cval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_STR:
                return strcmp(a.value.sval, b.value.sval) == 0;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_PTR:
                return a.value.pval == b.value.pval;
                break;
            default:
                fprintf(stderr, "TYPE ERROR: Equality check of invalid values not possible\n");
                break;
        }
    }
    return -EINVAL;
}

int loop_adapt_less_param_value(ParameterValue a, ParameterValue b)
{
    if (a.type == b.type)
    {
        switch(a.type)
        {
            case LOOP_ADAPT_PARAMETER_TYPE_INT:
                return a.value.ival < b.value.ival;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_UINT:
                return a.value.uval < b.value.uval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_LONG:
                return a.value.lval < b.value.lval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
                return a.value.ulval < b.value.ulval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
                return a.value.dval < b.value.dval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
                return a.value.fval < b.value.fval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
                return a.value.bval < b.value.bval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
                return a.value.cval < b.value.cval;
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_STR:
                fprintf(stderr, "TYPE ERROR: Less check of strings not possible\n");
                break;
            case LOOP_ADAPT_PARAMETER_TYPE_PTR:
                return a.value.pval < b.value.pval;
                break;
            default:
                fprintf(stderr, "TYPE ERROR: Less check of invalid values not possible\n");
                break;
        }
    }
    return -EINVAL;
}

#define LOOP_ADAPT_CAST_SWITCH(BASE, TYPE) \
    switch(a->type) \
    { \
        case LOOP_ADAPT_PARAMETER_TYPE_INT: \
            a->value.BASE = (TYPE)a->value.ival; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_UINT: \
            a->value.BASE = (TYPE)a->value.uval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_LONG: \
            a->value.BASE = (TYPE)a->value.lval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG: \
            a->value.BASE = (TYPE)a->value.ulval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE: \
            a->value.BASE = (TYPE)a->value.dval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT: \
            a->value.BASE = (TYPE)a->value.fval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL: \
            a->value.BASE = (TYPE)a->value.bval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_CHAR: \
            a->value.BASE = (TYPE)a->value.cval; \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_PTR: \
            fprintf(stderr, "TYPE ERROR: Cannot cast pointer to TYPE\n"); \
            break; \
        case LOOP_ADAPT_PARAMETER_TYPE_STR: \
            fprintf(stderr, "TYPE ERROR: Cannot cast string to TYPE\n"); \
            break; \
        default: \
            break; \
    }

int loop_adapt_cast_param_value(ParameterValue* a, ParameterValueType_t type)
{
    if (a && type >= LOOP_ADAPT_PARAMETER_VALUE_TYPE_MIN && type < LOOP_ADAPT_PARAMETER_TYPE_MAX )
    {
        if (type != a->type)
        {
            switch(type)
            {
                case LOOP_ADAPT_PARAMETER_TYPE_INT:
                    LOOP_ADAPT_CAST_SWITCH(ival, int)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_UINT:
                    LOOP_ADAPT_CAST_SWITCH(uval, unsigned int)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_LONG:
                    LOOP_ADAPT_CAST_SWITCH(lval, long)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
                    LOOP_ADAPT_CAST_SWITCH(ulval, unsigned long)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
                    LOOP_ADAPT_CAST_SWITCH(dval, double)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
                    LOOP_ADAPT_CAST_SWITCH(fval, float)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_CHAR:
                    LOOP_ADAPT_CAST_SWITCH(cval, char)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
                    LOOP_ADAPT_CAST_SWITCH(bval, unsigned int)
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_PTR:
                    fprintf(stderr, "TYPE ERROR: Casting to pointer values not possible\n");
                    break;
                case LOOP_ADAPT_PARAMETER_TYPE_STR:
                    fprintf(stderr, "TYPE ERROR: Casting to string values not possible\n");
                    break;
                default:
                    fprintf(stderr, "TYPE ERROR: Casting of invalid values not possible\n");
                    break;
            }
        }
    }
    return -EINVAL;
}

int loop_adapt_parse_param_value(char* string, ParameterValueType_t type, ParameterValue* value)
{
    int ret = 0;
    int slen = 0;
    switch(type)
    {
        case LOOP_ADAPT_PARAMETER_TYPE_INT:
            value->value.ival = atoi(string);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_UINT:
            value->value.uval = (unsigned int) strtoul(string, NULL, 10);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_LONG:
            value->value.lval = atol(string);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_ULONG:
            value->value.ulval = strtoul(string, NULL, 10);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_BOOL:
            value->value.bval = ((unsigned int) strtoul(string, NULL, 10) & 0x1);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_DOUBLE:
            value->value.dval = atof(string);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_PTR:
            value->value.dval = strtoul(string, NULL, 10);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_FLOAT:
            value->value.fval = (float)atof(string);
            break;
        case LOOP_ADAPT_PARAMETER_TYPE_STR:
            slen = strlen(string);
            value->value.sval = malloc((slen+2)*sizeof(char));
            if (value->value.sval)
            {
                ret = snprintf(value->value.sval, slen+1, "%s", string);
                if (ret >= 0) value->value.sval[ret] = '\0';
            }
            break;
        default:
            ERROR_PRINT(Invalid type for parameter value);
            return -EINVAL;
            break;
    }
    value->type = type;
    return 0;
}
