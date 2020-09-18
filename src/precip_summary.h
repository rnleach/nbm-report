
#pragma once

#include "nbm_data.h"

/**
 * Print a summary of the probability of reaching certain precipitation levels.
 *
 * \param nbm the data to summarize.
 * \param type must be \c r for rain/liquid equivalent or \c s for snow.
 */
void show_precip_summary(struct NBMData const *nbm, char type);
