#pragma once

#include <glib.h>

#include "nbm_data.h"
#include "utils.h"

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
GTree *extract_cdfs(NBMData const *nbm, char const *cdf_col_name_format, char const *pm_col_name,
                    Converter convert);

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

/** Get the maximum value in the \c CumulativeDistribution.
 *
 * Note that this may not be 100th percentile if the data only went up to the 90th or 99th as is
 * the case with our data.
 */
double cumulative_dist_max_value(CumulativeDistribution *cdf);

/** Get the minimum value in the \c CumulativeDistribution.
 *
 * Note that this may not be 0th percentile if the data started at the 1st percentile, as is often
 * the case with our data.
 */
double cumulative_dist_min_value(CumulativeDistribution *cdf);

/** Free a CDF object. */
void cumulative_dist_free(void *);

/** Write a cumulative distribution to a file. */
void cumulative_dist_write(CumulativeDistribution *cdf, FILE *f);

/*-------------------------------------------------------------------------------------------------
 *                                  Probability Distributions
 *-----------------------------------------------------------------------------------------------*/
/** A probability distribution function. */
typedef struct ProbabilityDistribution ProbabilityDistribution;

/** Create a \c ProbabilityDistribution from a \c CumulativeDistribution.
 *
 * \param cdf the \c CumulativeDistribution to use as the source of this PDF.
 * \param smooth_radius is a size scale parameter for guassian smoothing of the PDF when
 * calculating the modes. If this is too small the data will be too noisy and find to many
 * modes that are too close together.
 */
ProbabilityDistribution *probability_dist_calc(CumulativeDistribution *cdf, double smooth_radius);

/** Free resources associated with a \c ProbabilityDistribution */
void probability_dist_free(void *);

/** Write a probability distribution to a file. */
void probability_dist_write(ProbabilityDistribution *pdf, FILE *f);

/*-------------------------------------------------------------------------------------------------
 *                                          Scenarios
 *-----------------------------------------------------------------------------------------------*/
/** A scenario */
typedef struct Scenario Scenario;

/** Free memeory associated with a scenario. */
void scenario_free(void *ptr);

/** Get the value with the highest probability density in this scenario. */
double scenario_get_mode(Scenario const *sc);

/** Get the lowest value in this scenario. */
double scenario_get_minimum(Scenario const *sc);

/** Get the highest value in this scenario. */
double scenario_get_maximum(Scenario const *sc);

/** Get the probability associated with this scenario. */
double scenario_get_probability(Scenario const *sc);

/** Analyze a probability distribution and provide a list of scenarios.
 *
 * \returns a list of scenarios sorted from highest to lowest probability.
 */
GList *find_scenarios(ProbabilityDistribution const *pdf);
