#pragma once

#include "nbm.h"
#include "raw_nbm_data.h"

/*
 * Most functions in this module are declared in nbm.h as they are part of the public API.
 */

/** Parse the raw text data into a \c NBMData structure.
 *
 * The resulting object will need to be freed with \c nbm_data_free().
 **/
NBMData *parse_raw_nbm_data(RawNbmData *);
