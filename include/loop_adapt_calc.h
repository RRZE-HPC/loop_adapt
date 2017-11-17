#ifndef LOOP_ADAPT_CALC_H
#define LOOP_ADAPT_CALC_H

int la_calc_add_def(char* name, double value, int cpu);
int la_calc_add_lua_func(char* func);
double la_calc_calculate(char* f);
int la_calc_evaluate(Policy_t p, PolicyParameter_t param, double *opt_values, double *cur_values);

#endif
