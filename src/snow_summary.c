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

struct SnowSum {
    char *id;
    char *name;
    time_t init_time;

    int accum_hours;

    GTree *cdfs;
    GTree *pdfs;
    GTree *scenarios;
};

static GTree *
build_cdfs(NBMData const *nbm, int hours)
{
    char percentile_format[32] = {0};
    char deterministic_snow_key[32] = {0};

    sprintf(percentile_format, "ASNOW%dhr_surface_%%d%%%% level", hours);
    sprintf(deterministic_snow_key, "ASNOW%dhr_surface", hours);

    GTree *cdfs = extract_cdfs(nbm, percentile_format, deterministic_snow_key, m_to_in);
    Stopif(!cdfs, return 0, "Error extracting CDFs for snow.");

    return cdfs;
}

struct SnowSum *
snow_sum_build(NBMData const *nbm, int accum_hours)
{
    struct SnowSum *new = calloc(1, sizeof(struct SnowSum));
    assert(new);

    new->id = strdup(nbm_data_site_id(nbm));
    new->name = strdup(nbm_data_site_name(nbm));
    new->init_time = nbm_data_init_time(nbm);

    new->accum_hours = accum_hours;

    new->cdfs = build_cdfs(nbm, accum_hours);
    Stopif(!new->cdfs, goto ERR_RETURN, "Error extracting CDFs for snow.");

    new->pdfs = 0;
    new->scenarios = 0;

    return new;

ERR_RETURN:
    snow_sum_free(&new);
    return 0;
}

#define SUMMARY 0
#define SCENARIOS 1

static void
build_title(struct SnowSum const *ssum, Table *tbl, int type)
{
    char title_buf[256] = {0};
    struct tm init = *gmtime(&ssum->init_time);

    if (type == SUMMARY) {
        sprintf(title_buf, "%d Hr Probabilistic Snow for %s (%s) - ", ssum->accum_hours, ssum->name,
                ssum->id);
    } else if (type == SCENARIOS) {
        sprintf(title_buf, "%d Hr Snow Scenarios for %s (%s) - ", ssum->accum_hours, ssum->name,
                ssum->id);
    } else {
        assert(false);
    }

    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_prob_snow_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *dist = value;
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

    tbl_state->row++;

    return false;
}

void
show_snow_summary(struct SnowSum const *ssum)
{
    assert(ssum);

    char left_col_title[32] = {0};
    sprintf(left_col_title, "%d Hrs Ending / in.", ssum->accum_hours);

    GTree *cdfs = ssum->cdfs;
    assert(cdfs);

    int num_rows = g_tree_nnodes(cdfs);
    if (num_rows == 0) {
        printf("\n\n     ***** No snow summary for accumulation period %d. *****\n\n",
               ssum->accum_hours);
        return;
    }

    Table *tbl = table_new(15, num_rows);
    build_title(ssum, tbl, SUMMARY);

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
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 6);

    for (int i = 1; i <= 14; i++) {
        table_set_blank_value(tbl, i, 0.0);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_snow_exceedence_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);
}

static int
create_pdf_from_cdf_and_add_too_pdf_tree(void *key, void *val, void *data)
{
    CumulativeDistribution *cdf = val;
    GTree *pdfs = data;

    ProbabilityDistribution *pdf = probability_dist_calc(cdf, 0.2);

    g_tree_insert(pdfs, key, pdf);

    return false;
}

static void
snow_sum_build_pdfs(struct SnowSum *ssum)
{
    assert(ssum && ssum->cdfs);

    // Don't free the keys, we'll just point those to the same location as the keys
    // used in the GTree of CDFs.
    GTree *pdfs = g_tree_new_full(time_t_compare_func, 0, 0, probability_dist_free);

    g_tree_foreach(ssum->cdfs, create_pdf_from_cdf_and_add_too_pdf_tree, pdfs);

    ssum->pdfs = pdfs;
}

static int
create_scenarios_from_pdf_and_add_too_scenario_tree(void *key, void *val, void *data)
{
    ProbabilityDistribution *pdf = val;
    GTree *scenarios = data;

    GList *scs = find_scenarios(pdf);

    g_tree_insert(scenarios, key, scs);

    return false;
}

static void
free_glist_of_scenarios(void *ptr)
{
    GList *scs = ptr;
    g_clear_list(&scs, scenario_free);
}

static void
snow_sum_build_scenarios(struct SnowSum *ssum)
{
    assert(ssum && ssum->cdfs && ssum->pdfs);

    // Don't free the keys, we'll just point those to the same location as the keys
    // used in the GTree of CDFs.
    GTree *scenarios = g_tree_new_full(time_t_compare_func, 0, 0, free_glist_of_scenarios);

    g_tree_foreach(ssum->pdfs, create_scenarios_from_pdf_and_add_too_scenario_tree, scenarios);

    ssum->scenarios = scenarios;
}

static int
add_row_scenario_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    GList *scenarios = value;

    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    size_t scenario_num = 1;
    for (GList *curr = scenarios; curr && scenario_num <= 4; curr = curr->next, scenario_num++) {
        struct Scenario *sc = curr->data;

        double min = round(scenario_get_minimum(sc) * 10.0) / 10.0;
        double mode = round(scenario_get_mode(sc) * 10.0) / 10.0;
        double max = round(scenario_get_maximum(sc) * 10.0) / 10.0;
        double prob = round(scenario_get_probability(sc) * 100.0);

        int start_col = (scenario_num - 1) * 4 + 1;

        table_set_value(tbl, start_col + 0, row, min);
        table_set_value(tbl, start_col + 1, row, mode);
        table_set_value(tbl, start_col + 2, row, max);
        table_set_value(tbl, start_col + 3, row, prob);
    }

    tbl_state->row++;

    return false;
}

void
show_snow_scenarios(struct SnowSum *ssum)
{
    assert(ssum);
    if (!ssum->pdfs) {
        snow_sum_build_pdfs(ssum);
    }

    if (!ssum->scenarios) {
        snow_sum_build_scenarios(ssum);
    }

    assert(ssum->pdfs && ssum->scenarios);

    char left_col_title[32] = {0};
    sprintf(left_col_title, "%d Hrs Ending", ssum->accum_hours);

    GTree *scenarios = ssum->scenarios;
    assert(scenarios);

    int num_rows = g_tree_nnodes(scenarios);
    if (num_rows == 0) {
        printf("\n\n     ***** No snow scenarios for accumulation period %d. *****\n\n",
               ssum->accum_hours);
        return;
    }

    Table *tbl = table_new(17, num_rows);
    build_title(ssum, tbl, SCENARIOS);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT, left_col_title,     "%s", 19);

    table_add_column(tbl,  1, Table_ColumnType_VALUE,       "Min-1", "%5.1lf",  5);
    table_add_column(tbl,  2, Table_ColumnType_VALUE,       "Mode1", "%5.1lf",  5);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,       "Max-1", "%5.1lf",  5);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,       "Prob1", "%5.0lf",  5);
    
    table_add_column(tbl,  5, Table_ColumnType_VALUE,       "Min-2", "%5.1lf",  5);
    table_add_column(tbl,  6, Table_ColumnType_VALUE,       "Mode2", "%5.1lf",  5);
    table_add_column(tbl,  7, Table_ColumnType_VALUE,       "Max-2", "%5.1lf",  5);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,       "Prob2", "%5.0lf",  5);
    
    table_add_column(tbl,  9, Table_ColumnType_VALUE,       "Min-3", "%5.1lf",  5);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,       "Mode3", "%5.1lf",  5);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,       "Max-3", "%5.1lf",  5);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,       "Prob3", "%5.0lf",  5);

    table_add_column(tbl, 13, Table_ColumnType_VALUE,       "Min-4", "%5.1lf",  5);
    table_add_column(tbl, 14, Table_ColumnType_VALUE,       "Mode4", "%5.1lf",  5);
    table_add_column(tbl, 15, Table_ColumnType_VALUE,       "Max-4", "%5.1lf",  5);
    table_add_column(tbl, 16, Table_ColumnType_VALUE,       "Prob4", "%5.0lf",  5);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 5);
    table_set_double_left_border(tbl, 9);
    table_set_double_left_border(tbl, 13);

    for (int i = 1; i <= 2; i++) {
        table_set_blank_value(tbl, i, 0.0);
    }

    table_set_blank_value(tbl, 3, NAN);
    table_set_blank_value(tbl, 4, 100.0);

    for (int i = 5; i <= 16; i++) {
        table_set_blank_value(tbl, i, NAN);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(scenarios, add_row_scenario_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);
}

static int
write_cdf(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *dist = value;
    FILE *f = state;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    fprintf(f, "\n\n# Period ending: %s\n", datebuf);

    cumulative_dist_write(dist, f);

    return false;
}

static int
write_pdf(void *key, void *value, void *state)
{
    time_t *vt = key;
    ProbabilityDistribution *dist = value;
    FILE *f = state;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    fprintf(f, "\n\n# Period ending: %s\n", datebuf);

    probability_dist_write(dist, f);

    return false;
}

static int
write_scenario(void *key, void *value, void *state)
{
    time_t *vt = key;
    GList *scenarios = value;
    FILE *f = state;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    fprintf(f, "\n\n# Period ending: %s\n", datebuf);

    for (GList *curr = scenarios; curr; curr = curr->next) {
        Scenario *sc = curr->data;

        fprintf(f, "%8lf %8lf %8lf %8lf\n", scenario_get_minimum(sc), scenario_get_mode(sc),
                scenario_get_maximum(sc), scenario_get_probability(sc));
    }

    return false;
}

void
snow_sum_save(struct SnowSum *ssum, char const *directory, char const *file_prefix)
{
    assert(ssum && ssum->cdfs);

    if (!ssum->pdfs) {
        snow_sum_build_pdfs(ssum);
    }

    if (!ssum->scenarios) {
        snow_sum_build_scenarios(ssum);
    }

    char *sep = "";
    if (file_prefix)
        sep = "_";

    char cdf_path[256] = {0};
    sprintf(cdf_path, "%s/%s%ssnow_cdfs.dat", directory, file_prefix ? file_prefix : "", sep);

    char pdf_path[256] = {0};
    sprintf(pdf_path, "%s/%s%ssnow_pdfs.dat", directory, file_prefix ? file_prefix : "", sep);

    char scenario_path[256] = {0};
    sprintf(scenario_path, "%s/%s%ssnow_scenarios.dat", directory, file_prefix ? file_prefix : "",
            sep);

    FILE *cdf_f = fopen(cdf_path, "w");
    Stopif(!cdf_f, return, "Unable to open cdfs.dat");

    FILE *pdf_f = fopen(pdf_path, "w");
    Stopif(!pdf_f, return, "Unable to open pdfs.dat");

    FILE *scenario_f = fopen(scenario_path, "w");
    Stopif(!scenario_f, return, "Unable to open scenarios.dat");

    g_tree_foreach(ssum->cdfs, write_cdf, cdf_f);
    g_tree_foreach(ssum->pdfs, write_pdf, pdf_f);
    g_tree_foreach(ssum->scenarios, write_scenario, scenario_f);

    fclose(cdf_f);
    fclose(pdf_f);
    fclose(scenario_f);
}

void
snow_sum_free(struct SnowSum **ssum)
{
    assert(ssum);

    struct SnowSum *ptr = *ssum;

    if (ptr) {
        free(ptr->id);
        free(ptr->name);
        g_tree_unref(ptr->scenarios);
        g_tree_unref(ptr->pdfs);
        g_tree_unref(ptr->cdfs);
    }

    free(ptr);
}
