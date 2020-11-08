#pragma once

#include "nbm_data.h"

/**
 * Print a summary of the probability of reaching certain precipitation amounts.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the precipitation. It must be 24, 48, or 72.
 */
void show_precip_summary(NBMData const *nbm, int hours);

/**
 * Print a summary of the preciptation scenarios.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the preciptation. It must be 24, 48, or 72.
 */
void show_precip_scenarios(NBMData const *nbm, int hours);
