#ifndef LOOP_ADAPT_SEARCH_H
#define LOOP_ADAPT_SEARCH_H

#include <loop_adapt_types.h>



SearchAlgorithm_t loop_adapt_search_select(char* algoname);
//int loop_adapt_search_add_int_param(SearchAlgorithm_t s, char* name, int cur, int min, int max, int steps);
//int loop_adapt_search_add_dbl_param(SearchAlgorithm_t s, char* name, double cur, double min, double max, int steps);
int loop_adapt_search_param_init(SearchAlgorithm_t s, Nodeparameter_t np);
int loop_adapt_search_param_next(SearchAlgorithm_t s, Nodeparameter_t np);
int loop_adapt_search_param_best(SearchAlgorithm_t s, Nodeparameter_t np);
int loop_adapt_search_param_reset(SearchAlgorithm_t s, Nodeparameter_t np);
#endif
