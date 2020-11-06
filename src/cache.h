#pragma once

#include <time.h>

#include "utils.h"

/** Initialize the cache. */
void cache_initialize();

/** Finalize the cache.
 *
 * This will remove entries that are too old and close the sqlite connection.
 */
void cache_finalize();

/** Retrieve a text file associated with an NBM model time from the cache.
 *
 * \param file is the name of the file without the extension. Usually this is just the site name,
 *             but it could also be some relavent metadata like the locations.
 * \param init_time is the model initialization time.
 *
 * \returns a TextBuffer with the text data.
 */
struct TextBuffer cache_retrieve(char const *file, time_t init_time);

/** Add an entry to the cache.
 *
 * \param file is the name of the file without the extension. Usually this is just the site name,
 *             but it could also be some relavent metadata like the locations.
 * \param intit_time is the model initialization time.
 * \param buf is the downloaded string data.
 *
 * \returns 0 on success.
 */
int cache_add(char const *site, time_t init_time, struct TextBuffer const buf[static 1]);
