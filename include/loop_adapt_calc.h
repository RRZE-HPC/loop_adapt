#ifndef LOOP_ADAPT_CALC_H
#define LOOP_ADAPT_CALC_H

int add_def(char* name, double value, int cpu);
int add_lua_func(char* func);
double calculate(char* f);
int evaluate(Policy_t p, double *opt_values, double *cur_values);

#endif
