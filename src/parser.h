
#ifndef _NBM_REPORT_PARSER_H_
#define _NBM_REPORT_PARSER_H_

#include "download.h"

struct NBMData;

/**
 * Parse the raw text data into a \c NBMData structure.
 */
struct NBMData *parse_raw_nbm_data(struct RawNbmData *);

/**
 * Free memory associated with an \c NBMData object, and nullify the pointer.
 */
void free_nbm_data(struct NBMData **ptrptr);

#endif
