#pragma once

#include <time.h>

#include "utils.h"

/** Initialize the cache. */
void download_cache_initialize();

/** Finalize the cache.
 *
 * This will remove entries that are too old and close the sqlite connection.
 */
void download_cache_finalize();

/** Retrieve a site / model initialization time pair stored in the cache, if they exist.
 *
 * \param site the site name.
 * \param init_time is the model initialization time.
 *
 * \returns a pointer to the data or a null pointer if unable to retrieve the data.
 */
char *download_cache_retrieve(char const *site, time_t init_time);

/** Add an entry to the cache.
 *
 * \param site is the site name
 * \param intit_time is the model initialization time.
 * \param buf is the downloaded string data.
 *
 * \returns 0 on success.
 */
int download_cache_add(char const *site, time_t init_time, struct TextBuffer const buf[static 1]);
