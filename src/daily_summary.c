#include "daily_summary.h"
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
           isnan(sum->max_rh) || isnan(sum->min_rh);
}

static void *
daily_summary_new()
{
    struct DailySummary *new = malloc(sizeof(struct DailySummary));
    assert(new);

    *new = (struct DailySummary){
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

    return (void *)new;
}

static double *
daily_summary_access_aft_sky(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->aft_sky;
}

static double *
daily_summary_access_mrn_sky(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->mrn_sky;
}

static double *
daily_summary_access_prob_ltg(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->prob_ltg;
}

static double *
daily_summary_access_snow(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->snow;
}

static double *
daily_summary_access_precip(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->precip;
}

static double *
daily_summary_access_min_rh(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->min_rh;
}

static double *
daily_summary_access_max_rh(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->max_rh;
}

static double *
daily_summary_access_min_t_std(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->min_t_std;
}

static double *
daily_summary_access_min_t(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->min_t_f;
}

static double *
daily_summary_access_max_t_std(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->max_t_std;
}

static double *
daily_summary_access_max_t(void *sm)
{
    struct DailySummary *sum = sm;
    return &sum->max_t_f;
}

#define MAX_ROW_LEN 256
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

    int np = snprintf(nxt, end - nxt, "│ %3.0lf° ±%4.1lf ", round(sum->min_t_f),
                      round(sum->min_t_std * 10.0) / 10.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary min_t");
    nxt += 16;

    np = snprintf(nxt, end - nxt, "│ %3.0lf° ±%4.1lf ", round(sum->max_t_f),
                  round(sum->max_t_std * 10.0) / 10.0);
    Stopif(np >= end - nxt, *end = 0, "print buffer overflow daily summary max_t");
    nxt += 16;

    np = snprintf(nxt, end - nxt, "│ %3.0lf%% /%3.0lf%% ", round(sum->max_rh), round(sum->min_rh));
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
    char *c = 0;
    while ((c = strstr(buf, "nan"))) {
        c[0] = ' ';
        c[1] = '-';
        c[2] = ' ';
    }

    printf("%s\n", buf);

    return false;
}

static void
daily_summary_print_header(char const *site, time_t init_time)
{

    // Build a title.
    char title_buf[100] = {0};
    struct tm init = *gmtime(&init_time);
    strcpy(title_buf, site);
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    // Calculate white space to center the title.
    int line_len = 115 - 2;
    char header_buf[115 + 1] = {0};
    len = strlen(title_buf);
    int left = (line_len - len) / 2;

    // Print the white spaces and title.
    for (int i = 0; i < left; i++) {
        header_buf[i] = ' ';
    }
    strcpy(&header_buf[left], title_buf);
    for (int i = left + len; i < line_len; i++) {
        header_buf[i] = ' ';
    }

    // clang-format off
    char const *top_border = 
        "┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐";

    char const *header = 
        "├─────────────────┬───────────┬───────────┬────────────┬─────┬─────────┬─────────┬────────────┬─────┬──────┬──────┤\n"
        "│       Date      │   MinT    │   MaxT    │ Max/Min RH │ Dir │   Speed │    Gust │   Mrn/Aft  │ Prb │ Rain │ Snow │\n"
        "│                 │    °F     │    °F     │   percent  │ deg │    mph  │     mph │   sky pct  │ Ltg │  in  │  in  │\n"
        "╞═════════════════╪═══════════╪═══════════╪════════════╪═════╪═════════╪═════════╪════════════╪═════╪══════╪══════╡";
    // clang-format on

    puts(top_border);
    printf("│%s│\n", header_buf);
    puts(header);
}

static void
daily_summary_print_footer()
{
    // clang-format off
    puts("╘═════════════════╧═══════════╧═══════════╧════════════╧═════╧═════════╧═════════╧════════════╧═════╧══════╧══════╛");
    // clang-format on
}

// Most values can be considered in isolation, or one column at a time, winds are the exception. So
// winds get their own extractor function.
static void
extract_max_winds_to_summary(GTree *sums, struct NBMData const *nbm)
{
    struct NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
    Stopif(!it, exit(EXIT_FAILURE), "error creating wind iterator.");

    struct NBMDataRowIteratorWindValueView view = nbm_data_row_wind_iterator_next(it);
    while (view.valid_time) { // If valid time is good, assume everything is.

        time_t date = summary_date_06z(view.valid_time);

        struct DailySummary *sum = g_tree_lookup(sums, &date);
        if (!sum) {
            time_t *key = malloc(sizeof(time_t));
            *key = date;
            sum = (struct DailySummary *)daily_summary_new();
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
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     daily_summary_new, daily_summary_access_max_t);

    extract_daily_summary_for_column(sums, nbm, "TMAX12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_06z, change_in_kelvin_to_change_in_fahrenheit,
                                     accum_daily_rh_t, daily_summary_new,
                                     daily_summary_access_max_t_std);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     daily_summary_new, daily_summary_access_min_t);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_06z, change_in_kelvin_to_change_in_fahrenheit,
                                     accum_daily_rh_t, daily_summary_new,
                                     daily_summary_access_min_t_std);

    extract_daily_summary_for_column(sums, nbm, "MINRH12hr_2 m above ground", keep_all,
                                     summary_date_06z, id_func, accum_daily_rh_t, daily_summary_new,
                                     daily_summary_access_min_rh);

    extract_daily_summary_for_column(sums, nbm, "MAXRH12hr_2 m above ground", keep_all,
                                     summary_date_06z, id_func, accum_daily_rh_t, daily_summary_new,
                                     daily_summary_access_max_rh);

    extract_daily_summary_for_column(sums, nbm, "APCP24hr_surface", keep_all, summary_date_06z,
                                     mm_to_in, accum_last, daily_summary_new,
                                     daily_summary_access_precip);

    extract_daily_summary_for_column(sums, nbm, "ASNOW6hr_surface", keep_all, summary_date_06z,
                                     m_to_in, accum_sum, daily_summary_new,
                                     daily_summary_access_snow);

    extract_daily_summary_for_column(sums, nbm, "TSTM12hr_surface_probability forecast", keep_all,
                                     summary_date_06z, id_func, accum_max, daily_summary_new,
                                     daily_summary_access_prob_ltg);

    extract_daily_summary_for_column(sums, nbm, "TCDC_surface", keep_mrn, summary_date_06z, id_func,
                                     accum_avg, daily_summary_new, daily_summary_access_mrn_sky);

    extract_daily_summary_for_column(sums, nbm, "TCDC_surface", keep_aft, summary_date_06z, id_func,
                                     accum_avg, daily_summary_new, daily_summary_access_aft_sky);

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

    daily_summary_print_header(nbm_data_site(nbm), nbm_data_init_time(nbm));
    g_tree_foreach(sums, daily_summary_print_as_row, 0);
    daily_summary_print_footer();

    g_tree_unref(sums);
}
