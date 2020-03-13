/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_measurement.c
 *
 *      Description:  Implementation of the measurement backend for loop_adapt
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
#ifndef LOOP_ADAPT_LOCK_H
#define LOOP_ADAPT_LOCK_H

#define LOOP_ADAPT_LOCK_INIT -1
static inline int
lock_acquire(int* var, int newval)
{
    int oldval = LOOP_ADAPT_LOCK_INIT;
    return __sync_bool_compare_and_swap (var, oldval, newval);
}



#endif /* LOOP_ADAPT_LOCK_H */
