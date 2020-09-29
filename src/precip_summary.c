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
print_liquid_precip_header(struct NBMData const *nbm)
{
    // Build a title.
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "24 Hr Probabilistic Precipitation for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    // Calculate white space to center the title.
    int line_len = 84 - 2;
    char header_buf[84 + 1] = {0};
    len = strlen(title_buf);
    int left = (line_len - len) / 2;

    // Print the white spaces and title.
    for (int i = 0; i < left; i++) {
        header_buf[i] = ' ';
    }
    strcpy(&header_buf[left], title_buf);
    for (int i = left + len; i < line_len; i++) {
        header_buf[i] = ' ';
    }

    // clang-format off
    char const *top_border = 
        "┌──────────────────────────────────────────────────────────────────────────────────┐";

    char const *header = 
        "├─────────────────────┬──────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤\n"
        "│ 24 Hrs Ending / in. │Precip│ 0.01│ 0.05│ 0.10│ 0.25│ 0.50│ 0.75│ 1.00│ 1.50│ 2.00│\n"
        "╞═════════════════════╪══════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╡";
    // clang-format on

    puts(top_border);
    printf("│%s│\n", header_buf);
    puts(header);
}

static void
print_liquid_precip_footer()
{
    // clang-format off
    char const *footer = 
        "╘═════════════════════╧══════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╛";
    // clang-format on
    puts(footer);
}

static int
print_row_prob_liquid_exceedence(void *key, void *value, void *unused)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;

    double prob_001 = round(interpolate_prob_of_exceedance(dist, 0.01));
    double prob_005 = round(interpolate_prob_of_exceedance(dist, 0.05));
    double prob_010 = round(interpolate_prob_of_exceedance(dist, 0.10));
    double prob_025 = round(interpolate_prob_of_exceedance(dist, 0.25));
    double prob_050 = round(interpolate_prob_of_exceedance(dist, 0.50));
    double prob_075 = round(interpolate_prob_of_exceedance(dist, 0.75));
    double prob_100 = round(interpolate_prob_of_exceedance(dist, 1.0));
    double prob_150 = round(interpolate_prob_of_exceedance(dist, 1.5));
    double prob_200 = round(interpolate_prob_of_exceedance(dist, 2.0));
    double pm_value = round(cumulative_dist_pm_value(dist) * 100.0) / 100.0;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    char linebuf[512] = {0};
    sprintf(linebuf,
            "│ %s │ %4.2lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf "
            "│ %3.0lf │",
            datebuf, pm_value, prob_001, prob_005, prob_010, prob_025, prob_050, prob_075, prob_100,
            prob_150, prob_200);

    wipe_nans(linebuf);

    puts(linebuf);

    return false;
}

static void
show_liquid_summary(struct NBMData const *nbm)
{
    GTree *cdfs = extract_cdfs(nbm, "APCP24hr_surface_%d%% level", "APCP24hr_surface", mm_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for QPF.");
    print_liquid_precip_header(nbm);
    g_tree_foreach(cdfs, print_row_prob_liquid_exceedence, 0);
    print_liquid_precip_footer();

    g_tree_unref(cdfs);
}

/*-------------------------------------------------------------------------------------------------
 *                                            Snow Summary
 *-----------------------------------------------------------------------------------------------*/

static void
print_snow_precip_header(struct NBMData const *nbm)
{
    // Build a title.
    char title_buf[256] = {0};
    time_t init_time = nbm_data_init_time(nbm);
    struct tm init = *gmtime(&init_time);
    sprintf(title_buf, "24 Hr Probabilistic Snow for %s - ", nbm_data_site(nbm));
    int len = strlen(title_buf);
    strftime(&title_buf[len], sizeof(title_buf) - len, " %Y/%m/%d %Hz", &init);

    // Calculate white space to center the title.
    int line_len = 90 - 2;
    char header_buf[90 + 1] = {0};
    len = strlen(title_buf);
    int left = (line_len - len) / 2;

    // Print the white spaces and title.
    for (int i = 0; i < left; i++) {
        header_buf[i] = ' ';
    }
    strcpy(&header_buf[left], title_buf);
    for (int i = left + len; i < line_len; i++) {
        header_buf[i] = ' ';
    }

    // clang-format off
    char const *top_border = 
        "┌────────────────────────────────────────────────────────────────────────────────────────┐";

    char const *header = 
        "├─────────────────────┬──────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤\n"
        "│ 24 Hrs Ending / in. │ Snow │  0.1│  0.5│  1.0│  2.0│  4.0│  6.0│  8.0│ 12.0│ 18.0│ 24.0│\n"
        "╞═════════════════════╪══════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╪═════╡";
    // clang-format on

    puts(top_border);
    printf("│%s│\n", header_buf);
    puts(header);
}

static void
print_snow_precip_footer()
{
    // clang-format off
    char const *footer = 
        "╘═════════════════════╧══════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╧═════╛";
    // clang-format on
    puts(footer);
}

static int
print_row_prob_snow_exceedence(void *key, void *value, void *unused)
{
    time_t *vt = key;
    struct CumulativeDistribution *dist = value;

    double prob_01 = round(interpolate_prob_of_exceedance(dist, 0.1));
    double prob_05 = round(interpolate_prob_of_exceedance(dist, 0.5));
    double prob_10 = round(interpolate_prob_of_exceedance(dist, 1.0));
    double prob_20 = round(interpolate_prob_of_exceedance(dist, 2.0));
    double prob_40 = round(interpolate_prob_of_exceedance(dist, 4.0));
    double prob_60 = round(interpolate_prob_of_exceedance(dist, 6.0));
    double prob_80 = round(interpolate_prob_of_exceedance(dist, 8.0));
    double prob_120 = round(interpolate_prob_of_exceedance(dist, 12.0));
    double prob_180 = round(interpolate_prob_of_exceedance(dist, 18.0));
    double prob_240 = round(interpolate_prob_of_exceedance(dist, 24.0));
    double pm_value = round(cumulative_dist_pm_value(dist) * 10.0) / 10.0;

    char datebuf[64] = {0};
    strftime(datebuf, sizeof(datebuf), "%a, %Y-%m-%d %HZ", gmtime(vt));

    char linebuf[512] = {0};
    sprintf(linebuf,
            "│ %s │ %4.1lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf │ %3.0lf "
            "│ %3.0lf │ %3.0lf │",
            datebuf, pm_value, prob_01, prob_05, prob_10, prob_20, prob_40, prob_60, prob_80,
            prob_120, prob_180, prob_240);

    wipe_nans(linebuf);

    puts(linebuf);

    return false;
}

static void
show_snow_summary(struct NBMData const *nbm)
{
    GTree *cdfs = extract_cdfs(nbm, "ASNOW24hr_surface_%d%% level", "ASNOW24hr_surface", m_to_in);
    Stopif(!cdfs, return, "Error extracting CDFs for Snow.");
    print_snow_precip_header(nbm);
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
