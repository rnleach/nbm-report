#pragma once

#include "nbm_data.h"

/** A temperature summary. */
typedef struct TempSum TempSum;

/** Generate a \c TempSum from NBM data.
 *
 * \param nbm the data to summarize.
 */
TempSum *temp_sum_build(NBMData const *nbm);

/** Print a probabilistic summary. */
void show_temp_summary(TempSum *tsum);

/** Print a summary of temperatures scenarios. */
void show_temp_scenarios(TempSum *tsum);

/** Save two files with the CDF and PDF information in them.
 *
 * These files could be useful for plotting the PDF/CDF in gnuplot or another plotting program.
 *
 * \param tsum is the summary to save data for.
 * \param directory is the path to the directory to save the data in. The directory must exist.
 * \param file_prefix will prefix the file names with this string. If this is \c NULL, then
 * no prefix is added.
 */
void temp_sum_save(TempSum *tsum, char const *directory, char const *file_prefix);

/** Free the memory associated with a \c TempSum.*/
void temp_sum_free(TempSum **tsum);
