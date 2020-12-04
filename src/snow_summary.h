#pragma once

#include "nbm.h"

/** A snow summary. */
typedef struct SnowSum SnowSum;

/** Generate a \c SnowSum from NBM data.
 *
 * \param nbm the data to summarize.
 * \param accum_hours is the accumulation period of the snow. It must be 6, 24, 48, or 72.
 */
SnowSum *snow_sum_build(NBMData const *nbm, int accum_hours);

/** Print a probabilistic summary. */
void show_snow_summary(SnowSum const *ssum);

/** Print a summary of the preciptation scenarios. */
void show_snow_scenarios(SnowSum *ssum);

/** Save two files with the CDF and PDF information in them.
 *
 * These files could be useful for plotting the PDF/CDF in gnuplot or another plotting program.
 *
 * \param ssum is the summary to save data for.
 * \param directory is the path to the directory to save the data in. The directory must exist.
 * \param file_prefix will prefix the file names with this string. If this is \c NULL, then
 * no prefix is added.
 */
void snow_sum_save(SnowSum *ssum, char const *directory, char const *file_prefix);

/** Free the memory associated with a \c SnowSum.*/
void snow_sum_free(SnowSum **ssum);
