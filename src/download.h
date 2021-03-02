#pragma once

#include <time.h>

#include "raw_nbm_data.h"
#include "utils.h"

/** Download a file from the online archive.
 *
 * This is used by other routines to download files from the archive server.
 *
 * \param file_name - the name of the file on the server.
 * \param init_time - the NBM initialization time, so the routine knows where to go to get the file.
 *
 * \returns a \c TextBuffer. If there was an error downloading, the buffer will be empty.
 */
struct TextBuffer download_file(char const file_name[static 1], time_t init_time);

/** Retrieve the CSV data for a site.
 *
 * \param site is the name of the site you want to download data for.
 * \param file_name is the name of the file on the server.
 * \param init_time is the NBM initialization time you want from the online archive.
 *
 * returns \c RawNbmData that you are responsible for freeing with \c free_raw_nbm_data().
 */
RawNbmData *retrieve_data_for_site(char const site[static 1], char const site_nm[static 1],
                                   char const file_name[static 1], time_t init_time);

/** Initialize all the components of the download module.
 *
 * Connect to the cache. If the cache doesn't exist, create it first and then connect.
 *
 * This should be run during program initialization.
 */
void download_module_initialize();

/** Shutdown the download cache.
 *
 * Clean up any old, stale data and close the connection to the cache.
 * Clean up cURL.
 *
 * This function should be run during program shutdown after the requested data has been displayed.
 */
void download_module_finalize();
