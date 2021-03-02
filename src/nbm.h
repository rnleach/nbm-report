#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                     Library set up and tear down.
 *-----------------------------------------------------------------------------------------------*/
// implementations in src/lib.c
/** Initialize the NBM tools library. */
void nbm_tools_initialize();

/** Finalize the NBM tools library. */
void nbm_tools_finalize();

/*-------------------------------------------------------------------------------------------------
 *                                  Validate a site and request time.
 *-----------------------------------------------------------------------------------------------*/
// implementations in src/site_validation.c
/** The results of validating a site. */
typedef struct SiteValidation SiteValidation;

/** Create a \c SiteValidation object by validating \c site as if requested at \c request_time.
 *
 * \param site the site id or name you want to validate.
 * \param request_time perform the validation as if the request were being made at this time.
 *
 * \returns a \c SiteValidation object always. To find out if the validation succeeded or failed,
 * use \c site_validation_failed().
 */
SiteValidation *site_validation_create(char const *site, time_t request_time);

/** Check to see if this validation failed.
 *
 * \returns \c true if the validation failed.
 */
bool site_validation_failed(SiteValidation *validation);

/** Print a message explaining the validation failure.
 *
 * Before using this function you check us \c site_validation_failed() to make sure it actually
 * failed.
 */
void site_validation_print_failure_message(SiteValidation *validation);

/** Get an alias (pointer) to the site name.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *const site_validation_site_name_alias(SiteValidation *validation);

/** Get an alias (pointer) to the site id.
 *
 * The returned pointer is an alias and SHOULD NOT BE FREED.
 */
char const *const site_validation_site_id_alias(SiteValidation *validation);

/** Get the NBM model run time for the validation. */
time_t site_validation_init_time(SiteValidation *validation);

/** Free resources and nullify the object. */
void site_validation_free(SiteValidation **validation);

/*-------------------------------------------------------------------------------------------------
 *                                             NBMData
 *-----------------------------------------------------------------------------------------------*/
// implentations in src/nbm_data.c
/** A fully parsed NBM data file. */
typedef struct NBMData NBMData;

/** Retrieve the data for a site.
 *
 * \param validation is the result of doing a site validation.
 *
 * returns \c NBMData that you are responsible for freeing with \c nbm_data_free(), or \c NULL if
 * there was an error.
 */
NBMData *retrieve_data(SiteValidation *validation);

/** Free memory associated with an \c NBMData object, and nullify the pointer. */
void nbm_data_free(NBMData **ptrptr);

/** Get the age of the forecast in seconds. */
double nbm_data_age(NBMData const *);

/** Get the site this data is for. */
char const *const nbm_data_site_id(NBMData const *);

/** Get the site name this data is for. */
char const *const nbm_data_site_name(NBMData const *);

/** Get the initialization time of this model run. */
time_t nbm_data_init_time(NBMData const *);

/*-------------------------------------------------------------------------------------------------
 *                           Aquiring and using iterators over NBMData
 *-----------------------------------------------------------------------------------------------*/
// implentations in src/nbm_data.c
/** An iterator over the rows (valid times) of NBM forecasts for an element.
 *
 * This iterator will skip all rows without any data. The units of the output depend on the value
 * requested. For instance, probabilities are always in percent, precipitation is in inches, snow
 * is in inches, temperatures are in Fahrenheit.
 */
typedef struct NBMDataRowIterator NBMDataRowIterator;

/** An iterator over the rows (valid times) of NBM forecasts for a vector element.
 *
 * Currently wind is the only vector element, hence the name.
 *
 * This iterator will skip all rows without any data. Units are degrees for direction and miles per
 * hour for speeds.
 */
typedef struct NBMDataRowIteratorWind NBMDataRowIteratorWind;

/** A view into the current values stored in an iterator.
 *
 * The units of the output depend on the value requested. For instance, probabilities are always in
 * percent, precipitation is in inches, snow is in inches, temperatures are in Fahrenheit.
 */
struct NBMDataRowIteratorValueView {
    time_t *valid_time; /**< Valid time of data on this row. */
    double *value; /**< The semantic meaning depends on the column this value was drawnf rom. */
};

/** A view into the current values stored in a wind iterator.
 *
 * Units are degrees for direction and miles per hour for speeds.
 */
struct NBMDataRowIteratorWindValueView {
    time_t *valid_time; /**< Valid time of data on this row. */
    double *wspd;       /**< The wind speed in mph. */
    double *wspd_std;   /**< The standard deviation of the wind speeds (mph). */
    double *wdir;       /**< The compass direction the wind is coming from. */
    double *gust;       /**< The wind gust in mph. */
    double *gust_std;   /**< The standard deviation of the wind gusts. */
};

/** Get an iterator over a column.
 *
 * \param nbm the NBM data to query.
 * \param col_name The name of the column. To see the available columns, look into one of the raw
 * csv files.
 *
 * \returns an iterator over the rows.
 */
NBMDataRowIterator *nbm_data_rows(NBMData const *nbm, char const *col_name);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_iterator_free(NBMDataRowIterator **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the both of the pointers in the \c NBMDataRowIteratorValueView
 * object are set to null.
 */
struct NBMDataRowIteratorValueView nbm_data_row_iterator_next(NBMDataRowIterator *);

/** Get an iterator over the winds.
 *
 * \param nbm the NBM data to query.
 *
 * \returns an iterator over the rows of wind data.
 */
NBMDataRowIteratorWind *nbm_data_rows_wind(NBMData const *nbm);

/** Free memory associated with the iterator and nullify the pointer. */
void nbm_data_row_wind_iterator_free(NBMDataRowIteratorWind **);

/** Get the next values in the iterator.
 *
 * If there are no more values, the all of the pointers in the \c NBMDataRowIteratorWindValueView
 * object are set to null.
 */
struct NBMDataRowIteratorWindValueView nbm_data_row_wind_iterator_next(NBMDataRowIteratorWind *);

/*-------------------------------------------------------------------------------------------------
 *                         Compare function for soring time_t in ascending order.
 *-----------------------------------------------------------------------------------------------*/
/** Compare \c time_t values which are used as keys in the GLib \c Tree or \c qsort.
 *
 * Dates are sorted in ascending order.
 */
int time_t_compare_func(void const *a, void const *b, void *user_data);

/*-------------------------------------------------------------------------------------------------
 *                                          Accumulators
 *-----------------------------------------------------------------------------------------------*/
// implementations in src/utils.c
/** How to accumulate values from an NBMData column.
 *
 * Implentors should assume if acc is \c NAN, than it is to be replaced with val. If val is \c NAN,
 * then it should be ignored.
 */
typedef double (*Accumulator)(double acc, double val);

/** Sum all values to get a total. */
double accum_sum(double acc, double val);

/** Keep the maximum value, ignoring NaNs. */
double accum_max(double acc, double val);

/** Just keep the last value. */
double accum_last(double _acc, double val);

/*-------------------------------------------------------------------------------------------------
 *                                          Converters
 *-----------------------------------------------------------------------------------------------*/
// implementations in src/utils.c
/** Convert the value extracted from the NBM into the desired units. */
typedef double (*Converter)(double);

static inline double
kelvin_to_fahrenheit(double k)
{
    double c = (k - 273.15);
    double f = 9.0 / 5.0 * c + 32.0;
    return f;
}

static inline double
change_in_kelvin_to_change_in_fahrenheit(double dk)
{
    return 9.0 / 5.0 * dk;
}

static inline double
id_func(double val)
{
    return val;
}

static inline double
mps_to_mph(double val)
{
    return 2.23694 * val;
}

static inline double
mm_to_in(double val)
{
    return val / 25.4;
}

static inline double
m_to_in(double val)
{
    return val * 39.37008;
}

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
