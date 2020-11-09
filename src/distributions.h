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
