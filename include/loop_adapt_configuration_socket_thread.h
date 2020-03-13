/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_configuration_socket_thread.h
 *
 *      Description:  Interface for thread that reads and writes data to UNIX sockets
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
#ifndef LOOP_ADAPT_CONFIGURATION_SOCKET_THREAD_H
#define LOOP_ADAPT_CONFIGURATION_SOCKET_THREAD_H


int loop_adapt_config_socket_thread_init(Ringbuffer_t in, Ringbuffer_t out);
void loop_adapt_config_socket_thread_finalize();



#endif /* LOOP_ADAPT_CONFIGURATION_SOCKET_THREAD_H */
