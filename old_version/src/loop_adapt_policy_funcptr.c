#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <loop_adapt_policy_funcptr.h>
#include <loop_adapt_helper.h>
#include <loop_adapt_calc.h>


static pthread_mutex_t funcptr_lock = PTHREAD_MUTEX_INITIALIZER;

int loop_adapt_policy_funcptr_init(int num_cpus, int* cpulist, int num_profiles)
{
    topology_init();
    timer_init();
    int npros = num_profiles;
    npros++;
}

int loop_adapt_policy_funcptr_begin(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    TimerData* t = NULL;

    if (tdata && vals)
    {
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        t = loop_adapt_policy_profile_get_timer(pp);
        if (t)
        {
            timer_stop(t);
            timer_start(t);
        }
    }
    return ret;
}

int loop_adapt_policy_funcptr_end(int cpuid, hwloc_topology_t tree, hwloc_obj_t obj)
{
    int ret = 0;
    double* pdata = NULL;
    double *runtime = NULL;
    Policy_t p = NULL;
    TimerData *t = NULL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    if (tdata && vals)
    {
        p = tdata->cur_policy;
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        t = loop_adapt_policy_profile_get_timer(pp);
        if (t)
            timer_stop(t);
        runtime = loop_adapt_policy_profile_get_runtime(pp);
        *runtime = timer_print(t);
        pp->num_values = 0;
    }
    return ret;
}

int loop_adapt_policy_funcptr_eval(hwloc_topology_t tree, hwloc_obj_t obj)
{
    int eval = 0;
    unsigned int (*tcount_func)() = NULL;
    Treedata_t tdata = (Treedata_t)hwloc_topology_get_userdata(tree);
    Nodevalues_t vals = (Nodevalues_t)obj->userdata;
    double cur_runtime = 0;
    double opt_runtime = 0;
    double *cur_values = NULL;
    double *opt_values = NULL;
    if (tdata && vals)
    {
        Policy_t p = tdata->cur_policy;
        PolicyProfile_t pp = vals->policies[vals->cur_policy];
        if (pp->cur_profile >= 0)
        {
            cur_values = pp->profiles[pp->cur_profile-1]->values;
            cur_runtime = pp->profiles[pp->cur_profile-1]->runtime;
        }
        else
        {
            return 0;
        }
        if (pp->opt_profile >= 0)
        {
            opt_runtime = pp->profiles[pp->opt_profile]->runtime;
        }
        else
        {
            opt_runtime = pp->base_profile->runtime;
        }
        if (getLoopAdaptType(obj->type) == p->scope)
        {
            for (int i = 0; i < p->num_parameters; i++)
            {
                PolicyParameter_t pparam = &p->parameters[i];
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_FUNCPTR: Evaluate param %s for %s with idx %d (opt:%d, cur:%d)\n", pparam->name, loop_adapt_type_name((AdaptScope)obj->type), obj->logical_index, pp->opt_profile, pp->cur_profile-1);

                eval = la_calc_evaluate(p, pparam, opt_values, opt_runtime, cur_values, cur_runtime);
                if (loop_adapt_debug == 2)
                    fprintf(stderr, "DEBUG POL_FUNCPTR: Evaluation %s = %d\n", pparam->eval, eval);
            }
        }
    }
    // Walk up the tree to see whether adjustments are required.
    // Currently this does not wait until all subnodes have updated the
    // values.
    hwloc_obj_t walker = obj->parent;
    while (walker)
    {
        if (walker->type == HWLOC_OBJ_PACKAGE ||
            walker->type == HWLOC_OBJ_NUMANODE ||
            walker->type == HWLOC_OBJ_MACHINE ||
            walker->type == HWLOC_OBJ_PU)
        {
            eval += loop_adapt_policy_funcptr_eval(tree, walker);
            walker = walker->parent;
        }
        else
        {
            walker = walker->parent;
        }
    }
    return eval;
}

void loop_adapt_policy_funcptr_close()
{
    return;
}
