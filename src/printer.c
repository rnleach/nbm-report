#include "printer.h"
#include "parser.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Daily Summary
 *-----------------------------------------------------------------------------------------------*/
struct DailySummary {
    double max_t_f;
    double max_t_std;
};

static struct DailySummary
daily_summary_new()
{
    return (struct DailySummary){.max_t_f = NAN, .max_t_std = NAN};
}

/** Convert the times from 12Z one day to 12Z the next to a struct tm valid the first day.
 *
 * For a summary, the day runs from 12Z one day to 12Z the next. This works OK for the US, and
 * matches NBM timing anyway.
 * */
static time_t
summary_date(time_t const *valid_time)
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

#define MAX_ROW_LEN 120
static int
daily_summary_print_as_row(void *key, void *val, void *user_data)
{
    time_t *vt = key;
    struct DailySummary *sum = val;

    if (*vt == 0)
        return false;

    char buf[MAX_ROW_LEN] = {0};
    char *nxt = &buf[0];
    char *const end = &buf[MAX_ROW_LEN - 1];

    strftime(buf, MAX_ROW_LEN, "%a, %Y-%m-%d", gmtime(vt));
    nxt += 15;

    int num_printed = snprintf(nxt, end - nxt, " %3.0lf°F", sum->max_t_f);
    Stopif(num_printed >= end - nxt, , "print buffer overflow daily summary");
    nxt += 7;

    num_printed = snprintf(nxt, end - nxt, " (±%4.1lf)", sum->max_t_std);
    Stopif(num_printed >= end - nxt, , "print buffer overflow daily summary");
    nxt += 8;

    printf("%s\n", buf);

    return false;
}

static int
daily_summary_compare(void const *a, void const *b, void *user_data)
{
    time_t const *dsa = a;
    time_t const *dsb = b;

    if (*dsa < *dsb)
        return -1;
    if (*dsa > *dsb)
        return 1;
    return 0;
}

static GTree *
build_daily_summaries(struct NBMData const *nbm)
{
    GTree *sums = g_tree_new_full(daily_summary_compare, 0, free, free);

    struct NBMDataRowIterator *it = nbm_data_rows(nbm, "TMAX12hr_2 m above ground");
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator.");

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    while (view.valid_time && view.value) {

        time_t date = summary_date(view.valid_time);
        double t_val = kelvin_to_fahrenheit(*view.value);

        struct DailySummary *sum = g_tree_lookup(sums, &date);
        if (!sum) {
            time_t *key = malloc(sizeof(time_t));
            *key = date;
            sum = malloc(sizeof(struct DailySummary));
            *sum = daily_summary_new();
            g_tree_insert(sums, key, sum);
        }
        sum->max_t_f = t_val;

        view = nbm_data_row_iterator_next(it);
    }

    nbm_data_row_iterator_free(&it);

    return sums;
}
/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_daily_summary(struct NBMData const *nbm)
{
    GTree *sums = build_daily_summaries(nbm);

    g_tree_foreach(sums, daily_summary_print_as_row, 0);

    g_tree_unref(sums);
}
