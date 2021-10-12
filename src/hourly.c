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
struct Hourly {
    double t_f;
    double t_std;
};

static bool
hourly_not_printable(struct Hourly const *hrly)
{
    return isnan(hrly->t_f);
}

static void *
hourly_new()
{
    struct Hourly *new = malloc(sizeof(struct Hourly));
    assert(new);

    *new = (struct Hourly){
        .t_f = NAN,
        .t_std = NAN,
    };

    return (void *)new;
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

/*
// Most values can be considered in isolation, or one column at a time, winds are the exception. So
// winds get their own extractor function.
static void
extract_max_winds_to_summary(GTree *hrs, NBMData const *nbm)
{
    NBMDataRowIteratorWind *it = nbm_data_rows_wind(nbm);
    Stopif(!it, exit(EXIT_FAILURE), "error creating wind iterator.");

    struct NBMDataRowIteratorWindValueView view = nbm_data_row_wind_iterator_next(it);
    while (view.valid_time) { // If valid time is good, assume everything is.

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

        if (isnan(sum->max_wind_mph) || max_wind_mph > sum->max_wind_mph) {
            sum->max_wind_mph = max_wind_mph;
            sum->max_wind_std = max_wind_std;
            sum->max_wind_dir = max_wind_dir;
        }

        if (isnan(sum->max_wind_gust) || max_wind_gust > sum->max_wind_gust) {
            sum->max_wind_gust = max_wind_gust;
            sum->max_wind_gust_std = max_wind_gust_std;
        }

        view = nbm_data_row_wind_iterator_next(it);
    }

    nbm_data_row_wind_iterator_free(&it);
}
*/

static void
extract_hourly_values_for_column(GTree *hrs, NBMData const *nbm, char const *col_name,
                                 Converter convert, Creator create_new, Extractor extract)
{
    NBMDataRowIterator *it = nbm_data_rows(nbm, col_name);
    Stopif(!it, exit(EXIT_FAILURE), "error creating iterator for %s", col_name);

    struct NBMDataRowIteratorValueView view = nbm_data_row_iterator_next(it);
    while (view.valid_time && view.value) {

        void *hrly = g_tree_lookup(hrs, view.valid_time);
        if (!hrly) {
            time_t *key = malloc(sizeof(time_t));
            *key = *view.valid_time;
            hrly = create_new();
            g_tree_insert(hrs, key, hrly);
        }
        double *sum_val = extract(hrly);
        *sum_val = convert(*view.value);

        view = nbm_data_row_iterator_next(it);
    }

    nbm_data_row_iterator_free(&it);
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
    Table *tbl = table_new(2, g_tree_nnodes(hrs));

    build_title(nbm, tbl);

    // clang-format off
    table_add_column(tbl, 0, Table_ColumnType_TEXT,      "Day/Date",  "%s",                  20);
    table_add_column(tbl, 1, Table_ColumnType_AVG_STDEV, "T (F)",     " %3.0lf° ±%4.1lf ",   12);
    // clang-format on

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(hrs, add_row_to_table, &state);

    table_display(tbl, stdout);

    table_free(&tbl);

    g_tree_unref(hrs);
}
