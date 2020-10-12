#pragma once
#include "nbm_data.h"

/**
 * Print the quantiles of the temperature forecast.
 *
 * \param nbm the data to summarize.
 */
void show_temperature_summary(struct NBMData const *nbm);
