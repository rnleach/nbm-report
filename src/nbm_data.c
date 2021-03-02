#include "nbm_data.h"
#include "utils.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <csv.h>

/*-------------------------------------------------------------------------------------------------
 *                                           NBMData
 *-----------------------------------------------------------------------------------------------*/
/** Internal implementation of \ref NBMData.
 *
 * The main way to create one of these is via parse_raw_nbm_data().
 */
struct NBMData {
    char *site_id;
    char *site_name;
    time_t init_time;

    size_t num_cols;
    size_t num_rows;

    char **col_names;

    time_t *valid_times;
    double *vals;
};

void
nbm_data_free(struct NBMData **ptrptr)
{
    struct NBMData *ptr = *ptrptr;

    if (ptr) {

        free(ptr->site_id);
        free(ptr->site_name);
        for (int i = 0; i < ptr->num_cols; i++) {
            free(ptr->col_names[i]);
        }
        free(ptr->col_names);
        free(ptr->valid_times);
        free(ptr->vals);

        free(ptr);

        *ptrptr = 0;
    }

    return;
}

double
nbm_data_age(struct NBMData const *ptr)
{
    time_t now = time(0);
    return difftime(now, ptr->init_time);
}

char const *const
nbm_data_site_id(struct NBMData const *ptr)
{
    return ptr->site_id;
}

char const *const
nbm_data_site_name(struct NBMData const *ptr)
{
    return ptr->site_name;
}

time_t
nbm_data_init_time(struct NBMData const *ptr)
{
    return ptr->init_time;
}
/*-------------------------------------------------------------------------------------------------
 *                                     NBMDataRowIterator
 *-----------------------------------------------------------------------------------------------*/
/** Internal implementation of a row iterator. */
struct NBMDataRowIterator {
    int col_num;
    size_t curr_row;
    struct NBMData const *src;
};

struct NBMDataRowIterator *
nbm_data_rows(struct NBMData const *nbm, char const *col_name)
{
    int found_col_num = -1;
    for (int col_num = 0; col_num < nbm->num_cols; col_num++) {
        if (strcmp(col_name, nbm->col_names[col_num]) == 0) {
            found_col_num = col_num;
            break;
        }
    }

    if (found_col_num < 0) {
        return 0;
    }

    struct NBMDataRowIterator *it = malloc(sizeof(struct NBMDataRowIterator));
    *it = (struct NBMDataRowIterator){.curr_row = 0, .col_num = found_col_num, .src = nbm};
    return it;
}

void
nbm_data_row_iterator_free(struct NBMDataRowIterator **ptrptr)
{
    free(*ptrptr);
    *ptrptr = 0;
}

struct NBMDataRowIteratorValueView
nbm_data_row_iterator_next(struct NBMDataRowIterator *it)
{
    struct NBMDataRowIteratorValueView view = {0};

    size_t next_row = it->curr_row;
    double *vals = it->src->vals;
    double *val = 0;
    while (next_row < it->src->num_rows) {
        size_t index = next_row * it->src->num_cols + it->col_num;
        val = &vals[index];
        if (!isnan(*val)) {
            break;
        }
        next_row++;
    }

    if (next_row < it->src->num_rows) {
        view.value = val;
        view.valid_time = &it->src->valid_times[next_row];
        it->curr_row = next_row + 1;
    }

    return view;
}

/*-------------------------------------------------------------------------------------------------
 *                                   NBMDataRowIteratorWind
 *-----------------------------------------------------------------------------------------------*/
/** Internal implementation of a row iterator for wind, which has direction and gusts plus speed.*/
struct NBMDataRowIteratorWind {
    int wspd_col_num;
    int wspd_std_col_num;
    int wgst_col_num;
    int wgst_std_col_num;
    int wdir_col_num;

    size_t curr_row;
    struct NBMData const *src;
};

struct NBMDataRowIteratorWind *
nbm_data_rows_wind(struct NBMData const *nbm)
{
    int found_wspd_col_num = -1;
    int found_wspd_std_col_num = -1;
    int found_wgst_col_num = -1;
    int found_wgst_std_col_num = -1;
    int found_wdir_col_num = -1;
    for (int col_num = 0; col_num < nbm->num_cols; col_num++) {
        if (strcmp("WIND_10 m above ground", nbm->col_names[col_num]) == 0) {
            found_wspd_col_num = col_num;
        }

        if (strcmp("WIND_10 m above ground_ens std dev", nbm->col_names[col_num]) == 0) {
            found_wspd_std_col_num = col_num;
        }

        if (strcmp("GUST_10 m above ground", nbm->col_names[col_num]) == 0) {
            found_wgst_col_num = col_num;
        }

        if (strcmp("GUST_10 m above ground_ens std dev", nbm->col_names[col_num]) == 0) {
            found_wgst_std_col_num = col_num;
        }

        if (strcmp("WDIR_10 m above ground", nbm->col_names[col_num]) == 0) {
            found_wdir_col_num = col_num;
        }
    }
    Stopif(found_wdir_col_num < 0 || found_wspd_col_num < 0 || found_wgst_col_num < 0 ||
               found_wspd_std_col_num < 0 || found_wgst_std_col_num < 0,
           return 0, "Missing wind column.");

    struct NBMDataRowIteratorWind *it = malloc(sizeof(struct NBMDataRowIteratorWind));
    *it = (struct NBMDataRowIteratorWind){.curr_row = 0,
                                          .src = nbm,
                                          .wspd_col_num = found_wspd_col_num,
                                          .wgst_col_num = found_wgst_col_num,
                                          .wspd_std_col_num = found_wspd_std_col_num,
                                          .wgst_std_col_num = found_wgst_std_col_num,
                                          .wdir_col_num = found_wdir_col_num};

    return it;
}

void
nbm_data_row_wind_iterator_free(struct NBMDataRowIteratorWind **ptrptr)
{
    free(*ptrptr);
    *ptrptr = 0;
}

struct NBMDataRowIteratorWindValueView
nbm_data_row_wind_iterator_next(struct NBMDataRowIteratorWind *it)
{
    struct NBMDataRowIteratorWindValueView view = {0};

    size_t next_row = it->curr_row;
    double *vals = it->src->vals;
    double *spd_val = 0;
    double *spd_std_val = 0;
    double *gst_val = 0;
    double *gst_std_val = 0;
    double *dir_val = 0;
    while (next_row < it->src->num_rows) {
        size_t row_offset = next_row * it->src->num_cols;
        size_t index_spd = row_offset + it->wspd_col_num;
        size_t index_spd_std = row_offset + it->wspd_std_col_num;
        size_t index_gst = row_offset + it->wgst_col_num;
        size_t index_gst_std = row_offset + it->wgst_std_col_num;
        size_t index_dir = row_offset + it->wdir_col_num;
        spd_val = &vals[index_spd];
        spd_std_val = &vals[index_spd_std];
        gst_val = &vals[index_gst];
        gst_std_val = &vals[index_gst_std];
        dir_val = &vals[index_dir];
        if (!(isnan(*spd_val) || isnan(*spd_std_val) || isnan(*gst_val) || isnan(*gst_std_val) ||
              isnan(*dir_val))) {
            break;
        }
        next_row++;
    }

    if (next_row < it->src->num_rows) {
        view.valid_time = &it->src->valid_times[next_row];
        view.wspd = spd_val;
        view.wspd_std = spd_std_val;
        view.wdir = dir_val;
        view.gust = gst_val;
        view.gust_std = gst_std_val;
        it->curr_row = next_row + 1;
    }

    return view;
}
/*-------------------------------------------------------------------------------------------------
 *                                           CSVParserState - internal only
 *-----------------------------------------------------------------------------------------------*/
/** State for keeping track parsing CSV files. */
struct CSVParserState {
    int row;
    int col;

    // Never freed in this module, this is the ultimate return value from parse_raw_nbm_data()
    struct NBMData *nbm_data;
};

static struct CSVParserState
init_parser_state(char *text_data, time_t init_time, char const site_id[static 1],
                  char const site_name[static 1])
{
    assert(site_id);
    assert(site_name);

    // Count the rows and columns
    size_t rows = 0;
    size_t cols = 0;
    for (char *ch = text_data; *ch; ch++) {
        if (*ch == '\n')
            rows++;
        if (*ch == ',' && rows == 0)
            cols++;
    }
    cols++;

    char *site_nm = calloc(strlen(site_name) + 1, sizeof(char));
    strcpy(site_nm, site_name);

    char *id = calloc(strlen(site_id) + 1, sizeof(char));
    strcpy(id, site_id);

    struct NBMData *nbm = malloc(sizeof(struct NBMData));
    *nbm = (struct NBMData){.site_name = site_nm,
                            .site_id = id,
                            .init_time = init_time,
                            .num_cols = cols - 1, // First column is stored in .valid_times
                            .num_rows = rows - 1, // First row is stored in .col_names
                            .col_names = calloc(cols - 1, sizeof(char *)),
                            .valid_times = calloc(rows, sizeof(time_t)),
                            .vals = calloc((rows - 1) * (cols - 1), sizeof(double))};

    return (struct CSVParserState){.row = 0, .col = 0, .nbm_data = nbm};
}

static void
parse_column_header(char *col_name, size_t col_name_len, struct NBMData *nbm, size_t col)
{
    assert(col > 0);
    nbm->col_names[col - 1] = calloc(col_name_len + 1, sizeof(char));
    strncpy(nbm->col_names[col - 1], col_name, col_name_len);
}

static void
column_callback(void *data, size_t sz, void *state)
{
    char *txt = data;
    struct CSVParserState *st = state;
    struct NBMData *nbm = st->nbm_data;

    if (st->row == 0) {
        // Parse the column titles.
        if (st->col > 0) {
            parse_column_header(txt, sz, nbm, st->col);
        }
    } else if (st->col == 0) {
        // Handle valid time column
        struct tm vtime = {.tm_isdst = -1};
        strptime(txt, "%Y%m%d%H", &vtime);
        nbm->valid_times[st->row - 1] = timegm(&vtime);
    } else {
        // Handle all the other columns as doubles.
        double val;
        if (sz == 0) {
            val = NAN;
        } else {
#define MAX_CHARS_CELL 100
            char buf[MAX_CHARS_CELL] = {0};
            assert(sz < MAX_CHARS_CELL);
            memcpy(buf, data, sz);
            if (strstr(buf, "9.999e+20")) {
                val = NAN;
            } else {
                val = strtod(buf, 0);
            }
        }

        nbm->vals[nbm->num_cols * (st->row - 1) + (st->col - 1)] = val;
    }

    st->col++;
    return;
}

static void
row_callback(int _unused, void *state)
{
    struct CSVParserState *st = state;
    st->row++;
    st->col = 0;
    return;
}

static struct csv_parser
initialize_a_csv_parser()
{
    struct csv_parser parser = {0};
    int err = csv_init(&parser, 0);
    Stopif(err, exit(EXIT_FAILURE), "error initializing libcsv: (%d) %s", err, csv_strerror(err));
    return parser;
}

static struct NBMData *
do_parsing(struct csv_parser *parser, RawNbmData *raw)
{
    size_t data_len = raw_nbm_data_text_len(raw);
    char *raw_text = raw_nbm_data_text(raw);
    assert(raw_text);

    time_t init_time_tm = raw_nbm_data_init_time(raw);
    char const *site = raw_nbm_data_site_id(raw);
    char const *site_nm = raw_nbm_data_site_name(raw);

    struct CSVParserState state = init_parser_state(raw_text, init_time_tm, site, site_nm);

    size_t parsed_len =
        csv_parse(parser, raw_text, data_len, column_callback, row_callback, &state);
    Stopif(parsed_len != data_len, exit(EXIT_FAILURE), "error parsing csv data: %s",
           csv_strerror(csv_error(parser)));

    int err = csv_fini(parser, column_callback, row_callback, &state);
    Stopif(err, exit(EXIT_FAILURE), "error with csv_fini(): %s", csv_strerror(csv_error(parser)));

    return state.nbm_data;
}

/*-------------------------------------------------------------------------------------------------
 *                             Other module functions in the public API.
 *-----------------------------------------------------------------------------------------------*/
struct NBMData *
parse_raw_nbm_data(RawNbmData *raw)
{
    struct csv_parser parser = initialize_a_csv_parser();

    struct NBMData *nbm_data = do_parsing(&parser, raw);
    assert(nbm_data);

    csv_free(&parser);

    return nbm_data;
}
