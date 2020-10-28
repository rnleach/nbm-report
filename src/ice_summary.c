#include "ice_summary.h"
#include "nbm_data.h"
#include "summarize.h"
#include "table.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

static void
build_title_ice(struct NBMData const *nbm, struct Table *tbl, int hours)
{

    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "%d Hr Probabilistic Ice for %s - ", hours, nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_prob_ice_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;
    struct TableFillerState *tbl_state = state;
    struct Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double pm_value = round(cumulative_dist_pm_value(dist) * 100.0) / 100.0;

    double p10th = round(cumulative_dist_percentile_value(dist, 10.0) * 100.0) / 100.0;
    double p25th = round(cumulative_dist_percentile_value(dist, 25.0) * 100.0) / 100.0;
    double p50th = round(cumulative_dist_percentile_value(dist, 50.0) * 100.0) / 100.0;
    double p75th = round(cumulative_dist_percentile_value(dist, 75.0) * 100.0) / 100.0;
    double p90th = round(cumulative_dist_percentile_value(dist, 90.0) * 100.0) / 100.0;

    double prob_01 = round(interpolate_prob_of_exceedance(dist, 0.01));
    double prob_02 = round(interpolate_prob_of_exceedance(dist, 0.02));
    double prob_05 = round(interpolate_prob_of_exceedance(dist, 0.05));
    double prob_10 = round(interpolate_prob_of_exceedance(dist, 0.1));
    double prob_25 = round(interpolate_prob_of_exceedance(dist, 0.25));

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 1, row, pm_value);

    table_set_value(tbl, 2, row, p10th);
    table_set_value(tbl, 3, row, p25th);
    table_set_value(tbl, 4, row, p50th);
    table_set_value(tbl, 5, row, p75th);
    table_set_value(tbl, 6, row, p90th);

    table_set_value(tbl, 7, row, prob_01);
    table_set_value(tbl, 8, row, prob_02);
    table_set_value(tbl, 9, row, prob_05);
    table_set_value(tbl, 10, row, prob_10);
    table_set_value(tbl, 11, row, prob_25);

    tbl_state->row++;

    return false;
}

void
show_ice_summary(struct NBMData const *nbm, int hours)
{
    char percentile_format[32] = {0};
    char deterministic_ice_key[32] = {0};
    char left_col_title[32] = {0};

    sprintf(percentile_format, "FICEAC%dhr_surface_%%d%%%% level", hours);
    sprintf(deterministic_ice_key, "FICEAC%dhr_surface", hours);
    sprintf(left_col_title, "%d Hrs Ending / in.", hours);

    GTree *cdfs = extract_cdfs(nbm, percentile_format, deterministic_ice_key, mm_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for Ice.");

    int num_rows = g_tree_nnodes(cdfs);
    if (num_rows == 0) {
        printf("\n\n     ***** No ice summary for accumulation period %d. *****\n\n", hours);
        return;
    }

    struct Table *tbl = table_new(12, num_rows);
    build_title_ice(nbm, tbl, hours);

    // clang-format off
    table_add_column(tbl, 0,  Table_ColumnType_TEXT, left_col_title,     "%s", 19);
    table_add_column(tbl, 1,  Table_ColumnType_VALUE,  "Ice",        "%6.2lf",  6);
    table_add_column(tbl, 2,  Table_ColumnType_VALUE, "10th",        "%4.2lf",  4);
    table_add_column(tbl, 3,  Table_ColumnType_VALUE, "25th",        "%4.2lf",  4);
    table_add_column(tbl, 4,  Table_ColumnType_VALUE, "50th",        "%4.2lf",  4);
    table_add_column(tbl, 5,  Table_ColumnType_VALUE, "75th",        "%4.2lf",  4);
    table_add_column(tbl, 6,  Table_ColumnType_VALUE, "90th",        "%4.2lf",  4);
    table_add_column(tbl, 7,  Table_ColumnType_VALUE, "0.01",        "%5.0lf",  5);
    table_add_column(tbl, 8,  Table_ColumnType_VALUE, "0.02",        "%5.0lf",  5);
    table_add_column(tbl, 9,  Table_ColumnType_VALUE, "0.05",        "%5.0lf",  5);
    table_add_column(tbl, 10, Table_ColumnType_VALUE, "0.10",        "%5.0lf",  5);
    table_add_column(tbl, 11, Table_ColumnType_VALUE, "0.25",        "%5.0lf",  5);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 2);
    table_set_double_left_border(tbl, 7);

    for (int i = 1; i <= 11; i++) {
        table_set_blank_zeros(tbl, i);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_ice_exceedence_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(cdfs);

    table_free(&tbl);
}
