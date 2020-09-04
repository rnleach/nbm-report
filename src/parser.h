
#ifndef _NBM_REPORT_PARSER_H_
#define _NBM_REPORT_PARSER_H_

#include "download.h"

struct NBMData;
struct NBMDataRowIterator;

struct NBMDataRowIteratorValueView {
    time_t *valid_time;
    double *value;
};

/** Parse the raw text data into a \c NBMData structure. */
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

#endif
