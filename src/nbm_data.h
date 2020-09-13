#pragma once

#include "download.h"

struct NBMData;
struct NBMDataRowIterator;
struct NBMDataRowIteratorWind;

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
struct NBMData *parse_raw_nbm_data(struct RawNbmData *);

/** Free memory associated with an \c NBMData object, and nullify the pointer. */
void nbm_data_free(struct NBMData **ptrptr);

/** Get an iterator over a column.
 *
 * \param nbm the NBM data to query.
 * \param col_name The name of the column.
 *
 * \returns an iterator over the rows.
 */
struct NBMDataRowIterator *nbm_data_rows(struct NBMData const *nbm, char const *col_name);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_iterator_free(struct NBMDataRowIterator **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the both of the pointers in the \c NBMDataRowIteratorValueView
 * object are set to null.
 */
struct NBMDataRowIteratorValueView nbm_data_row_iterator_next(struct NBMDataRowIterator *);

/** Get an iterator over the winds.
 *
 * \param nbm the NBM data to query.
 *
 * \returns an iterator over the rows of wind data.
 */
struct NBMDataRowIteratorWind *nbm_data_rows_wind(struct NBMData const *nbm);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_wind_iterator_free(struct NBMDataRowIteratorWind **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the all of the pointers in the \c NBMDataRowIteratorWindValueView
 * object are set to null.
 */
struct NBMDataRowIteratorWindValueView
nbm_data_row_wind_iterator_next(struct NBMDataRowIteratorWind *);

