// standard lib
#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Program developed headers
#include "daily_summary.h"
#include "download.h"
#include "ice_summary.h"
#include "nbm_data.h"
#include "precip_summary.h"
#include "raw_nbm_data.h"
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
    download_module_initialize();
}

static void
program_finalization()
{
    download_module_finalize();
}

/*-------------------------------------------------------------------------------------------------
 *                                    Argument Parsing
 *-----------------------------------------------------------------------------------------------*/
static void
print_usage(char **argv)
{
    char *progname = argv[0];

    printf("Usage: %s site_id\n", progname);
    printf("e.g. : %s kmso\n\n", progname);

    char *options = "Options:\n"
                    "   -t show temperature forecast quantiles.                                 \n"
                    "   -r show summary of rain / liquid equivalent forecast.                   \n"
                    "   -s show summary of snow forecast                                        \n"
                    "   -i show summary of ice forecast                                         \n"
                    "   -a <hours>  where hours is 6, 12, 24, 48, or 72. This                   \n"
                    "      is the accumulation period for rain and snow.                        \n"
                    "   -n do not show main summary.                                            \n"
                    "\n                                                                         \n"
                    "For the purpose of this program, days run from 06Z to 06Z.                 \n"
                    "This only applies to variables that are sampled or summed                  \n"
                    "over hourly, 3-hourly, or 6-hourly forecast values. If a                   \n"
                    "parameter is a 12, 24, 48, or 72 hour summary from the NBM,                \n"
                    "then the time valid at the end of the period is reported.                  \n"
                    "So 24-hr probability of precipitation is looking back 24 hours.            \n";

    puts(options);
}

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
    int accum_hours[4];
    bool show_temperature;
};

static struct OptArgs
parse_cmd_line(int argc, char *argv[argc + 1])
{
    Stopif(argc < 2, goto ERR_RETURN, "Not enough arguments.\n");

    int accum_periods = 0;

    struct OptArgs result = {
        .site = 0,
        .show_summary = true,
        .show_rain = false,
        .show_snow = false,
        .show_ice = false,
        .accum_hours = {24, 0, 0, 0},
        .show_temperature = false,
    };

    int opt;
    while ((opt = getopt(argc, argv, "a:inrst")) != -1) {
        switch (opt) {
        case 'a':
            Stopif(accum_periods >= sizeof(result.accum_hours), goto ERR_RETURN,
                   "Too many accum arguments.");
            result.accum_hours[accum_periods] = atoi(optarg);
            accum_periods++;
            break;
        case 'i':
            result.show_ice = true;
            break;
        case 'n':
            result.show_summary = false;
            break;
        case 'r':
            result.show_rain = true;
            break;
        case 's':
            result.show_snow = true;
            break;
        case 't':
            result.show_temperature = true;
            break;
        default:
            Stopif(true, goto ERR_RETURN, "Unknown argument: %c", opt);
        }
    }

    if ((result.show_snow || result.show_rain) && accum_periods == 0)
        accum_periods = 1;

    for (int i = 0; i < accum_periods; i++) {
        Stopif(!is_valid_accum_period(result.accum_hours[i]), print_usage(argv);
               exit(EXIT_FAILURE), "Invalid accumulation period.");
    }
    Stopif(optind >= argc, print_usage(argv); exit(EXIT_FAILURE), "Missing site argument.");

    result.site = argv[optind];

    return result;

ERR_RETURN:
    print_usage(argv);
    exit(EXIT_FAILURE);
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
    program_initialization();

    struct OptArgs opt_args = parse_cmd_line(argc, argv);

    struct RawNbmData *raw_nbm_data = retrieve_data_for_site(opt_args.site);
    Stopif(!raw_nbm_data, exit(EXIT_FAILURE), "Null data retrieved.");

    struct NBMData *parsed_nbm_data = parse_raw_nbm_data(raw_nbm_data);
    raw_nbm_data_free(&raw_nbm_data);
    Stopif(!parsed_nbm_data, exit(EXIT_FAILURE), "Null data returned from parsing.");

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

    nbm_data_free(&parsed_nbm_data);
    program_finalization();
}
