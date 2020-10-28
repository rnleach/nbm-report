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
struct Percentile {
    double pct;
    double val;
};

static int
percentile_compare(void const *a, void const *b)
{
    struct Percentile const *pa = a;
    struct Percentile const *pb = b;

    if (pa->pct < pb->pct)
        return -1;
    if (pa->pct > pb->pct)
        return 1;
    return 0;
}

struct CumulativeDistribution {
    double quantile_mapped_value;
    struct Percentile *percentiles;
    int size;
    int capacity;
    bool sorted;
};

static struct CumulativeDistribution *
cumulative_dist_new()
{
    struct CumulativeDistribution *new = calloc(1, sizeof(struct CumulativeDistribution));
    assert(new);

    new->percentiles = calloc(10, sizeof(struct Percentile));
    assert(new->percentiles);
    new->capacity = 10;
    new->quantile_mapped_value = NAN;
    new->sorted = false;

    return new;
}

static bool
cumulative_dist_append_pair(struct CumulativeDistribution *dist, double percentile, double value)
{
    assert(dist->capacity >= dist->size);

    // Expand storage if needed.
    if (dist->capacity == dist->size) {
        assert(dist->capacity == 10);
        dist->percentiles = realloc(dist->percentiles, 100 * sizeof(struct Percentile));
        assert(dist->percentiles);
        dist->capacity = 100;
    }

    dist->sorted = false;
    dist->percentiles[dist->size] = (struct Percentile){.pct = percentile, .val = value};
    dist->size++;

    return true;
}

GTree *
extract_cdfs(struct NBMData const *nbm, char const *cdf_col_name_format, char const *pm_col_name,
             Converter convert)
{
    GTree *cdfs = g_tree_new_full(time_t_compare_func, 0, free, cumulative_dist_free);

    char col_name[256] = {0};
    for (int i = 1; i <= 99; i++) {
        int num_chars = snprintf(col_name, sizeof(col_name), cdf_col_name_format, i);
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

            struct CumulativeDistribution *cd = g_tree_lookup(cdfs, view.valid_time);
            if (!cd) {
                time_t *key = malloc(sizeof(time_t));
                *key = *view.valid_time;
                cd = cumulative_dist_new();
                g_tree_insert(cdfs, key, cd);
            }

            cumulative_dist_append_pair(cd, i + 0.0, convert(*view.value));

            view = nbm_data_row_iterator_next(it);
        }

        nbm_data_row_iterator_free(&it);
    }

    // Get the probability matched info.
    struct NBMDataRowIterator *it = nbm_data_rows(nbm, pm_col_name);
    if (it) {
        struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
        while (view.valid_time && view.value) {

            struct CumulativeDistribution *cd = g_tree_lookup(cdfs, view.valid_time);
            // Skip times we don't have probability information for.
            if (cd) {
                cd->quantile_mapped_value = convert(*view.value);
            }

            view = nbm_data_row_iterator_next(it);
        }

        nbm_data_row_iterator_free(&it);
    }

    return cdfs;

ERR_RETURN:
    g_tree_unref(cdfs);
    return 0;
}

double
cumulative_dist_pm_value(struct CumulativeDistribution const *ptr)
{
    return ptr->quantile_mapped_value;
}

static void
cumulative_dist_sort(struct CumulativeDistribution *ptr)
{
    qsort(ptr->percentiles, ptr->size, sizeof(struct Percentile), percentile_compare);
    ptr->sorted = true;
}

double
interpolate_prob_of_exceedance(struct CumulativeDistribution *cdf, double target_val)
{
    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }

    struct Percentile *ps = cdf->percentiles;
    int sz = cdf->size;

    assert(sz > 1);

    // Bracket the target value
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].val <= target_val && ps[i].val >= target_val) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket the target value, then 100% of data was below value and the probability
    // of exceedance is 0, or the first percentile marker we had was greater than the target value
    // and and the probability of exceedance is 100.0
    if (left == right) {
        if (ps[0].val >= target_val) {
            return 100.0;
        } else {
            return 0.0;
        }
    }

    double left_x = ps[left].val;
    double right_x = ps[right].val;
    double left_y = ps[left].pct;
    double right_y = ps[right].pct;

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

double
cumulative_dist_percentile_value(struct CumulativeDistribution *cdf, double target_percentile)
{

    if (!cdf->sorted) {
        cumulative_dist_sort(cdf);
    }
    struct Percentile *ps = cdf->percentiles;
    int sz = cdf->size;

    assert(sz > 1);

    // Bracket the target percentile
    int left = 0, right = 0;
    for (int i = 1; i < sz; i++) {
        if (ps[i - 1].pct <= target_percentile && ps[i].pct >= target_percentile) {
            left = i - 1;
            right = i;
            break;
        }
    }

    // If unable to bracket, then it wasn't available.
    if (left == 0 && right == 0)
        return NAN;

    double left_pct = ps[left].pct;
    double right_pct = ps[right].pct;
    double left_val = ps[left].val;
    double right_val = ps[right].val;

    double rise = right_val - left_val;
    double run = right_pct - left_pct;
    assert(run > 0.0);
    double slope = rise / run;

    return slope * (target_percentile - left_pct) + left_val;
}

void
cumulative_dist_free(void *void_cdf)
{
    struct CumulativeDistribution *cdf = void_cdf;
    free(cdf->percentiles);
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
