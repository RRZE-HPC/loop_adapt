#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <loop_adapt.h>
#include <loop_adapt_search.h>
#include <loop_adapt_helper.h>

int basic_search_init(Nodeparameter_t np)
{
    switch (np->type)
    {
        case NODEPARAMETER_INT:
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Init parameter %s to %d\n", np->name, np->min.ival);
            np->cur.ival = np->min.ival;
            break;
        case NODEPARAMETER_DOUBLE:
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Init parameter %s to %f\n", np->name, np->min.dval);
            np->cur.dval = np->min.dval;
            break;
    }
}

int basic_search_next(Nodeparameter_t np)
{
    Value old;
    if (np->change == 0)
        np->change = 1;
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
                    np->cur.ival = MIN((int)(np->change >= 0 ? np->min.ival + (s*np->change) : np->min.ival), np->max.ival);
                    if (loop_adapt_debug > 1)
                        fprintf(stderr, "Next parameter %s to %d\n", np->name, np->cur.ival);
                }
                else if (np->has_best)
                {
                    np->cur.ival = np->best.ival;
                    if (loop_adapt_debug > 1)
                        fprintf(stderr, "Next parameter %s to %d (best)\n", np->name, np->cur.ival);
                }
                else
                {
                    np->cur.ival = np->start.ival;
                    if (loop_adapt_debug > 1)
                        fprintf(stderr, "Next parameter %s to %d (start)\n", np->name, np->cur.ival);
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
                    double s = abs(max-min+1) / (np->steps+1);
                    np->cur.dval = MIN((int)(np->change >= 0 ? np->min.ival + (s*np->change) : np->min.dval), np->max.dval);
                    if (loop_adapt_debug > 1)
                    {
                        printf("Min %f Max %f Step %f Abs %f\n", min, max, s, abs(max-min+1));
                        fprintf(stderr, "Next parameter %s to %f\n", np->name, np->cur.dval);
                    }
                }
                else if (np->has_best)
                {
                    np->cur.ival = np->best.ival;
                    if (loop_adapt_debug > 1)
                        fprintf(stderr, "Next parameter %s to %f (best)\n", np->name, np->cur.dval);
                }
                else
                {
                    np->cur.ival = np->start.ival;
                    if (loop_adapt_debug > 1)
                        fprintf(stderr, "Next parameter %s to %f (start)\n", np->name, np->cur.dval);
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
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Best parameter %s to %d\n", np->name, np->best.ival);
            break;
        case NODEPARAMETER_DOUBLE:
            np->best.dval = np->cur.dval;
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Best parameter %s to %f\n", np->name, np->best.dval);
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
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Reset parameter %s to %d\n", np->name, np->cur.ival);
            break;
        case NODEPARAMETER_DOUBLE:
            np->cur.dval = (!np->has_best ? np->start.dval : np->best.dval);
            if (loop_adapt_debug > 1)
                fprintf(stderr, "Reset parameter %s to %f\n", np->name, np->cur.dval);
            break;
    }
    np->change = np->steps+1;
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
        switch (np->type)
        {
            case NODEPARAMETER_INT:
                if (np->old_vals[np->num_old_vals-1].ival != old.ival)
                {
                    np->old_vals[np->num_old_vals] = old;
                    np->num_old_vals++;
                }
                break;
            case NODEPARAMETER_DOUBLE:
                if (np->old_vals[np->num_old_vals-1].dval != old.dval)
                {
                    np->old_vals[np->num_old_vals] = old;
                    np->num_old_vals++;
                }
                break;
        }
    }
    return 0;
}



