#include "snow_summary.h"
#include "distributions.h"
#include "nbm_data.h"
#include "table.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

#define SUMMARY 0
#define SCENARIOS 1

static void
build_title(NBMData const *nbm, Table *tbl, int hours, int mode)
{

    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "%d Hr Probabilistic Snow for %s (%s) - ", hours, nbm_data_site_name(nbm),
            nbm_data_site_id(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static GTree *
build_cdfs(NBMData const *nbm, int hours)
{
    char percentile_format[32] = {0};
    char deterministic_snow_key[32] = {0};

    sprintf(percentile_format, "ASNOW%dhr_surface_%%d%%%% level", hours);
    sprintf(deterministic_snow_key, "ASNOW%dhr_surface", hours);

    GTree *cdfs = extract_cdfs(nbm, percentile_format, deterministic_snow_key, m_to_in);

    return cdfs;
}
/*-------------------------------------------------------------------------------------------------
 *                                      Precipitation Summary
 *-----------------------------------------------------------------------------------------------*/
static int
add_row_prob_snow_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *dist = value;
    ProbabilityDistribution *pdf = probability_dist_calc(dist, 0.0, 36.0, 0.2);
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double p10th = round(cumulative_dist_percentile_value(dist, 10.0) * 10.0) / 10.0;
    double p25th = round(cumulative_dist_percentile_value(dist, 25.0) * 10.0) / 10.0;
    double p50th = round(cumulative_dist_percentile_value(dist, 50.0) * 10.0) / 10.0;
    double p75th = round(cumulative_dist_percentile_value(dist, 75.0) * 10.0) / 10.0;
    double p90th = round(cumulative_dist_percentile_value(dist, 90.0) * 10.0) / 10.0;

    double prob_01 = round(interpolate_prob_of_exceedance(dist, 0.1));
    double prob_05 = round(interpolate_prob_of_exceedance(dist, 0.5));
    double prob_10 = round(interpolate_prob_of_exceedance(dist, 1.0));
    double prob_30 = round(interpolate_prob_of_exceedance(dist, 3.0));
    double prob_60 = round(interpolate_prob_of_exceedance(dist, 6.0));
    double prob_80 = round(interpolate_prob_of_exceedance(dist, 8.0));
    double prob_120 = round(interpolate_prob_of_exceedance(dist, 12.0));
    double prob_180 = round(interpolate_prob_of_exceedance(dist, 18.0));
    double prob_240 = round(interpolate_prob_of_exceedance(dist, 24.0));

    double mode1 = round(probability_dist_get_mode(pdf, 1) * 10.0) / 10.0;
    double mode2 = round(probability_dist_get_mode(pdf, 2) * 10.0) / 10.0;
    double wgt2 = round(probability_dist_get_mode_weight(pdf, 2) * 100.0) / 100.0;
    double mode3 = round(probability_dist_get_mode(pdf, 3) * 10.0) / 10.0;
    double wgt3 = round(probability_dist_get_mode_weight(pdf, 3) * 100.0) / 100.0;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 1, row, p10th);
    table_set_value(tbl, 2, row, p25th);
    table_set_value(tbl, 3, row, p50th);
    table_set_value(tbl, 4, row, p75th);
    table_set_value(tbl, 5, row, p90th);

    table_set_value(tbl, 6, row, prob_01);
    table_set_value(tbl, 7, row, prob_05);
    table_set_value(tbl, 8, row, prob_10);
    table_set_value(tbl, 9, row, prob_30);
    table_set_value(tbl, 10, row, prob_60);
    table_set_value(tbl, 11, row, prob_80);
    table_set_value(tbl, 12, row, prob_120);
    table_set_value(tbl, 13, row, prob_180);
    table_set_value(tbl, 14, row, prob_240);

    table_set_value(tbl, 15, row, mode1);
    table_set_value(tbl, 16, row, mode2);
    table_set_value(tbl, 17, row, wgt2);
    table_set_value(tbl, 18, row, mode3);
    table_set_value(tbl, 19, row, wgt3);

    probability_dist_free(pdf);

    tbl_state->row++;

    return false;
}

void
show_snow_summary(NBMData const *nbm, int hours)
{
    char left_col_title[32] = {0};

    sprintf(left_col_title, "%d Hrs Ending / in.", hours);

    GTree *cdfs = build_cdfs(nbm, hours);
    Stopif(!cdfs, return, "Error extracting CDFs for Snow.");

    int num_rows = g_tree_nnodes(cdfs);
    if (num_rows == 0) {
        printf("\n\n     ***** No snow summary for accumulation period %d. *****\n\n", hours);
        return;
    }

    Table *tbl = table_new(20, num_rows);
    build_title(nbm, tbl, hours, SUMMARY);

    // clang-format off
    table_add_column(tbl, 0,  Table_ColumnType_TEXT, left_col_title,     "%s", 19);
    table_add_column(tbl, 1,  Table_ColumnType_VALUE, "10th",        "%4.1lf",  4);
    table_add_column(tbl, 2,  Table_ColumnType_VALUE, "25th",        "%4.1lf",  4);
    table_add_column(tbl, 3,  Table_ColumnType_VALUE, "50th",        "%4.1lf",  4);
    table_add_column(tbl, 4,  Table_ColumnType_VALUE, "75th",        "%4.1lf",  4);
    table_add_column(tbl, 5,  Table_ColumnType_VALUE, "90th",        "%4.1lf",  4);

    table_add_column(tbl, 6,  Table_ColumnType_VALUE,  "0.1",        "%5.0lf",  5);
    table_add_column(tbl, 7,  Table_ColumnType_VALUE,  "0.5",        "%5.0lf",  5);
    table_add_column(tbl, 8,  Table_ColumnType_VALUE,  "1.0",        "%5.0lf",  5);
    table_add_column(tbl, 9,  Table_ColumnType_VALUE,  "3.0",        "%5.0lf",  5);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,  "6.0",        "%5.0lf",  5);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,  "8.0",        "%5.0lf",  5);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,  "12.0",       "%5.0lf",  5);
    table_add_column(tbl, 13, Table_ColumnType_VALUE,  "18.0",       "%5.0lf",  5);
    table_add_column(tbl, 14, Table_ColumnType_VALUE,  "24.0",       "%5.0lf",  5);

    table_add_column(tbl, 15, Table_ColumnType_VALUE,  "Mode-1",     "%6.1lf",  6);
    table_add_column(tbl, 16, Table_ColumnType_VALUE,  "Mode-2",     "%6.1lf",  6);
    table_add_column(tbl, 17, Table_ColumnType_VALUE,   "Wgt-2",     "%5.2lf",  5);
    table_add_column(tbl, 18, Table_ColumnType_VALUE,  "Mode-3",     "%6.1lf",  6);
    table_add_column(tbl, 19, Table_ColumnType_VALUE,   "Wgt-3",     "%5.2lf",  5);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 6);
    table_set_double_left_border(tbl, 15);

    for (int i = 1; i <= 15; i++) {
        table_set_blank_value(tbl, i, 0.0);
    }

    for (int i = 16; i <= 19; i++) {
        table_set_blank_value(tbl, i, NAN);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_snow_exceedence_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(cdfs);

    table_free(&tbl);
}

/*-------------------------------------------------------------------------------------------------
 *                                      Precipitation Scenarios
 *-----------------------------------------------------------------------------------------------*/
void
show_snow_scenarios(NBMData const *nbm, int hours)
{
    assert(nbm);

    char left_col_title[32] = {0};
    sprintf(left_col_title, "%d Hrs Ending", hours);

    GTree *cdfs = build_cdfs(nbm, hours);
    Stopif(!cdfs, return, "Error extracting CDFs for Snow.");

    int num_rows = g_tree_nnodes(cdfs);
    if (num_rows == 0) {
        printf("\n\n     ***** No snow scenarios for accumulation period %d. *****\n\n", hours);
        return;
    }

    Table *tbl = table_new(18, num_rows);
    build_title(nbm, tbl, hours, SCENARIOS);

    // TODO
    assert(false);
}
