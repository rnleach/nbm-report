#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "distributions.h"
#include "summarize.h"
#include "table.h"
#include "temp_summary.h"

/*-------------------------------------------------------------------------------------------------
 *                                      Temperature Summary
 *-----------------------------------------------------------------------------------------------*/
struct TempSum {
    char *id;
    char *name;
    time_t init_time;

    NBMData const *src;

    GTree *max_cdfs;
    GTree *max_pdfs;
    GTree *max_scenarios;

    GTree *min_cdfs;
    GTree *min_pdfs;
    GTree *min_scenarios;
};

struct TempSum *
temp_sum_build(NBMData const *nbm)
{
    struct TempSum *new = calloc(1, sizeof(struct TempSum));
    assert(new);

    new->id = strdup(nbm_data_site_id(nbm));
    new->name = strdup(nbm_data_site_name(nbm));
    new->init_time = nbm_data_init_time(nbm);

    new->src = nbm;

    new->max_pdfs = 0;
    new->max_cdfs = 0;
    new->max_scenarios = 0;

    new->min_pdfs = 0;
    new->min_cdfs = 0;
    new->min_scenarios = 0;

    return new;
}

static void
temp_sum_build_cdfs(struct TempSum *tsum)
{
    assert(tsum && tsum->src);
    NBMData const *nbm = tsum->src;

    char *max_percentile_format = "TMP_Max_2 m above ground_%d%% level";
    char *deterministic_max_temp_key = "TMP_Max_2 m above ground";

    GTree *max_cdfs =
        extract_cdfs(nbm, max_percentile_format, deterministic_max_temp_key, kelvin_to_fahrenheit);

    tsum->max_cdfs = max_cdfs;

    char *min_percentile_format = "TMP_Min_2 m above ground_%d%% level";
    char *deterministic_min_temp_key = "TMP_Min_2 m above ground";

    GTree *min_cdfs =
        extract_cdfs(nbm, min_percentile_format, deterministic_min_temp_key, kelvin_to_fahrenheit);

    tsum->min_cdfs = min_cdfs;
}

static int
create_pdf_from_cdf_and_add_too_pdf_tree(void *key, void *val, void *data)
{
    CumulativeDistribution *cdf = val;
    GTree *pdfs = data;

    ProbabilityDistribution *pdf = probability_dist_calc(cdf, 0.5);

    g_tree_insert(pdfs, key, pdf);

    return false;
}

static void
temp_sum_build_pdfs(struct TempSum *tsum)
{
    assert(tsum && tsum->src);

    if (!tsum->max_cdfs || !tsum->min_cdfs) {
        temp_sum_build_cdfs(tsum);
    }
    assert(tsum->max_cdfs && tsum->min_cdfs);

    // Don't free the keys, we'll just point those to the same location as the keys
    // used in the GTree of CDFs.
    GTree *max_pdfs = g_tree_new_full(time_t_compare_func, 0, 0, probability_dist_free);
    GTree *min_pdfs = g_tree_new_full(time_t_compare_func, 0, 0, probability_dist_free);

    g_tree_foreach(tsum->max_cdfs, create_pdf_from_cdf_and_add_too_pdf_tree, max_pdfs);
    g_tree_foreach(tsum->min_cdfs, create_pdf_from_cdf_and_add_too_pdf_tree, min_pdfs);

    tsum->max_pdfs = max_pdfs;
    tsum->min_pdfs = min_pdfs;
}

static void
temp_sum_build_scenarios(struct TempSum *tsum)
{
    assert(tsum && tsum->src);
    if (!tsum->max_pdfs || !tsum->min_pdfs) {
        temp_sum_build_pdfs(tsum);
    }

    // Don't free the keys, we'll just point those to the same location as the keys
    // used in the GTree of CDFs.
    GTree *max_scenarios = g_tree_new_full(time_t_compare_func, 0, 0, free_glist_of_scenarios);
    GTree *min_scenarios = g_tree_new_full(time_t_compare_func, 0, 0, free_glist_of_scenarios);

    g_tree_foreach(tsum->max_pdfs, create_scenarios_from_pdf_and_add_too_scenario_tree,
                   max_scenarios);
    g_tree_foreach(tsum->min_pdfs, create_scenarios_from_pdf_and_add_too_scenario_tree,
                   min_scenarios);

    tsum->max_scenarios = max_scenarios;
    tsum->min_scenarios = min_scenarios;
}

/*-------------------------------------------------------------------------------------------------
 *                                     Table Filling
 *-----------------------------------------------------------------------------------------------*/
#define SUMMARY 0
#define SCENARIOS 1

static void
build_title(struct TempSum const *tsum, Table *tbl, char const *desc, int type)
{
    char title_buf[256] = {0};
    struct tm init = *gmtime(&tsum->init_time);

    if (type == SUMMARY) {
        sprintf(title_buf, "Temperature Quantiles for %s (%s) - ", tsum->name, tsum->id);
    } else if (type == SCENARIOS) {
        sprintf(title_buf, "%s Temperature Scenarios for %s (%s) - ", desc, tsum->name, tsum->id);
    } else {
        assert(false);
    }

    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_min_summary_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *cdf = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double mint = round(cumulative_dist_pm_value(cdf));

    double mint_10th = round(cumulative_dist_percentile_value(cdf, 10.0));
    double mint_25th = round(cumulative_dist_percentile_value(cdf, 25.0));
    double mint_50th = round(cumulative_dist_percentile_value(cdf, 50.0));
    double mint_75th = round(cumulative_dist_percentile_value(cdf, 75.0));
    double mint_90th = round(cumulative_dist_percentile_value(cdf, 90.0));

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 1, row, mint);
    table_set_value(tbl, 2, row, mint_10th);
    table_set_value(tbl, 3, row, mint_25th);
    table_set_value(tbl, 4, row, mint_50th);
    table_set_value(tbl, 5, row, mint_75th);
    table_set_value(tbl, 6, row, mint_90th);

    tbl_state->row++;

    return false;
}

static int
add_max_summary_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *cdf = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double maxt = round(cumulative_dist_pm_value(cdf));

    double maxt_10th = round(cumulative_dist_percentile_value(cdf, 10.0));
    double maxt_25th = round(cumulative_dist_percentile_value(cdf, 25.0));
    double maxt_50th = round(cumulative_dist_percentile_value(cdf, 50.0));
    double maxt_75th = round(cumulative_dist_percentile_value(cdf, 75.0));
    double maxt_90th = round(cumulative_dist_percentile_value(cdf, 90.0));

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 7, row, maxt);
    table_set_value(tbl, 8, row, maxt_10th);
    table_set_value(tbl, 9, row, maxt_25th);
    table_set_value(tbl, 10, row, maxt_50th);
    table_set_value(tbl, 11, row, maxt_75th);
    table_set_value(tbl, 12, row, maxt_90th);

    tbl_state->row++;

    return false;
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
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    size_t scenario_num = 1;
    for (GList *curr = scenarios; curr && scenario_num <= 4; curr = curr->next, scenario_num++) {
        struct Scenario *sc = curr->data;

        double min = round(scenario_get_minimum(sc));
        double mode = round(scenario_get_mode(sc));
        double max = round(scenario_get_maximum(sc));
        double prob = round(scenario_get_probability(sc) * 100.0);

        table_set_scenario(tbl, scenario_num, row, mode, min, max, prob);
    }

    tbl_state->row++;

    return false;
}

/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_temp_summary(struct TempSum *tsum)
{
    if (!tsum->max_cdfs || !tsum->min_cdfs) {
        temp_sum_build_cdfs(tsum);
    }

    GTree *max_temps = tsum->max_cdfs;
    GTree *min_temps = tsum->min_cdfs;

    int num_max_rows = g_tree_nnodes(max_temps);
    int num_min_rows = g_tree_nnodes(min_temps);
    int num_rows = num_max_rows > num_min_rows ? num_max_rows : num_min_rows;

    Table *tbl = table_new(13, num_rows);
    build_title(tsum, tbl, 0, SUMMARY);

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

    struct TableFillerState state = {.row = num_rows - num_max_rows, .tbl = tbl};
    g_tree_foreach(max_temps, add_max_summary_row_to_table, &state);

    state = (struct TableFillerState){.row = num_rows - num_min_rows, .tbl = tbl};
    g_tree_foreach(min_temps, add_min_summary_row_to_table, &state);

    table_display(tbl, stdout);
    table_free(&tbl);
}

void
show_temp_scenarios(struct TempSum *tsum)
{
    assert(tsum);
    if (!tsum->max_scenarios || !tsum->min_scenarios) {
        temp_sum_build_scenarios(tsum);
    }

    assert(tsum->max_scenarios && tsum->min_scenarios);

    GTree *max_scenarios = tsum->max_scenarios;
    GTree *min_scenarios = tsum->min_scenarios;
    assert(max_scenarios && min_scenarios);

    // Build the MaxT scenarios table..
    int num_rows = g_tree_nnodes(max_scenarios);
    if (num_rows == 0) {
        printf("\n\n     ***** No max temperature scenarios. *****\n\n");
        return;
    }

    Table *tbl = table_new(5, num_rows);
    build_title(tsum, tbl, "Max", SCENARIOS);

    // clang-format off
    table_add_column(tbl, 0, Table_ColumnType_TEXT,     "Day/Date",   "%s",                               17);
    
    table_add_column(tbl, 1, Table_ColumnType_SCENARIO, "Scenario 1", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 2, Table_ColumnType_SCENARIO, "Scenario 2", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 3, Table_ColumnType_SCENARIO, "Scenario 3", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 4, Table_ColumnType_SCENARIO, "Scenario 4", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 2);
    table_set_double_left_border(tbl, 3);
    table_set_double_left_border(tbl, 4);

    for (int i = 1; i <= 4; i++) {
        table_set_blank_value(tbl, i, NAN);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(max_scenarios, add_row_scenario_to_table, &state);

    table_display(tbl, stdout);
    table_free(&tbl);

    // Show the scenarios for MinT
    num_rows = g_tree_nnodes(min_scenarios);
    if (num_rows == 0) {
        printf("\n\n     ***** No min temperature scenarios. *****\n\n");
        return;
    }

    tbl = table_new(5, num_rows);
    build_title(tsum, tbl, "Min", SCENARIOS);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT,  "Day/Date",          "%s", 17);
    
    table_add_column(tbl, 1, Table_ColumnType_SCENARIO, "Scenario 1", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 2, Table_ColumnType_SCENARIO, "Scenario 1", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 3, Table_ColumnType_SCENARIO, "Scenario 1", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    table_add_column(tbl, 4, Table_ColumnType_SCENARIO, "Scenario 1", "%3.0lf° [%3.0lf-%3.0lf] %3.0lf%%", 19);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 2);
    table_set_double_left_border(tbl, 3);
    table_set_double_left_border(tbl, 4);

    for (int i = 1; i <= 4; i++) {
        table_set_blank_value(tbl, i, NAN);
    }

    state = (struct TableFillerState){.row = 0, .tbl = tbl};
    g_tree_foreach(min_scenarios, add_row_scenario_to_table, &state);

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
    strftime(datebuf, sizeof(datebuf), "%a %Y-%m-%d", gmtime(vt));

    fprintf(f, "\n\n\"%s\"\n", datebuf);

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
    strftime(datebuf, sizeof(datebuf), "%a %Y-%m-%d", gmtime(vt));

    fprintf(f, "\n\n\"%s\"\n", datebuf);

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
    strftime(datebuf, sizeof(datebuf), "%a %Y-%m-%d", gmtime(vt));

    fprintf(f, "\n\n\"%s\"\n", datebuf);

    for (GList *curr = scenarios; curr; curr = curr->next) {
        Scenario *sc = curr->data;

        fprintf(f, "%8lf %8lf %8lf %8lf\n", scenario_get_minimum(sc), scenario_get_mode(sc),
                scenario_get_maximum(sc), scenario_get_probability(sc));
    }

    return false;
}

static void
do_save_distributions(char const *dir, char const *f_pfx, char const *element, GTree *cdfs,
                      GTree *pdfs, GTree *scenarios)
{
    char *sep = "";
    if (f_pfx)
        sep = "_";

    char cdf_path[256] = {0};
    sprintf(cdf_path, "%s/%s%s%s_temp_cdfs.dat", dir, f_pfx ? f_pfx : "", sep, element);

    char pdf_path[256] = {0};
    sprintf(pdf_path, "%s/%s%s%s_temp_pdfs.dat", dir, f_pfx ? f_pfx : "", sep, element);

    char scenario_path[256] = {0};
    sprintf(scenario_path, "%s/%s%s%s_temp_scenarios.dat", dir, f_pfx ? f_pfx : "", sep, element);

    FILE *cdf_f = fopen(cdf_path, "w");
    Stopif(!cdf_f, return, "Unable to open cdfs.dat");

    FILE *pdf_f = fopen(pdf_path, "w");
    Stopif(!pdf_f, return, "Unable to open pdfs.dat");

    FILE *scenario_f = fopen(scenario_path, "w");
    Stopif(!scenario_f, return, "Unable to open scenarios.dat");

    g_tree_foreach(cdfs, write_cdf, cdf_f);
    g_tree_foreach(pdfs, write_pdf, pdf_f);
    g_tree_foreach(scenarios, write_scenario, scenario_f);

    fclose(cdf_f);
    fclose(pdf_f);
    fclose(scenario_f);
}

void
temp_sum_save(struct TempSum *tsum, char const *directory, char const *file_prefix)
{
    assert(tsum);

    if (!tsum->max_cdfs || !tsum->min_cdfs) {
        temp_sum_build_cdfs(tsum);
    }

    if (!tsum->max_pdfs || !tsum->min_pdfs) {
        temp_sum_build_pdfs(tsum);
    }

    if (!tsum->max_scenarios || !tsum->min_scenarios) {
        temp_sum_build_scenarios(tsum);
    }

    do_save_distributions(directory, file_prefix, "max", tsum->max_cdfs, tsum->max_pdfs,
                          tsum->max_scenarios);

    do_save_distributions(directory, file_prefix, "min", tsum->min_cdfs, tsum->min_pdfs,
                          tsum->min_scenarios);
}

void
temp_sum_free(struct TempSum **tsum)
{

    struct TempSum *ptr = *tsum;

    if (ptr->max_cdfs) {
        g_tree_unref(ptr->max_cdfs);
    }
    if (ptr->max_pdfs) {
        g_tree_unref(ptr->max_pdfs);
    }
    if (ptr->max_scenarios) {
        g_tree_unref(ptr->max_scenarios);
    }

    if (ptr->min_cdfs) {
        g_tree_unref(ptr->min_cdfs);
    }
    if (ptr->min_pdfs) {
        g_tree_unref(ptr->min_pdfs);
    }
    if (ptr->min_scenarios) {
        g_tree_unref(ptr->min_scenarios);
    }

    free(ptr->id);
    free(ptr->name);

    free(ptr);
}
