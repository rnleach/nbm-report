#include "daily_summary.h"
#include "nbm.h"
#include "summarize.h"
#include "table.h"

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
    double precip;
    double snow;
    double prob_ltg;
    double mrn_sky;
    double aft_sky;
};

static bool
daily_summary_not_printable(struct DailySummary const *sum)
{
    return isnan(sum->max_t_f) || isnan(sum->max_t_std) || isnan(sum->min_t_f) ||
           isnan(sum->min_t_std) || isnan(sum->max_wind_mph) || isnan(sum->max_wind_std) ||
           isnan(sum->max_wind_gust) || isnan(sum->max_wind_gust_std) || isnan(sum->max_wind_dir);
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

// Most values can be considered in isolation, or one column at a time, winds are the exception. So
// winds get their own extractor function.
static void
extract_max_winds_to_summary(GTree *sums, NBMData const *nbm)
{
    NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
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
build_daily_summaries(NBMData const *nbm)
{
    GTree *sums = g_tree_new_full(time_t_compare_func, 0, free, free);

    extract_daily_summary_for_column(sums, nbm, "TMAX12hr_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     daily_summary_new, daily_summary_access_max_t);

    extract_daily_summary_for_column(sums, nbm, "TMAX12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_06z, change_in_kelvin_to_change_in_fahrenheit,
                                     accum_last, daily_summary_new, daily_summary_access_max_t_std);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     daily_summary_new, daily_summary_access_min_t);

    extract_daily_summary_for_column(sums, nbm, "TMIN12hr_2 m above ground_ens std dev", keep_all,
                                     summary_date_06z, change_in_kelvin_to_change_in_fahrenheit,
                                     accum_last, daily_summary_new, daily_summary_access_min_t_std);

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
 *                                     Table Filling
 *-----------------------------------------------------------------------------------------------*/
static void
build_title(NBMData const *nbm, Table *tbl)
{
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "Daily Summary for %s (%s) - ", nbm_data_site_name(nbm),
            nbm_data_site_id(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct DailySummary *sum = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    if (daily_summary_not_printable(sum))
        return false;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);
    table_set_avg_std(tbl, 1, row, sum->min_t_f, sum->min_t_std);
    table_set_avg_std(tbl, 2, row, sum->max_t_f, sum->max_t_std);
    table_set_value(tbl, 3, row, sum->max_wind_dir);
    table_set_avg_std(tbl, 4, row, sum->max_wind_mph, sum->max_wind_std);
    table_set_avg_std(tbl, 5, row, sum->max_wind_gust, sum->max_wind_gust_std);
    table_set_avg_std(tbl, 6, row, sum->mrn_sky, sum->aft_sky);
    table_set_value(tbl, 7, row, sum->prob_ltg);
    table_set_value(tbl, 8, row, sum->precip);
    table_set_value(tbl, 9, row, sum->snow);

    tbl_state->row++;

    return false;
}

/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_daily_summary(NBMData const *nbm)
{
    GTree *sums = build_daily_summaries(nbm);

    // --
    Table *tbl = table_new(10, g_tree_nnodes(sums));

    build_title(nbm, tbl);

    // clang-format off
    table_add_column(tbl, 0, Table_ColumnType_TEXT,      "Day/Date",  "%s",                  17);
    table_add_column(tbl, 1, Table_ColumnType_AVG_STDEV, "MinT (F)",  " %3.0lf° ±%4.1lf ",   12);
    table_add_column(tbl, 2, Table_ColumnType_AVG_STDEV, "MaxT (F)",  " %3.0lf° ±%4.1lf ",   12);
    table_add_column(tbl, 3, Table_ColumnType_VALUE,     "Dir",       " %3.0lf ",             5);
    table_add_column(tbl, 4, Table_ColumnType_AVG_STDEV, "Spd (mph)", " %3.0lf ±%2.0lf ",     9);
    table_add_column(tbl, 5, Table_ColumnType_AVG_STDEV, "Gust",      " %3.0lf ±%2.0lf ",     9);
    table_add_column(tbl, 6, Table_ColumnType_AVG_STDEV, "Sky Pct",   "%3.0lf%% /%3.0lf%% ", 12);
    table_add_column(tbl, 7, Table_ColumnType_VALUE,     "Ltg (%)",   "%3.0lf%% ",            7);
    table_add_column(tbl, 8, Table_ColumnType_VALUE,     "Precip",    "%5.2lf ",              6);
    table_add_column(tbl, 9, Table_ColumnType_VALUE,     "Snow",      "%5.1lf ",              6);
    // clang-format on

    table_set_blank_value(tbl, 7, 0.0);
    table_set_blank_value(tbl, 8, 0.0);
    table_set_blank_value(tbl, 9, 0.0);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(sums, add_row_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);

    g_tree_unref(sums);
}
