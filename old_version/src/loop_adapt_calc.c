#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <loop_adapt.h>
#include <loop_adapt_types.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>

char* in_func_str = "require('math')\nfunction SUM(...);local s = 0;for k,v in pairs({...}) do;s = s + v;end;return s;end\nfunction ARGS(...) return #arg end;function AVG(...) return SUM(...)/ARGS(...) end;MEAN = AVG\nfunction MIN(...) return math.min(...) end\nfunction MAX(...) return math.max(...) end\nfunction MEDIAN(...);local x = {...};local l = ARGS(...);table.sort(x);return x[math.floor(l/2)];end\nfunction PERCENTILE(perc, ...);local x = {...};local xlen = #x;table.sort(x);local idx = math.ceil((perc/100.0)*xlen);return x[idx];end\nfunction IFELSE(cond, valid, invalid);if cond then;return valid;else;return invalid;end;end";


lua_State** lua_states = NULL;
int num_states = 0;
struct bstrList** bdefines = NULL;
struct bstrList* bglobal_defs = NULL;
bstring in_user_funcs;
bstring bnewline;


int la_calc_add_var(char* name, char* suffix, double value, bstring bvars)
{
    if (name && suffix)
    {
        printf("%s %s %f\n", name, suffix, value);
        bstring tmp = bformat("%s_%s = %.25f\n", name, suffix, value);
        bconcat(bvars, tmp);
        bdestroy(tmp);
    }
    return 0;
}

int la_calc_add_def(char* name, double value, int cpu)
{
    int slen = 0;
    int ret = 0;
    bstring tmp = bformat("%s = %.25f", name, value);
    if (cpu >= 0)
    {
        bstrListAdd(bdefines[cpu], tmp);
    }
    else
    {
        bstrListAdd(bglobal_defs, tmp);
    }
    bdestroy(tmp);
    return 0;
}

int la_calc_add_lua_func(char* func)
{

    bcatcstr(in_user_funcs, func);
    return 0;
}

int la_calc_evaluate(Policy_t p, PolicyParameter_t param, double *opt_values, double opt_runtime, double *cur_values, double cur_runtime)
{
    int cpu = sched_getcpu();
    char* out = NULL;
    int out_int = 0;
    char* vars = NULL;
    bstring bvars = bfromcstr("");
    
    lua_State *L = lua_states[cpu];

    if (!L)
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        lua_states[cpu] = L;
    }

    for (int i = 0; i < p->num_metrics; i++)
    {
        la_calc_add_var(p->metrics[i].var, "opt", opt_values[i], bvars);
        la_calc_add_var(p->metrics[i].var, "cur", cur_values[i], bvars);
/*        if (loop_adapt_debug == 2)*/
/*        {*/
            printf("Opt Metric %s_opt : %f\n", p->metrics[i].var, opt_values[i] );
            printf("Cur Metric %s_cur : %f\n", p->metrics[i].var, cur_values[i] );
            fflush(stdout);
/*        }*/
    }
    la_calc_add_var("runtime", "opt", opt_runtime, bvars);
    la_calc_add_var("runtime", "cur", cur_runtime, bvars);

    bstring gldef = bjoin(bglobal_defs, bnewline);
    bstring cdef = bjoin(bdefines[cpu], bnewline);
    bstring f = bformat("%s\n%s\n%s\n%s\n%s\nreturn tostring(%s)", in_func_str,
                                                                   (blength(gldef) > 0 ? bdata(gldef) : ""),
                                                                   (blength(in_user_funcs) ? bdata(in_user_funcs) : ""),
                                                                   (blength(cdef) ? bdata(cdef) : ""),
                                                                   bdata(bvars),
                                                                   param->eval);
                                        
    int ret = luaL_dostring(L, bdata(f));
    if (!ret)
    {
        out = (char*)lua_tostring(L, -1);
        if (out && strncmp(out, "true", 4) == 0)
            out_int = 1;
    }
    bdestroy(f);
    bdestroy(bvars);
    return out_int;
}

double la_calc_calculate(char* f)
{
    int cpu = sched_getcpu();
    char* out = NULL;
    double out_dbl = 0;
    lua_State *L = lua_states[cpu];

    if (!L)
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        lua_states[cpu] = L;
    }

    bstring cdefs = bjoin(bdefines[cpu], bnewline);
    bstring t = bformat("%s\n%s\n%s\nreturn tostring(%s)", in_func_str,
                            (blength(in_user_funcs) ? bdata(in_user_funcs) : ""),
                            (blength(cdefs) ? bdata(cdefs) : ""));

    int ret = luaL_dostring (L, bdata(t));
    if (!ret)
    {
        out = (char*)lua_tostring(L, -1);
        if (out)
        {
            out_dbl = atof(out);
        }
    }
    bdestroy(cdefs);
    bdestroy(t);
    return out_dbl;
}

void __attribute__((constructor (103))) init_loop_adapt_calc(void)
{
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);
    lua_states = (lua_State**)malloc(cpus * sizeof(lua_State*));
    if (lua_states)
    {
        memset(lua_states, 0, cpus * sizeof(lua_State*));
    }
    num_states = cpus;

    bdefines = (struct bstrList**) malloc(cpus * sizeof(struct bstrList*));
    if (bdefines)
    {
        for (int i = 0; i < cpus; i++)
        {
            bdefines[i] = bstrListCreate();
        }
    }
    bglobal_defs = bstrListCreate();
    in_user_funcs = bfromcstr("");
    bnewline = bfromcstr("\n");

    la_calc_add_def("SIZEOF_DOUBLE", (double)sizeof(double), -1);
    la_calc_add_def("SIZEOF_INT", (double)sizeof(int), -1);
    la_calc_add_def("SIZEOF_FLOAT", (double)sizeof(float), -1);
    la_calc_add_def("TRUE", 1.0, -1);
    la_calc_add_def("FALSE", 0.0, -1);
}

void __attribute__((destructor (103))) close_loop_adapt_calc(void)
{

    for (int i = 0; i < num_states; i++)
    {
        if (bdefines[i])
            bstrListDestroy(bdefines[i]);
    }
    if (bdefines)
        free(bdefines);
    bstrListDestroy(bglobal_defs);
    bdestroy(in_user_funcs);
    bdestroy(bnewline);

}
