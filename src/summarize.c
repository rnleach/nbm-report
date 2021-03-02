#include "summarize.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                             Extract Values for a Daily Summary.
 *-----------------------------------------------------------------------------------------------*/
void
extract_daily_summary_for_column(GTree *sums, NBMData const *nbm, char const *col_name,
                                 KeepFilter filter, SummarizeDate date_sum, Converter convert,
                                 Accumulator accumulate, Creator create_new, Extractor extract)
{
    NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator for %s", col_name);

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
