#pragma once

#include "nbm.h"

/** A precipitation Summary. */
typedef struct PrecipSum PrecipSum;

/** Generate a \c PrecipSum from NBM data.
 *
 * \param nbm the data to summarize.
 * \param accum_hours is the accumulation period of the precipitation. It must be 24, 48, or 72.
 */
PrecipSum *precip_sum_build(NBMData const *nbm, int accum_hours);

/** Print a probabilistic summary. */
void show_precip_summary(PrecipSum const *psum);

/** Print a summary of the preciptation scenarios. */
void show_precip_scenarios(PrecipSum *psum);

/** Save two files with the CDF and PDF information in them.
 *
 * These files could be useful for plotting the PDF/CDF in gnuplot or another plotting program.
 *
 * \param psum is the summary to save data for.
 * \param directory is the path to the directory to save the data in. The directory must exist.
 * \param file_prefix will prefix the file names with this string. If this is \c NULL, then
 * no prefix is added.
 */
void precip_sum_save(PrecipSum *psum, char const *directory, char const *file_prefix);

/** Free the memory associated with a \c PrecipSum.*/
void precip_sum_free(PrecipSum **psum);
