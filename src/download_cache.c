#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sqlite3.h>

#include "download_cache.h"
#include "utils.h"

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

    // TODO - build cache table layout.
}

void
download_cache_finalize()
{
    // TODO - remove too old entries.

    int result = sqlite3_close(cache);

    if (result != SQLITE_OK) {
        fprintf(stderr, "ERROR CLOSING DOWNLOAD CACHE\n");
        fprintf(stderr, "sqlite3 message: %s\n", sqlite3_errmsg(cache));
    }
}

char *
download_cache_retrieve(char const *site, time_t init_time)
{
    // TODO - implement.
    return 0;
}

int
download_cache_add(char const *site, time_t init_time, char const *text_data)
{
    // TODO - implement
    return -1;
}
