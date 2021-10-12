#include "hourly.h"
#include "summarize.h" // Needed for some function typedef's
#include "table.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                        Hourly Data
 *-----------------------------------------------------------------------------------------------*/
#define MAX_LEAD_TIME_HRS 36

struct Hourly {
    double t_f;
    double t_std;
    double dp_f;
    double dp_std;
    double rh;
    double wind_dir;
    double wind_spd;
    double wind_spd_sd;
    double wind_gust;
    double wind_gust_sd;
    double sky;
    double pop;
    double qpf_1hr;
    double prob_ltg;
    double cape;
    double slr;
    double snow;
};

static bool
hourly_not_printable(struct Hourly const *hrly)
{
    return isnan(hrly->t_f) && isnan(hrly->dp_f) && isnan(hrly->rh) && isnan(hrly->wind_dir) &&
           isnan(hrly->wind_spd) && isnan(hrly->wind_gust);
}

static void *
hourly_new()
{
    struct Hourly *new = malloc(sizeof(struct Hourly));
    assert(new);

    *new = (struct Hourly){
        .t_f = NAN,
        .t_std = NAN,
        .dp_f = NAN,
        .dp_std = NAN,
        .rh = NAN,
        .wind_dir = NAN,
        .wind_spd = NAN,
        .wind_spd_sd = NAN,
        .wind_gust = NAN,
        .wind_gust_sd = NAN,
        .sky = NAN,
        .pop = NAN,
        .qpf_1hr = NAN,
        .prob_ltg = NAN,
        .cape = NAN,
        .slr = NAN,
        .snow = NAN,
    };

    return (void *)new;
}

static double *
hourly_access_slr(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->slr;
}

static double *
hourly_access_snow(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->snow;
}

static double *
hourly_access_prob_ltg(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->prob_ltg;
}

static double *
hourly_access_cape(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->cape;
}

static double *
hourly_access_pop(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->pop;
}

static double *
hourly_access_qpf_1hr(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->qpf_1hr;
}

static double *
hourly_access_sky(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->sky;
}

static double *
hourly_access_t(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->t_f;
}

static double *
hourly_access_t_std(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->t_std;
}

static double *
hourly_access_dp(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->dp_f;
}

static double *
hourly_access_dp_std(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->dp_std;
}

static double *
hourly_access_rh(void *sm)
{
    struct Hourly *hrly = sm;
    return &hrly->rh;
}

static void
extract_hourly_values_for_column(GTree *hrs, NBMData const *nbm, char const *col_name,
                                 Converter convert, Creator create_new, Extractor extract)
{
    NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator for %s", col_name);

    time_t init_time = nbm_data_init_time(nbm);

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    double age = difftime(*view.valid_time, init_time) / 3600.0;
    while (view.valid_time && view.value && age < MAX_LEAD_TIME_HRS) {

        if (age >= 0.0) {

            void *hrly = g_tree_lookup(hrs, view.valid_time);
            if (!hrly) {
                time_t *key = malloc(sizeof(time_t));
                *key = *view.valid_time;
                hrly = create_new();
                g_tree_insert(hrs, key, hrly);
            }
            double *sum_val = extract(hrly);
            *sum_val = convert(*view.value);
        }

        // update loop variables
        view = nbm_data_row_iterator_next(it);
        if (view.valid_time) {
            age = difftime(*view.valid_time, init_time) / 3600.0;
        } else {
            break;
        }
    }

    nbm_data_row_iterator_free(&it);
}

static void
extract_max_winds_to_summary(GTree *hrs, NBMData const *nbm)
{
    NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
    Stopif(!it, exit(EXIT_FAILURE), "error creating wind iterator.");

    time_t init_time = nbm_data_init_time(nbm);

    struct NBMDataRowIteratorWindValueView view = nbm_data_row_wind_iterator_next(it);
    double age = difftime(*view.valid_time, init_time) / 3600.0;
    while (view.valid_time && age < MAX_LEAD_TIME_HRS) {

        if (age >= 0.0) {

            struct Hourly *hrly = g_tree_lookup(hrs, view.valid_time);
            if (!hrly) {
                time_t *key = malloc(sizeof(time_t));
                *key = *view.valid_time;
                hrly = (struct Hourly *)hourly_new();
                g_tree_insert(hrs, key, hrly);
            }
            double wind_mph = mps_to_mph(*view.wspd);
            double wind_std = mps_to_mph(*view.wspd_std);
            double wind_gust = mps_to_mph(*view.gust);
            double wind_gust_std = mps_to_mph(*view.gust_std);
            double wind_dir = *view.wdir;

            if (isnan(hrly->wind_spd)) {
                hrly->wind_spd = wind_mph;
                hrly->wind_spd_sd = wind_std;
                hrly->wind_gust = wind_gust;
                hrly->wind_gust_sd = wind_gust_std;
                hrly->wind_dir = wind_dir;
            }
        }

        // update loop variables
        view = nbm_data_row_wind_iterator_next(it);
        age = difftime(*view.valid_time, init_time) / 3600.0;
    }

    nbm_data_row_wind_iterator_free(&it);
}

/** Build a sorted list (\c Tree) of hourly data from an \c NBMData object. */
static GTree *
build_hourlies(NBMData const *nbm)
{
    GTree *hrs = g_tree_new_full(time_t_compare_func, 0, free, free);

    extract_hourly_values_for_column(hrs, nbm, "TMP_2 m above ground", kelvin_to_fahrenheit,
                                     hourly_new, hourly_access_t);

    extract_hourly_values_for_column(hrs, nbm, "TMP_2 m above ground_ens std dev",
                                     change_in_kelvin_to_change_in_fahrenheit, hourly_new,
                                     hourly_access_t_std);

    extract_hourly_values_for_column(hrs, nbm, "DPT_2 m above ground", kelvin_to_fahrenheit,
                                     hourly_new, hourly_access_dp);

    extract_hourly_values_for_column(hrs, nbm, "DPT_2 m above ground_ens std dev",
                                     change_in_kelvin_to_change_in_fahrenheit, hourly_new,
                                     hourly_access_dp_std);

    extract_hourly_values_for_column(hrs, nbm, "RH_2 m above ground", id_func, hourly_new,
                                     hourly_access_rh);

    extract_hourly_values_for_column(hrs, nbm, "TCDC_surface", id_func, hourly_new,
                                     hourly_access_sky);

    extract_hourly_values_for_column(hrs, nbm, "APCP1hr_surface_prob >0.254", id_func, hourly_new,
                                     hourly_access_pop);

    extract_hourly_values_for_column(hrs, nbm, "APCP1hr_surface", id_func, hourly_new,
                                     hourly_access_qpf_1hr);

    extract_hourly_values_for_column(hrs, nbm, "TSTM1hr_surface_probability forecast", id_func,
                                     hourly_new, hourly_access_prob_ltg);

    extract_hourly_values_for_column(hrs, nbm, "CAPE_surface", id_func, hourly_new,
                                     hourly_access_cape);

    extract_hourly_values_for_column(hrs, nbm, "SNOWLR_surface", id_func, hourly_new,
                                     hourly_access_slr);

    extract_hourly_values_for_column(hrs, nbm, "ASNOW1hr_surface", m_to_in, hourly_new,
                                     hourly_access_snow);

    extract_max_winds_to_summary(hrs, nbm);

    return hrs;
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
    sprintf(title_buf, "Hourly data for %s (%s) - ", nbm_data_site_name(nbm),
            nbm_data_site_id(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static int
add_row_to_table(void *key, void *value, void *state)
{
    time_t *vt = key;
    struct Hourly *hrly = value;
    struct TableFillerState *tbl_state = state;
    Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    if (hourly_not_printable(hrly))
        return false;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d %H ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);
    table_set_avg_std(tbl, 1, row, hrly->t_f, hrly->t_std);
    table_set_avg_std(tbl, 2, row, hrly->dp_f, hrly->dp_std);
    table_set_value(tbl, 3, row, hrly->rh);
    table_set_value(tbl, 4, row, hrly->wind_dir);
    table_set_avg_std(tbl, 5, row, hrly->wind_spd, hrly->wind_spd_sd);
    table_set_avg_std(tbl, 6, row, hrly->wind_gust, hrly->wind_gust_sd);
    table_set_value(tbl, 7, row, hrly->sky);
    table_set_value(tbl, 8, row, hrly->pop);
    table_set_value(tbl, 9, row, hrly->qpf_1hr);
    table_set_value(tbl, 10, row, hrly->cape);
    table_set_value(tbl, 11, row, hrly->prob_ltg);
    table_set_value(tbl, 12, row, hrly->slr);
    table_set_value(tbl, 13, row, hrly->snow);

    tbl_state->row++;

    return false;
}

/*-------------------------------------------------------------------------------------------------
 *                                    External API functions.
 *-----------------------------------------------------------------------------------------------*/
void
show_hourly(NBMData const *nbm)
{

    GTree *hrs = build_hourlies(nbm);

    // --
    Table *tbl = table_new(14, g_tree_nnodes(hrs));

    build_title(nbm, tbl);

    // clang-format off
    table_add_column(tbl,  0, Table_ColumnType_TEXT,      "Valid Time (Z)",  "%s",                  20);
    table_add_column(tbl,  1, Table_ColumnType_AVG_STDEV, "T (F)",           " %3.0lf° ±%4.1lf ",   12);
    table_add_column(tbl,  2, Table_ColumnType_AVG_STDEV, "DP (F)",          " %3.0lf° ±%4.1lf ",   12);
    table_add_column(tbl,  3, Table_ColumnType_VALUE,     "RH (%)",          " %3.0lf%% ",           7);
    table_add_column(tbl,  4, Table_ColumnType_VALUE,     "Dir",             " %3.0lf ",             5);
    table_add_column(tbl,  5, Table_ColumnType_AVG_STDEV, "Spd (mph)",       " %3.0lf ±%2.0lf ",     9);
    table_add_column(tbl,  6, Table_ColumnType_AVG_STDEV, "Gust",            " %3.0lf ±%2.0lf ",     9);
    table_add_column(tbl,  7, Table_ColumnType_VALUE,     "Sky",             " %3.0lf%% ",           6);
    table_add_column(tbl,  8, Table_ColumnType_VALUE,     "PoP",             " %3.0lf%% ",           6);
    table_add_column(tbl,  9, Table_ColumnType_VALUE,     "Precip",          " %3.2lf ",             6);
    table_add_column(tbl, 10, Table_ColumnType_VALUE,     "CAPE",            " %4.0lf ",             6);
    table_add_column(tbl, 11, Table_ColumnType_VALUE,     "Ltg (%)",         " %3.0lf%% ",           7);
    table_add_column(tbl, 12, Table_ColumnType_VALUE,     "SLR",             " %3.0lf ",             5);
    table_add_column(tbl, 13, Table_ColumnType_VALUE,     "Snow",            " %4.1lf ",             6);
    // clang-format on

    table_set_double_left_border(tbl, 1);
    table_set_double_left_border(tbl, 4);
    table_set_double_left_border(tbl, 7);
    table_set_double_left_border(tbl, 10);
    table_set_double_left_border(tbl, 12);

    table_set_blank_value(tbl, 7, 0.0);
    table_set_blank_value(tbl, 8, 0.0);
    table_set_blank_value(tbl, 9, 0.0);
    table_set_blank_value(tbl, 10, 0.0);
    table_set_blank_value(tbl, 11, 0.0);
    table_set_blank_value(tbl, 13, 0.0);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(hrs, add_row_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);

    g_tree_unref(hrs);
}
