#pragma once

#include "nbm_data.h"

/**
 * Print a summary of the probability of reaching certain snow amounts.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the snow fall. It must be 24, 48, or 72.
 */
void show_snow_summary(struct NBMData const *nbm, int hours);