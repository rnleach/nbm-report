#pragma once

#include "nbm_data.h"

/**
 * Print a summary of the probability of reaching certain precipitation amounts.
 *
 * \param nbm the data to summarize.
 * \param accum_hours is the accumulation period of the precipitation. It must be 24, 48, or 72.
 */
void show_precip_summary(NBMData const *nbm, char const *name, char const *id, time_t init_time,
                         int accum_hours);

/**
 * Print a summary of the preciptation scenarios.
 *
 * \param nbm the data to summarize.
 * \param accum_hours is the accumulation period of the preciptation. It must be 24, 48, or 72.
 */
void show_precip_scenarios(NBMData const *nbm, char const *name, char const *id, time_t init_time,
                           int accum_hours);
