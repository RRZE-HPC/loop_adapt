/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_limit.c
 *
 *      Description:  Functions to work with ParameterValueLimits, a list or range of
 *                    ParameterValues
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@fau.de
 *      Project:  loop_adapt
 *
 *      Copyright (C) 2020 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <error.h>

#include <loop_adapt_parameter_limit.h>
#include <loop_adapt_parameter_value.h>


char* loop_adapt_param_limit_type(ParameterValueLimitType_t t)
{
    switch(t)
    {
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
            return "RANGE";
            break;
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
            return "LIST";
            break;
        default:
            return "INVALID";
            break;
    }
    return NULL;
}

char* loop_adapt_param_limit_str(ParameterValueLimit l)
{
    int i = 0;
    int ret = 0;
    int slen = 51;
    char* t1, *t2, *t3;
    if (l.type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST)
    {
        if (l.limit.list.num_values > 0 && l.limit.list.values[0].type == LOOP_ADAPT_PARAMETER_TYPE_STR)
        {
            slen = 0;
            for (i = 0; i < l.limit.list.num_values; i++)
            {
                slen += strlen(l.limit.list.values[i].value.sval)+2;
            }
            slen += 10;
        }
    }
    char *s = malloc(slen * sizeof(char));
    if (s)
    {
        switch(l.type)
        {
            case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
                t1 = loop_adapt_param_value_str(l.limit.range.start);
                t2 = loop_adapt_param_value_str(l.limit.range.end);
                t3 = loop_adapt_param_value_str(l.limit.range.step);
                if (t1 && t2 && t3)
                {
                    ret = snprintf(s, slen-1, "%s -> %s (%s)", t1, t2, t3);
                }
                break;
            case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
/*                printf("%d %p\n", l.limit.list.num_values, l.limit.list.values);*/
                if (l.limit.list.num_values > 0)
                {
                    ret = 0;
                    t1 = loop_adapt_param_value_str(l.limit.list.values[0]);
                    if (t1)
                    {
                        ret += snprintf(&s[ret], slen-1-ret, "%s", t1);
                        free(t1);
                    }
                    for (i = 1; i < l.limit.list.num_values; i++)
                    {
                        t1 = loop_adapt_param_value_str(l.limit.list.values[i]);
                        if (t1)
                        {
                            ret += snprintf(&s[ret], slen-1-ret, ",%s", t1);
                            free(t1);
                        }
                    }
                }
                break;
            default:
                ret = snprintf(s, slen, "INVALID_LIMIT");
                break;
        }
        if (ret >= 0)
        {
            s[ret] = '\0';
        }
    }
    return s;
}

ParameterValueLimit loop_adapt_new_param_limit_list()
{
    ParameterValueLimit l = { .type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST,
                              .limit.list.num_values = 0,
                              .limit.list.values = NULL };
    return l;
}

int loop_adapt_add_param_limit_list(ParameterValueLimit *l, ParameterValue v)
{
/*    printf("%d %p\n", l->limit.list.num_values, l->limit.list.values);*/
    if (l->limit.list.num_values == 0 && l->limit.list.values == NULL)
    {
        l->limit.list.values = malloc(sizeof(ParameterValue));
        if (!l->limit.list.values)
        {
            return -ENOMEM;
        }
    }
    else
    {
        ParameterValue* t = realloc(l->limit.list.values, (l->limit.list.num_values+1)*sizeof(ParameterValue));
        if (!t)
        {
            return -ENOMEM;
        }
        l->limit.list.values = t;
    }
/*    printf("%p\n", l->limit.list.values);*/
    return loop_adapt_copy_param_value(v, &l->limit.list.values[l->limit.list.num_values++]);
}

void loop_adapt_destroy_param_limit(ParameterValueLimit l)
{
    int i = 0;
    switch(l.type)
    {
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
            loop_adapt_destroy_param_value(l.limit.range.start);
            loop_adapt_destroy_param_value(l.limit.range.end);
            loop_adapt_destroy_param_value(l.limit.range.step);
            break;
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
            for (i = 0; i < l.limit.list.num_values; i++)
            {
                loop_adapt_destroy_param_value(l.limit.list.values[i]);
            }
            free(l.limit.list.values);
            l.limit.list.values = NULL;
            l.limit.list.num_values = 0;
            l.limit.list.value_idx = -1;
            break;
        default:
            break;
    }
}

int loop_adapt_next_limit_value(ParameterValueLimit *l, ParameterValue* v)
{
    int err = 0;
/*    printf("%s\n", loop_adapt_print_param_valuetype(l->limit.range.current.type));*/
    switch(l->type)
    {
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
            if (l->limit.range.current.type == LOOP_ADAPT_PARAMETER_TYPE_INVALID)
            {
                loop_adapt_copy_param_value(l->limit.range.start, &l->limit.range.current);
                loop_adapt_copy_param_value(l->limit.range.current, v);
                char* pstr = loop_adapt_param_value_str(l->limit.range.current);
/*                printf("Here '%s' Type '%s'\n", pstr, loop_adapt_print_param_valuetype(l->limit.range.current.type));*/
                free(pstr);
            }
            else
            {
                ParameterValue x;
                loop_adapt_add_param_value(l->limit.range.current, l->limit.range.step, &x);
                char* pstr = loop_adapt_param_value_str(x);
/*                printf("There '%s' Type '%s'\n", pstr, loop_adapt_print_param_valuetype(x.type));*/
                free(pstr);
                if (!loop_adapt_equal_param_value(x, l->limit.range.end))
                {
                    loop_adapt_copy_param_value(x, v);
                    loop_adapt_copy_param_value(x, &l->limit.range.current);
                }
                else
                {
                    v->type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
                    l->limit.range.current.type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
                    err = -ERANGE;
                }
                loop_adapt_destroy_param_value(x);
            }
            break;
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
            if (l->limit.list.value_idx < 0 && l->limit.list.values != NULL)
            {
                l->limit.list.value_idx = 0;
                loop_adapt_copy_param_value(l->limit.list.values[0], v);
            }
            else if (l->limit.list.value_idx < l->limit.list.num_values)
            {
                loop_adapt_copy_param_value(l->limit.list.values[l->limit.list.value_idx++], v);
            }
            else
            {
                v->type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
                err = -ERANGE;
            }

            break;
        default:
            fprintf(stderr, "INVALID LIMIT TYPE\n");
            break;
    }
    return err;
}

int loop_adapt_param_limit_tolist(ParameterValueLimit in, ParameterValueLimit *out)
{
    if (out && in.type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE)
    {
        out->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST;
        out->limit.list.num_values = 0;
        out->limit.list.value_idx = -1;
        out->limit.list.values = NULL;
        ParameterValue v;
        int max = 5;
        loop_adapt_next_limit_value(&in, &v);
/*        printf("First %s\n", loop_adapt_print_param_valuetype(v.type));*/
        while (v.type != LOOP_ADAPT_PARAMETER_TYPE_INVALID && max > 0)
        {
/*            printf("Add to list\n");*/
            loop_adapt_add_param_limit_list(out, v);
/*            printf("Destroy\n");*/
            loop_adapt_destroy_param_value(v);
/*            printf("Next\n");*/
            loop_adapt_next_limit_value(&in, &v);
/*            printf("Final %s\n", loop_adapt_print_param_valuetype(v.type));*/
            max--;
        }
        return out->limit.list.num_values;
    }
    return -1;
}


int loop_adapt_copy_param_value_limit(ParameterValueLimit in, ParameterValueLimit *out)
{
    int i = 0;
    if (out && in.type >= LOOP_ADAPT_PARAMETER_LIMIT_TYPE_MIN && in.type < LOOP_ADAPT_PARAMETER_LIMIT_TYPE_MAX)
    {
        out->type = in.type;
        switch(in.type)
        {
            case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
                loop_adapt_copy_param_value(in.limit.range.start, &out->limit.range.start);
                loop_adapt_copy_param_value(in.limit.range.end, &out->limit.range.end);
                if (in.limit.range.step.type != LOOP_ADAPT_PARAMETER_TYPE_INVALID)
                {
                    loop_adapt_copy_param_value(in.limit.range.step, &out->limit.range.step);
                }
                if (in.limit.range.current.type != LOOP_ADAPT_PARAMETER_TYPE_INVALID)
                {
                    loop_adapt_copy_param_value(in.limit.range.current, &out->limit.range.current);
                }
                break;
            case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
                out->type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST;
                out->limit.list.num_values = 0;
                out->limit.list.value_idx = -1;
                out->limit.list.values = NULL;
                for (i = 0; i < in.limit.list.num_values; i++)
                {
                    loop_adapt_add_param_limit_list(out, in.limit.list.values[i]);
                }
                break;
        }
        return 0;
    }
    return -EINVAL;
}


int loop_adapt_check_param_limit(ParameterValue p, ParameterValueLimit l)
{
    int i = 0;
    int ret = 0;
    switch(l.type)
    {
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE:
            if (   loop_adapt_greater_param_value(p, l.limit.range.start)
                || loop_adapt_equal_param_value(p, l.limit.range.start))
            {
                if (loop_adapt_less_param_value(p, l.limit.range.end))
                {
                    if (l.limit.range.step.type == LOOP_ADAPT_PARAMETER_TYPE_INVALID)
                    {
                        ret = 1;
                    }
                    else
                    {
                        ParameterValue x = DEC_NEW_INVALID_PARAM_VALUE;
                        loop_adapt_copy_param_value(l.limit.range.start, &x);
                        while (loop_adapt_less_param_value(x, l.limit.range.end))
                        {
                            if (loop_adapt_equal_param_value(p, x))
                            {
                                ret = 1;
                                break;
                            }
                            loop_adapt_add_param_value(x, l.limit.range.step, &x);
                        }
                    }
                }
            }
            break;
        case LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST:
            for (i = 0; i < l.limit.list.num_values; i++)
            {
                if (loop_adapt_equal_param_value(p, l.limit.list.values[i]))
                {
                    ret = 1;
                    break;
                }
            }
            break;
        default:
            break;
    }
    return ret;
}
