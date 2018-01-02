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
    FILE *fp = (FILE *)user_data;
    Nodeparameter_t param = (Nodeparameter_t)val;
    fprintf(fp, "- Param %s\n", param->name);
    if (param->desc != NULL)
    {
        fprintf(fp, "-- Description %s\n", param->desc);
    }
    if (param->type == NODEPARAMETER_INT)
    {
        fprintf(fp, "-- Type int\n");
        fprintf(fp, "-- Min %d\n", param->min.imin);
        fprintf(fp, "-- Max %d\n", param->max.imax);
        fprintf(fp, "-- Start %d\n", param->start.istart);
        fprintf(fp, "-- Best %d\n", param->best.ibest);
        fprintf(fp, "-- Cur %d\n", param->cur.icur);
    }
    else if (param->type == NODEPARAMETER_DOUBLE)
    {
        fprintf(fp, "-- Type double\n");
        fprintf(fp, "-- Min %f\n", param->min.dmin);
        fprintf(fp, "-- Max %f\n", param->max.dmax);
        fprintf(fp, "-- Start %f\n", param->start.dstart);
        fprintf(fp, "-- Best %f\n", param->best.dbest);
        fprintf(fp, "-- Cur %f\n", param->cur.dcur);
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
        
        ret = asprintf(&fname, "%s_%s_%d.txt", fileprefix, loop_adapt_type_name(obj->type), obj->os_index);
        //printf("Write to file %s\n", fname);
        fp = fopen(fname, "w");
        if (fp)
        {
            fprintf(fp, "Node Type %s Index %d\n", loop_adapt_type_name(obj->type), obj->os_index);
            for (int p=0; p < vals->num_policies; p++)
            {
                fprintf(fp, "- Policy: %d %s\n", p, tdata->policies[p]->name);
                fprintf(fp, "- Current policy: %d\n", vals->cur_policy);
                fprintf(fp, "- Number of profiles: %d\n", vals->num_profiles[p]);
                fprintf(fp, "- Current profile: %d\n", vals->cur_profile);
                fprintf(fp, "- Number of values per profile: %d\n", tdata->policies[p]->num_metrics);
                fprintf(fp, "- Number of iterations: %d\n", vals->profile_iters[p]);
                fprintf(fp, "- Optimal profile: %d\n", vals->opt_profiles[p]);
                fprintf(fp, "- Start Profile\n", 0);
                for (int v = 0; v < tdata->policies[p]->num_metrics; v++)
                {
                    fprintf(fp, "-- Value %s: %f\n", tdata->policies[p]->metrics[v].var, vals->profiles[p][0][v]);
                }
                for (int k = 1; k < vals->num_profiles[p]; k++)
                {
                    fprintf(fp, "- Profile: %d\n", k);
                    for (int v = 0; v < tdata->policies[p]->num_metrics; v++)
                    {
                        fprintf(fp, "-- Value %s: %f\n", tdata->policies[p]->metrics[v].var, vals->profiles[p][k][v]);
                    }
                }
            }
/*            if (vals->param_hash)*/
/*            {*/
/*                g_hash_table_foreach(vals->param_hash, _loop_adapt_write_type_parameter_func, fp);*/
/*            }*/
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
        fp = fopen(fname, "w");
        if (fp)
        {
            if (vals->param_hash)
            {
                g_hash_table_foreach(vals->param_hash, _loop_adapt_write_type_parameter_func, fp);
            }
            fclose(fp);
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
        printf("Write to fileprefix %s\n", fileprefix);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_PU);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_PACKAGE);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_NUMANODE);
        _loop_adapt_write_type_params(fileprefix, tree, HWLOC_OBJ_MACHINE);
    }
    return 0;
}

__attribute__((destructor)) void loop_adapt_config_finalize (void)
{
    if (loop_adapt_configdir)
    {
        free(loop_adapt_configdir);
        loop_adapt_configdir = NULL;
    }
}
