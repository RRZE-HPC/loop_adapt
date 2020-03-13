/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_configuration_socket.h
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
#ifndef LOOP_ADAPT_CONFIGURATION_SOCKET_H
#define LOOP_ADAPT_CONFIGURATION_SOCKET_H

#include <loop_adapt_configuration_types.h>

int loop_adapt_config_socket_init();

LoopAdaptConfiguration_t loop_adapt_get_new_config_socket(char* string);

LoopAdaptConfiguration_t loop_adapt_get_current_config_socket(char* string);

int loop_adapt_config_socket_write(LoopAdaptConfiguration_t config, int num_results, ParameterValue* results);

void loop_adapt_config_socket_finalize();


#endif /* LOOP_ADAPT_CONFIGURATION_SOCKET_H */
