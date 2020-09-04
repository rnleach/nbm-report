#ifndef _NBM_REPORT_DOWNLOAD_H_
#define _NBM_REPORT_DOWNLOAD_H_

#include <stdlib.h>
#include <time.h>

/**
 * The downloaded data and some meta data about it.
 *
 * This structure may contain null pointers.
 */
struct RawNbmData;

/**
 * Free all allocated data in a RawNbmData, and set the pointer to null.
 */
void free_raw_nbm_data(struct RawNbmData **data);

/**
 * Get the init time of the NBM run for this data.
 */
struct tm raw_nbm_data_init_time(struct RawNbmData *);

/**
 * Get the site name for this data.
 */
char const *raw_nbm_data_site(struct RawNbmData *);

/**
 * Get the raw text data.
 */
char *raw_nbm_data_text(struct RawNbmData *);

/**
 * Get the size of the raw text data in characters.
 */
size_t raw_nbm_data_text_len(struct RawNbmData *);

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

#endif
