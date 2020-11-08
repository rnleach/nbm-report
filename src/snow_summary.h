#pragma once

#include "nbm_data.h"

/**
 * Print a summary of the probability of reaching certain snow amounts.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the snow fall.
 */
void show_snow_summary(NBMData const *nbm, int hours);

/**
 * Print a summary of the snow scenarios.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the snow fall.
 */
void show_snow_scenarios(NBMData const *nbm, int hours);
