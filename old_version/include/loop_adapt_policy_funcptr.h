#ifndef LOOP_ADAPT_EVAL_FUNCPTR_H
#define LOOP_ADAPT_EVAL_FUNCPTR_H


#include <loop_adapt_policy.h>
#include <loop_adapt_helper.h>


POL_FUNCS(funcptr)

Policy POL_FUNCPTR = {
    .internalname = "POL_FUNCPTR",
    .name = "Switch through function pointers",
    .desc = "This policy tries to test multiple implementations by exchanging the function pointer.",
    STR_POL_FUNCS(funcptr)
    .scope = LOOP_ADAPT_SCOPE_MACHINE,
    
    .num_parameters = 1,
    .parameters = { {"funcptr",
                     "Variable that contains the function pointer",
                     "NULL",
                     "NULL",
                     "runtime_opt > runtime_cur"} },
/*    .num_metrics = 1,*/
/*    .metrics = {{ "time", "Runtime (RDTSC) [s]"},},*/
    //.eval = "l2l3_opt > l2l3_cur"
};









#endif /* LOOP_ADAPT_EVAL_FUNCPTR_H */
