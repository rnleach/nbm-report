#include "precip_summary.h"
#include "nbm_data.h"
#include "summarize.h"
#include "utils.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <glib.h>

/*-------------------------------------------------------------------------------------------------
 *                                            Liquid Summary
 *-----------------------------------------------------------------------------------------------*/
static void
print_liquid_precip_header()
{
    // clang-format off
    char const *header = 
        "┌─────────────────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐\n"
        "│   Date / Rain   │0.01\"│0.05\"│0.10\"│0.25\"│0.50\"│0.75\"│1.00\"│1.50\"│2.00\"│\n"
        "╞═════════════════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╡";
    // clang-format on

    puts(header);
}

static void
print_liquid_precip_footer()
{
    // clang-format off
    char const *footer = 
        "╘═════════════════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╛";
    // clang-format on
    puts(footer);
}

static int
print_row_prob_liquid_exceedence(void *key, void *value, void *unused)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;

    double prob_001 = interpolate_prob_of_exceedance(dist, 0.01);
    double prob_005 = interpolate_prob_of_exceedance(dist, 0.05);
    double prob_010 = interpolate_prob_of_exceedance(dist, 0.10);
    double prob_025 = interpolate_prob_of_exceedance(dist, 0.25);
    double prob_050 = interpolate_prob_of_exceedance(dist, 0.50);
    double prob_075 = interpolate_prob_of_exceedance(dist, 0.75);
    double prob_100 = interpolate_prob_of_exceedance(dist, 1.0);
    double prob_150 = interpolate_prob_of_exceedance(dist, 1.5);
    double prob_200 = interpolate_prob_of_exceedance(dist, 2.0);

    char datebuf[32] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d ", gmtime(vt));
    printf(
        "│ %s│ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │\n",
        datebuf, prob_001, prob_005, prob_010, prob_025, prob_050, prob_075, prob_100, prob_150,
        prob_200);

    return false;
}

static void
show_liquid_summary(struct NBMData const *nbm)
{
    GTree *cdfs =
        extract_cdfs(nbm, "APCP24hr_surface_%d%% level", summary_date_12z, mm_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for QPF.");
    print_liquid_precip_header();
    g_tree_foreach(cdfs, print_row_prob_liquid_exceedence, 0);
    print_liquid_precip_footer();

    g_tree_unref(cdfs);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Snow Summary
 *-----------------------------------------------------------------------------------------------*/

static void
print_snow_precip_header()
{
    // clang-format off
    char const *header = 
        "┌─────────────────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐\n"
        "│   Date / Rain   │ 0.1\"│ 0.5\"│ 1.0\"│ 2.5\"│ 5.0\"│ 7.5\"│10.0\"│15.0\"│20.0\"│\n"
        "╞═════════════════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╡";
    // clang-format on

    puts(header);
}

static void
print_snow_precip_footer()
{
    // clang-format off
    char const *footer = 
        "╘═════════════════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╛";
    // clang-format on
    puts(footer);
}

static int
print_row_prob_snow_exceedence(void *key, void *value, void *unused)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;

    double prob_01 = interpolate_prob_of_exceedance(dist, 0.1);
    double prob_05 = interpolate_prob_of_exceedance(dist, 0.5);
    double prob_10 = interpolate_prob_of_exceedance(dist, 1.0);
    double prob_25 = interpolate_prob_of_exceedance(dist, 2.5);
    double prob_50 = interpolate_prob_of_exceedance(dist, 5.0);
    double prob_75 = interpolate_prob_of_exceedance(dist, 7.5);
    double prob_100 = interpolate_prob_of_exceedance(dist, 10.0);
    double prob_150 = interpolate_prob_of_exceedance(dist, 15.0);
    double prob_200 = interpolate_prob_of_exceedance(dist, 20.0);

    char datebuf[32] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d ", gmtime(vt));
    printf(
        "│ %s│ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │\n",
        datebuf, prob_01, prob_05, prob_10, prob_25, prob_50, prob_75, prob_100, prob_150,
        prob_200);

    return false;
}

static void
show_snow_summary(struct NBMData const *nbm)
{
    GTree *cdfs =
        extract_cdfs(nbm, "ASNOW24hr_surface_%d%% level", summary_date_12z, m_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for Snow.");
    print_snow_precip_header();
    g_tree_foreach(cdfs, print_row_prob_snow_exceedence, 0);
    print_snow_precip_footer();

    g_tree_unref(cdfs);
    
}

/*-------------------------------------------------------------------------------------------------
 *                                            Public API
 *-----------------------------------------------------------------------------------------------*/
void
show_precip_summary(struct NBMData const *nbm, char type)
{
    assert(type == 'r' || type == 's');

    switch (type) {
    case 'r':
        show_liquid_summary(nbm);
        break;
    case 's':
        show_snow_summary(nbm);
        break;
    default:
        assert(false);
    }
}
