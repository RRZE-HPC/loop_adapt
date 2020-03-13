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

#include <error.h>

#include <hwloc.h>

static hwloc_topology_t _loop_adapt_hwloc_tree = NULL;

int loop_adapt_copy_hwloc_tree(hwloc_topology_t* copy)
{
    int err = 0;
    if (!_loop_adapt_hwloc_tree)
    {
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Initialize hwloc topology tree);
        err = hwloc_topology_init(&_loop_adapt_hwloc_tree);
        if (err < 0)
        {
            ERROR_PRINT(Failed to initialize hwloc topology tree);
            return err;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Load hwloc topology tree);
        err = hwloc_topology_load(_loop_adapt_hwloc_tree);
        if (err < 0)
        {
            ERROR_PRINT(Failed to load hwloc topology tree);
            return err;
        }
    }
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Duplicate hwloc topology tree);
    err = hwloc_topology_dup(copy, _loop_adapt_hwloc_tree);
    if (err < 0)
    {
        ERROR_PRINT(Failed to duplicate hwloc topology tree);
        return err;
    }
    return err;
}

void loop_adapt_copydestroy_hwloc_tree(hwloc_topology_t tree)
{
    hwloc_topology_destroy(tree);
}

void loop_adapt_hwloc_tree_finalize()
{
    if (_loop_adapt_hwloc_tree)
    {
        hwloc_topology_destroy(_loop_adapt_hwloc_tree);
        _loop_adapt_hwloc_tree = NULL;
    }
}
