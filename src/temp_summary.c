#include "assert.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <glib.h>

#include "table.h"
#include "temp_summary.h"
#include "utils.h"

static void
build_title(struct NBMData const *nbm, struct Table *tbl)
{
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "Temperature Quantiles for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    table_add_title(tbl, strlen(title_buf), title_buf);
}

static GTree *
extract_temperature_data(struct NBMData const *nbm)
{
    assert(false);
}

struct TableFillerState {
    int row;
    struct Table *tbl;
};

static int
add_row_to_table(void *key, void *value, void *state)
{
    assert(false);
#if 0
    time_t *vt = key;
    struct DailySummary *sum = value;
    struct TableFillerState *tbl_state = state;
    struct Table *tbl = tbl_state->tbl;
    int row = tbl_state->row;

    if (daily_summary_not_printable(sum))
        return false;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), " %a, %Y-%m-%d ", gmtime(vt));

    table_set_string_value(tbl, 0, row, strlen(datebuf), datebuf);
    table_set_avg_std(tbl, 1, row, sum->min_t_f, sum->min_t_std);
    table_set_avg_std(tbl, 2, row, sum->max_t_f, sum->max_t_std);
    table_set_value(tbl, 3, row, sum->max_wind_dir);
    table_set_avg_std(tbl, 4, row, sum->max_wind_mph, sum->max_wind_std);
    table_set_avg_std(tbl, 5, row, sum->max_wind_gust, sum->max_wind_gust_std);
    table_set_avg_std(tbl, 6, row, sum->mrn_sky, sum->aft_sky);
    table_set_value(tbl, 7, row, sum->prob_ltg);
    table_set_value(tbl, 8, row, sum->precip);
    table_set_value(tbl, 9, row, sum->snow);

    tbl_state->row++;

#endif
    return false;
}

void
show_temperature_summary(struct NBMData const *nbm)
{
    GTree *temps = extract_temperature_data(nbm);
    Stopif(!temps, return, "Error extracting CDFs for QPF.");

    struct Table *tbl = table_new(13, g_tree_nnodes(temps));
    build_title(nbm, tbl);

    table_add_column(tbl, 0, Table_ColumnType_TEXT, strlen("Day/Date"), "Day/Date", strlen("%s"),
                     "%s", 17);

    table_add_column(tbl, 1, Table_ColumnType_AVG_STDEV, strlen("MinT (F)"), "MinT (F)",
                     strlen(" %3.0lf° ±%4.1lf "), " %3.0lf° ±%4.1lf ", 12);

    table_add_column(tbl, 2, Table_ColumnType_VALUE, strlen("10th"), "10th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 3, Table_ColumnType_VALUE, strlen("25th"), "25th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 4, Table_ColumnType_VALUE, strlen("50th"), "50th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 5, Table_ColumnType_VALUE, strlen("75th"), "75th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 6, Table_ColumnType_VALUE, strlen("90th"), "90th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 7, Table_ColumnType_AVG_STDEV, strlen("MaxT (F)"), "MaxT (F)",
                     strlen(" %3.0lf° ±%4.1lf "), " %3.0lf° ±%4.1lf ", 12);

    table_add_column(tbl, 8, Table_ColumnType_VALUE, strlen("10th"), "10th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 9, Table_ColumnType_VALUE, strlen("25th"), "25th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 10, Table_ColumnType_VALUE, strlen("50th"), "50th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 11, Table_ColumnType_VALUE, strlen("75th"), "75th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    table_add_column(tbl, 12, Table_ColumnType_VALUE, strlen("90th"), "90th", strlen(" %3.0lf "),
                     " %3.0lf ", 5);

    struct TableFillerState state = {.row = 0, .tbl = tbl};
    g_tree_foreach(temps, add_row_to_table, &state);

    table_display(tbl, stdout);

    g_tree_unref(temps);

    table_free(&tbl);
}
