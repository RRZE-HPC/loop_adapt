/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_parameter_template.h
 *
 *      Description:  Template for parameter functions
 *
 *      Version:   <VERSION>
 *      Released:  <DATE>
 *
 *      Author:   Thoams Gruber (tg), thomas.gruber@fau.de
 *      Project:  likwid
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

#include <loop_adapt_parameter_types.h>

// This function is called when more operations are required to reflect the parameter change
int loop_adapt_parameter_template_set(int instance, ParameterValue value)
{
    return 0;
}

// This function is called to get the actual value at system level (e.g. state of a prefetcher)
int loop_adapt_parameter_template_get(int instance, ParameterValue* value)
{
    return 0;
}

// This function is called to get the available parameter values for validation and iterating
int loop_adapt_parameter_template_avail(int instance, ParameterValueLimit* limit)
{
    return 0;
}
