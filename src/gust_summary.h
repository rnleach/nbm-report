#pragma once

#include "nbm_data.h"

/** A Wind Gust Summary. */
typedef struct GustSum GustSum;

/** Generate a \c GustSum from NBM data.
 *
 * \param nbm the data to summarize.
 */
GustSum *gust_sum_build(NBMData const *nbm);

/** Print a probabilistic summary. */
void show_gust_summary(GustSum const *gsum);

/** Print a summary of the wind gust scenarios. */
void show_gust_scenarios(GustSum *gsum);

/** Save two files with the CDF and PDF information in them.
 *
 * These files could be useful for plotting the PDF/CDF in gnuplot or another plotting program.
 *
 * \param gsum is the summary to save data for.
 * \param directory is the path to the directory to save the data in. The directory must exist.
 * \param file_prefix will prefix the file names with this string. If this is \c NULL, then
 * no prefix is added.
 */
void gust_sum_save(GustSum *gsum, char const *directory, char const *file_prefix);

/** Free the memory associated with a \c GustSum.*/
void gust_sum_free(GustSum **gsum);
