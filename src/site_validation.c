#include "site_validation.h"

#include <math.h>

#include <csv.h>
#include <glib.h>
#include <sqlite3.h>

#include "download.h"

#define MAX_VERSIONS_TO_ATTEMP_DOWNLOADING 20

#define HOURSEC (60 * 60)

#define FAILURE_MODE_DID_NOT_FAIL 0
#define FAILURE_MODE_NOT_ENOUGH 1
#define FAILURE_MODE_TOO_MANY 2
#define FAILURE_MODE_UNABLE_TO_CONNECT 3

extern bool global_verbose;
/*-------------------------------------------------------------------------------------------------
 *                                      Utility Functions
 *-----------------------------------------------------------------------------------------------*/
/** Given a starting time, calculate the most recent NBM initialization time.
 *
 * This currently assumes we are only interested in the 1, 7, 13, or 19Z NBM runs.
 */
static time_t
calc_most_recent_init_time(time_t starting_time)
{
    struct tm working_time = *gmtime(&starting_time);

    // Force time to be on an hour by truncating minutes and seconds.
    working_time.tm_min = 0;
    working_time.tm_sec = 0;

    time_t result = timegm(&working_time);

    int hour = working_time.tm_hour;
    assert(hour >= 0 && hour < 24);

    int shift_secs = 0;
    if (hour >= 19) {
        shift_secs = (hour - 19) * HOURSEC;
    } else if (hour >= 13) {
        shift_secs = (hour - 13) * HOURSEC;
    } else if (hour >= 7) {
        shift_secs = (hour - 7) * HOURSEC;
    } else if (hour >= 1) {
        shift_secs = (hour - 1) * HOURSEC;
    } else {
        // hour == 0 - so use 24
        shift_secs = (24 - 19) * HOURSEC;
    }

    return result - shift_secs;
}

struct LocationsCSV {
    struct TextBuffer buf;
    time_t init_time;
};

/** Search back in time from \c request_time and retrieve the most recent locations.csv file. */
static struct LocationsCSV
get_locations_csv_file(time_t request_time)
{
    time_t init_time = request_time;
    int num_attempts_left = MAX_VERSIONS_TO_ATTEMP_DOWNLOADING;
    struct TextBuffer buf = text_buffer_with_capacity(0);
    while (num_attempts_left) {
        init_time = calc_most_recent_init_time(init_time);

        buf = download_file("locations.csv", init_time);
        if (!text_buffer_is_empty(buf)) {
            break;
        }

        init_time -= HOURSEC;
        num_attempts_left--;
    }

    return (struct LocationsCSV){.buf = buf, .init_time = init_time};
}

/*-------------------------------------------------------------------------------------------------
 *                                        MatchedSitesRecord
 *-----------------------------------------------------------------------------------------------*/
struct MatchedSitesRecord {
    char *file_name;
    char *id;
    char *name;
    char *state;
    double lat;
    double lon;
};

void
matched_sites_record_free(void *data)
{
    struct MatchedSitesRecord *rec = data;

    free(rec->file_name);
    free(rec->id);
    free(rec->name);
    free(rec->state);
    free(rec);
}

/** Callback to print all the strings in a \c GSList via g_slist_foreach. */
static void
print_list(void *data, void *unused)
{
    struct MatchedSitesRecord *rec = data;
    char namebuf[256] = {0};

    sprintf(namebuf, "%s, %s", rec->name, rec->state);
    printf("%-30s %6.3lf %8.3lf %s\n", namebuf, rec->lat, rec->lon, rec->id);
}

/*-------------------------------------------------------------------------------------------------
 *                           site_validation_create helper functions
 *-----------------------------------------------------------------------------------------------*/
static sqlite3 *
create_locations_database()
{
    sqlite3 *db = 0;
    int sqlite_res = sqlite3_open_v2(":memory:", &db, SQLITE_OPEN_READWRITE, 0);
    Stopif(sqlite_res != SQLITE_OK, goto ERR_RETURN, "Unable to open :memory: db: %s",
           sqlite3_errstr(sqlite_res));

    sqlite_res = sqlite3_exec(db,
                              "CREATE TABLE locations (                \n"
                              "   id    TEXT NOT NULL,                 \n"
                              "   name  TEXT NOT NULL,                 \n"
                              "   state TEXT NOT NULL,                 \n"
                              "   lat   REAL NOT NULL,                 \n"
                              "   lon   REAL NOT NULL,                 \n"
                              "   PRIMARY KEY (id) ON CONFLICT IGNORE) \n",
                              0, 0, 0);

    Stopif(sqlite_res != SQLITE_OK, goto ERR_RETURN, "Unable to create table in :memory: db: %s",
           sqlite3_errstr(sqlite_res));

    return db;

ERR_RETURN:
    sqlite_res = sqlite3_close(db);
    Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE),
           "Error closing :memory: db after another error: %s", sqlite3_errstr(sqlite_res));
    return 0;
}

#define MAX_FIELD_LEN 128
struct CSVState {
    sqlite3_stmt *stmt;
    int col;
    char id[MAX_FIELD_LEN];
    char name[MAX_FIELD_LEN];
    char state[MAX_FIELD_LEN];
    float lat;
    float lon;
    bool invalid_record;
};

static void
process_col(void *data, size_t num_bytes, void *state)
{
    char *text = data;
    struct CSVState *st = state;

    if (text && *text) {
        switch (st->col) {
        case 0:
            assert(num_bytes + 1 < MAX_FIELD_LEN);
            strcpy(st->id, text);
            break;
        case 1:
            assert(num_bytes + 1 < MAX_FIELD_LEN);
            strcpy(st->name, text);
            break;
        case 2:
            assert(num_bytes + 1 < MAX_FIELD_LEN);
            strcpy(st->state, text);
            break;
        case 3:
            st->lat = atof(text);
            break;
        case 4:
            st->lon = atof(text);
            break;
        default:
            assert(false);
        }
    } else {
        st->invalid_record = true;
    }

    st->col++;
}

static void
process_row(int unused, void *state)
{
    struct CSVState *st = state;

    if (st->invalid_record) {
        if (global_verbose) {
            fprintf(stderr, "\nInvalid record encountered in locations.csv\n");
            fprintf(stderr, "\"%s\" \"%s\" \"%s\" \"%lf\" \"%lf\"\n", st->id, st->name, st->state,
                    st->lat, st->lon);
        }
    } else {

        sqlite3_stmt *stmt = st->stmt;

        int sqlite_res = sqlite3_bind_text(stmt, 1, st->id, -1, 0);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite bind error: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_bind_text(stmt, 2, st->name, -1, 0);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite bind error: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_bind_text(stmt, 3, st->state, -1, 0);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite bind error: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_bind_double(stmt, 4, st->lat);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite bind error: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_bind_double(stmt, 5, st->lon);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite bind error: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_step(stmt);
        Stopif(sqlite_res != SQLITE_DONE, exit(EXIT_FAILURE), "sqlite not done: %s",
               sqlite3_errstr(sqlite_res));

        sqlite_res = sqlite3_reset(stmt);
        Stopif(sqlite_res != SQLITE_OK, exit(EXIT_FAILURE), "sqlite unable to reset: %s",
               sqlite3_errstr(sqlite_res));
    }

    // Reset the state for the next row.
    memset(st->id, 0, MAX_FIELD_LEN);
    memset(st->name, 0, MAX_FIELD_LEN);
    memset(st->state, 0, MAX_FIELD_LEN);
    st->lat = NAN;
    st->lon = NAN;
    st->col = 0;
    st->invalid_record = false;
}

static sqlite3 *
build_locations_database(struct TextBuffer *buf)
{

    bool csv_initialized = false;
    sqlite3_stmt *stmt = 0;

    sqlite3 *db = create_locations_database();
    Stopif(!db, return 0, "Unable to create locations database.");

    int sqlite_res = sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
    Stopif(sqlite_res != SQLITE_OK, goto ERR_RETURN, "error starting transaction: %s",
           sqlite3_errstr(sqlite_res));

    sqlite_res = sqlite3_prepare_v2(
        db, "INSERT INTO locations (id, name, state, lat, lon) VALUES (?, ?, ?, ?, ?)", -1, &stmt,
        0);

    struct CSVState csv_state = {.stmt = stmt,
                                 .col = 0,
                                 .id = {0},
                                 .name = {0},
                                 .state = {0},
                                 .lat = NAN,
                                 .lon = NAN,
                                 .invalid_record = false};

    struct csv_parser p = {0};

    csv_initialized = csv_init(&p, CSV_APPEND_NULL | CSV_EMPTY_IS_NULL) == 0;
    Stopif(!csv_initialized, goto ERR_RETURN, "error initializing csv");

    int csv_bytes = csv_parse(&p, buf->text_data, buf->size, process_col, process_row, &csv_state);
    Stopif(csv_bytes != buf->size, goto ERR_RETURN, "error parsing locations csv");

    int csv_fini_err = csv_fini(&p, process_col, process_row, &csv_state);
    Stopif(csv_fini_err != 0, goto ERR_RETURN, "error with csv fini");

    csv_free(&p);

    sqlite_res = sqlite3_finalize(stmt);
    stmt = 0;
    Stopif(sqlite_res != SQLITE_OK, goto ERR_RETURN, "error finalizing: %s",
           sqlite3_errstr(sqlite_res));

    sqlite_res = sqlite3_exec(db, "COMMIT TRANSACTION", 0, 0, 0);
    Stopif(sqlite_res != SQLITE_OK, goto ERR_RETURN, "error committing transaction: %s",
           sqlite3_errstr(sqlite_res));

    return db;

ERR_RETURN:

    if (csv_initialized)
        csv_free(&p);
    if (stmt)
        sqlite3_finalize(stmt);

    return 0;
}

static void
destroy_locations_database(sqlite3 *db)
{
    int sqlite_res = sqlite3_close(db);
    if (sqlite_res != SQLITE_OK) {
        fprintf(stderr, "Error closing :memory: db after done using it: %s",
                sqlite3_errstr(sqlite_res));
    }
}

static struct MatchedSitesRecord *
create_record_from_row(sqlite3_stmt *stmt)
{
    char *id = 0;
    char *name = 0;
    char *state = 0;
    char *file_name = 0;

    asprintf(&id, "%s", sqlite3_column_text(stmt, 0));
    asprintf(&file_name, "%s.csv", id);
    asprintf(&name, "%s", sqlite3_column_text(stmt, 1));
    asprintf(&state, "%s", sqlite3_column_text(stmt, 2));
    double lat = sqlite3_column_double(stmt, 3);
    double lon = sqlite3_column_double(stmt, 4);

    struct MatchedSitesRecord *rec = malloc(sizeof(struct MatchedSitesRecord));

    *rec = (struct MatchedSitesRecord){
        .file_name = file_name, .id = id, .name = name, .state = state, .lat = lat, .lon = lon};

    return rec;
}

static GSList *
find_exact_case_insensitive_match(sqlite3 *db, char const site[static 1])
{
    GSList *ret = 0;

    char *upper_case_site = 0;
    asprintf(&upper_case_site, "%s", site);
    to_uppercase(upper_case_site);

    char *query = 0;
    asprintf(&query, "SELECT id, name, state, lat, lon FROM locations WHERE id = '%s'",
             upper_case_site);

    sqlite3_stmt *stmt = 0;
    int res = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    Stopif(res != SQLITE_OK, goto ERR_RETURN, "error preparing exact case statement: %s",
           sqlite3_errstr(res));

    res = sqlite3_step(stmt);
    if (res == SQLITE_ROW) {
        ret = g_slist_append(ret, create_record_from_row(stmt));
    }

ERR_RETURN:

    if (stmt) {
        sqlite3_finalize(stmt);
    }

    free(upper_case_site);
    free(query);

    return ret;
}

static GSList *
find_similar_sites(sqlite3 *db, char const site[static 1])
{
    GSList *ret = 0;

    char *query = 0;
    asprintf(&query,
             "SELECT id, name, state, lat, lon "
             "FROM locations                   "
             "WHERE id LIKE '%%%s%%'           "
             "    OR name LIKE '%%%s%%'        "
             "    OR state LIKE '%s'           ",
             site, site, site);

    sqlite3_stmt *stmt = 0;
    int res = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    Stopif(res != SQLITE_OK, goto ERR_RETURN, "error preparing like statement: %s",
           sqlite3_errstr(res));

    res = sqlite3_step(stmt);
    while (res == SQLITE_ROW) {
        ret = g_slist_append(ret, create_record_from_row(stmt));
        res = sqlite3_step(stmt);
    }

ERR_RETURN:

    if (stmt) {
        sqlite3_finalize(stmt);
    }

    free(query);

    return ret;
}

/*-------------------------------------------------------------------------------------------------
 *                                        SiteValidation
 *-----------------------------------------------------------------------------------------------*/
struct SiteValidation {
    time_t init_time;
    GSList *matched_sites;
    bool unable_to_connect;
};

static int
get_failure_mode(struct SiteValidation *validation)
{
    // Failure occurs when there are no matches or more more than 1 match.

    if (validation->unable_to_connect) {
        return FAILURE_MODE_UNABLE_TO_CONNECT;
    }

    if (validation->matched_sites == 0) {
        // In this case there were no matches.
        return FAILURE_MODE_NOT_ENOUGH;
    }

    if (validation->matched_sites->next != 0) {
        // In this case there were more than 1 matched site.
        return FAILURE_MODE_TOO_MANY;
    }

    return FAILURE_MODE_DID_NOT_FAIL;
}

bool
site_validation_failed(struct SiteValidation *validation)
{
    return get_failure_mode(validation) != FAILURE_MODE_DID_NOT_FAIL;
}

void
site_validation_print_failure_message(struct SiteValidation *validation)
{
    switch (get_failure_mode(validation)) {
    case FAILURE_MODE_NOT_ENOUGH:
        printf("\nNo sites matched request.\n");
        break;
    case FAILURE_MODE_TOO_MANY:
        printf("\nAmbiguous site with multiple matches:\n");
        printf("%-30s %-6s %-8s %s\n", "Station Name", "Lat", "Lon", "ID");
        printf("----------------------------------------------------\n");
        g_slist_foreach(validation->matched_sites, print_list, 0);
        break;
    case FAILURE_MODE_UNABLE_TO_CONNECT:
        printf("\nUnable to connect to server for last %d model cycles.\n",
               MAX_VERSIONS_TO_ATTEMP_DOWNLOADING);
        break;
    default:
        assert(false);
    }
}

char const *
site_validation_site_name_alias(struct SiteValidation *validation)
{
    assert(!site_validation_failed(validation));
    struct MatchedSitesRecord *rec = validation->matched_sites->data;

    return rec->name;
}

char const *
site_validation_site_id_alias(struct SiteValidation *validation)
{
    assert(!site_validation_failed(validation));
    struct MatchedSitesRecord *rec = validation->matched_sites->data;

    return rec->id;
}

char const *
site_validation_file_name_alias(struct SiteValidation *validation)
{
    assert(!site_validation_failed(validation));
    struct MatchedSitesRecord *rec = validation->matched_sites->data;

    return rec->file_name;
}

time_t
site_validation_init_time(struct SiteValidation *validation)
{
    assert(!site_validation_failed(validation));
    return validation->init_time;
}

struct SiteValidation *
site_validation_create(char const site[static 1], time_t request_time)
{
    assert(site);

    struct SiteValidation *res = calloc(1, sizeof(struct SiteValidation));
    assert(res);

    struct LocationsCSV locations_info = get_locations_csv_file(request_time);
    struct TextBuffer buf = locations_info.buf;
    time_t init_time = locations_info.init_time;

    if (text_buffer_is_empty(buf)) {
        res->unable_to_connect = true;
        return res;
    }

    sqlite3 *db = build_locations_database(&buf);
    text_buffer_clear(&buf); // We're done with the text.
    assert(db);

    GSList *matches = find_exact_case_insensitive_match(db, site);
    if (!matches) {
        matches = find_similar_sites(db, site);
    }

    destroy_locations_database(db);

    res->init_time = init_time;
    res->matched_sites = matches;
    return res;
}

void
site_validation_free(struct SiteValidation **validation)
{
    struct SiteValidation *ptr = *validation;

    if (ptr) {
        g_slist_free_full(ptr->matched_sites, matched_sites_record_free);
        free(ptr);
        *validation = 0;
    }
}
