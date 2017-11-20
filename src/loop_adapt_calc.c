#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <loop_adapt_types.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>

char* in_func_str = "require('math')\nfunction SUM(...);local s = 0;for k,v in pairs({...}) do;s = s + v;end;return s;end\nfunction ARGS(...) return #arg end;function AVG(...) return SUM(...)/ARGS(...) end;MEAN = AVG\nfunction MIN(...) return math.min(...) end\nfunction MAX(...) return math.max(...) end\nfunction MEDIAN(...);local x = {...};local l = ARGS(...);table.sort(x);return x[math.floor(l/2)];end\nfunction PERCENTILE(perc, ...);local x = {...};local xlen = #x;table.sort(x);local idx = math.ceil((perc/100.0)*xlen);return x[idx];end\nfunction IFELSE(cond, valid, invalid);if cond then;return valid;else;return invalid;end;end";

char* in_user_funcs = NULL;

lua_State** lua_states = NULL;
int num_states = 0;
char** defines = NULL;
int* num_defines = NULL;

/*static inline void *realloc_buffer(void *ptrmem, size_t size) {*/
/*    void *ptr = realloc(ptrmem, size);*/
/*    if (!ptr)  {*/
/*        fprintf(stderr, "realloc(%p, %lu): errno=%d\n", ptrmem, size, errno);*/
/*        free (ptrmem);*/
/*    }*/
/*    if (!ptrmem)*/
/*    {*/
/*        memset(ptr, 0, size);*/
/*    }*/
/*    return ptr;*/
/*}*/

int la_calc_add_var(char* name, char* suffix, double value, char** varstr)
{
    int slen = 0;
    int ret = 0;
    if (*varstr)
        slen = strlen(*varstr);
    int add = strlen(name) + 54;
    *varstr = realloc_buffer(*varstr, slen + add + 4);
    if (!*varstr)
    {
        return -ENOMEM;
    }
    ret = snprintf(&((*varstr)[slen]), add+4, "%s_%s = %.25f\n", name, suffix, value);
    return 0;
}

int la_calc_add_def(char* name, double value, int cpu)
{
    int slen = 0;
    int ret = 0;
    if (defines[cpu])
        slen = strlen(defines[cpu]);
    int add = strlen(name) + 54;
    defines[cpu] = realloc_buffer(defines[cpu], slen + add + 2);
    if (!defines[cpu])
    {
        return -ENOMEM;
    }
    ret = snprintf(&(defines[cpu][slen]), add+2, "%s = %.25f\n", name, value);
    num_defines[cpu]++;
    return 0;
}

int la_calc_add_lua_func(char* func)
{
    int slen = strlen(in_user_funcs);
    in_user_funcs = realloc_buffer(in_user_funcs, slen + strlen(func)+10);
    if (!in_user_funcs)
        return -ENOMEM;
    int ret = snprintf(&(in_user_funcs[slen]), strlen(func)+4, "%s\n", func);
    return 0;
}

int la_calc_evaluate(Policy_t p, PolicyParameter_t param, double *opt_values, double *cur_values)
{
    int cpu = sched_getcpu();
    char* out = NULL;
    int out_int = 0;
    char* vars = NULL;
    lua_State *L = lua_states[cpu];

    if (!L)
    {
        L = luaL_newstate();
        luaL_openlibs(L);
        lua_states[cpu] = L;
    }
    
    for (int i = 0; i < p->num_metrics; i++)
    {
        int idx = p->metric_idx[i];
        la_calc_add_var(p->metrics[i].var, "opt", opt_values[idx], &vars);
        la_calc_add_var(p->metrics[i].var, "cur", cur_values[idx], &vars);
        //printf("Metric %d : %f\n", idx,cur_values[idx] );
    }
    
    char* t = NULL;
    int ret = asprintf(&t, "%s\n%s\n%s\n%s\nreturn tostring(%s)", in_func_str,
                            (in_user_funcs ? in_user_funcs : ""),
                            (defines[cpu] ? defines[cpu] : ""),
                            vars, param->eval);
    //printf("%s\n",t);
    ret = luaL_dostring (L, t);
    if (!ret)
    {
        out = (char*)lua_tostring(L, -1);
        if (out && strncmp(out, "true", 4) == 0)
            out_int = 1;
    }
    free(t);
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
    
    char* t = NULL;
    int ret = asprintf(&t, "%s\n%s\n%s\nreturn tostring(%s)", in_func_str,
                            (in_user_funcs ? in_user_funcs : ""),
                            (defines[cpu] ? defines[cpu] : ""), f);
    //printf("%s\n", t);
    ret = luaL_dostring (L, t);
    if (!ret)
    {
        out = (char*)lua_tostring(L, -1);
        if (out)
        {
            out_dbl = atof(out);
        }
    }
    free(t);
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
    defines = (char**)malloc(cpus * sizeof(char*));
    if (defines)
    {
        memset(defines, 0, cpus * sizeof(char*));
    }
    num_defines = (int*)malloc(cpus * sizeof(int));
    if (defines)
    {
        memset(num_defines, 0, cpus * sizeof(int));
    }
    for (int c = 0; c < cpus; c++)
    {
        la_calc_add_def("SIZEOF_DOUBLE", (double)sizeof(double), c);
        la_calc_add_def("SIZEOF_INT", (double)sizeof(int), c);
        la_calc_add_def("SIZEOF_FLOAT", (double)sizeof(float), c);
        la_calc_add_def("TRUE", 1.0, c);
        la_calc_add_def("FALSE", 0.0, c);
    }
}

void __attribute__((destructor (103))) close_loop_adapt_calc(void)
{
    if (lua_states && num_states > 0)
    {
        for (int i = 0; i < num_states; i++)
        {
            if (lua_states[i])
                lua_close(lua_states[i]);
        }
        free(lua_states);
    }
    for (int i = 0; i < num_states; i++)
    {
        if (defines[i])
        {
            free(defines[i]);
            num_defines[i] = 0;
        }
    }
    free(defines);
    free(num_defines);
}
