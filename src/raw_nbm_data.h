#pragma once

#include <time.h>

/**
 * The raw NBM data in text format.
 *
 * This structure can be populated by loading from a file, a database, or downloading from the
 * internet.
 */
typedef struct RawNbmData RawNbmData;

/** Create a new RawNbmData object.
 *
 * \param init_time is the run time or model initialization time of this NBM data.
 * \param site is the identifier of this location.
 * \param name is the name of this location.
 * \param text is the raw, unparsed text.
 * \param text_len is the length of the \c text array.
 *
 * \returns a new allocated RawNbmData object that needs to be freed with \c raw_nbm_data_free.
 **/
RawNbmData *raw_nbm_data_new(time_t init_time, char *site, char *name, char *text, size_t text_len);

/**
 * Free all allocated data in a RawNbmData, and set the pointer to null.
 */
void raw_nbm_data_free(RawNbmData **data);

/**
 * Get the init time of the NBM run for this data.
 */
time_t raw_nbm_data_init_time(RawNbmData const *);

/**
 * Get the site id for this data.
 */
char const *raw_nbm_data_site_id(RawNbmData const *);

/**
 * Get the site name for this data.
 */
char const *raw_nbm_data_site_name(RawNbmData const *);

/**
 * Get the raw text data.
 *
 * This method allows for the raw text to be modified in place.
 */
char *raw_nbm_data_text(RawNbmData *);

/**
 * Get the size of the raw text data in characters.
 */
size_t raw_nbm_data_text_len(RawNbmData *);
