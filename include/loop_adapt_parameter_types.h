/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_types.h
 *
 *      Description:  Header file with the type definitions for Parameters
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

#ifndef LOOP_ADAPT_PARAMETER_TYPES_H
#define LOOP_ADAPT_PARAMETER_TYPES_H

#include <loop_adapt_parameter_value_types.h>
#include <loop_adapt_parameter_limit_types.h>

#include <loop_adapt_scopes.h>

typedef struct {
    int param_list_idx;
    int instance;
    ParameterValue value;
    ParameterValue init;
    ParameterValueLimit limit;
} Parameter;
typedef Parameter* Parameter_t;

typedef int (*parameter_init_function)(void);
typedef int (*parameter_set_function)(int instance, ParameterValue value);
typedef int (*parameter_get_function)(int instance, ParameterValue* value);
typedef int (*parameter_available_function)(int instance, ParameterValueLimit *limit);
typedef void (*parameter_finalize_function)(void);

typedef struct {
    char* name;
    char* description;
    LoopAdaptScope_t scope;
    ParameterValue value;
    ParameterValueLimit limit;
    parameter_init_function init;
    parameter_set_function set;
    parameter_get_function get;
    parameter_available_function avail;
    parameter_finalize_function finalize;
    int user;
} ParameterDefinition;






#endif /* LOOP_ADAPT_PARAMETER_TYPES_H */
