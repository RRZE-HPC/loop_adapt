/*
 * =======================================================================================
 *
 *      Filename:  map.c
 *
 *      Description:  Implementation a hashmap in C using ghash as backend
 *
 *      Version:   5.0
 *      Released:  10.11.2019
 *
 *      Author:   Thoams Roehl (tr), thomas.roehl@gmail.com
 *      Project:  likwid
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
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
#include <error.h>
#include <stdint.h>

#include <map.h>

#include <loop_adapt_parameter_types.h>
#include <loop_adapt_parameter_value.h>
#include <loop_adapt_parameter_limit.h>
#include <loop_adapt_parameter_list.h>
#include <loop_adapt_parameter.h>

#include <loop_adapt_hwloc_tree.h>

/*! \brief  This is the hwloc topology tree used for registering parameters */
static hwloc_topology_t loop_adapt_parameter_tree = NULL;
static int loop_adapt_parameter_debug = 0;

static ParameterDefinition* loop_adapt_active_parameters = NULL;
static int loop_adapt_num_active_parameters = 0;

/*static struct bstrList* loop_adapt_parameter_names = NULL;*/

static Parameter_t _loop_adapt_new_parameter()
{
    Parameter_t p = malloc(sizeof(Parameter));
    if (!p)
    {
        fprintf(stderr, "ERROR: Cannot allocate new parameter\n");
        return NULL;
    }
    /* Initialize data struct representing a single loop */
    memset(p, 0, sizeof(Parameter));
    return p;
}

static int _loop_adapt_copy_parameter_definition(ParameterDefinition* in, ParameterDefinition*out)
{
    if (in && out)
    {
        out->name = malloc(sizeof(char) * (strlen(in->name)+2));
        int err = snprintf(out->name, strlen(in->name)+1, "%s", in->name);
        if (err > 0)
        {
            out->name[err] = '\0';
        }
        out->scope = in->scope;
        out->init = in->init;
        out->get = in->get;
        out->set = in->set;
        out->avail = in->avail;
        out->finalize = in->finalize;
        loop_adapt_copy_param_value(in->value, &out->value);
        return 0;
    }
    return -1;
}

void _loop_adapt_free_parameter(mpointer val)
{
    Parameter_t p = (Parameter_t)val;
    if (p)
    {
        loop_adapt_destroy_param_value(p->value);
        loop_adapt_destroy_param_limit(p->limit);
    }
}

static int _loop_adapt_add_parameter_to_tree(ParameterDefinition* def, int idx_in_list)
{
    int err = 0;
    int i = 0;
    int strlength = 0;
    for (i = 0; i < hwloc_get_nbobjs_by_type(loop_adapt_parameter_tree, def->scope); i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_parameter_tree, def->scope, i);
        if (obj)
        {
            Map_t params = (Map_t)obj->userdata;
            if (!params)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Creating parameter map for %s %d, hwloc_obj_type_string(def->scope), i);
                err = init_smap(&params, _loop_adapt_free_parameter);
                if (err != 0)
                {
                    fprintf(stderr, "Failed to create parameter hash map for %s %d\n", hwloc_obj_type_string(def->scope), i);
                    return -1;
                }
                else
                {;
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Creating parameter map for %s %d successful, hwloc_obj_type_string(def->scope), i);
                }
            }
            else
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Parameter map for %s %d already exists, hwloc_obj_type_string(def->scope), i);
            }
            Parameter_t p = NULL;
            err = get_smap_by_key(params, def->name, (void**)&p);
            if (err == 0)
            {
                fprintf(stderr, "Failed to create parameter %s for %s %d: already in hash\n", def->name, hwloc_obj_type_string(def->scope), i);
                return -1;
            }
            p = _loop_adapt_new_parameter();
            if (p)
            {
                p->param_list_idx = idx_in_list;
                p->instance = i;
                p->limit.type = LOOP_ADAPT_PARAMETER_LIMIT_TYPE_INVALID;
                loop_adapt_copy_param_value(def->value, &p->value);

                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Adding parameter '%s' to %s %d, def->name, hwloc_obj_type_string(def->scope), i);
                err = add_smap(params, def->name,(void*) p);
                if (err < 0)
                {
                    printf("Addition failed\n");
                }
            }
            obj->userdata = (void*)params;
        }
    }
    return 0;
}

int loop_adapt_parameter_initialize()
{
    if (!loop_adapt_parameter_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize parameter tree);
        int err = loop_adapt_copy_hwloc_tree(&loop_adapt_parameter_tree);
        if (err)
        {
            ERROR_PRINT(Failed to get copy of hwloc tree for parameter tree);
            return -1;
        }
    }
    int pd_idx = 0;
    ParameterDefinition* pd = &loop_adapt_parameter_list[pd_idx];
    while (pd->name != NULL)
    {
        // Next parameter in list
        pd_idx++;
        pd = &loop_adapt_parameter_list[pd_idx];
    }
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize active parameters with %d compiled-in parameters, pd_idx);

    loop_adapt_active_parameters = malloc(pd_idx * sizeof(ParameterDefinition));
    if (!loop_adapt_active_parameters)
    {
        fprintf(stderr, "Failed to allocate space for active parameters\n");
        return 1;
    }
    for (int j = 0; j < pd_idx; j++)
    {
        ParameterDefinition* in = &loop_adapt_parameter_list[j];
        ParameterDefinition* out = &loop_adapt_active_parameters[j];
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Active parameter %s for scope %s, in->name, hwloc_obj_type_string(in->scope));
        _loop_adapt_copy_parameter_definition(in, out);

        pd = &loop_adapt_active_parameters[loop_adapt_num_active_parameters];
        if (pd->init)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initializing parameter %s for scope %s, in->name, hwloc_obj_type_string(in->scope));
            int err = pd->init();
            if (err)
            {
                ERROR_PRINT(Initializing parameter %s for scope %s failed, in->name, hwloc_obj_type_string(in->scope));
            }
        }
        if (pd->get)
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get current value for parameter %s for scope %s, in->name, hwloc_obj_type_string(in->scope));
            int err = pd->get(0, &pd->value);
            if (err)
            {
                ERROR_PRINT(Getting current value for parameter %s for scope %s failed, in->name, hwloc_obj_type_string(in->scope));
            }
            loop_adapt_print_param_value(pd->value);
        }
        _loop_adapt_add_parameter_to_tree(pd, loop_adapt_num_active_parameters);
        loop_adapt_num_active_parameters++;
    }
/*    loop_adapt_parameter_names = bstrListCreate();*/
}

int loop_adapt_parameter_add(char* name, LoopAdaptScope_t scope,
                             ParameterValueType_t valuetype,
                             parameter_set_function set,
                             parameter_get_function get,
                             parameter_available_function avail)
{
    int strlength = 0;
    ParameterDefinition* def = realloc(loop_adapt_active_parameters, (loop_adapt_num_active_parameters+1)*sizeof(ParameterDefinition));
    if (!def)
    {
        fprintf(stderr, "Failed to extend list of active parameters\n");
        return -1;
    }
    loop_adapt_active_parameters = def;

    def = &loop_adapt_active_parameters[loop_adapt_num_active_parameters];
    def->name = malloc(sizeof(char) * (strlen(name)+2));
    int err = snprintf(def->name, strlen(name)+1, "%s", name);
    if (err > 0)
    {
        def->name[err] = '\0';
    }
    def->scope = scope;

    def->set = set;
    def->get = get;
    def->avail = avail;
    ParameterValue value;
    value.type = valuetype;

    if (get)
    {
        err = get(0, &value);
        if (err)
        {
            ERROR_PRINT(Failed to get initial value for parameter %s, name);
            value = loop_adapt_new_param_value(valuetype);
        }
        loop_adapt_print_param_value(value);
    }
    else
    {
        value = loop_adapt_new_param_value(valuetype);
    }

    loop_adapt_copy_param_value(value, &def->value);


    _loop_adapt_add_parameter_to_tree(def, loop_adapt_num_active_parameters);
/*    bstrListAddChar(loop_adapt_parameter_names, name);*/
    loop_adapt_num_active_parameters++;
}

int loop_adapt_parameter_add_user(char* name, LoopAdaptScope_t scope, ParameterValue value)
{
    int err = 0;
    int strlength = 0;
    ParameterDefinition* def = realloc(loop_adapt_active_parameters, (loop_adapt_num_active_parameters+1)*sizeof(ParameterDefinition));
    if (!def)
    {
        fprintf(stderr, "Failed to extend list of active parameters\n");
        return -1;
    }
    loop_adapt_active_parameters = def;
    def = &loop_adapt_active_parameters[loop_adapt_num_active_parameters];
    def->name = malloc(sizeof(char) * (strlen(name)+2));
    if (def->name)
    {
        err = snprintf(def->name, strlen(name)+1, "%s", name);
        if (err > 0)
        {
            def->name[err] = '\0';
        }
    }
    def->scope = scope;

    def->user = 1;
    loop_adapt_copy_param_value(value, &def->value);

    def->set = NULL;
    def->get = NULL;
    def->avail = NULL;
    printf("Adding user parameter %s of type %s\n", def->name, loop_adapt_print_param_valuetype(def->value.type));

    _loop_adapt_add_parameter_to_tree(def, loop_adapt_num_active_parameters);
/*    bstrListAddChar(loop_adapt_parameter_names, name);*/
    loop_adapt_num_active_parameters++;
}

ParameterValueType_t loop_adapt_parameter_type(char* name)
{
    int i = 0;
    ParameterDefinition* def = NULL;
    for (i = 0; i < loop_adapt_num_active_parameters; i++)
    {
        def = &loop_adapt_active_parameters[i];
        if (!def) break;
        if (strncmp(def->name, name, strlen(def->name)) == 0)
        {
            return def->value.type;
        }
    }
    return LOOP_ADAPT_PARAMETER_TYPE_INVALID;
}

LoopAdaptScope_t loop_adapt_parameter_scope(char* name)
{
    int i = 0;
    ParameterDefinition* def = NULL;
    for (i = 0; i < loop_adapt_num_active_parameters; i++)
    {
        def = &loop_adapt_active_parameters[i];
        if (!def) break;
        if (strncmp(def->name, name, strlen(def->name)) == 0)
        {
            return def->scope;
        }
    }
    return LOOP_ADAPT_SCOPE_MAX;
}

int loop_adapt_parameter_scope_count(char* name)
{
    int i = 0;
    ParameterDefinition* def = NULL;
    for (i = 0; i < loop_adapt_num_active_parameters; i++)
    {
        def = &loop_adapt_active_parameters[i];
        if (!def) break;
        if (strncmp(def->name, name, strlen(def->name)) == 0)
        {
            return hwloc_get_nbobjs_by_type(loop_adapt_parameter_tree, def->scope);
        }
    }
    return 0;
}

void loop_adapt_parameter_finalize()
{
    int i = 0;
    int j = 0;
    int err = 0;
    int pd_idx = 0;

    if (loop_adapt_num_active_parameters > 0)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Emptying parameter tree for %d parameters, loop_adapt_num_active_parameters);
        for (i = 0; i < loop_adapt_num_active_parameters; i++)
        {
            ParameterDefinition* pd = &loop_adapt_active_parameters[i];
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Delete parameter %s for scope %s (%d objects), pd->name, hwloc_obj_type_string(pd->scope), hwloc_get_nbobjs_by_type(loop_adapt_parameter_tree, pd->scope));
            for (j = 0; j < hwloc_get_nbobjs_by_type(loop_adapt_parameter_tree, pd->scope); j++)
            {
                hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_parameter_tree, pd->scope, j);

                if (obj)
                {
                    Map_t params = (Map_t)obj->userdata;
                    if (!params) continue;
                    Parameter_t p = NULL;
                    err = get_smap_by_key(params, pd->name, (void**)&p);
                    if (err == 0)
                    {
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Delete parameter %s from map at %s %d/%d, pd->name, hwloc_obj_type_string(pd->scope), obj->logical_index, j);
                        del_smap(params, pd->name);
                        if (get_smap_size(params) == 0)
                        {
                            destroy_smap(params);
                            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Delete map at %s %d/%d, hwloc_obj_type_string(pd->scope), obj->logical_index, j);
                            obj->userdata = NULL;
                        }
                    }
                    else
                    {
                        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, No parameter %s in map at %s %d/%d, pd->name, hwloc_obj_type_string(pd->scope), obj->logical_index, j);
                    }
                }
                else
                {
                    ERROR_PRINT(No obj at %s %d, hwloc_obj_type_string(pd->scope), j);
                }
            }
            free(pd->name);
            if (pd->finalize)
            {
                pd->finalize();
            }
        }
    }
    if (loop_adapt_active_parameters)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Free active parameter list);
        free(loop_adapt_active_parameters);
        loop_adapt_active_parameters = NULL;
        loop_adapt_num_active_parameters = 0;
    }
    if (loop_adapt_parameter_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Free parameter tree);
        hwloc_topology_destroy(loop_adapt_parameter_tree);
        loop_adapt_parameter_tree = NULL;
    }
/*    if (loop_adapt_parameter_names)*/
/*    {*/
/*        bstrListDestroy(loop_adapt_parameter_names);*/
/*        loop_adapt_parameter_names = NULL;*/
/*    }*/
}


int loop_adapt_parameter_set(ThreadData_t thread, char* parameter, ParameterValue value)
{
    if ((!thread) || (!parameter))
    {
        return -EINVAL;
    }
    if (value.type < LOOP_ADAPT_PARAMETER_VALUE_TYPE_MIN || value.type >= LOOP_ADAPT_PARAMETER_TYPE_MAX)
    {
        return -EINVAL;
    }
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_parameter_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Parameter_t p = NULL;
            Map_t params = (Map_t)obj->userdata;
            if (!params)
            {
                continue;
            }
            int err = get_smap_by_key(params, parameter, (void**)&p);
            if (err == 0)
            {
                if (value.type == p->value.type)
                {
                    if (p->limit.type != LOOP_ADAPT_PARAMETER_LIMIT_TYPE_INVALID)
                    {
                        if (!loop_adapt_check_param_limit(value, p->limit))
                        {
                            ERROR_PRINT(Value not in limits of parameter);
                            return -1;
                        }
                    }
                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Setting parameter %s, parameter);
                    loop_adapt_copy_param_value(value, &p->value);
                    parameter_set_function f = loop_adapt_active_parameters[p->param_list_idx].set;
                    if (f)
                    {
                        err = f(p->instance, p->value);
                        if (err)
                        {
                            ERROR_PRINT(Set function for parameter %s failed, parameter);
                            return -1;
                        }
                    }
                }
                else
                {
                    ERROR_PRINT(Parameter value for %s has wrong type (%s vs %s), parameter,loop_adapt_print_param_valuetype(value.type), loop_adapt_print_param_valuetype(p->value.type));
                    return -1;
                }
            }
        }
    }
    return 0;
}

int loop_adapt_parameter_get(ThreadData_t thread, char* parameter, ParameterValue* value)
{
    if ((!thread) || (!parameter) || (!value))
    {
        return -EINVAL;
    }
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_parameter_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Parameter_t p = NULL;
            Map_t params = (Map_t)obj->userdata;
            if (!params)
            {
                continue;
            }
            int err = get_smap_by_key(params, parameter, (void**)&p);
            if (err == 0)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Getting parameter %s, parameter);
                loop_adapt_copy_param_value(p->value, value);
                value->type = p->value.type;
            }
        }
    }
    return 0;
}

int loop_adapt_parameter_getcurrent(ThreadData_t thread, char* parameter, ParameterValue* value)
{
    if ((!thread) || (!parameter) || (!value))
    {
        return -EINVAL;
    }
    for (int s = 0; s < LOOP_ADAPT_NUM_SCOPES; s++)
    {
        if (thread->scopeOffsets[s] < 0) continue;
        hwloc_obj_t obj = hwloc_get_obj_by_type(loop_adapt_parameter_tree, LoopAdaptScopeList[s], thread->scopeOffsets[s]);
        if (obj)
        {
            Parameter_t p = NULL;
            Map_t params = (Map_t)obj->userdata;
            if (!params)
            {
                continue;
            }
            int err = get_smap_by_key(params, parameter, (void**)&p);
            if (err == 0)
            {
                ParameterValue v;
                parameter_get_function f = loop_adapt_active_parameters[p->param_list_idx].get;
                if (f)
                {
                    f(thread->objidx, &v);
                    loop_adapt_copy_param_value(v, &p->value);
                    loop_adapt_copy_param_value(v, value);
                    value->type = p->value.type;
                }
            }
        }
    }
    return 0;
}

int loop_adapt_parameter_getnames(int* count, char*** parameters)
{
    int i = 0, j = 0;
    char **p = NULL;

    p = malloc(sizeof(char*) * loop_adapt_num_active_parameters);
    if (!p)
    {
        return -ENOMEM;
    }
    for (i = 0; i < loop_adapt_num_active_parameters; i++)
    {
        ParameterDefinition* pd = &loop_adapt_active_parameters[i];
        int slen = strlen(pd->name) + 2;
        p[i] = malloc(sizeof(char) * slen);
        if (!p[i])
        {
            for (j = i - 1; j >= 0; j--)
            {
                if (p[j]) free(p[j]);
            }
            free(p);
            return -ENOMEM;
        }
        int ret = snprintf(p[i], slen, "%s", pd->name);
        if (ret >= 0)
        {
            p[i][ret] = '\0';
        }
    }
    *count = loop_adapt_num_active_parameters;
    *parameters = p;
    return 0;
}

int loop_adapt_parameter_configs(struct bstrList* configs)
{
    int i = 0;
    int count = 0;
    for (i = 0; i < loop_adapt_num_active_parameters; i++)
    {
        ParameterDefinition* pd = &loop_adapt_active_parameters[i];
        if (pd->avail)
        {
            ParameterValueLimit limit = DEC_NEW_INVALID_PARAM_LIMIT;
            int err = pd->avail(0, &limit);
            if (!err)
            {
                if (limit.type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_RANGE)
                {
                    char* s = loop_adapt_param_value_str(limit.limit.range.start);
                    char* e = loop_adapt_param_value_str(limit.limit.range.end);
                    char* t = loop_adapt_print_param_valuetype(limit.limit.range.start.type);
                    bstring c = bformat("%s,%s,%s,%s", pd->name, t, s, e);
                    bstrListAdd(configs, c);
                    bdestroy(c);
                    free(s);
                    free(e);
                    free(t);
                    count++;
                }
                else if (limit.type == LOOP_ADAPT_PARAMETER_LIMIT_TYPE_LIST)
                {
                    ERROR_PRINT(Parameter limit (list) not supported by loop_adapt_parameter_configs);
                }
            }
        }
    }
    return count;
}