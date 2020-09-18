#include "nbm_data.h"

#include <stdbool.h>
#include <time.h>

#include <glib.h>

/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

/** Create a new summary of a given type as an opaque, typeless pointer. */
typedef void *(Creator)(void);

/** Extract a pointer to the summary value that needs to be updated. */
typedef double *(*Extractor)(void *);

/** Convert all values in a day to a single \c time_t value so they can be associated in a tree.
 *
 * The period defined as a day may vary between weather elements.
 */
typedef time_t (*SummarizeDate)(time_t const *);

/** How to accumulate values from a day, eg take the first, last, sum, max, min. */
typedef double (*Accumulator)(double acc, double val);

/** Filter values out for consideration. */
typedef bool (*KeepFilter)(time_t const *);

/** Compare \c time_t values which are used as keys in the GLib \c Tree.
 *
 * Dates are sorted in ascending order.
 */
int time_t_compare_func(void const *a, void const *b, void *user_data);

void extract_daily_summary_for_column(GTree *sums, struct NBMData const *nbm, char const *col_name,
                                 KeepFilter filter, SummarizeDate date_sum, Converter convert,
                                 Accumulator accumulate, Creator create, Extractor extract);

/*-------------------------------------------------------------------------------------------------
 *                                 KeepFilter implementations.
 *-----------------------------------------------------------------------------------------------*/
bool keep_all(time_t const *vt);

bool keep_aft(time_t const *vt);

bool keep_mrn(time_t const *vt);

/*-------------------------------------------------------------------------------------------------
 *                                 SummarizeDate implementations.
 *-----------------------------------------------------------------------------------------------*/
/** Convert the times from 18Z one day to 18Z the next to a \c struct \c tm valid the first day.
 *
 * For a summary, the day runs from 18Z one day to 18Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for MaxT/MinT/MaxRH/MinRH.
 * */
time_t summary_date_18z(time_t const *valid_time);

/** Convert the times from 12Z one day to 12Z the next to a \c struct \c tm valid the first day.
 *
 * For a summary, the day runs from 12Z one day to 12Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for wind and precip.
 */
time_t summary_date_12z(time_t const *valid_time);

/** Convert the times from 06Z one day to 06Z the next to a \c struct \c tm valid the first day.
 *
 * For a summary, the day runs from 06Z one day to 06Z the next. This works OK for the US, and
 * matches NBM timing anyway. This works for sky cover and any hourly or six hourly NBM component.
 */
time_t summary_date_06z(time_t const *valid_time);

/*-------------------------------------------------------------------------------------------------
 *                                 Accumulator implementations.
 *-----------------------------------------------------------------------------------------------*/

// The iterators should only return 1 value a day for these, so just use that single value.
double accum_daily_rh_t(double acc, double val);

/** Sum all values to get a total. */
double accum_sum(double acc, double val);

/** Keep the maximum value, ignoring NaNs. */
double accum_max(double acc, double val);

/** Just keep the last value. */
double accum_last(double _acc, double val);

/** Average the values. */
double accum_avg(double acc, double val);

