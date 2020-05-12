#ifndef LOOP_ADAPT_POLICY_LIST_H
#define LOOP_ADAPT_POLICY_LIST_H

#include <loop_adapt_policy_types.h>
#include <loop_adapt_policy_functions.h>

#define NUM_LOOP_ADAPT_POLICIES 9

int loop_adapt_policy_list_count = NUM_LOOP_ADAPT_POLICIES;

_PolicyDefinition loop_adapt_policy_list[NUM_LOOP_ADAPT_POLICIES] = {
    {.name = "MIN_TIME",
     .backend = "TIMER", 
     .config = "REALTIME",
     .eval = loop_adapt_policy_function_min,
    },
    {.name = "MAX_TIME",
     .backend = "TIMER", 
     .config = "REALTIME",
     .eval = loop_adapt_policy_function_max,
    },
    {.name = "SUM_TIME",
     .backend = "TIMER", 
     .config = "REALTIME",
     .eval = loop_adapt_policy_function_sum,
    },
    {.name = "MIN_L3VOL",
     .backend = "LIKWID",
     .config = "L3",
     .match = "L3 data volume",
     .eval = loop_adapt_policy_function_min,
    },
    {.name = "MAX_L3VOL",
     .backend = "LIKWID",
     .config = "L3",
     .match = "L3 data volume",
     .eval = loop_adapt_policy_function_max,
    },
    {.name = "SUM_L3VOL",
     .backend = "LIKWID",
     .config = "L3",
     .match = "L3 data volume",
     .eval = loop_adapt_policy_function_sum,
    },
    {.name = "MIN_L2VOL",
     .backend = "LIKWID",
     .config = "L2",
     .match = "L2 data volume",
     .eval = loop_adapt_policy_function_min,
    },
    {.name = "MAX_L2VOL",
     .backend = "LIKWID",
     .config = "L2",
     .match = "L2 data volume",
     .eval = loop_adapt_policy_function_max,
    },
    {.name = "SUM_L2VOL",
     .backend = "LIKWID",
     .config = "L2",
     .match = "L2 data volume",
     .eval = loop_adapt_policy_function_sum,
    }


};

#endif