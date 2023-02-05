#include "gust_summary.h"
#include "distributions.h"
#include "table.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include <glib.h>

struct GustSum {
    char *id;
    char *name;
    time_t init_time;

    GTree *cdfs;
    GTree *pdfs;
    GTree *scenarios;
};

#define NUM_PROB_EXC_VALS 6
static char const *exc_vals[NUM_PROB_EXC_VALS] = {
    "11", "17", "21", "24", "28", "32",
};

static GTree *
build_cdfs(NBMData const *nbm)
{
    char percentile_format[40] = {0};
    char deterministic_gust_key[32] = {0};

    sprintf(percentile_format, "GUST24hr_10 m above ground_%%d%%%% level");
    sprintf(deterministic_gust_key, "GUST24hr_10 m above ground");

    GTree *cdfs = extract_cdfs(nbm, percentile_format, deterministic_gust_key, mps_to_mph);
    Stopif(!cdfs, return 0, "Error extracting CDFs for Wind.");

    char prob_exceedence_format[40] = {0};
    sprintf(prob_exceedence_format, "GUST24hr_10 m above ground_prob >%%s");

    cdfs = extract_exceedence_to_cdfs(cdfs, nbm, prob_exceedence_format, NUM_PROB_EXC_VALS,
                                      exc_vals, mps_to_mph);

    return cdfs;
}

struct GustSum *
gust_sum_build(NBMData const *nbm)
{
    struct GustSum *new = calloc(1, sizeof(struct GustSum));
    assert(new);

    new->id = strdup(nbm_data_site_id(nbm));
    new->name = strdup(nbm_data_site_name(nbm));
    new->init_time = nbm_data_init_time(nbm);

    new->cdfs = build_cdfs(nbm);
    Stopif(!new->cdfs, goto ERR_RETURN, "Error extracting CDFs for Wind.");

    new->pdfs = 0;
    new->scenarios = 0;

    return new;

ERR_RETURN:
    gust_sum_free(&new);
    return 0;
}

#define SUMMARY 0
#define SCENARIOS 1

static void
build_title(struct GustSum const *gsum, Table *tbl, int type)
{
    char title_buf[256] = {0};
    struct tm init = *gmtime(&gsum->init_time);

    if (type == SUMMARY) {
        sprintf(title_buf, "24 Hr Probabilistic Max Gust Speed for %s (%s) - ", gsum->name,
                gsum->id);
    } else if (type == SCENARIOS) {
        sprintf(title_buf, "24 Hr Max Wind Gust Scenarios for %s (%s) - ", gsum->name, gsum->id);
    } else {
        assert(false);
    }

    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_prob_gust_exceedence_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    CumulativeDistribution *dist = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    double pm_value = round(cumulative_dist_pm_value(dist));

    double p10th = round(cumulative_dist_percentile_value(dist, 10.0));
    double p25th = round(cumulative_dist_percentile_value(dist, 25.0));
    double p50th = round(cumulative_dist_percentile_value(dist, 50.0));
    double p75th = round(cumulative_dist_percentile_value(dist, 75.0));
    double p90th = round(cumulative_dist_percentile_value(dist, 90.0));

    double prob_20 = round(interpolate_prob_of_exceedance(dist, 20));
    double prob_25 = round(interpolate_prob_of_exceedance(dist, 25));
    double prob_30 = round(interpolate_prob_of_exceedance(dist, 30));
    double prob_40 = round(interpolate_prob_of_exceedance(dist, 40));
    double prob_50 = round(interpolate_prob_of_exceedance(dist, 50));
    double prob_60 = round(interpolate_prob_of_exceedance(dist, 60));

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);

    table_set_value(tbl, 1, row, pm_value);

    table_set_value(tbl, 2, row, p10th);
    table_set_value(tbl, 3, row, p25th);
    table_set_value(tbl, 4, row, p50th);
    table_set_value(tbl, 5, row, p75th);
    table_set_value(tbl, 6, row, p90th);

    table_set_value(tbl, 7, row, prob_20);
    table_set_value(tbl, 8, row, prob_25);
    table_set_value(tbl, 9, row, prob_30);
    table_set_value(tbl, 10, row, prob_40);
    table_set_value(tbl, 11, row, prob_50);
    table_set_value(tbl, 12, row, prob_60);

    tbl_state->row++;

    return false;
}

void
show_gust_summary(struct GustSum const *gsum)
{
    assert(gsum);

    char left_col_title[32] = {0};
    sprintf(left_col_title, "24 Hrs Ending");

    GTree *cdfs = gsum->cdfs;
    assert(cdfs);

    int num_rows = g_tree_nnodes(cdfs);
    if (num_rows == 0) {
        printf("\n\n     ***** No gust summary. *****\n\n");
        return;
    }

    Table *tbl = table_new(13, num_rows);
    build_title(gsum, tbl, SUMMARY);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT, left_col_title,     "%s", 19);

    table_add_column(tbl,  1, Table_ColumnType_VALUE,    "Gust Spd", "%3.0lf ",  8);
    table_add_column(tbl,  2, Table_ColumnType_VALUE,        "10th", "%3.0lf ",  4);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,        "25th", "%3.0lf ",  4);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,        "50th", "%3.0lf ",  4);
    table_add_column(tbl,  5, Table_ColumnType_VALUE,        "75th", "%3.0lf ",  4);
    table_add_column(tbl,  6, Table_ColumnType_VALUE,        "90th", "%3.0lf ",  4);

    table_add_column(tbl,  7, Table_ColumnType_VALUE,        " 20 ", "%5.0lf",  5);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,        " 25 ", "%5.0lf",  5);
    table_add_column(tbl,  9, Table_ColumnType_VALUE,        " 30 ", "%5.0lf",  5);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,        " 40 ", "%5.0lf",  5);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,        " 50 ", "%5.0lf",  5);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,        " 60 ", "%5.0lf",  5);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 2);
    table_set_double_left_border(tbl, 7);

    for (int i = 1; i <= 12; i++) {
        table_set_blank_value(tbl, i, 0.0);
    }

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(cdfs, add_row_prob_gust_exceedence_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);

    return;
}

static int
create_pdf_from_cdf_and_add_too_pdf_tree(void *key, void *val, void *data)
{
    CumulativeDistribution *cdf = val;
    GTree *pdfs = data;

    ProbabilityDistribution *pdf = probability_dist_calc(cdf);

    g_tree_insert(pdfs, key, pdf);

    return false;
}

static void
gust_sum_build_pdfs(struct GustSum *gsum)
{
    assert(gsum && gsum->cdfs);

    // Don't free the keys, we'll just point those to the same location as the keys
    // used in the GTree of CDFs.
    GTree *pdfs = g_tree_new_full(time_t_compare_func, 0, 0, probability_dist_free);

    g_tree_foreach(gsum->cdfs, create_pdf_from_cdf_and_add_too_pdf_tree, pdfs);

    gsum->pdfs = pdfs;
}

static void
gust_sum_build_scenarios(struct GustSum *gsum)
{
    assert(gsum && gsum->cdfs && gsum->pdfs);

    GTree *scenarios = create_scenarios_from_pdfs(gsum->pdfs, 1.0, 2.0);
    assert(scenarios);

    gsum->scenarios = scenarios;
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

        double min = round(scenario_get_minimum(sc));
        double mode = round(scenario_get_mode(sc));
        double max = round(scenario_get_maximum(sc));
        double prob = round(scenario_get_probability(sc) * 100.0);

        table_set_scenario(tbl, scenario_num, row, mode, min, max, prob);
    }

    tbl_state->row++;

    return false;
}

void
show_gust_scenarios(struct GustSum *gsum)
{
    assert(gsum);
    if (!gsum->pdfs) {
        gust_sum_build_pdfs(gsum);
    }

    if (!gsum->scenarios) {
        gust_sum_build_scenarios(gsum);
    }

    assert(gsum->pdfs && gsum->scenarios);

    char left_col_title[32] = {0};
    sprintf(left_col_title, "24 Hrs Ending");

    GTree *scenarios = gsum->scenarios;
    assert(scenarios);

    int num_rows = g_tree_nnodes(scenarios);
    if (num_rows == 0) {
        printf("\n\n     ***** No wind scenarios. *****\n\n");
        return;
    }

    Table *tbl = table_new(5, num_rows);
    build_title(gsum, tbl, SCENARIOS);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT, left_col_title,     "%s", 19);

    table_add_column(tbl,  1, Table_ColumnType_SCENARIO, "Scenario-1", "%3.0lf [%3.0lf-%3.0lf] %3.0lf", 17);
    table_add_column(tbl,  2, Table_ColumnType_SCENARIO, "Scenario-2", "%3.0lf [%3.0lf-%3.0lf] %3.0lf", 17);
    table_add_column(tbl,  3, Table_ColumnType_SCENARIO, "Scenario-3", "%3.0lf [%3.0lf-%3.0lf] %3.0lf", 17);
    table_add_column(tbl,  4, Table_ColumnType_SCENARIO, "Scenario-4", "%3.0lf [%3.0lf-%3.0lf] %3.0lf", 17);
    // clang-format on

    for (int col = 1; col <= 4; col++) {
        table_set_blank_value(tbl, col, 0.0);
    }

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 2);
    table_set_double_left_border(tbl, 3);
    table_set_double_left_border(tbl, 4);

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

    fprintf(f, "\n\n\"Period ending: %s\"\n", datebuf);

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

    fprintf(f, "\n\n\"Period ending: %s\"\n", datebuf);

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

    fprintf(f, "\n\n\"Period ending: %s\"\n", datebuf);

    for (GList *curr = scenarios; curr; curr = curr->next) {
        Scenario *sc = curr->data;

        fprintf(f, "%8lf %8lf %8lf %8lf\n", scenario_get_minimum(sc), scenario_get_mode(sc),
                scenario_get_maximum(sc), scenario_get_probability(sc));
    }

    return false;
}

void
gust_sum_save(struct GustSum *gsum, char const *directory, char const *file_prefix)
{
    assert(gsum && gsum->cdfs);

    if (!gsum->cdfs) {
        gust_sum_build_pdfs(gsum);
    }

    if (!gsum->scenarios) {
        gust_sum_build_scenarios(gsum);
    }

    char *sep = "";
    if (file_prefix)
        sep = "_";

    char cdf_path[256] = {0};
    sprintf(cdf_path, "%s/%s%sgust_cdfs.dat", directory, file_prefix ? file_prefix : "", sep);

    char pdf_path[256] = {0};
    sprintf(pdf_path, "%s/%s%sgust_pdfs.dat", directory, file_prefix ? file_prefix : "", sep);

    char scenario_path[256] = {0};
    sprintf(scenario_path, "%s/%s%sgust_scenarios.dat", directory, file_prefix ? file_prefix : "",
            sep);

    FILE *cdf_f = fopen(cdf_path, "w");
    Stopif(!cdf_f, return, "Unable to open cdfs.dat");

    FILE *pdf_f = fopen(pdf_path, "w");
    Stopif(!pdf_f, return, "Unable to open pdfs.dat");

    FILE *scenario_f = fopen(scenario_path, "w");
    Stopif(!scenario_f, return, "Unable to open scenarios.dat");

    g_tree_foreach(gsum->cdfs, write_cdf, cdf_f);
    g_tree_foreach(gsum->pdfs, write_pdf, pdf_f);
    g_tree_foreach(gsum->scenarios, write_scenario, scenario_f);

    fclose(cdf_f);
    fclose(pdf_f);
    fclose(scenario_f);
}

void
gust_sum_free(struct GustSum **gsum)
{
    assert(gsum);

    struct GustSum *ptr = *gsum;

    if (ptr) {
        free(ptr->id);
        free(ptr->name);

        if (ptr->scenarios) {
            g_tree_unref(ptr->scenarios);
        }
        if (ptr->pdfs) {
            g_tree_unref(ptr->pdfs);
        }
        if (ptr->cdfs) {
            g_tree_unref(ptr->cdfs);
        }
    }

    free(ptr);

    *gsum = 0;
}

#undef NUM_PROB_EXC_VALS
#undef SUMMARY
#undef SCENARIOS
