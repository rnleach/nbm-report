#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include <sqlite3.h>

#include "download_cache.h"
#include "utils.h"

static struct ByteBuffer
compress_text_buffer(struct TextBuffer const in_text[static 1])
{
    assert(in_text);

    struct ByteBuffer out_buf = byte_buffer_with_capacity(in_text->size + 2);

    int z_ret = Z_OK;
    z_stream strm = {.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL};

    z_ret = deflateInit(&strm, 9);
    Stopif(z_ret != Z_OK, exit(EXIT_FAILURE), "zlib deflate init error.");

    strm.avail_in = in_text->size;
    strm.next_in = in_text->byte_data;

    do {
        int out_start = byte_buffer_remaining_capacity(&out_buf);
        strm.avail_out = out_start;
        strm.next_out = byte_buffer_next_write_pos(&out_buf);
        z_ret = deflate(&strm, Z_FINISH);
        Stopif(z_ret == Z_STREAM_ERROR, exit(EXIT_FAILURE), "zlib stream clobbered");

        byte_buffer_increase_size(&out_buf, out_start - strm.avail_out);

        if (strm.avail_out == 0) {
            byte_buffer_set_capacity(&out_buf, strm.avail_in + 256);
        }

    } while (strm.avail_out == 0);

    deflateEnd(&strm);

    return out_buf;
}

static struct TextBuffer
uncompress_text(int in_size, unsigned char in[in_size])
{
    assert(in);

    struct TextBuffer out_buf = text_buffer_with_capacity(in_size * 10);

    int z_ret = Z_OK;
    z_stream strm = {
        .zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL, .avail_in = 0, .next_in = Z_NULL};

    z_ret = inflateInit(&strm);
    Stopif(z_ret != Z_OK, exit(EXIT_FAILURE), "zlib inflate init error.");

    strm.avail_in = in_size;
    strm.next_in = in;

    do {

        int out_start = out_buf.capacity - out_buf.size;
        strm.avail_out = out_start;
        strm.next_out = &out_buf.byte_data[out_buf.size];
        z_ret = inflate(&strm, Z_FINISH);

        switch (z_ret) {
        case Z_NEED_DICT:  // fall through
        case Z_DATA_ERROR: // fall through
        case Z_MEM_ERROR:  // fall through
            Stopif(true, exit(EXIT_FAILURE), "zlib error inflating.");
        }

        out_buf.size += out_start - strm.avail_out;

        if (strm.avail_out == 0) {
            text_buffer_set_capacity(&out_buf, out_buf.capacity + strm.avail_in * 6);
        }

    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    return out_buf;
}

static const char *
get_or_create_cache_path()
{
    static char path[64] = {0};

    char const *home = getenv("HOME");
    Stopif(!home, exit(EXIT_FAILURE), "could not find user's home directory.");

    sprintf(path, "%s/.local/", home);

    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0774);
    }

    sprintf(path, "%s/.local/share/", home);
    if (stat(path, &st) == -1) {
        mkdir(path, 0774);
    }

    sprintf(path, "%s/.local/share/nbm-report/", home);
    if (stat(path, &st) == -1) {
        mkdir(path, 0774);
    }

    sprintf(path, "%s/.local/share/nbm-report/cache.sqlite", home);

    return path;
}

/** Global handle to the cache. */
static sqlite3 *cache = 0;

void
download_cache_initialize()
{
    char const *path = get_or_create_cache_path();
    int result = sqlite3_open(path, &cache);
    Stopif(result != SQLITE_OK, exit(EXIT_FAILURE), "unable to open download cache.");

    char *sql = "CREATE TABLE IF NOT EXISTS nbm (   \n"
                "  site      TEXT    NOT NULL,      \n"
                "  init_time INTEGER NOT NULL,      \n"
                "  data      BLOB,                  \n"
                "  PRIMARY KEY (site, init_time));  \n";

    sqlite3_stmt *statement = 0;

    int rc = sqlite3_prepare_v2(cache, sql, -1, &statement, 0);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error preparing cache initialization sql: %s",
           sqlite3_errstr(rc));

    rc = sqlite3_step(statement);
    Stopif(rc != SQLITE_DONE, exit(EXIT_FAILURE), "error executing cache initialization sql.");

    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing cache initialization sql.");
}

void
download_cache_finalize()
{
    time_t now = time(0);
    time_t too_old = now - 60 * 60 * 24 * 555; // About 555 days. That's over 1.5 years!

    char *sql = "DELETE FROM nbm WHERE init_time < ?";

    sqlite3_stmt *statement = 0;
    int rc = sqlite3_prepare_v2(cache, sql, -1, &statement, 0);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error preparing delete statement: %s",
           sqlite3_errstr(rc));

    rc = sqlite3_bind_int64(statement, 1, too_old);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error binding init_time in delete.");

    rc = sqlite3_step(statement);
    Stopif(rc != SQLITE_ROW && rc != SQLITE_DONE, exit(EXIT_FAILURE),
           "error executing select sql: %s", sqlite3_errstr(rc));

    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing delete statement");

    int result = sqlite3_close(cache);

    if (result != SQLITE_OK) {
        fprintf(stderr, "ERROR CLOSING DOWNLOAD CACHE\n");
        fprintf(stderr, "sqlite3 message: %s\n", sqlite3_errmsg(cache));
    }
}

char *
download_cache_retrieve(char const *site, time_t init_time)
{
    assert(site);

    char const *sql = "SELECT data FROM nbm WHERE site = ? AND init_time = ?";

    sqlite3_stmt *statement = 0;
    int rc = sqlite3_prepare_v2(cache, sql, -1, &statement, 0);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error preparing select statement: %s",
           sqlite3_errstr(rc));

    rc = sqlite3_bind_text(statement, 1, site, -1, 0);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error binding site in select.");

    rc = sqlite3_bind_int64(statement, 2, init_time);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error binding init_time in select.");

    rc = sqlite3_step(statement);
    Stopif(rc != SQLITE_ROW && rc != SQLITE_DONE, goto ERR_RETURN, "error executing select sql: %s",
           sqlite3_errstr(rc));

    if (rc == SQLITE_DONE) { // nothing retrieved
        goto ERR_RETURN;     // not really an error, but the cleanup at this point is the same.
    }

    int col_type = sqlite3_column_type(statement, 0);
    Stopif(col_type == SQLITE_NULL, goto ERR_RETURN, "null data retrieved from cache");
    Stopif(col_type != SQLITE_BLOB, goto ERR_RETURN, "invalid data type in cache");

    unsigned char const *blob_data = sqlite3_column_blob(statement, 0);
    int blob_size = sqlite3_column_bytes(statement, 0);

    struct TextBuffer out_buf = uncompress_text(blob_size, (unsigned char *)blob_data);

    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing select statement");

    return text_buffer_steal_text(&out_buf);

ERR_RETURN:

    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing select statement");

    return 0;
}

int
download_cache_add(char const *site, time_t init_time, struct TextBuffer const buf[static 1])
{
    assert(site);
    assert(buf);

    struct ByteBuffer compressed_buf = compress_text_buffer(buf);

    char const *sql = "INSERT OR REPLACE INTO nbm (site, init_time, data) VALUES (?, ?, ?)";

    sqlite3_stmt *statement = 0;
    int rc = sqlite3_prepare_v2(cache, sql, -1, &statement, 0);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error preparing insert statement: %s",
           sqlite3_errstr(rc));

    rc = sqlite3_bind_text(statement, 1, site, -1, 0);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error binding site.");

    rc = sqlite3_bind_int64(statement, 2, init_time);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error binding init_time.");

    rc = sqlite3_bind_blob(statement, 3, compressed_buf.data, compressed_buf.size, 0);
    Stopif(rc != SQLITE_OK, goto ERR_RETURN, "error binding compressed data.");

    rc = sqlite3_step(statement);
    Stopif(rc != SQLITE_DONE, goto ERR_RETURN, "error executing insert sql");

    byte_buffer_clear(&compressed_buf);

    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing insert statement");

    return 0;

ERR_RETURN:
    rc = sqlite3_finalize(statement);
    Stopif(rc != SQLITE_OK, exit(EXIT_FAILURE), "error finalizing insert sql.");

    byte_buffer_clear(&compressed_buf);

    return -1;
}
