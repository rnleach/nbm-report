#include "parser.h"
#include "utils.h"

#include <assert.h>
#include <csv.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*-------------------------------------------------------------------------------------------------
 *                                           NBMData
 *-----------------------------------------------------------------------------------------------*/
// The main way to create one of these is via parse_raw_nbm_data(), which is at the bottom of the
// file.
struct NBMData {
    char *site;
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

    free(ptr->site);
    for (int i = 0; i < ptr->num_cols; i++) {
        free(ptr->col_names[i]);
    }
    free(ptr->col_names);
    free(ptr->valid_times);
    free(ptr->vals);

    free(ptr);

    *ptrptr = 0;
    return;
}

/*-------------------------------------------------------------------------------------------------
 *                                     NBMDataRowIterator
 *-----------------------------------------------------------------------------------------------*/
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
    Stopif(found_col_num < 0, return 0, "No such column: %s", col_name);

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
 *                                           CSVParserState - internal only
 *-----------------------------------------------------------------------------------------------*/
struct CSVParserState {
    int row;
    int col;

    // Never freed in this module, this is the ultimate return value from parse_raw_nbm_data()
    struct NBMData *nbm_data;
};

static struct CSVParserState
init_parser_state(char *text_data, struct tm init_time, char const *site)
{
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

    char *site_name = calloc(strlen(site) + 1, sizeof(char));
    strcpy(site_name, site);

    struct NBMData *nbm = malloc(sizeof(struct NBMData));
    *nbm = (struct NBMData){.site = site_name,
                            .init_time = timegm(&init_time),
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
            val = strtod(txt, 0);
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
do_parsing(struct csv_parser *parser, struct RawNbmData *raw)
{
    size_t data_len = raw_nbm_data_text_len(raw);
    char *raw_text = raw_nbm_data_text(raw);
    assert(raw_text);

    struct tm init_time_tm = raw_nbm_data_init_time(raw);
    char const *site = raw_nbm_data_site(raw);

    struct CSVParserState state = init_parser_state(raw_text, init_time_tm, site);

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
parse_raw_nbm_data(struct RawNbmData *raw)
{
    struct csv_parser parser = initialize_a_csv_parser();

    struct NBMData *nbm_data = do_parsing(&parser, raw);
    assert(nbm_data);

    csv_free(&parser);

    return nbm_data;
}
