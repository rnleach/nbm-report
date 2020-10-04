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

/*-------------------------------------------------------------------------------------------------
 *                                            Liquid Summary
 *-----------------------------------------------------------------------------------------------*/
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
    double prob_005 = round(interpolate_prob_of_exceedance(dist, 0.05));
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

    table_add_row_label(tbl, row, strlen(datebuf), datebuf);
    table_add_value(tbl, 0, row, pm_value);
    table_add_value(tbl, 1, row, prob_001);
    table_add_value(tbl, 2, row, prob_005);
    table_add_value(tbl, 3, row, prob_010);
    table_add_value(tbl, 4, row, prob_025);
    table_add_value(tbl, 5, row, prob_050);
    table_add_value(tbl, 6, row, prob_075);
    table_add_value(tbl, 7, row, prob_100);
    table_add_value(tbl, 8, row, prob_150);
    table_add_value(tbl, 9, row, prob_200);

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

static void
show_liquid_summary(struct NBMData const *nbm)
{
    GTree *cdfs = extract_cdfs(nbm, "APCP24hr_surface_%d%% level", "APCP24hr_surface", mm_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for QPF.");

    struct Table *tbl = table_new(10, g_tree_nnodes(cdfs));
    build_title_liquid(nbm, tbl);
    table_add_column(tbl, -1, strlen("24 Hrs Ending / in."), "24 Hrs Ending / in.", 0, 0, 19);
    table_add_column(tbl, 0, strlen("Precip"), "Precip", strlen("%4.2lf"), "%6.2lf", 6);
    table_add_column(tbl, 1, strlen("0.01"), "0.01", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 2, strlen("0.05"), "0.05", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 3, strlen("0.10"), "0.10", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 4, strlen("0.25"), "0.25", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 5, strlen("0.50"), "0.50", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 6, strlen("0.75"), "0.75", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 7, strlen("1.00"), "1.00", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 8, strlen("1.50"), "1.50", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 9, strlen("2.00"), "2.00", strlen("%3.0lf"), "%5.0lf", 5);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_liquid_exceedence_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(cdfs);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Snow Summary
 *-----------------------------------------------------------------------------------------------*/
static void
build_title_snow(struct NBMData const *nbm, struct Table *tbl)
{

    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "24 Hr Probabilistic Snow for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_prob_snow_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;
    struct TableFillerState *tbl_state = state;
    struct Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double prob_01 = round(interpolate_prob_of_exceedance(dist, 0.1));
    double prob_05 = round(interpolate_prob_of_exceedance(dist, 0.5));
    double prob_10 = round(interpolate_prob_of_exceedance(dist, 1.0));
    double prob_20 = round(interpolate_prob_of_exceedance(dist, 2.0));
    double prob_40 = round(interpolate_prob_of_exceedance(dist, 4.0));
    double prob_60 = round(interpolate_prob_of_exceedance(dist, 6.0));
    double prob_80 = round(interpolate_prob_of_exceedance(dist, 8.0));
    double prob_120 = round(interpolate_prob_of_exceedance(dist, 12.0));
    double prob_180 = round(interpolate_prob_of_exceedance(dist, 18.0));
    double prob_240 = round(interpolate_prob_of_exceedance(dist, 24.0));
    double pm_value = round(cumulative_dist_pm_value(dist) * 10.0) / 10.0;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_add_row_label(tbl, row, strlen(datebuf), datebuf);
    table_add_value(tbl, 0, row, pm_value);
    table_add_value(tbl, 1, row, prob_01);
    table_add_value(tbl, 2, row, prob_05);
    table_add_value(tbl, 3, row, prob_10);
    table_add_value(tbl, 4, row, prob_20);
    table_add_value(tbl, 5, row, prob_40);
    table_add_value(tbl, 6, row, prob_60);
    table_add_value(tbl, 7, row, prob_80);
    table_add_value(tbl, 8, row, prob_120);
    table_add_value(tbl, 9, row, prob_180);
    table_add_value(tbl, 10, row, prob_240);

    tbl_state->row++;

    return false;
}

static void
show_snow_summary(struct NBMData const *nbm)
{
    GTree *cdfs = extract_cdfs(nbm, "ASNOW24hr_surface_%d%% level", "ASNOW24hr_surface", m_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for Snow.");

    struct Table *tbl = table_new(11, g_tree_nnodes(cdfs));
    build_title_snow(nbm, tbl);

    table_add_column(tbl, -1, strlen("24 Hrs Ending / in."), "24 Hrs Ending / in.", 0, 0, 19);
    table_add_column(tbl, 0, strlen("Snow"), "Snow", strlen("%4.1lf"), "%6.2lf", 6);
    table_add_column(tbl, 1, strlen("0.1"), "0.1", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 2, strlen("0.5"), "0.5", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 3, strlen("1.0"), "1.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 4, strlen("2.0"), "2.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 5, strlen("4.0"), "4.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 6, strlen("6.0"), "6.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 7, strlen("8.0"), "8.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 8, strlen("12.0"), "12.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 9, strlen("18.0"), "18.0", strlen("%3.0lf"), "%5.0lf", 5);
    table_add_column(tbl, 10, strlen("24.0"), "24.0", strlen("%3.0lf"), "%5.0lf", 5);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_snow_exceedence_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(cdfs);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Public API
 *-----------------------------------------------------------------------------------------------*/
void
show_precip_summary(struct NBMData const *nbm, char type)
{
    assert(type == 'r' || type == 's');

    switch (type) {
    case 'r':
        show_liquid_summary(nbm);
        break;
    case 's':
        show_snow_summary(nbm);
        break;
    default:
        assert(false);
    }
}
