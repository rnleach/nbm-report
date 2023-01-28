#pragma once

#include "nbm_data.h"

/** A Wind Speed Summary. */
typedef struct WindSum WindSum;

/** Generate a \c WindSum from NBM data.
 *
 * \param nbm the data to summarize.
 */
WindSum *wind_sum_build(NBMData const *nbm);

/** Print a probabilistic summary. */
void show_wind_summary(WindSum const *wsum);

/** Print a summary of the wind scenarios. */
void show_wind_scenarios(WindSum *wsum);

/** Save two files with the CDF and PDF information in them.
 *
 * These files could be useful for plotting the PDF/CDF in gnuplot or another plotting program.
 *
 * \param wsum is the summary to save data for.
 * \param directory is the path to the directory to save the data in. The directory must exist.
 * \param file_prefix will prefix the file names with this string. If this is \c NULL, then
 * no prefix is added.
 */
void wind_sum_save(WindSum *wsum, char const *directory, char const *file_prefix);

/** Free the memory associated with a \c WindSum.*/
void wind_sum_free(WindSum **wsum);
