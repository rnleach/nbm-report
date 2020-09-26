#include "nbm_data.h"
#include "summarize.h"
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

struct CumulativeDistribution {
    double *percents;
    double *values;
    int size;
    int capacity;
};

static struct CumulativeDistribution *
cumulative_dist_new()
{
    struct CumulativeDistribution *new = calloc(1, sizeof(struct CumulativeDistribution));
    assert(new);

    new->percents = calloc(10, sizeof(double));
    assert(new->percents);
    new->values = calloc(10, sizeof(double));
    assert(new->values);
    new->capacity = 10;

    return new;
}

static bool
cumulative_dist_append_pair(struct CumulativeDistribution *dist, double percentile, double value)
{
    assert(dist->capacity >= dist->size);

    // Expand storage if needed.
    if (dist->capacity == dist->size) {
        assert(dist->capacity == 10);
        dist->percents = realloc(dist->percents, 100 * sizeof(double));
        dist->values = realloc(dist->values, 100 * sizeof(double));
        assert(dist->percents);
        assert(dist->values);
        dist->capacity = 100;
    }

    int last_index = dist->size - 1;
    if (last_index >= 0 && dist->percents[last_index] == percentile) {
        dist->values[last_index] = value;
    } else {
        dist->percents[dist->size] = percentile;
        dist->values[dist->size] = value;
        dist->size += 1;
    }

    return true;
}

GTree *
extract_cdfs(struct NBMData const *nbm, char const *col_name_format, SummarizeDate date_sum,
             Converter convert)
{
    GTree *cdfs = g_tree_new_full(time_t_compare_func, 0, free, cumulative_dist_free);

    char col_name[256] = {0};
    for (int i = 1; i <= 99; i++) {
        int num_chars = snprintf(col_name, sizeof(col_name), col_name_format, i);
        Stopif(num_chars >= sizeof(col_name), goto ERR_RETURN,
               "error with snprintf, buffer too small.");

        // Create an iterator, if the pointer is 0, no such column, continue.
        struct NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
        if (!it) {
            // This column doesn't exist, so skip it!
            continue;
        }

        struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
        while (view.valid_time && view.value) {
            time_t date = date_sum(view.valid_time);

            struct CumulativeDistribution *cd = g_tree_lookup(cdfs, &date);
            if (!cd) {
                time_t *key = malloc(sizeof(time_t));
                *key = date;
                cd = cumulative_dist_new();
                g_tree_insert(cdfs, key, cd);
            }

            cumulative_dist_append_pair(cd, i + 0.0, convert(*view.value));

            view = nbm_data_row_iterator_next(it);
        }
    }

    return cdfs;

ERR_RETURN:
    g_tree_unref(cdfs);
    return 0;
}

double
interpolate_prob_of_exceedance(struct CumulativeDistribution *cdf, double target_val)
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

    double left_x = xs[left];
    double right_x = xs[right];
    double left_y = ys[left];
    double right_y = ys[right];

    double rise = right_y - left_y;
    double run = right_x - left_x;
    assert(run > 0.0);
    double slope = rise / run;
    double cdf_val = slope * (target_val - left_x) + left_y;

    double prob_exc = 100.0 - cdf_val;
    assert(prob_exc >= 0.0);
    assert(prob_exc <= 100.0);
    return prob_exc;
}

void
cumulative_dist_free(void *void_cdf)
{
    struct CumulativeDistribution *cdf = void_cdf;
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
    if (tmp.tm_hour >= 18) {
        return true;
    }
    return false;
}

bool
keep_mrn(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    if (tmp.tm_hour < 18 && tmp.tm_hour >= 12) {
        return true;
    }
    return false;
}

bool
keep_eve(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    if (tmp.tm_hour < 6) {
        return true;
    }
    return false;
}

bool
keep_night(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    if (tmp.tm_hour < 12 && tmp.tm_hour >= 6) {
        return true;
    }
    return false;
}

bool
keep_00z(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    return tmp.tm_hour == 0;
}

/*-------------------------------------------------------------------------------------------------
 *                                 SummarizeDate implementations.
 *-----------------------------------------------------------------------------------------------*/
time_t
summary_date_18z(time_t const *valid_time)
{
    struct tm tmp = *gmtime(valid_time);
    if (tmp.tm_hour == 18) {
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
    if (tmp.tm_hour == 12) {
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
    if (tmp.tm_hour == 6) {
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
