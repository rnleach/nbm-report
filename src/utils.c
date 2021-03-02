#include "utils.h"

#include <math.h>
#include <time.h>

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
    } else if (isnan(val)) {
        return acc;
    }

    return acc + val;
}

double
accum_max(double acc, double val)
{
    if (isnan(acc)) {
        return val;
    }

    // If isnan(val), then the comparison is false and acc is returned.
    return acc < val ? val : acc;
}

double
accum_last(double acc, double val)
{
    if (isnan(val)) {
        return acc;
    }

    return val;
}
