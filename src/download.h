#pragma once

#include "raw_nbm_data.h"

/**
 * Retrieve the CSV data for a site.
 *
 * This function will retrieve the latest available NBM run data for the given site.
 *
 * \param site is the name of the site you want to download data for.
 *
 * \returns the contents of the downloaded CSV file as a string. If there is an error and it cannot
 * retrieve the data, it returns the null pointer. The returned struct is allocated, and you will
 * responsible for freeing it with \c free_raw_nbm_data().
 */
struct RawNbmData *retrieve_data_for_site(char const site[static 1]);
