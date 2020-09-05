#include "parser.h"
#include "printer.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/** Convert the times from 12Z one day to 12Z the next to a struct tm valid the first day.
 *
 * For a summary, the day runs from 12Z one day to 12Z the next. This works OK for the US, and
 * matches NBM timing anyway.
 * */
static time_t
summary_date(time_t const *valid_time, bool is_rh)
{
    struct tm tmp = *gmtime(valid_time);
    if (is_rh) {
        if (tmp.tm_hour <= 18) {
            tmp.tm_mday--;
        }
    } else {
        if (tmp.tm_hour <= 12) {
            tmp.tm_mday--;
        }
    }

    tmp.tm_isdst = -1;
    tmp.tm_hour = 0;
    tmp.tm_min = 0;
    tmp.tm_sec = 0;

    return timegm(&tmp);
}

/*-------------------------------------------------------------------------------------------------
 *                                        Daily Summary
 *-----------------------------------------------------------------------------------------------*/
struct DailySummary {
    double max_t_f;
    double max_t_std;
    double min_t_f;
    double min_t_std;
    double max_wind_mph;
    double max_wind_std;
    double max_wind_gust;
    double max_wind_gust_std;
    double max_wind_dir;
};

static bool
daily_summary_any_nan(struct DailySummary const *sum)
{
    return isnan(sum->max_t_f) || isnan(sum->max_t_std) || isnan(sum->min_t_f) ||
           isnan(sum->min_t_std) || isnan(sum->max_wind_mph) || isnan(sum->max_wind_std) ||
           isnan(sum->max_wind_gust) || isnan(sum->max_wind_gust_std) || isnan(sum->max_wind_dir);
}

static struct DailySummary
daily_summary_new()
{
    return (struct DailySummary){.max_t_f = NAN,
                                 .max_t_std = NAN,
                                 .min_t_f = NAN,
                                 .min_t_std = NAN,
                                 .max_wind_mph = NAN,
                                 .max_wind_std = NAN,
                                 .max_wind_gust = NAN,
                                 .max_wind_gust_std = NAN,
                                 .max_wind_dir = NAN};
}

static double *
daily_summary_access_min_t_std(struct DailySummary *sum)
{
    return &sum->min_t_std;
}

static double *
daily_summary_access_min_t(struct DailySummary *sum)
{
    return &sum->min_t_f;
}

static double *
daily_summary_access_max_t_std(struct DailySummary *sum)
{
    return &sum->max_t_std;
}

static double *
daily_summary_access_max_t(struct DailySummary *sum)
{
    return &sum->max_t_f;
}

#define MAX_ROW_LEN 120
static int
daily_summary_print_as_row(void *key, void *val, void *user_data)
{
    time_t *vt = key;
    struct DailySummary *sum = val;

    if (*vt == 0)
        return false;

    if (daily_summary_any_nan(val)) return false;

    char buf[MAX_ROW_LEN] = {0};
    char *nxt = &buf[0];
    char *const end = &buf[MAX_ROW_LEN - 1];

    strftime(buf, MAX_ROW_LEN, "%a, %Y-%m-%d", gmtime(vt));
    nxt += 15;

    int num_printed = snprintf(nxt, end - nxt, " %3.0lf°", round(sum->max_t_f));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary max_t");
    nxt += 6;

    num_printed = snprintf(nxt, end - nxt, " (±%4.1lf)", round(sum->max_t_std));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary max_t");
    nxt += 9;

    num_printed = snprintf(nxt, end - nxt, " %3.0lf°", round(sum->min_t_f));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary min_t");
    nxt += 6;

    num_printed = snprintf(nxt, end - nxt, " (±%4.1lf)", round(sum->min_t_std));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary min_t");
    nxt += 9;

    num_printed = snprintf(nxt, end - nxt, " %3.0lf", round(sum->max_wind_dir));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary wdir: %lf",
           sum->max_wind_dir);
    nxt += 4;

    num_printed = snprintf(nxt, end - nxt, " %3.0lf", round(sum->max_wind_mph));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary wind spd");
    nxt += 4;

    num_printed = snprintf(nxt, end - nxt, " (±%2.0lf)", round(sum->max_wind_std));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary wind spd");
    nxt += 7;

    num_printed = snprintf(nxt, end - nxt, " %3.0lf", round(sum->max_wind_gust));
    Stopif(num_printed >= end - nxt, *end = 0, "print buffer overflow daily summary gust: g %lf",
           sum->max_wind_gust);
    nxt += 4;

    num_printed = snprintf(nxt, end - nxt, " (±%2.0lf)", round(sum->max_wind_gust_std));
    Stopif(num_printed >= end - nxt, *end = 0,
           "print buffer overflow daily summary gust: g_std %lf", sum->max_wind_gust_std);
    nxt += 7;

    printf("%s\n", buf);

    return false;
}

static int
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

static void
extract_daily_value_to_summary(GTree *sums, struct NBMData const *nbm, char const *col_name,
                               double (*map_fun)(double),
                               double *(*extractor_fun)(struct DailySummary *))
{
    struct NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator.");

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    while (view.valid_time && view.value) {

        bool is_rh = !!strstr(col_name, "RH");
        time_t date = summary_date(view.valid_time, is_rh);
        double x_val = map_fun(*view.value);

        struct DailySummary *sum = g_tree_lookup(sums, &date);
        if (!sum) {
            time_t *key = malloc(sizeof(time_t));
            *key = date;
            sum = malloc(sizeof(struct DailySummary));
            *sum = daily_summary_new();
            g_tree_insert(sums, key, sum);
        }
        double *sum_val = extractor_fun(sum);
        *sum_val = x_val;

        view = nbm_data_row_iterator_next(it);
    }

    nbm_data_row_iterator_free(&it);
}

static void
extract_max_winds_to_summary(GTree *sums, struct NBMData const *nbm)
{
    struct NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
    Stopif(!it, exit(EXIT_FAILURE), "error creating wind iterator.");

    struct NBMDataRowIteratorWindValueView view = nbm_data_row_wind_iterator_next(it);
    while (view.valid_time) { // If valid time is good, assume everything is.

        time_t date = summary_date(view.valid_time, false);

        struct DailySummary *sum = g_tree_lookup(sums, &date);
        if (!sum) {
            time_t *key = malloc(sizeof(time_t));
            *key = date;
            sum = malloc(sizeof(struct DailySummary));
            *sum = daily_summary_new();
            g_tree_insert(sums, key, sum);
        }
        double max_wind_mph = mps_to_mph(*view.wspd);
        double max_wind_std = mps_to_mph(*view.wspd_std);
        double max_wind_gust = mps_to_mph(*view.gust);
        double max_wind_gust_std = mps_to_mph(*view.gust_std);
        double max_wind_dir = *view.wdir;

        if (isnan(sum->max_wind_mph) || max_wind_mph > sum->max_wind_mph) {
            sum->max_wind_mph = max_wind_mph;
            sum->max_wind_std = max_wind_std;
            sum->max_wind_dir = max_wind_dir;
        }

        if (isnan(sum->max_wind_gust) || max_wind_gust > sum->max_wind_gust) {
            sum->max_wind_gust = max_wind_gust;
            sum->max_wind_gust_std = max_wind_gust_std;
        }

        view = nbm_data_row_wind_iterator_next(it);
    }

    nbm_data_row_wind_iterator_free(&it);
}

static GTree *
build_daily_summaries(struct NBMData const *nbm)
{
    GTree *sums = g_tree_new_full(time_t_compare_func, 0, free, free);

    extract_daily_value_to_summary(sums, nbm, "TMAX12hr_2 m above ground", kelvin_to_fahrenheit,
                                   daily_summary_access_max_t);

    extract_daily_value_to_summary(sums, nbm, "TMAX12hr_2 m above ground_ens std dev",
                                   change_in_kelvin_to_change_in_fahrenheit,
                                   daily_summary_access_max_t_std);

    extract_daily_value_to_summary(sums, nbm, "TMIN12hr_2 m above ground", kelvin_to_fahrenheit,
                                   daily_summary_access_min_t);

    extract_daily_value_to_summary(sums, nbm, "TMIN12hr_2 m above ground_ens std dev",
                                   change_in_kelvin_to_change_in_fahrenheit,
                                   daily_summary_access_min_t_std);

    extract_max_winds_to_summary(sums, nbm);

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
