// standard lib
#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Third party libs
#include <glib.h>

// Program developed headers
#include "cache.h"
#include "daily_summary.h"
#include "download.h"
#include "ice_summary.h"
#include "nbm_data.h"
#include "precip_summary.h"
#include "raw_nbm_data.h"
#include "site_validation.h"
#include "snow_summary.h"
#include "temp_summary.h"
#include "utils.h"

/*-------------------------------------------------------------------------------------------------
 *                                    Program Setup and Teardown.
 *-----------------------------------------------------------------------------------------------*/
static void
program_initialization()
{
    setlocale(LC_ALL, "");
    cache_initialize();
    download_module_initialize();
}

static void
program_finalization()
{
    download_module_finalize();
    cache_finalize();
}

/*-------------------------------------------------------------------------------------------------
 *                                    Argument Parsing
 *-----------------------------------------------------------------------------------------------*/
static bool
is_valid_accum_period(int hours)
{
    return hours == 6 || hours == 12 || hours == 24 || hours == 48 || hours == 72;
}

struct OptArgs {
    char *site;
    bool show_summary;
    bool show_rain;
    bool show_snow;
    bool show_ice;
    int num_accum_periods;
    int accum_hours[4];
    bool show_temperature;
    bool error_parsing_options;
};

static gboolean
option_callback(const char *name, const char *value, void *data, GError **unused)
{
    struct OptArgs *opts = data;

    if (strcmp(name, "--no-summary") == 0 || strcmp(name, "-n") == 0) {
        opts->show_summary = false;
    } else if (strcmp(name, "--precipitation") == 0 || strcmp(name, "-r") == 0) {
        opts->show_rain = true;
    } else if (strcmp(name, "--snow") == 0 || strcmp(name, "-s") == 0) {
        opts->show_snow = true;
    } else if (strcmp(name, "--ice") == 0 || strcmp(name, "-i") == 0) {
        opts->show_ice = true;
    } else if (strcmp(name, "--temperature") == 0 || strcmp(name, "-t") == 0) {
        opts->show_temperature = true;
    } else if (strcmp(name, "--accumulation-period") == 0 || strcmp(name, "-a") == 0) {
        if (opts->num_accum_periods < sizeof(opts->accum_hours)) {
            opts->accum_hours[opts->num_accum_periods] = atoi(value);
            opts->num_accum_periods++;
        } else {
            fprintf(stderr, "Too many accumulation periods!\n");
            return false;
        }
    } else {
        return false;
    }

    return true;
}

static GOptionEntry entries[] = {
    {.long_name = "accumulation-period",
     .short_name = 'a',
     .flags = G_OPTION_FLAG_NONE,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "the number of hours to report accumulation summaries, H must be 6, 12, 24, "
                    "48, or 72 hours.",
     .arg_description = "H"},
    {.long_name = "no-summary",
     .short_name = 'n',
     .flags = G_OPTION_FLAG_NO_ARG,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "skip the overall summary",
     .arg_description = 0},
    {.long_name = "precipitation",
     .short_name = 'r',
     .flags = G_OPTION_FLAG_NO_ARG,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "show summary of precipitation",
     .arg_description = 0},
    {.long_name = "snow",
     .short_name = 's',
     .flags = G_OPTION_FLAG_NO_ARG,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "show summary of snow",
     .arg_description = 0},
    {.long_name = "ice",
     .short_name = 'i',
     .flags = G_OPTION_FLAG_NO_ARG,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "show summary of ice forecast",
     .arg_description = 0},
    {.long_name = "temperature",
     .short_name = 't',
     .flags = G_OPTION_FLAG_NO_ARG,
     .arg = G_OPTION_ARG_CALLBACK,
     .arg_data = option_callback,
     .description = "show summary of temperatures",
     .arg_description = 0},
    {0},
};

static struct OptArgs
parse_cmd_line(int argc, char *argv[argc + 1])
{
    struct OptArgs result = {
        .site = 0,
        .show_summary = true,
        .show_rain = false,
        .show_snow = false,
        .show_ice = false,
        .num_accum_periods = 0,
        .accum_hours = {24, 0, 0, 0},
        .show_temperature = false,
        .error_parsing_options = false,
    };

    GOptionContext *context = g_option_context_new("SITE");
    GOptionGroup *main = g_option_group_new("main", "main options", "main help", &result, 0);
    g_option_group_add_entries(main, entries);
    g_option_context_set_main_group(context, main);

    bool success = g_option_context_parse(context, &argc, &argv, 0);
    Stopif(!success, goto ERR_RETURN, "Error parsing command line arguments.");

    if ((result.show_snow || result.show_rain || result.show_ice) && result.num_accum_periods == 0)
        result.num_accum_periods = 1;

    for (int i = 0; i < result.num_accum_periods; i++) {
        Stopif(!is_valid_accum_period(result.accum_hours[i]), goto ERR_RETURN,
               "Invalid accumulation period: %d - %d", i, result.accum_hours[i]);
    }
    Stopif(argc < 2, goto ERR_RETURN, "Missing site argument.");

    result.site = argv[1];

    g_option_context_free(context);

    return result;

ERR_RETURN:;

    char *err_msg = g_option_context_get_help(context, true, 0);
    puts(err_msg);
    g_free(err_msg);
    g_option_context_free(context);
    result.error_parsing_options = true;

    return result;
}

/*-------------------------------------------------------------------------------------------------
 *                                    Quality checks/alerts.
 *-----------------------------------------------------------------------------------------------*/
static void
alert_age(struct NBMData const *nbm)
{
    double age_secs = nbm_data_age(nbm);
    int age_hrs = (int)round(age_secs / 3600.0);

    if (age_hrs >= 12) {
        int age_days = age_hrs / 24;
        age_hrs -= age_days * 24;

        printf("     *\n");
        printf("     * OLD NBM DATA - data is: ");
        if (age_days > 1) {
            printf("%d days and", age_days);
        } else if (age_days > 0) {
            printf("%d day and", age_days);
        }
        if (age_hrs > 1) {
            printf(" %d hours old", age_hrs);
        } else if (age_hrs > 0) {
            printf(" %d hour old", age_hrs);
        }
        printf("\n");
        printf("     *\n");
    }
}

/*-------------------------------------------------------------------------------------------------
 *                                    Main Program
 *-----------------------------------------------------------------------------------------------*/
int
main(int argc, char *argv[argc + 1])
{
    int exit_code = EXIT_FAILURE;

    // Variables that hold allocated memory.
    struct SiteValidation *validation = 0;
    struct RawNbmData *raw_nbm_data = 0;
    struct NBMData *parsed_nbm_data = 0;

    program_initialization();

    struct OptArgs opt_args = parse_cmd_line(argc, argv);
    Stopif(opt_args.error_parsing_options, goto EXIT_ERR, "Error parsing command line.");

    validation = site_validation_create(opt_args.site, time(0));
    if (site_validation_failed(validation)) {
        site_validation_print_failure_message(validation);
        goto EXIT_ERR;
    }

    char const *file_name = site_validation_file_name_alias(validation);
    char const *site_nm = site_validation_site_name_alias(validation);
    char const *site_id = site_validation_site_id_alias(validation);
    time_t init_time = site_validation_init_time(validation);

    raw_nbm_data = retrieve_data_for_site(site_id, site_nm, file_name, init_time);
    Stopif(!raw_nbm_data, goto EXIT_ERR, "Null data retrieved for %s.", opt_args.site);

    parsed_nbm_data = parse_raw_nbm_data(raw_nbm_data);
    Stopif(!parsed_nbm_data, goto EXIT_ERR, "Error parsing %s.", file_name);

    alert_age(parsed_nbm_data);

    if (opt_args.show_summary)
        show_daily_summary(parsed_nbm_data);

    if (opt_args.show_temperature)
        show_temperature_summary(parsed_nbm_data);

    if (opt_args.show_snow || opt_args.show_rain || opt_args.show_ice) {
        for (int i = 0;
             i < sizeof(opt_args.accum_hours) && is_valid_accum_period(opt_args.accum_hours[i]);
             i++) {
            if (opt_args.show_rain) {
                show_precip_summary(parsed_nbm_data, opt_args.accum_hours[i]);
            }
            if (opt_args.show_snow) {
                show_snow_summary(parsed_nbm_data, opt_args.accum_hours[i]);
            }
            if (opt_args.show_ice) {
                show_ice_summary(parsed_nbm_data, opt_args.accum_hours[i]);
            }
        }
    }

    exit_code = EXIT_SUCCESS;

EXIT_ERR:
    nbm_data_free(&parsed_nbm_data);
    raw_nbm_data_free(&raw_nbm_data);
    site_validation_free(&validation);
    program_finalization();

    return exit_code;
}
