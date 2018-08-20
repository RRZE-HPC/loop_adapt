#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <hwloc.h>
#include <ghash.h>
#include <likwid.h>

#include <loop_adapt_types.h>


char* loop_adapt_configdir = NULL;

__attribute__((constructor)) void loop_adapt_config_init (void)
{
    int ret = 0;
    if (getenv("LOOP_ADAPT_CONFIGDIR") != NULL)
    {
        char* d = getenv("LOOP_ADAPT_CONFIGDIR");
        if (!access(d, R_OK|X_OK|W_OK))
        {
            ret = asprintf(&loop_adapt_configdir, "%s/", d);
            return;
        }
        else
        {
            goto fallback_local;
        }
    }
fallback_local:
    loop_adapt_configdir = malloc(200 * sizeof(char));
    if (loop_adapt_configdir)
    {
        loop_adapt_configdir = getcwd(loop_adapt_configdir, 199);
    }
    return;
}

void _loop_adapt_write_type_parameter_func(gpointer key, gpointer val, gpointer user_data)
{
    char* name = (char*) key;
    FILE *fp = NULL;
    char* filename = (char*)user_data;
    Nodeparameter_t param = (Nodeparameter_t)val;
    fp = fopen(filename, "w");
    if (fp)
    {
        fprintf(fp, "---\n");
        fprintf(fp, "%s:\n", param->name);
        if (param->desc != NULL)
        {
            fprintf(fp, " Description: %s\n", param->desc);
        }
        if (param->type == NODEPARAMETER_INT)
        {
            fprintf(fp, " Type: int\n");
            fprintf(fp, " Min: %d\n", param->min.ival);
            fprintf(fp, " Max: %d\n", param->max.ival);
            fprintf(fp, " Start: %d\n", param->start.ival);
            if (param->has_best)
                fprintf(fp, " Best: %d\n", param->best.ival);
            else
                fprintf(fp, " Best: %d\n", param->start.ival);
            fprintf(fp, " Cur: %d\n", param->cur.ival);
            fprintf(fp, " Steps:\n");
            for (int i = 0; i < param->num_old_vals; i++)
            {
                fprintf(fp, "  - %d\n", param->old_vals[i].ival);
            }
            fprintf(fp, "\n");
        }
        else if (param->type == NODEPARAMETER_DOUBLE)
        {
            fprintf(fp, " Type: double\n");
            fprintf(fp, " Min: %f\n", param->min.dval);
            fprintf(fp, " Max: %f\n", param->max.dval);
            fprintf(fp, " Start: %f\n", param->start.dval);
            if (param->has_best)
                fprintf(fp, " Best: %f\n", param->best.dval);
            else
                fprintf(fp, " Best: %f\n", param->start.dval);
            fprintf(fp, " Cur: %f\n", param->cur.dval);
            fprintf(fp, " Steps:\n");
            for (int i = 0; i < param->num_old_vals; i++)
            {
                fprintf(fp, "  - %f\n", param->old_vals[i].dval);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
}

static int _loop_adapt_write_type_profiles(char* fileprefix, hwloc_topology_t tree, hwloc_obj_type_t type)
{
    int ret = 0;
    FILE *fp = NULL;
    char* fname = NULL;
    int nobj = hwloc_get_nbobjs_by_type(tree, type);
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    for (int i=0; i < nobj; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        if (!obj->userdata)
        {
            printf("No userdata\n");
            continue;
        }
        Nodevalues_t vals = (Nodevalues_t)obj->userdata;
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        ret = asprintf(&fname, "%s_%s_%d.txt", fileprefix, loop_adapt_type_name(obj->type), obj->os_index);
        //printf("Write to file %s\n", fname);
        fp = fopen(fname, "w");
        if (fp)
        {
            fprintf(fp, "---\n");
            fprintf(fp, "%s-%d:\n", loop_adapt_type_name(obj->type), obj->os_index);
            fprintf(fp, " CurrentPolicy: %d\n", vals->cur_policy);
            for (int p=0; p < vals->num_policies; p++)
            {
                fprintf(fp, " Policy%d:\n", p);
                fprintf(fp, "  Name: %s\n", tdata->policies[p]->name);
                fprintf(fp, "  InternalName: %s\n", tdata->policies[p]->internalname);
                fprintf(fp, "  Description: %s\n", tdata->policies[p]->desc);
                fprintf(fp, "  NumProfiles: %d\n", pp->num_profiles);
                fprintf(fp, "  NumValues: %d\n", tdata->policies[p]->num_metrics);
                fprintf(fp, "  NumIterations: %d\n", pp->profile_iters);
                fprintf(fp, "  CurrentProfile: %d\n", pp->cur_profile);
                fprintf(fp, "  OptimalProfile: %d\n", pp->opt_profile);
                fprintf(fp, "  BaseProfile:\n");
                for (int v = 0; v < tdata->policies[p]->num_metrics; v++)
                {
                    fprintf(fp, "    %s: %f\n", tdata->policies[p]->metrics[v].var, pp->base_profile->values[v]);
                }
                for (int k = 1; k < pp->num_profiles; k++)
                {
                    fprintf(fp, "  Profile%d:\n", k);
                    for (int v = 0; v < tdata->policies[p]->num_metrics; v++)
                    {
                        fprintf(fp, "    %s: %f\n", tdata->policies[p]->metrics[v].var, pp->profiles[k]->values[v]);
                    }
                }
            }
            fclose(fp);
            free(fname);
        }
        else
        {
            printf("Cannot open file %s\n", fname);
            printf("%s\n", strerror(errno));
        }
    }
}


int loop_adapt_write_profiles(char* loopname, hwloc_topology_t tree)
{
    FILE* fp = NULL;
    char* fpname = NULL;
    char fileprefix[250];
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    if (tdata && loopname)
    {
        snprintf(fileprefix, 249, "%s/%s", loop_adapt_configdir, loopname);
        if (loop_adapt_debug)
            printf("Write to fileprefix %s\n", fileprefix);
        _loop_adapt_write_type_profiles(fileprefix, tree, HWLOC_OBJ_PU);
        _loop_adapt_write_type_profiles(fileprefix, tree, HWLOC_OBJ_PACKAGE);
        _loop_adapt_write_type_profiles(fileprefix, tree, HWLOC_OBJ_NUMANODE);
        _loop_adapt_write_type_profiles(fileprefix, tree, HWLOC_OBJ_MACHINE);
    }
    return 0;
}

static int _loop_adapt_write_type_params(char* fileprefix, hwloc_topology_t tree, hwloc_obj_type_t type)
{
    int ret = 0;
    FILE *fp = NULL;
    char* fname = NULL;
    int nobj = hwloc_get_nbobjs_by_type(tree, type);
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    for (int i=0; i < nobj; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        if (!obj->userdata)
        {
            printf("No userdata\n");
            continue;
        }
        Nodevalues_t vals = (Nodevalues_t)obj->userdata;
        if (vals->param_hash && g_hash_table_size(vals->param_hash) == 0)
        {
            continue;
        }
        ret = asprintf(&fname, "%s_%s_%d.txt", fileprefix, loop_adapt_type_name(obj->type), obj->os_index);
        if (ret > 0)
        {
            if (vals->param_hash)
            {
                g_hash_table_foreach(vals->param_hash, _loop_adapt_write_type_parameter_func, fname);
            }
            free(fname);
        }
    }
    return 0;
}

int loop_adapt_write_parameters(char* loopname, hwloc_topology_t tree)
{
    FILE* fp = NULL;
    char* fpname = NULL;
    char fileprefix[250];
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    if (tdata && loopname)
    {
        snprintf(fileprefix, 249, "%s/%s_Param", loop_adapt_configdir, loopname);
        if (loop_adapt_debug)
            printf("Write to fileprefix %s\n", fileprefix);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_PU);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_PACKAGE);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_NUMANODE);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_MACHINE);
    }
    return 0;
}

static int _loop_adapt_read_parameters(char* fileprefix, hwloc_topology_t tree, hwloc_obj_type_t type)
{
    int ret = 0;
    FILE *fp = NULL;
    char* fname = NULL;
    char buff[513];
    char key[128];
    char val[128];
    int ival = 0;
    int nobj = hwloc_get_nbobjs_by_type(tree, type);
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    for (int i=0; i < nobj; i++)
    {
        hwloc_obj_t obj = hwloc_get_obj_by_type(tree, type, i);
        if (!obj->userdata)
        {
            printf("No userdata, allocate new one\n");
            //allocate_nodevalues
        }
        Nodevalues_t vals = (Nodevalues_t)obj->userdata;
        if (!vals->param_hash)
        {
            vals->param_hash = g_hash_table_new(g_str_hash, g_str_equal);
        }
        ret = asprintf(&fname, "%s_%s_%d.txt", fileprefix, loop_adapt_type_name(obj->type), obj->os_index);
        if (!access(fname, R_OK))
        {
            Nodeparameter_t p = malloc(sizeof(Nodeparameter));
            fp = fopen(fname, "w");
            if (fp)
            {
                while(fgets(buff, 512, fp) != NULL)
                {
                    if (strncmp(buff, "Param", 5) == 0)
                    {
                        ret = sscanf(&(buff[6]), "%s", val);
                        if (ret == 1)
                        {
                            ret = asprintf(&p->name, "%s", val);
                        }
                    }
                    else if (strncmp(buff, "\tType:", 6) == 0)
                    {
                        ret = sscanf(&(buff[7]), "%s", val);
                        if (strncmp(val, "int", 3) == 0)
                        {
                            p->type = NODEPARAMETER_INT;
                        }
                        else if (strncmp(val, "double", 6) == 0)
                        {
                            p->type = NODEPARAMETER_DOUBLE;
                        }
                    }
                    else if (strncmp(buff, "\tSteps:", 7) == 0)
                    {
                        break;
                    }
                    else
                    {
                        
                    }
                }
            }
        }
    }
}

int loop_adapt_read_parameters(char* loopname, hwloc_topology_t tree)
{
    FILE* fp = NULL;
    char fileprefix[250];
    Treedata_t tdata = hwloc_topology_get_userdata(tree);
    if (tdata && loopname)
    {
        snprintf(fileprefix, 249, "%s/%s_Param", loop_adapt_configdir, loopname);
        printf("Read from fileprefix %s\n", fileprefix);
        _loop_adapt_read_parameters(fileprefix, tree, HWLOC_OBJ_PU);
        _loop_adapt_read_parameters(fileprefix, tree, HWLOC_OBJ_PACKAGE);
        _loop_adapt_read_parameters(fileprefix, tree, HWLOC_OBJ_NUMANODE);
        _loop_adapt_read_parameters(fileprefix, tree, HWLOC_OBJ_MACHINE);
    }
}



__attribute__((destructor)) void loop_adapt_config_finalize (void)
{
    if (loop_adapt_configdir)
    {
        free(loop_adapt_configdir);
        loop_adapt_configdir = NULL;
    }
}
