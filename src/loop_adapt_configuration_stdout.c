/*
 * =======================================================================================
 *
 *      Filename:  loop_adapt_configuration_txt.c
 *
 *      Description:  Configuration module to read and write to txt files
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

#include <bstrlib.h>
#include <bstrlib_helper.h>

static FILE* loop_adapt_config_stdout_fd = NULL;

int loop_adapt_config_stdout_init()
{
    if (!loop_adapt_config_stdout_fd)
    {
        char* outname = getenv("LA_CONFIG_STDOUT_OUTPUT");
        if (outname)
        {
            if (strncmp(outname, "stderr", 6) == 0)
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Configuration stdout backend: use stderr);
                loop_adapt_config_stdout_fd = stderr;
            }
            else
            {
                DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Configuration stdout backend: use stdout);
                loop_adapt_config_stdout_fd = stdout;
            }
        }
        else
        {
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Configuration stdout backend: LA_CONFIG_STDOUT_OUTPUT not set - using stdout);
            loop_adapt_config_stdout_fd = stdout;
        }
    }
    return 0;
}

int loop_adapt_config_stdout_output_raw(char* loopname, char* rawstring)
{
    if (loopname && rawstring)
    {
        bstring line = bformat("LOOP=%s|%s\n", loopname, rawstring);
        fprintf(loop_adapt_config_stdout_fd, "%s", bdata(line));
        bdestroy(line);
        fflush(loop_adapt_config_stdout_fd);
    }
}

int loop_adapt_config_stdout_write(ThreadData_t thread, char* loopname, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
    int i = 0, j = 0;
    if (config && num_results > 0 && results)
    {
        if (config->num_measurements != num_results)
        {
            return -EINVAL;
        }
        bstring line = bfromcstr("");
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Writing %d parameters and %d measurements, config->num_parameters, config->num_measurements);
        for (i = 0; i < config->num_parameters; i++)
        {
            LoopAdaptConfigurationParameter *p = &config->parameters[i];
            if (p)
            {
                for (j = 0; j < p->num_values; j++)
                {
                    if (p->values[j].type != LOOP_ADAPT_PARAMETER_TYPE_INVALID)
                    {
                        char* c = loop_adapt_param_value_str(p->values[j]);
                        bstring x = loop_adapt_config_parse_default_entry_bdc(p->parameter, j, c);
                        free(c);
                        bconcat(line, x);
                        bconchar(line, ';');
                        bdestroy(x);
                    }
                }
            }
        }
        btrunc(line, blength(line) - 1);
        bconchar(line, '|');
        for (i = 0; i < config->num_measurements; i++)
        {
            LoopAdaptConfigurationMeasurement *m = &config->measurements[i];
            if (m)
            {
                char* c = loop_adapt_param_value_str(results[i]);
                bstring x = loop_adapt_config_parse_default_entry_bbb(m->measurement, m->config, m->metric);
                bconchar(x, ':');
                bcatcstr(x, c);
                free(c);
                bconcat(line, x);
                bconchar(line, ';');
                bdestroy(x);
            }
        }
        btrunc(line, blength(line) - 1);
        fprintf(loop_adapt_config_stdout_fd, "LOOP=%s;THREAD=%d:%d|%s\n", loopname, thread->thread, thread->cpu, bdata(line));
        fflush(loop_adapt_config_stdout_fd);
        bdestroy(line);
    }
    else
    {
        printf("Maaeeeee\n");
    }
}
