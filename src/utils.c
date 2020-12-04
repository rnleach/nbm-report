
#include "utils.h"

/*-------------------------------------------------------------------------------------------------
 *                         Compare function for soring time_t in ascending order.
 *-----------------------------------------------------------------------------------------------*/
int
time_t_compare_func(void const *a, void const *b, void *user_data)
{
    time_t const *dsa = a;
    time_t const *dsb = b;

    if (*dsa < *dsb)
        return -1;
    if (*dsa > *dsb)
        return 1;
    return 0;
}

/*-------------------------------------------------------------------------------------------------
 *                                          Accumulators
 *-----------------------------------------------------------------------------------------------*/
double
accum_sum(double acc, double val)
{
    if (isnan(acc)) {
        return val;
    }

    return acc + val;
}

double
accum_max(double acc, double val)
{
    if (isnan(acc)) {
        return val;
    }

    return acc > val ? acc : val;
}

double
accum_last(double _acc, double val)
{
    return val;
}

double
accum_avg(double acc, double val)
{
    static int count = 0;
    if (isnan(acc)) {
        count = 1;
        return val;
    }

    count++;
    return acc * ((count - 1.0) / (double)count) + val / count;
}
