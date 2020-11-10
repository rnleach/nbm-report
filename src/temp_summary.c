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
#include "utils.h"

/*-------------------------------------------------------------------------------------------------
 *                                    Daily Temperature Summary
 *-----------------------------------------------------------------------------------------------*/
struct DailyTempSum {
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
temperature_sum_not_printable(struct DailyTempSum const *sum)
{
    return isnan(sum->maxt) || isnan(sum->maxt_10th);
}

static void *
temp_sum_new()
{
    struct DailyTempSum *new = malloc(sizeof(struct DailyTempSum));
    assert(new);

    *new = (struct DailyTempSum){
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
    struct DailyTempSum *sum = sm;
    return &sum->mint;
}

static double *
temp_sum_access_mint_10th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->mint_10th;
}

static double *
temp_sum_access_mint_25th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->mint_25th;
}

static double *
temp_sum_access_mint_50th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->mint_50th;
}

static double *
temp_sum_access_mint_75th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->mint_75th;
}

static double *
temp_sum_access_mint_90th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->mint_90th;
}

static double *
temp_sum_access_maxt(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt;
}

static double *
temp_sum_access_maxt_10th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt_10th;
}

static double *
temp_sum_access_maxt_25th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt_25th;
}

static double *
temp_sum_access_maxt_50th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt_50th;
}

static double *
temp_sum_access_maxt_75th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt_75th;
}

static double *
temp_sum_access_maxt_90th(void *sm)
{
    struct DailyTempSum *sum = sm;
    return &sum->maxt_90th;
}

/*-------------------------------------------------------------------------------------------------
 *                                    Full Temperature Summary
 *-----------------------------------------------------------------------------------------------*/
struct TempSum {
    char *id;
    char *name;
    time_t init_time;

    NBMData const *src;

    GTree *dailys;

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

    new->dailys = 0;

    new->max_pdfs = 0;
    new->max_cdfs = 0;
    new->max_scenarios = 0;

    new->min_pdfs = 0;
    new->min_cdfs = 0;
    new->min_scenarios = 0;

    return new;
}

static void
temp_sum_build_dailys(struct TempSum *tsum)
{
    assert(tsum && tsum->src);

    GTree *sums = g_tree_new_full(time_t_compare_func, 0, free, free);
    NBMData const *nbm = tsum->src;

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_10% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint_10th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_25% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint_25th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_50% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint_50th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_75% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint_75th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Min_2 m above ground_90% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_mint_90th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_10% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt_10th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_25% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt_25th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_50% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt_50th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_75% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt_75th);

    extract_daily_summary_for_column(sums, nbm, "TMP_Max_2 m above ground_90% level", keep_all,
                                     summary_date_06z, kelvin_to_fahrenheit, accum_last,
                                     temp_sum_new, temp_sum_access_maxt_90th);

    tsum->dailys = sums;
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
add_summary_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct DailyTempSum *sum = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
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

        int start_col = (scenario_num - 1) * 4 + 1;

        table_set_value(tbl, start_col + 0, row, min);
        table_set_value(tbl, start_col + 1, row, mode);
        table_set_value(tbl, start_col + 2, row, max);
        table_set_value(tbl, start_col + 3, row, prob);
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
    if (!tsum->dailys) {
        temp_sum_build_dailys(tsum);
    }

    GTree *temps = tsum->dailys;
    Table *tbl = table_new(13, g_tree_nnodes(temps));
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

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(temps, add_summary_row_to_table, &state);

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

    Table *tbl = table_new(17, num_rows);
    build_title(tsum, tbl, "Max", SCENARIOS);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT,  "Day/Date",          "%s", 17);
    
    table_add_column(tbl,  1, Table_ColumnType_VALUE,    "Min-1",   " %3.0lf° ",  6);
    table_add_column(tbl,  2, Table_ColumnType_VALUE,    "Mode1",   " %3.0lf° ",  6);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,    "Max-1",   " %3.0lf° ",  6);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,    "Prob1",    " %4.0lf ",  6);

    table_add_column(tbl,  5, Table_ColumnType_VALUE,    "Min-2",   " %3.0lf° ",  6);
    table_add_column(tbl,  6, Table_ColumnType_VALUE,    "Mode2",   " %3.0lf° ",  6);
    table_add_column(tbl,  7, Table_ColumnType_VALUE,    "Max-2",   " %3.0lf° ",  6);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,    "Prob2",    " %4.0lf ",  6);

    table_add_column(tbl,  9, Table_ColumnType_VALUE,    "Min-3",   " %3.0lf° ",  6);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,    "Mode3",   " %3.0lf° ",  6);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,    "Max-3",   " %3.0lf° ",  6);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,    "Prob3",    " %4.0lf ",  6);

    table_add_column(tbl, 13, Table_ColumnType_VALUE,    "Min-4",   " %3.0lf° ",  6);
    table_add_column(tbl, 14, Table_ColumnType_VALUE,    "Mode4",   " %3.0lf° ",  6);
    table_add_column(tbl, 15, Table_ColumnType_VALUE,    "Max-4",   " %3.0lf° ",  6);
    table_add_column(tbl, 16, Table_ColumnType_VALUE,    "Prob4",    " %4.0lf ",  6);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 5);
    table_set_double_left_border(tbl, 9);
    table_set_double_left_border(tbl, 13);

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

    tbl = table_new(17, num_rows);
    build_title(tsum, tbl, "Min", SCENARIOS);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT,  "Day/Date",          "%s", 17);
    
    table_add_column(tbl,  1, Table_ColumnType_VALUE,    "Min-1",   " %3.0lf° ",  6);
    table_add_column(tbl,  2, Table_ColumnType_VALUE,    "Mode1",   " %3.0lf° ",  6);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,    "Max-1",   " %3.0lf° ",  6);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,    "Prob1",    " %4.0lf ",  6);

    table_add_column(tbl,  5, Table_ColumnType_VALUE,    "Min-2",   " %3.0lf° ",  6);
    table_add_column(tbl,  6, Table_ColumnType_VALUE,    "Mode2",   " %3.0lf° ",  6);
    table_add_column(tbl,  7, Table_ColumnType_VALUE,    "Max-2",   " %3.0lf° ",  6);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,    "Prob2",    " %4.0lf ",  6);

    table_add_column(tbl,  9, Table_ColumnType_VALUE,    "Min-3",   " %3.0lf° ",  6);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,    "Mode3",   " %3.0lf° ",  6);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,    "Max-3",   " %3.0lf° ",  6);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,    "Prob3",    " %4.0lf ",  6);

    table_add_column(tbl, 13, Table_ColumnType_VALUE,    "Min-4",   " %3.0lf° ",  6);
    table_add_column(tbl, 14, Table_ColumnType_VALUE,    "Mode4",   " %3.0lf° ",  6);
    table_add_column(tbl, 15, Table_ColumnType_VALUE,    "Max-4",   " %3.0lf° ",  6);
    table_add_column(tbl, 16, Table_ColumnType_VALUE,    "Prob4",    " %4.0lf ",  6);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 5);
    table_set_double_left_border(tbl, 9);
    table_set_double_left_border(tbl, 13);

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

    if (ptr->dailys) {
        g_tree_unref(ptr->dailys);
    }

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
