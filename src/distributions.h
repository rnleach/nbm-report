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
 * of the columns that contain the CDF information in the form of percentiles.
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

/** Extract probability threshold data from the NBM.
 *
 * This is related to the extract_cdfs() function. That function uses percentiles information to
 * build the CDF. This function extracts the probability of exceedence columns from the NBM to add
 * data to the CDF. This function will take the GTree created by extract_cdfs() and add more points
 * to the CDF based on the probabilities of exceedence.
 *
 * \param tree is a GTree as returned by extract_cdfs().
 * \param nbm is the source to extract the CDF data from.
 * \param col_name_format is a \c printf style format string used to generate the column names
 * of the columns that contain the CDF information in the form of probability of exceedance.
 * \param num_vals is the number of values in the array pointed to by the next argument.
 * \param vals is an array of the values which you want to try and extract probability of exceedance
 * for. Note these values are in the same units that the source NBM data file is in. They must also
 * be strings that exactly match the number of decimal places as the column name in the NBM and be
 * parseable by \c strtod().
 * \param convert is a simple mapping. It may do nothing or map units of mm to in or some other
 * relavent conversion. It is used to map the values in \a vals into the units you want them to be
 * in before adding them to the CDF.
 *
 * \returns a \c GTree* with \c time_t objects as keys and \c CumulativeDistribution
 * objects as values. This is the same tree that was passed in as the \a tree argument.
 */
GTree *extract_exceedence_to_cdfs(GTree *tree, NBMData const *nbm, char const *col_name_format,
                                  size_t num_vals, char const *const vals[num_vals],
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

/** Create a ProbabilityDistribution from a CumulativeDistribution.
 *
 * \param cdf the CumulativeDistribution to use as the source of this PDF.
 */
ProbabilityDistribution *probability_dist_calc(CumulativeDistribution *cdf);

/** Create a copy of a PDF. */
ProbabilityDistribution *probability_dist_copy(ProbabilityDistribution *src);

/** Free resources associated with a ProbabilityDistribution */
void probability_dist_free(void *);

/** Write a probability distribution to a file. */
void probability_dist_write(ProbabilityDistribution *pdf, FILE *f);

/*-------------------------------------------------------------------------------------------------
 *                                          Scenarios
 *-----------------------------------------------------------------------------------------------*/
/** A scenario */
typedef struct Scenario Scenario;

/** Free memory associated with a scenario. */
void scenario_free(void *ptr);

/** Get the value with the highest probability density in this scenario. */
double scenario_get_mode(Scenario const *sc);

/** Get the lowest value in this scenario. */
double scenario_get_minimum(Scenario const *sc);

/** Get the highest value in this scenario. */
double scenario_get_maximum(Scenario const *sc);

/** Get the probability associated with this scenario. */
double scenario_get_probability(Scenario const *sc);

/** Analyze a of probability distribution and provide a list of scenarios.
 *
 * \param pdf the ProbabilityDistribution to operate on.
 * \param minimum_smooth_radius is a minimum size scale parameter for Guassian smoothing of the PDF
 * when calculating the scenarios.
 * \param smooth_radius_inc is an increment that \a minimum_smooth_radius is increased by if too
 * many scenarios are found. This algorithm keeps increasing the smooth radius until at most 4
 * scenarios are found.
 *
 * \returns a GList of scenarios sorted from highest to lowest probability.
 */
GList *find_scenarios(ProbabilityDistribution *pdf, double minimum_smooth_radius,
                      double smooth_radius_inc);

/** Analyze a collection of probability distributions and provide scenarios.
 *
 * Applies find_scenarios() to each value in a GTree to produce a GTree of scenario lists.
 *
 * \param pdfs the ProbabilityDistribution objects to operate on.
 * \param minimum_smooth_radius is a minimum size scale parameter for Guassian smoothing of the PDF
 * when calculating the scenarios.
 * \param smooth_radius_inc is an increment that \a minimum_smooth_radius is increased by if too
 * many scenarios are found. This algorithm keeps increasing the smooth radius until at most 4
 * scenarios are found.
 *
 * \returns a GTree of scenario lists. Each list is sorted from highest to lowest probability. The
 * keys to the GTree are the valid time as \c time_t objects.
 */
GTree *create_scenarios_from_pdfs(GTree *pdfs, double minimum_smooth_radius,
                                  double smooth_radius_inc);

/*-------------------------------------------------------------------------------------------------
 *                  Utility functions for working with collections of distributions
 *-----------------------------------------------------------------------------------------------*/
/** Free a glist of scenarios. */
void free_glist_of_scenarios(void *ptr);
