#include "precip_summary.h"
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

struct TableFillerState {
    int row;
    struct Table *tbl;
};

static int
add_row_prob_liquid_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;
    struct TableFillerState *tbl_state = state;
    struct Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double prob_001 = round(interpolate_prob_of_exceedance(dist, 0.01));
    double prob_010 = round(interpolate_prob_of_exceedance(dist, 0.10));
    double prob_025 = round(interpolate_prob_of_exceedance(dist, 0.25));
    double prob_050 = round(interpolate_prob_of_exceedance(dist, 0.50));
    double prob_075 = round(interpolate_prob_of_exceedance(dist, 0.75));
    double prob_100 = round(interpolate_prob_of_exceedance(dist, 1.0));
    double prob_150 = round(interpolate_prob_of_exceedance(dist, 1.5));
    double prob_200 = round(interpolate_prob_of_exceedance(dist, 2.0));
    double pm_value = round(cumulative_dist_pm_value(dist) * 100.0) / 100.0;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);
    table_set_value(tbl, 1, row, pm_value);
    table_set_value(tbl, 2, row, prob_001);
    table_set_value(tbl, 3, row, prob_010);
    table_set_value(tbl, 4, row, prob_025);
    table_set_value(tbl, 5, row, prob_050);
    table_set_value(tbl, 6, row, prob_075);
    table_set_value(tbl, 7, row, prob_100);
    table_set_value(tbl, 8, row, prob_150);
    table_set_value(tbl, 9, row, prob_200);

    tbl_state->row++;

    return false;
}

static void
build_title_liquid(struct NBMData const *nbm, struct Table *tbl)
{
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "24 Hr Probabilistic Precipitation for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

void
show_precip_summary(struct NBMData const *nbm)
{
    GTree *cdfs = extract_cdfs(nbm, "APCP24hr_surface_%d%% level", "APCP24hr_surface", mm_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for QPF.");

    struct Table *tbl = table_new(10, g_tree_nnodes(cdfs));
    build_title_liquid(nbm, tbl);
    table_add_column(tbl, 0, Table_ColumnType_TEXT, strlen("24 Hrs Ending / in."),
                     "24 Hrs Ending / in.", strlen("%s"), "%s", 19);

    table_add_column(tbl, 1, Table_ColumnType_VALUE, strlen("Precip"), "Precip", strlen("%6.2lf"),
                     "%6.2lf", 6);

    table_set_double_left_border(tbl, 1);

    table_add_column(tbl, 2, Table_ColumnType_VALUE, strlen("0.01"), "0.01", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_set_double_left_border(tbl, 2);

    table_add_column(tbl, 3, Table_ColumnType_VALUE, strlen("0.10"), "0.10", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 4, Table_ColumnType_VALUE, strlen("0.25"), "0.25", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 5, Table_ColumnType_VALUE, strlen("0.50"), "0.50", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 6, Table_ColumnType_VALUE, strlen("0.75"), "0.75", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 7, Table_ColumnType_VALUE, strlen("1.00"), "1.00", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 8, Table_ColumnType_VALUE, strlen("1.50"), "1.50", strlen("%3.0lf"),
                     "%5.0lf", 5);

    table_add_column(tbl, 9, Table_ColumnType_VALUE, strlen("2.00"), "2.00", strlen("%3.0lf"),
                     "%5.0lf", 5);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_liquid_exceedence_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(cdfs);

    table_free(&tbl);
}
