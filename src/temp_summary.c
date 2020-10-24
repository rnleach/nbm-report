#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "summarize.h"
#include "table.h"
#include "temp_summary.h"
#include "utils.h"

/*-------------------------------------------------------------------------------------------------
 *                                     Temperature Summary
 *-----------------------------------------------------------------------------------------------*/
struct TemperatureSum {
    double mint;
    double mint_10th;
    double mint_25th;
    double mint_50th;
    double mint_75th;
    double mint_90th;

    double maxt;
    double maxt_10th;
    double maxt_25th;
    double maxt_50th;
    double maxt_75th;
    double maxt_90th;
};

static bool
temperature_sum_not_printable(struct TemperatureSum const *sum)
{
    return isnan(sum->maxt) || isnan(sum->maxt_10th);
}

static void *
temp_sum_new()
{
    struct TemperatureSum *new = malloc(sizeof(struct TemperatureSum));
    assert(new);

    *new = (struct TemperatureSum){
        .mint = NAN,
        .mint_10th = NAN,
        .mint_25th = NAN,
        .mint_50th = NAN,
        .mint_75th = NAN,
        .mint_90th = NAN,
        .maxt = NAN,
        .maxt_10th = NAN,
        .maxt_25th = NAN,
        .maxt_50th = NAN,
        .maxt_75th = NAN,
        .maxt_90th = NAN,
    };

    return (void *)new;
}

static double *
temp_sum_access_mint(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint;
}

static double *
temp_sum_access_mint_10th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint_10th;
}

static double *
temp_sum_access_mint_25th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint_25th;
}

static double *
temp_sum_access_mint_50th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint_50th;
}

static double *
temp_sum_access_mint_75th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint_75th;
}

static double *
temp_sum_access_mint_90th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->mint_90th;
}

static double *
temp_sum_access_maxt(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt;
}

static double *
temp_sum_access_maxt_10th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt_10th;
}

static double *
temp_sum_access_maxt_25th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt_25th;
}

static double *
temp_sum_access_maxt_50th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt_50th;
}

static double *
temp_sum_access_maxt_75th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt_75th;
}

static double *
temp_sum_access_maxt_90th(void *sm)
{
    struct TemperatureSum *sum = sm;
    return &sum->maxt_90th;
}

static GTree *
extract_temperature_data(struct NBMData const *nbm)
{
    GTree *sums = g_tree_new_full(time_t_compare_func, 0, free, free);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_10% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint_10th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_25% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint_25th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_50% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint_50th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_75% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint_75th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_90% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_mint_90th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_10% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt_10th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_25% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt_25th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_50% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt_50th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_75% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt_75th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_90% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_daily_rh_t,
                                     temp_sum_new, temp_sum_access_maxt_90th);

    return sums;
}

/*-------------------------------------------------------------------------------------------------
 *                                     Table Filling
 *-----------------------------------------------------------------------------------------------*/
struct TableFillerState {
    int row;
    struct Table *tbl;
};

static void
build_title(struct NBMData const *nbm, struct Table *tbl)
{
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "Temperature Quantiles for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct TemperatureSum *sum = value;
    struct TableFillerState *tbl_state = state;
    struct Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    if (temperature_sum_not_printable(sum))
        return false;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 1, row, sum->mint);
    table_set_value(tbl, 2, row, sum->mint_10th);
    table_set_value(tbl, 3, row, sum->mint_25th);
    table_set_value(tbl, 4, row, sum->mint_50th);
    table_set_value(tbl, 5, row, sum->mint_75th);
    table_set_value(tbl, 6, row, sum->mint_90th);

    table_set_value(tbl, 7, row, sum->maxt);
    table_set_value(tbl, 8, row, sum->maxt_10th);
    table_set_value(tbl, 9, row, sum->maxt_25th);
    table_set_value(tbl, 10, row, sum->maxt_50th);
    table_set_value(tbl, 11, row, sum->maxt_75th);
    table_set_value(tbl, 12, row, sum->maxt_90th);

    tbl_state->row++;

    return false;
}

/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_temperature_summary(struct NBMData const *nbm)
{
    GTree *temps = extract_temperature_data(nbm);
    Stopif(!temps, return, "Error extracting CDFs for QPF.");

    struct Table *tbl = table_new(13, g_tree_nnodes(temps));
    build_title(nbm, tbl);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT,  "Day/Date",          "%s", 17);
    table_add_column(tbl,  1, Table_ColumnType_VALUE, "MinT (F)", "   %3.0lf° ",  8);
    table_add_column(tbl,  2, Table_ColumnType_VALUE,     "10th",   " %3.0lf° ",  6);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,     "25th",   " %3.0lf° ",  6);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,     "50th",   " %3.0lf° ",  6);
    table_add_column(tbl,  5, Table_ColumnType_VALUE,     "75th",   " %3.0lf° ",  6);
    table_add_column(tbl,  6, Table_ColumnType_VALUE,     "90th",   " %3.0lf° ",  6);
    table_add_column(tbl,  7, Table_ColumnType_VALUE, "MaxT (F)", "   %3.0lf° ",  8);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,     "10th",   " %3.0lf° ",  6);
    table_add_column(tbl,  9, Table_ColumnType_VALUE,     "25th",   " %3.0lf° ",  6);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,     "50th",   " %3.0lf° ",  6);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,     "75th",   " %3.0lf° ",  6);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,     "90th",   " %3.0lf° ",  6);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 7);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(temps, add_row_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(temps);

    table_free(&tbl);
}
