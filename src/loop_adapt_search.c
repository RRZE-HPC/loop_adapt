#include <stdlib.h>
#include <stdio.h>
#include <math.h>


#include <loop_adapt_search.h>
#include <loop_adapt_helper.h>

int basic_search_init(Nodeparameter_t np)
{
    switch (np->type)
    {
        case NODEPARAMETER_INT:
            np->cur.ival = np->min.ival;
            break;
        case NODEPARAMETER_DOUBLE:
            np->cur.dval = np->min.dval;
            break;
    }
}

int basic_search_next(Nodeparameter_t np)
{
    Value old;
    switch (np->type)
    {
        case NODEPARAMETER_INT:
            {
                if (np->change <= np->steps+1)
                {
                    old.ival = np->cur.ival;
                    double min = (double)np->min.ival;
                    double max = (double)np->max.ival;
                    double s = ceil(abs(max-min+1) / (np->steps+1));
                    np->cur.ival = MIN((int)(np->change > 0 ? np->cur.ival + s : np->min.ival), np->max.ival);
                }
                else if (np->has_best)
                {
                    np->cur.ival = np->best.ival;
                }
                else
                {
                    np->cur.ival = np->start.ival;
                }
            }
            break;
        case NODEPARAMETER_DOUBLE:
            {
                if (np->change <= np->steps+1)
                {
                    old.dval = np->cur.dval;
                    double min = (double)np->min.dval;
                    double max = (double)np->max.dval;
                    double s = ceil(abs(max-min+1) / (np->steps+1));
                    np->cur.dval = MIN((int)(np->change > 0 ? np->cur.dval + s : np->min.dval), np->max.dval);
                }
                else if (np->has_best)
                {
                    np->cur.ival = np->best.ival;
                }
                else
                {
                    np->cur.ival = np->start.ival;
                }
            }
            break;
    }
    np->change++;
    return 0;
}

int basic_search_markbest(Nodeparameter_t np)
{
    switch (np->type)
    {
        case NODEPARAMETER_INT:
            np->best.ival = np->cur.ival;
            break;
        case NODEPARAMETER_DOUBLE:
            np->best.dval = np->cur.dval;
            break;
    }
    np->has_best = 1;
}

int basic_search_reset(Nodeparameter_t np)
{
    switch (np->type)
    {
        case NODEPARAMETER_INT:
            np->cur.ival = (!np->has_best ? np->start.ival : np->best.ival);
            break;
        case NODEPARAMETER_DOUBLE:
            np->cur.dval = (!np->has_best ? np->start.dval : np->best.dval);
            break;
    }
}

/* Or use the dlopen method to be prepared to load various search algorithms
 * provided as shared libraries
 */
#define NUM_LA_SEARCH_ALGOS 1
static SearchAlgorithm search_algos[NUM_LA_SEARCH_ALGOS] = {
    {"SEARCH_BASIC", basic_search_init, basic_search_next, basic_search_markbest, basic_search_reset},
};

SearchAlgorithm_t loop_adapt_search_select(char *algoname)
{
    for (int i = 0; i < NUM_LA_SEARCH_ALGOS; i++)
    {
        if (strncmp(algoname, search_algos[i].name, strlen(search_algos[i].name)) == 0)
        {
            SearchAlgorithm_t s = malloc(sizeof(SearchAlgorithm));
            if (s)
            {
                s->name = search_algos[i].name;
                s->init = search_algos[i].init;
                s->next = search_algos[i].next;
                s->best = search_algos[i].next;
                s->reset = search_algos[i].reset;
            }
            return s;
        }
    }
    return NULL;
}

int loop_adapt_search_param_init(SearchAlgorithm_t s, Nodeparameter_t np)
{
    if (!s || !np)
    {
        return -EINVAL;
    }

    if (s->init)
    {
        s->init(np);
    }
    return 0;
}


int loop_adapt_search_param_next(SearchAlgorithm_t s, Nodeparameter_t np)
{
    if (!s || !np)
    {
        return -EINVAL;
    }

    if (s->next)
    {
        Value old = np->cur;
        s->next(np);
        np->old_vals[np->num_old_vals] = old;
        np->num_old_vals++;
    }
    return 0;
}

int loop_adapt_search_param_best(SearchAlgorithm_t s, Nodeparameter_t np)
{
    if (!s || !np)
    {
        return -EINVAL;
    }

    if (s->best)
    {
        s->best(np);
    }
    return 0;
}

int loop_adapt_search_param_reset(SearchAlgorithm_t s, Nodeparameter_t np)
{
    if (!s || !np)
    {
        return -EINVAL;
    }

    if (s->reset)
    {
        Value old = np->cur;
        s->reset(np);
        if (np->old_vals[np->num_old_vals-1].ival != old.ival)
        {
            np->old_vals[np->num_old_vals] = old;
            np->num_old_vals++;
        }
    }
    return 0;
}



