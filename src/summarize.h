#include "nbm_data.h"

#include <stdbool.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                  Helper functions for the various extractor functions
 *-----------------------------------------------------------------------------------------------*/

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

/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

/** Compare \c time_t values which are used as keys in the GLib \c Tree.
 *
 * Dates are sorted in ascending order.
 */
int time_t_compare_func(void const *a, void const *b, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                  Extract values from the NBM and insert them into the summary.
 *-----------------------------------------------------------------------------------------------*/

/** Filter values out for consideration. */
typedef bool (*KeepFilter)(time_t const *);

/** Extract a single value per day from the provided column.
 *
 * This is intended to be used on deterministic columns (e.g. precipitation amount) and not a
 * probability column when there are several columns to describe the cumulative distribution
 * function of an element.
 *
 * \param sums a GLib tree where the keys are \c time_t types and the values are of the type of
 * summary you want to populate.
 * \param nbm is the parsed NBMData.
 * \param col_name is the column name from the NBM text file to extract data from.
 * \param filter determines whether to use a value from the NBM based on its valid time.
 * \param date_sum rounds a \c time_t to a single value that is representative of the day. These
 * are used as keys for sorting in the tree.
 * \param convert is a simple mapping. It may do nothing or map units of mm to in or some other
 * relavent conversion.
 * \param accumulate is a stateless function that takes an accumulator variable and the next value
 * to "accumulate". Examples can be a sum, first, last, max, min, etc. By convention the value NaN
 * signals that nothing has yet been added to the accumulator.
 * \param create is a function that creates a new summary of whatever type so that it can be added
 * to the \c GTree.
 * \param extractor is a function that retrieves a reference to the value that will eventually be
 * passed to \c accumulate as the accumulation variable.
 */
void extract_daily_summary_for_column(GTree *sums, struct NBMData const *nbm, char const *col_name,
                                      KeepFilter filter, SummarizeDate date_sum, Converter convert,
                                      Accumulator accumulate, Creator create, Extractor extract);

/*-------------------------------------------------------------------------------------------------
 *                                  Cumulative Distributions
 *-----------------------------------------------------------------------------------------------*/
/** A cumulative distribution function. (CDF) */
typedef struct CumulativeDistribution CumulativeDistribution;

/** Extract CDF information from some \c NBMData.
 *
 * \param nbm is the source to extract the CDF from.
 * \param cdf_col_name_format is a \c printf style format string used to generate the column names
 * of the columns that contain the CDF information.
 * \param pm_col_name is the column name of the probability matched value.
 * \param convert is a simple mapping. It may do nothing or map units of mm to in or some other
 * relavent conversion.
 *
 * \returns a \c GTree* with \c time_t objects as keys and \c CumulativeDistribution
 * objects as values.
 *
 **/
GTree *extract_cdfs(struct NBMData const *nbm, char const *cdf_col_name_format,
                    char const *pm_col_name, Converter convert);

/** Get the probability matched (or quantile mapped) value associated with CDF.
 *
 * If there was no PM (QMD) value, returns \c NAN.
 */
double cumulative_dist_pm_value(CumulativeDistribution const *);

/** Get a probability of exceedence for a given value.
 *
 * \param cdf is the function you want to sample.
 * \param target_val is the value you want to get the probability of exceedence for.
 *
 * \returns the probability of matching or exceeding the \c target_val.
 */
double interpolate_prob_of_exceedance(CumulativeDistribution *cdf, double target_val);

/** Get the value from a CDF at specific percentile.
 *
 * \param cdf is the function you want to sample.
 * \param target_percentile is the percentile you want to get the value for.
 *
 * \returns the value at the percentile, or NAN if it was out of the possible range.
 */
double cumulative_dist_percentile_value(CumulativeDistribution *cdf, double target_percentile);

/** Free a CDF object. */
void cumulative_dist_free(void *);

/*-------------------------------------------------------------------------------------------------
 *                                  Probability Distributions
 *-----------------------------------------------------------------------------------------------*/
/** A probability distribution function. */
typedef struct ProbabilityDistribution ProbabilityDistribution;

/** Create a \c ProbabilityDistribution from a \c CumulativeDistribution.
 *
 * \param cdf the \c CumulativeDistribution to use as the source of this PDF.
 * \param abs_min is the absolute minimum value considered in the PDF. For many variables, like
 * precipitation and snow, could be zero.
 * \param abs_max is the absolute maximum value considered in the PDF. The CDFs may not actually
 * include the max and min values in them, so we use these parameters to set hard limits. If there
 * is data in the distribution, it will be clipped to this level.
 * \param smooth_radius is a size scale parameter for guassian smoothing of the PDF when
 * calculating the modes. If this is too small the data will be too noisy and find to many
 * modes that are too close together.
 */
ProbabilityDistribution *probability_dist_calc(CumulativeDistribution *cdf, double abs_min,
                                               double abs_max, double smooth_radius);

/** Get the number of modes (local maxima) in a \c ProbabilityDistribution. */
int probability_dist_num_modes(ProbabilityDistribution *pdf);

/** Get the value of the mode.
 *
 * If the distribution has more than one mode, then get the \c mode_num mode. \c mode_num 1 is the
 * mode with the largest probabiltiy in the PDF, and the next modes arrive in decreasing order. If
 * there is no \c mode_num mode, then \c NAN is returned.
 */
double probability_dist_get_mode(ProbabilityDistribution *pdf, int mode_num);

/** Get the weight, or probability value associated with a num.
 *
 * \see \c probability_dist_get_mode() for an explanation of \c mode_num.
 */
double probability_dist_get_mode_weight(ProbabilityDistribution *pdf, int mode_num);

/** Free resources associated with a \c ProbabilityDistribution */
void probability_dist_free(void *);
/*-------------------------------------------------------------------------------------------------
 *                                 KeepFilter implementations.
 *-----------------------------------------------------------------------------------------------*/
bool keep_all(time_t const *vt);

bool keep_aft(time_t const *vt);

bool keep_mrn(time_t const *vt);

bool keep_eve(time_t const *vt);

bool keep_night(time_t const *vt);

bool keep_00z(time_t const *vt);
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
