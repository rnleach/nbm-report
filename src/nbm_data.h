#pragma once

#include "raw_nbm_data.h"
#include "site_validation.h"

/*-------------------------------------------------------------------------------------------------
 *                                             NBMData
 *-----------------------------------------------------------------------------------------------*/
/** A fully parsed NBM data file. */
typedef struct NBMData NBMData;

/** Parse the raw text data into a \c NBMData structure.
 *
 * The resulting object will need to be freed with \c nbm_data_free().
 **/
NBMData *parse_raw_nbm_data(RawNbmData *);

/** Retrieve the data for a site.
 *
 * \param validation is the result of doing a site validation.
 *
 * returns \c NBMData that you are responsible for freeing with \c nbm_data_free(), or \c NULL if
 * there was an error.
 */
NBMData *retrieve_data(SiteValidation *validation);

/** Free memory associated with an \c NBMData object, and nullify the pointer. */
void nbm_data_free(NBMData **ptrptr);

/** Get the age of the forecast in seconds. */
double nbm_data_age(NBMData const *);

/** Get the site this data is for. */
char const *const nbm_data_site_id(NBMData const *);

/** Get the site name this data is for. */
char const *const nbm_data_site_name(NBMData const *);

/** Get the initialization time of this model run. */
time_t nbm_data_init_time(NBMData const *);

/*-------------------------------------------------------------------------------------------------
 *                           Aquiring and using iterators over NBMData
 *-----------------------------------------------------------------------------------------------*/
// implentations in src/nbm_data.c
/** An iterator over the rows (valid times) of NBM forecasts for an element.
 *
 * This iterator will skip all rows without any data. The units of the output depend on the value
 * requested. For instance, probabilities are always in percent, precipitation is in inches, snow
 * is in inches, temperatures are in Fahrenheit.
 */
typedef struct NBMDataRowIterator NBMDataRowIterator;

/** An iterator over the rows (valid times) of NBM forecasts for a vector element.
 *
 * Currently wind is the only vector element, hence the name.
 *
 * This iterator will skip all rows without any data. Units are degrees for direction and miles per
 * hour for speeds.
 */
typedef struct NBMDataRowIteratorWind NBMDataRowIteratorWind;

/** A view into the current values stored in an iterator.
 *
 * The units of the output depend on the value requested. For instance, probabilities are always in
 * percent, precipitation is in inches, snow is in inches, temperatures are in Fahrenheit.
 */
struct NBMDataRowIteratorValueView {
    time_t *valid_time; /**< Valid time of data on this row. */
    double *value; /**< The semantic meaning depends on the column this value was drawnf rom. */
};

/** A view into the current values stored in a wind iterator.
 *
 * Units are degrees for direction and miles per hour for speeds.
 */
struct NBMDataRowIteratorWindValueView {
    time_t *valid_time; /**< Valid time of data on this row. */
    double *wspd;       /**< The wind speed in mph. */
    double *wspd_std;   /**< The standard deviation of the wind speeds (mph). */
    double *wdir;       /**< The compass direction the wind is coming from. */
    double *gust;       /**< The wind gust in mph. */
    double *gust_std;   /**< The standard deviation of the wind gusts. */
};

/** Get an iterator over a column.
 *
 * \param nbm the NBM data to query.
 * \param col_name The name of the column. To see the available columns, look into one of the raw
 * csv files.
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
