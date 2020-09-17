#include "daily_summary.h"
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
    double min_rh;
    double max_rh;
    double precip;
    double snow;
    double prob_ltg;
    double mrn_sky;
    double aft_sky;
};

static bool
daily_summary_printable(struct DailySummary const *sum)
{
    return isnan(sum->max_t_f) || isnan(sum->max_t_std) || isnan(sum->min_t_f) ||
           isnan(sum->min_t_std) || isnan(sum->max_wind_mph) || isnan(sum->max_wind_std) ||
           isnan(sum->max_wind_gust) || isnan(sum->max_wind_gust_std) || isnan(sum->max_wind_dir) ||
           isnan(sum->max_rh) || isnan(sum->min_rh) || isnan(sum->precip) || isnan(sum->snow) ||
           isnan(sum->mrn_sky) || isnan(sum->aft_sky);
}

static struct DailySummary
daily_summary_new()
{
    return (struct DailySummary){
        .max_t_f = NAN,
        .max_t_std = NAN,
        .min_t_f = NAN,
        .min_t_std = NAN,
        .max_wind_mph = NAN,
        .max_wind_std = NAN,
        .max_wind_gust = NAN,
        .max_wind_gust_std = NAN,
        .max_wind_dir = NAN,
        .max_rh = NAN,
        .min_rh = NAN,
        .precip = NAN,
        .snow = NAN,
        .prob_ltg = NAN,
        .mrn_sky = NAN,
        .aft_sky = NAN,
    };
}

static double *
daily_summary_access_aft_sky(struct DailySummary *sum)
{
    return &sum->aft_sky;
}

static double *
daily_summary_access_mrn_sky(struct DailySummary *sum)
{
    return &sum->mrn_sky;
}

static double *
daily_summary_access_prob_ltg(struct DailySummary *sum)
{
    return &sum->prob_ltg;
}

static double *
daily_summary_access_snow(struct DailySummary *sum)
{
    return &sum->snow;
}

static double *
daily_summary_access_precip(struct DailySummary *sum)
{
    return &sum->precip;
}

static double *
daily_summary_access_min_rh(struct DailySummary *sum)
{
    return &sum->min_rh;
}

static double *
daily_summary_access_max_rh(struct DailySummary *sum)
{
    return &sum->max_rh;
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

#define MAX_ROW_LEN 150
static int
daily_summary_print_as_row(void *key, void *val, void *user_data)
{
    time_t *vt = key;
    struct DailySummary *sum = val;

    if (*vt == 0)
        return false;

    if (daily_summary_printable(val))
        return false;

    char buf[MAX_ROW_LEN] = {0};
    char *nxt = &buf[0];
    char *const end = &buf[MAX_ROW_LEN - 1];

    strftime(buf, MAX_ROW_LEN, "│ %a, %Y-%m-%d ", gmtime(vt));
    nxt += 20;

    int np = snprintf(nxt, end - nxt, "│ %3.0lf° ±%4.1lf ", round(sum->max_t_f),
                      round(sum->max_t_std * 10.0) / 10.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary max_t");
    nxt += 16;

    np = snprintf(nxt, end - nxt, "│ %3.0lf° ±%4.1lf ", round(sum->min_t_f),
                  round(sum->min_t_std * 10.0) / 10.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary min_t");
    nxt += 16;

    np = snprintf(nxt, end - nxt, "│ %3.0lf%% /%3.0lf%% ", round(sum->min_rh), round(sum->max_rh));
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary RH");
    nxt += 15;

    np = snprintf(nxt, end - nxt, "│ %03.0lf ", round(sum->max_wind_dir));
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary wdir: %lf",
           sum->max_wind_dir);
    nxt += 8;

    np = snprintf(nxt, end - nxt, "│ %3.0lf ±%2.0lf ", round(sum->max_wind_mph),
                  round(sum->max_wind_std));
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary wind spd");
    nxt += 13;

    np = snprintf(nxt, end - nxt, "│ %3.0lf ±%2.0lf ", round(sum->max_wind_gust),
                  round(sum->max_wind_gust_std));
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary gust: g %lf",
           sum->max_wind_gust);
    nxt += 13;

    np =
        snprintf(nxt, end - nxt, "│ %3.0lf%% /%3.0lf%% ", round(sum->mrn_sky), round(sum->aft_sky));
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary sky");
    nxt += 15;

    np = snprintf(nxt, end - nxt, "│ %3.0lf ", sum->prob_ltg);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary ltg");
    nxt += 8;

    np = snprintf(nxt, end - nxt, "│ %4.2lf ", round(sum->precip * 100.0) / 100.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary precip");
    nxt += 9;

    np = snprintf(nxt, end - nxt, "│ %4.1lf │", round(sum->snow * 10.0) / 10.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary snow");
    nxt += 11;

    // Remove NANs
    char *c = buf;
    while (*c) {
        if ((*c == 'n' || *c == 'a') && c - buf > 10) {
            *c = ' ';
        }
        ++c;
    }

    printf("%s\n", buf);

    return false;
}

static void
daily_summary_print_header()
{
    // clang-format off
    char const *header = 
        "┌─────────────────┬───────────┬───────────┬────────────┬─────┬─────────┬─────────┬────────────┬─────┬──────┬──────┐\n"
        "│       Date      │   MaxT    │   MinT    │ Min/Max RH │ Dir │   Speed │    Gust │   Mrn/Aft  │ Prb │ Rain │ Snow │\n"
        "│                 │    °F     │    °F     │   percent  │ deg │    mph  │     mph │   sky pct  │ Ltg │  in  │  in  │\n"
        "╞═════════════════╪═══════════╪═══════════╪════════════╪═════╪═════════╪═════════╪════════════╪═════╪══════╪══════╡";
    // clang-format on

    puts(header);
}

static void
daily_summary_print_footer()
{
    // clang-format off
    puts("╘═════════════════╧═══════════╧═══════════╧════════════╧═════╧═════════╧═════════╧════════════╧═════╧══════╧══════╛");
    // clang-format on
}

/*-------------------------------------------------------------------------------------------------
 *                      Building a Binary Tree of DailySummary Objects
 * ----------------------------------------------------------------------------------------------*/

/** Compare \c time_t values which are used as keys in the GLib \c Tree.
 *
 * Dates are sorted in ascending order.
 */
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

/*-------------------------------------------------------------------------------------------------
 *        Extract data from the NBMData object into a sorted list of DailySummary objects.
 * ----------------------------------------------------------------------------------------------*/

/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

/** Extract a pointer to the summary value that needs to be updated. */
typedef double *(*Extractor)(struct DailySummary *);

/** Convert all values in a day to a single \c time_t value so they can be associated in a tree.
 *
 * The period defined as a day may vary between weather elements.
 */
typedef time_t (*SummarizeDate)(time_t const *);

/** How to accumulate values from a day, eg take the first, last, sum, max, min. */
typedef double (*Accumulator)(double acc, double val);

/** Filter values out for consideration. */
typedef bool (*KeepFilter)(time_t const *);

static void
extract_daily_summary_for_column(GTree *sums, struct NBMData const *nbm, char const *col_name,
                                 KeepFilter filter, SummarizeDate date_sum, Converter convert,
                                 Extractor extract, Accumulator accumulate)
{
    struct NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator.");

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    while (view.valid_time && view.value) {

        if (filter(view.valid_time)) {
            time_t date = date_sum(view.valid_time);
            double x_val = convert(*view.value);

            struct DailySummary *sum = g_tree_lookup(sums, &date);
            if (!sum) {
                time_t *key = malloc(sizeof(time_t));
                *key = date;
                sum = malloc(sizeof(struct DailySummary));
                *sum = daily_summary_new();
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
static bool
keep_all(time_t const *vt)
{
    return true;
}

static bool
keep_aft(time_t const *vt)
{
    struct tm tmp = *gmtime(vt);
    if (tmp.tm_hour < 18 && tmp.tm_hour >= 6) {
        return false;
    }

    return true;
}

static bool
keep_mrn(time_t const *vt)
{
    return !keep_aft(vt);
}

/*-------------------------------------------------------------------------------------------------
 *                                 SummarizeDate implementations.
 *-----------------------------------------------------------------------------------------------*/
/** Convert the times from 18Z one day to 18Z the next to a struct tm valid the first day.
 *
 * For a summary, the day runs from 18Z one day to 18Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for MaxT/MinT/MaxRH/MinRH.
 * */
static time_t
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

/** Convert the times from 12Z one day to 12Z the next to a struct tm valid the first day.
 *
 * For a summary, the day runs from 12Z one day to 12Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for wind and precip.
 */
static time_t
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

/** Convert the times from 06Z one day to 06Z the next to a struct tm valid the first day.
 *
 * For a summary, the day runs from 06Z one day to 06Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for sky cover and any hourly or six hourly NBM component.
 */
static time_t
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
static double
accum_daily_rh_t(double acc, double val)
{
    assert(isnan(acc));
    return val;
}

static double
accum_sum(double acc, double val)
{
    if (isnan(acc)) {
        return val;
    }

    return acc + val;
}

static double
accum_max(double acc, double val)
{
    if (isnan(acc)) {
        return val;
    }

    return acc > val ? acc : val;
}

static double
accum_last(double _acc, double val)
{
    return val;
}

static double
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

/*-------------------------------------------------------------------------------------------------
 *                                 Extractor and Accumulator implementations.
 *-----------------------------------------------------------------------------------------------*/
// The Extractor implementations are up above with the DailySummary struct definition.
// The Converter implementations are in the utils.h file.
// TODO: Move converters here.

// Most values can be considered in isolation, or one column at a time, winds are the exception. So
// winds get their own extractor function.
static void
extract_max_winds_to_summary(GTree *sums, struct NBMData const *nbm)
{
    struct NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
    Stopif(!it, exit(EXIT_FAILURE), "error creating wind iterator.");

    struct NBMDataRowIteratorWindValueView view = nbm_data_row_wind_iterator_next(it);
    while (view.valid_time) { // If valid time is good, assume everything is.

        time_t date = summary_date_12z(view.valid_time);

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

/** Build a sorted list (\c Tree) of daily summaries from an \c NBMData object. */
static GTree *
build_daily_summaries(struct NBMData const *nbm)
{
    GTree *sums = g_tree_new_full(time_t_compare_func, 0, free, free);

    extract_daily_summary_for_column(sums, nbm, "TMAX12hr_2 m above ground", keep_all,
                                     summary_date_18z, kelvin_to_fahrenheit,
                                     daily_summary_access_max_t, accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "TMAX12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_18z, change_in_kelvin_to_change_in_fahrenheit,
                                     daily_summary_access_max_t_std, accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground", keep_all,
                                     summary_date_18z, kelvin_to_fahrenheit,
                                     daily_summary_access_min_t, accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_18z, change_in_kelvin_to_change_in_fahrenheit,
                                     daily_summary_access_min_t_std, accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "MINRH12hr_2 m above ground", keep_all,
                                     summary_date_18z, id_func, daily_summary_access_min_rh,
                                     accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "MAXRH12hr_2 m above ground", keep_all,
                                     summary_date_18z, id_func, daily_summary_access_max_rh,
                                     accum_daily_rh_t);

    extract_daily_summary_for_column(sums, nbm, "APCP24hr_surface", keep_all, summary_date_12z,
                                     mm_to_in, daily_summary_access_precip, accum_last);

    extract_daily_summary_for_column(sums, nbm, "ASNOW6hr_surface", keep_all, summary_date_12z,
                                     m_to_in, daily_summary_access_snow, accum_sum);

    extract_daily_summary_for_column(sums, nbm, "TSTM12hr_surface_probability forecast", keep_all,
                                     summary_date_12z, id_func, daily_summary_access_prob_ltg,
                                     accum_max);

    extract_daily_summary_for_column(sums, nbm, "TCDC_surface", keep_mrn, summary_date_06z, id_func,
                                     daily_summary_access_mrn_sky, accum_avg);

    extract_daily_summary_for_column(sums, nbm, "TCDC_surface", keep_aft, summary_date_06z, id_func,
                                     daily_summary_access_aft_sky, accum_avg);

    extract_max_winds_to_summary(sums, nbm);

    return sums;
}

/*-------------------------------------------------------------------------------------------------
 *                                    Quality checks/alerts.
 *-----------------------------------------------------------------------------------------------*/
static void
alert_age(struct NBMData const *nbm)
{
    double age_secs = nbm_data_age(nbm);
    int age_hrs = (int)round(age_secs / 3600.0);

    if (age_hrs >= 12) {
        int age_days = age_hrs / 24;
        age_hrs -= age_days * 24;

        printf("     *\n");
        printf("     * OLD NBM DATA - data is: ");
        if (age_days > 1) {
            printf("%d days and", age_days);
        } else if (age_days > 0) {
            printf("%d day and", age_days);
        }
        if (age_hrs > 1) {
            printf(" %d hours old", age_hrs);
        } else if (age_hrs > 0) {
            printf(" %d hour old", age_hrs);
        }
        printf("\n");
        printf("     *\n");
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_daily_summary(struct NBMData const *nbm)
{
    GTree *sums = build_daily_summaries(nbm);

    alert_age(nbm);

    daily_summary_print_header();
    g_tree_foreach(sums, daily_summary_print_as_row, 0);
    daily_summary_print_footer();

    g_tree_unref(sums);
}
