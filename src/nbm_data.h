#pragma once

#include "download.h"

typedef struct NBMData NBMData;
typedef struct NBMDataRowIterator NBMDataRowIterator;
typedef struct NBMDataRowIteratorWind NBMDataRowIteratorWind;

struct NBMDataRowIteratorValueView {
    time_t *valid_time;
    double *value;
};

struct NBMDataRowIteratorWindValueView {
    time_t *valid_time;
    double *wspd;
    double *wspd_std;
    double *wdir;
    double *gust;
    double *gust_std;
};

/** Parse the raw text data into a \c NBMData structure.
 *
 * The resulting object will need to be freed with \c nbm_data_free().
 **/
NBMData *parse_raw_nbm_data(RawNbmData *);

/** Free memory associated with an \c NBMData object, and nullify the pointer. */
void nbm_data_free(NBMData **ptrptr);

/** Get the age of the forecast in seconds. */
double nbm_data_age(NBMData const *);

/** Get the site this data is for. */
char const *nbm_data_site_id(NBMData const *);

/** Get the site name this data is for. */
char const *nbm_data_site_name(NBMData const *);

/** Get the initialization time of this model run. */
time_t nbm_data_init_time(NBMData const *);

/** Get an iterator over a column.
 *
 * \param nbm the NBM data to query.
 * \param col_name The name of the column.
 *
 * \returns an iterator over the rows.
 */
NBMDataRowIterator *nbm_data_rows(NBMData const *nbm, char const *col_name);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_iterator_free(NBMDataRowIterator **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the both of the pointers in the \c NBMDataRowIteratorValueView
 * object are set to null.
 */
struct NBMDataRowIteratorValueView nbm_data_row_iterator_next(NBMDataRowIterator *);

/** Get an iterator over the winds.
 *
 * \param nbm the NBM data to query.
 *
 * \returns an iterator over the rows of wind data.
 */
NBMDataRowIteratorWind *nbm_data_rows_wind(NBMData const *nbm);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_wind_iterator_free(NBMDataRowIteratorWind **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the all of the pointers in the \c NBMDataRowIteratorWindValueView
 * object are set to null.
 */
struct NBMDataRowIteratorWindValueView nbm_data_row_wind_iterator_next(NBMDataRowIteratorWind *);
