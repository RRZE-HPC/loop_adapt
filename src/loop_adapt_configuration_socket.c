/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_configuration_socket.c
 *
 *      Description:  Configuration module to read and write to a UNIX socket
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
#include <string.h>
#include <error.h>

#include <loop_adapt_configuration_types.h>
#include <loop_adapt_parameter_value.h>
#include <loop_adapt_parameter.h>
#include <loop_adapt_measurement.h>
#include <loop_adapt_configuration.h>

#include <loop_adapt_configuration_socket_ringbuffer.h>
#include <loop_adapt_configuration_socket_thread.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <map.h>




static Ringbuffer_t loop_adapt_config_socket_ring_in = NULL;
static Ringbuffer_t loop_adapt_config_socket_ring_out = NULL;
static LoopAdaptConfiguration_t loop_adapt_config_socket_current = NULL;

int loop_adapt_config_socket_input_init()
{

    if (!loop_adapt_config_socket_ring_in)
    {
        ringbuffer_init(5, &loop_adapt_config_socket_ring_in);
    }
    if (!loop_adapt_config_socket_ring_out)
    {
        ringbuffer_init(5, &loop_adapt_config_socket_ring_out);
    }
    loop_adapt_config_socket_thread_init(loop_adapt_config_socket_ring_in, loop_adapt_config_socket_ring_out);
    return 0;
}

LoopAdaptConfiguration_t loop_adapt_get_new_config_socket(char* string)
{
    if (loop_adapt_config_socket_ring_in)
    {
        LoopAdaptConfiguration_t new;
        int ret = ringbuffer_get(loop_adapt_config_socket_ring_in, (void**)new);
        if (ret == 0)
        {
            if (loop_adapt_config_socket_current)
            {
                loop_adapt_configuration_destroy_config(loop_adapt_config_socket_current);
                loop_adapt_config_socket_current = NULL;
            }
            loop_adapt_config_socket_current = new;
            return new;
        }
    }
    return NULL;
}

LoopAdaptConfiguration_t loop_adapt_get_current_config_socket(char* string)
{
    if (loop_adapt_config_socket_ring_in && loop_adapt_config_socket_current)
    {
        return loop_adapt_config_socket_current;
    }
    return NULL;
}

int loop_adapt_config_socket_write(LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
}

void loop_adapt_config_socket_finalize()
{
    if (loop_adapt_config_socket_ring_in)
    {
        ringbuffer_destroy(loop_adapt_config_socket_ring_in);
        loop_adapt_config_socket_ring_in = NULL;
    }
    if (loop_adapt_config_socket_ring_out)
    {
        ringbuffer_destroy(loop_adapt_config_socket_ring_out);
        loop_adapt_config_socket_ring_out = NULL;
    }
    if (loop_adapt_config_socket_current)
    {
        loop_adapt_configuration_destroy_config(loop_adapt_config_socket_current);
        loop_adapt_config_socket_current = NULL;
    }

    return;
}
