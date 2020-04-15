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
#include <map.h>

// Format:
// <param1>=<value1>(:<ids1>);<param2>=<value2>(:<ids2>)|<measurement1>=<config1>:<metrics1>;<measurement2>=<config2>:<metrics2>
// <param1>=<id0>:<value0>,<param1>=<id1>:<value1>,|<measurement1>=<config1>:<metrics1>

typedef struct {
    bstring filename;
    int line_idx;
    struct bstrList* lines;
    LoopAdaptConfiguration_t current;
} ConfigurationFile;
static Map_t loop_adapt_config_txt_input_hash = NULL;
static char* dirname = NULL;



static Map_t loop_adapt_config_txt_output_hash = NULL;
static char* outputdir = NULL;

void _loop_adapt_free_config_txt_input(gpointer val)
{
    ConfigurationFile* cfile = (ConfigurationFile*) val;
    if (cfile)
    {
        if (cfile->current)
        {
            loop_adapt_configuration_destroy_config(cfile->current);
            cfile->current = NULL;
        }
        if (cfile->lines)
        {
            bstrListDestroy(cfile->lines);
        }
        bdestroy(cfile->filename);
        memset(cfile, 0, sizeof(ConfigurationFile));
        free(cfile);
    }
}

void _loop_adapt_free_config_txt_output(gpointer val)
{
    FILE* fp = (FILE*)val;
    fclose(fp);
}

int loop_adapt_config_txt_input_init()
{
    int err = 0;
    char* dname = getenv("LA_CONFIG_TXT_INPUT");


    
    if (!dname)
    {
        ERROR_PRINT(Environment variable LA_CONFIG_TXT_INPUT not set. Need directory name)
        return -EINVAL;
    }
    if (access(dname, X_OK|R_OK))
    {
        ERROR_PRINT(Directory %s not accessible, dname)
        return -ENOENT;
    }
    dirname = dname;

    if (!loop_adapt_config_txt_input_hash)
    {
        err = init_smap(&loop_adapt_config_txt_input_hash, _loop_adapt_free_config_txt_input);
        if (err)
        {
            ERROR_PRINT(Failed to initialize hash table for loopname -> inputfile relation);
        }
    }

    return 0;
}


void loop_adapt_config_txt_input_finalize()
{
    if (loop_adapt_config_txt_input_hash)
    {
        destroy_smap(loop_adapt_config_txt_input_hash);
        loop_adapt_config_txt_input_hash = NULL;
        dirname = NULL;
    }
    return;
}



static int _loop_adapt_get_new_config_txt_resize_values(LoopAdaptConfigurationParameter* parameter, int num_values)
{
    if (parameter && num_values > 0)
    {
        if (parameter->num_values < num_values)
        {
            int j = 0;
            int old_num_values = parameter->num_values;
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Allocating space for %d parameter values of parameter %s, num_values, bdata(parameter->parameter));
            ParameterValue* vtmp = realloc(parameter->values, num_values * sizeof(ParameterValue));
            if (!vtmp)
            {
                return -ENOMEM;
            }
            parameter->values = vtmp;
            parameter->num_values = num_values;
            for (j = old_num_values; j < parameter->num_values; j++)
            {
                parameter->values[j].type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;
                parameter->values[j].value.sval = NULL;
            }
        }
        return 0;
    }
    return -EINVAL;
}

static int  _loop_adapt_get_new_config_txt_read_file(char* filename)
{
    int err = 0;
    int i = 0;
    ConfigurationFile* config = NULL;


    bstring content = read_file(filename);
    struct bstrList* lines = bsplit(content, '\n');
    bdestroy(content);
    if (lines->qty == 0)
    {
        return -ENOMSG;
    }

    config = malloc(sizeof(ConfigurationFile));
    if (!config)
    {
        bstrListDestroy(lines);
    }
    config->lines = bstrListCreate();

    for (i = 0; i < lines->qty; i++)
    {
        btrimws(lines->entry[i]);
        if (blength(lines->entry[i]) == 0 || bchar(lines->entry[i], 0) == '#')
        {
            continue;
        }
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, TXT: %s, bdata(lines->entry[i]));
        bstrListAdd(config->lines, lines->entry[i]);
    }
    bstrListDestroy(lines);

    if (config->lines->qty == 0)
    {
        free(config);
        bstrListDestroy(config->lines);
        return -ENOMSG;
    }

    config->line_idx = 0;
    config->filename = bfromcstr(filename);
    config->current = NULL;



    bstring t = bfromcstr(basename(filename));
    bstring loop = bmidstr(t, 0, blength(t)-4);
    bdestroy(t);

    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Add file %s for loop %s, filename, bdata(loop));
    add_smap(loop_adapt_config_txt_input_hash, bdata(loop), config);

    bdestroy(loop);



    return 0;
}

LoopAdaptConfiguration_t loop_adapt_get_new_config_txt(char* string)
{
    int i = 0;
    int j = 0;
    int err = 0;
    ConfigurationFile* cfile = 0;
    if ((!string) || (!loop_adapt_config_txt_input_hash))
    {
        return NULL;
    }
    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Get new config for loop %s, string);
    err = get_smap_by_key(loop_adapt_config_txt_input_hash, string, (void*)&cfile);
    if (err != 0 && dirname)
    {
        bstring filename = bformat("%s/%s.txt", dirname, string);
        _loop_adapt_get_new_config_txt_read_file(bdata(filename));
        bdestroy(filename);
        err = get_smap_by_key(loop_adapt_config_txt_input_hash, string, (void*)&cfile);
    }
    if (err == 0)
    {
        if (cfile->line_idx == cfile->lines->qty)
        {
            return NULL;
        }

        int pcount = 0;
        int mcount = 0;
        struct bstrList* first = bsplit(cfile->lines->entry[cfile->line_idx], '|');
        struct bstrList* params = bsplit(first->entry[0], ';');
        struct bstrList* measure = bsplit(first->entry[1], ';');
        bstrListDestroy(first);
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Line %d has %d parameters and %d measurements, cfile->line_idx, params->qty, measure->qty);
/*        LoopAdaptConfiguration_t config = malloc(sizeof(LoopAdaptConfiguration));*/
/*        if (!config)*/
/*        {*/
/*            bstrListDestroy(params);*/
/*            bstrListDestroy(measure);*/
/*            return NULL;*/
/*        }*/
/*        config->parameters = malloc(params->qty * sizeof(LoopAdaptConfigurationParameter));*/
/*        if (!config->parameters)*/
/*        {*/
/*            bstrListDestroy(params);*/
/*            bstrListDestroy(measure);*/
/*            free(config);*/
/*            return NULL;*/
/*        }*/
/*        memset(config->parameters, 0, params->qty * sizeof(LoopAdaptConfigurationParameter));*/
/*        config->measurements = malloc(measure->qty * sizeof(LoopAdaptConfigurationMeasurement));*/
/*        if (!config->measurements)*/
/*        {*/
/*            bstrListDestroy(params);*/
/*            bstrListDestroy(measure);*/
/*            free(config->parameters);*/
/*            free(config);*/
/*            return NULL;*/
/*        }*/
/*        memset(config->measurements, 0, measure->qty * sizeof(LoopAdaptConfigurationMeasurement));*/
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Resize configuration for %d parameters and %d measurements, params->qty, measure->qty);
        err = loop_adapt_configuration_resize_config(&cfile->current, params->qty, measure->qty);
        LoopAdaptConfiguration_t config = cfile->current;
        if (err != 0)
        {
            bstrListDestroy(params);
            bstrListDestroy(measure);
            return NULL;
        }

        for (i = 0; i < params->qty; i++)
        {
            bstring f, s, t;
            loop_adapt_config_parse_default_entry(params->entry[i], &f, &s, &t);
            DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, '%s' '%s' '%s', bdata(f), bdata(s), bdata(t));
            //struct bstrList* plist = bsplit(params->entry[i], '=');
            ParameterValueType_t type = loop_adapt_parameter_type(bdata(f));
            int scope_count = loop_adapt_parameter_scope_count(bdata(f));
            if (type != LOOP_ADAPT_PARAMETER_TYPE_INVALID)
            {
                LoopAdaptConfigurationParameter* p = &config->parameters[pcount];
                p->parameter = bstrcpy(f);
                err = _loop_adapt_get_new_config_txt_resize_values(p, scope_count);

/*                if (p->num_values == 0 || (!p->values))*/
/*                {*/
/*                    p->num_values = scope_count;*/
/*                    DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, Allocating space for %d parameter values of parameter %s, p->num_values, bdata(p->parameter));*/
/*                    p->values = malloc(p->num_values * sizeof(ParameterValue));*/
/*                    for (j = 0; j < p->num_values; j++)*/
/*                    {*/
/*                        p->values[j].type = LOOP_ADAPT_PARAMETER_TYPE_INVALID;*/
/*                        p->values[j].value.sval = NULL;*/
/*                    }*/
/*                }*/
                if (err == 0)
                {
                    if (strncmp(bdata(s), "ALL", 3) == 0)
                    {
                        int id = 0;
                        for (id = 0; id < p->num_values; id++)
                        {
                            ParameterValue* pv = &p->values[id];
                            loop_adapt_parse_param_value(bdata(t), type, pv);
                        }
                    }
                    else
                    {
                        int id = batoi(s);
                        if (id < 0 || id >= p->num_values)
                        {
                            ERROR_PRINT(Faulty ID %d for parameter %s, id, bdata(f));
                            continue;
                        }
                        ParameterValue* pv = &p->values[id];
                        loop_adapt_parse_param_value(bdata(t), type, pv);
                    }
                    p->type = type;
                    pcount++;
                }

            }
            bdestroy(t);
            bdestroy(s);
            bdestroy(f);
            //bstrListDestroy(plist);
        }
        config->num_parameters = pcount;
        bstrListDestroy(params);
        for (i = 0; i < measure->qty; i++)
        {
            struct bstrList* plist = bsplit(measure->entry[i], '=');

            if (loop_adapt_measurement_available(bdata(plist->entry[0])))
            {
                LoopAdaptConfigurationMeasurement* m = &config->measurements[mcount];
                m->measurement = bstrcpy(plist->entry[0]);
                struct bstrList* mlist = bsplit(plist->entry[1], ':');
                m->config = bstrcpy(mlist->entry[0]);
                m->metric = bstrcpy(mlist->entry[1]);
                bstrListDestroy(mlist);
                mcount++;
            }
            bstrListDestroy(plist);
        }
        bstrListDestroy(measure);
        config->num_measurements = mcount;
        config->configuration_id = cfile->line_idx;

/*        if (cfile->current)*/
/*        {*/
/*            loop_adapt_configuration_destroy_config(cfile->current);*/
/*            cfile->current = NULL;*/
/*        }*/
        cfile->current = config;
        cfile->line_idx++;
        DEBUG_PRINT(LOOP_ADAPT_DEBUGLEVEL_DEBUG, New line idx %d, cfile->line_idx);

        return config;
    }
    return NULL;
}


LoopAdaptConfiguration_t loop_adapt_get_current_config_txt(char* string)
{
    int err = 0;
    ConfigurationFile* cfile = 0;
    if (string && loop_adapt_config_txt_input_hash)
    {
        err = get_smap_by_key(loop_adapt_config_txt_input_hash, string, (void*)&cfile);
        if (err == 0)
        {
            return cfile->current;
        }
    }
    return NULL;
}


int loop_adapt_config_txt_output_init()
{
    if (!outputdir)
    {
        char* outdir = getenv("LA_CONFIG_TXT_OUTPUT");
        if (outdir)
        {
            outputdir = outdir;
        }
    }
    if (!loop_adapt_config_txt_output_hash)
    {
        int err = init_smap(&loop_adapt_config_txt_output_hash, _loop_adapt_free_config_txt_output);
        if (err)
        {
            ERROR_PRINT(Failed to initialize hash table for loopname -> outputfile relation);
        }
    }
}

void loop_adapt_config_txt_output_finalize()
{
    if (loop_adapt_config_txt_output_hash)
    {
        destroy_smap(loop_adapt_config_txt_output_hash);
        loop_adapt_config_txt_output_hash = NULL;
        outputdir = NULL;
    }
    return;
}

int loop_adapt_config_txt_output_raw(char* loopname, char* rawstring)
{
    if (outputdir && loopname && rawstring)
    {
        FILE* outputfile = NULL;
        int err = get_smap_by_key(loop_adapt_config_txt_output_hash, loopname, (void*)&outputfile);
        if (err != 0)
        {
            bstring fname = bformat("%s/%s.txt", outputdir, loopname);
            outputfile = fopen(bdata(fname), "w");
            if (outputfile)
            {
                add_smap(loop_adapt_config_txt_output_hash, loopname, (void*)outputfile);
            }
        }
        bstring line = bformat("LOOP=%s|%s\n", loopname, rawstring);
        fwrite(bdata(line), sizeof(char), blength(line), outputfile);
        bdestroy(line);
        fflush(outputfile);
    }
}

int loop_adapt_config_txt_output_write(ThreadData_t thread, char* loopname, LoopAdaptConfiguration_t config, int num_results, ParameterValue* results)
{
    int i = 0, j = 0;
    int err = 0;
    if (outputdir && config && results)
    {
        FILE* outputfile = NULL;
        err = get_smap_by_key(loop_adapt_config_txt_output_hash, loopname, (void*)&outputfile);
        if (err != 0)
        {
            bstring fname = bformat("%s/%s.txt", outputdir, loopname);
            outputfile = fopen(bdata(fname), "w");
            if (outputfile)
            {
                add_smap(loop_adapt_config_txt_output_hash, loopname, (void*)outputfile);
            }
        }

        bstring line = bformat("THREAD=%d:%d|", thread->thread, thread->cpu);
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
        bconchar(line, '\n');

        fwrite(bdata(line), sizeof(char), blength(line), outputfile);
        fflush(outputfile);
        bdestroy(line);
    }
}
