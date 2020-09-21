#include "summarize.h"
#include "nbm_data.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

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
 *                             Extract Values for a Daily Summary.
 *-----------------------------------------------------------------------------------------------*/
void
extract_daily_summary_for_column(GTree *sums, struct NBMData const *nbm, char const *col_name,
                                 KeepFilter filter, SummarizeDate date_sum, Converter convert,
                                 Accumulator accumulate, Creator create_new, Extractor extract)
{
    struct NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator.");

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    while (view.valid_time && view.value) {

        if (filter(view.valid_time)) {
            time_t date = date_sum(view.valid_time);
            double x_val = convert(*view.value);

            void *sum = g_tree_lookup(sums, &date);
            if (!sum) {
                time_t *key = malloc(sizeof(time_t));
                *key = date;
                sum = create_new();
                g_tree_insert(sums, key, sum);
            }
            double *sum_val = extract(sum);
            *sum_val = accumulate(*sum_val, x_val);
        }

        view = nbm_data_row_iterator_next(it);
    }

    nbm_data_row_iterator_free(&it);
}

/*-------------------------------------------------------------------------------------------------
 *                                    CDF implementations.
 *-----------------------------------------------------------------------------------------------*/

struct CumulativeDistributionFunction {
    double *percents;
    double *values;
    int size;
};

static struct CumulativeDistributionFunction *
cumulative_dist_new()
{
    struct CumulativeDistributionFunction *new =
        calloc(1, sizeof(struct CumulativeDistributionFunction));
    assert(new);
    return new;
}

struct GTree *
extract_cdfs(struct NBMData const *nbm, char const *col_name_format, SummarizeDate date_sum)
{
    // TODO
    assert(false);
}

double
interpolate_prob_of_exceedance(struct CumulativeDistributionFunction *cdf, double target_val)
{
    double *xs = cdf->values;
    double *ys = cdf->percents;
    int sz = cdf->size;

    assert(sz > 1);

    // Bracket the target value
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (xs[i - 1] <= target_val && xs[i] >= target_val) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket the target value, then 100% of data was below value and prob
    // of exceedance is 0.
    if (left == right) {
        return 0.0;
    }

    // linear interpolation
    double rise = ys[right] - ys[left];
    double run = xs[right] - xs[left];
    assert(run > 0.0);

    double slope = rise / run;
    double frac = target_val - xs[left];

    return frac * slope + ys[left];
}

void
cumulative_dist_free(struct CumulativeDistributionFunction *cdf)
{
    free(cdf->percents);
    free(cdf->values);
    free(cdf);
}

/*-------------------------------------------------------------------------------------------------
 *                                 KeepFilter implementations.
 *-----------------------------------------------------------------------------------------------*/
bool
keep_all(time_t const *vt)
{
    return true;
}

bool
keep_aft(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    if (tmp.tm_hour < 18 && tmp.tm_hour >= 6) {
        return false;
    }

    return true;
}

bool
keep_mrn(time_t const *vt)
{
    return !keep_aft(vt);
}

/*-------------------------------------------------------------------------------------------------
 *                                 SummarizeDate implementations.
 *-----------------------------------------------------------------------------------------------*/
time_t
summary_date_18z(time_t const *valid_time)
{
    struct tm tmp = *gmtime(valid_time);
    if (tmp.tm_hour <= 18) {
        tmp.tm_mday--;
    }

    tmp.tm_isdst = -1;
    tmp.tm_hour = 0;
    tmp.tm_min = 0;
    tmp.tm_sec = 0;

    return timegm(&tmp);
}

time_t
summary_date_12z(time_t const *valid_time)
{
    struct tm tmp = *gmtime(valid_time);
    if (tmp.tm_hour <= 12) {
        tmp.tm_mday--;
    }

    tmp.tm_isdst = -1;
    tmp.tm_hour = 0;
    tmp.tm_min = 0;
    tmp.tm_sec = 0;

    return timegm(&tmp);
}

time_t
summary_date_06z(time_t const *valid_time)
{
    struct tm tmp = *gmtime(valid_time);
    if (tmp.tm_hour <= 6) {
        tmp.tm_mday--;
    }

    tmp.tm_isdst = -1;
    tmp.tm_hour = 0;
    tmp.tm_min = 0;
    tmp.tm_sec = 0;

    return timegm(&tmp);
}

/*-------------------------------------------------------------------------------------------------
 *                                 Accumulator implementations.
 *-----------------------------------------------------------------------------------------------*/

// The iterators should only return 1 value a day for these, so just use that single value.
double
accum_daily_rh_t(double acc, double val)
{
    assert(isnan(acc));
    return val;
}

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
