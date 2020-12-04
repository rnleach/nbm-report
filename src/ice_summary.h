#pragma once

#include "nbm.h"

/**
 * Print a summary of the probability of reaching certain ice amounts.
 *
 * \param nbm the data to summarize.
 * \param hours is the accumulation period of the ice accumulation.
 */
void show_ice_summary(NBMData const *nbm, int hours);
