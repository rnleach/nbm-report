// standard lib
#include <locale.h>
#include <math.h>
#include <stdio.h>

#include <glib.h>

// Program developed headers
#include "cache.h"
#include "daily_summary.h"
#include "download.h"
#include "gust_summary.h"
#include "hourly.h"
#include "ice_summary.h"
#include "options.h"
#include "precip_summary.h"
#include "snow_summary.h"
#include "temp_summary.h"
#include "utils.h"
#include "wind_summary.h"

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
 *                                    Quality checks/alerts.
 *-----------------------------------------------------------------------------------------------*/
static void
alert_age(NBMData const *nbm)
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
 *                                    Main Output
 *-----------------------------------------------------------------------------------------------*/
static void
do_output(NBMData const *nbm, struct OptArgs opt_args)
{
    // Check the time we requested data for, if it is more than an hour ago, don't bother alerting
    // for the age, since we are probably requesting an archived run and not the most recent. If
    // it is more recent than an hour, we probably requested the most recent run and should be
    // alerted if it is too old.
    if (difftime(time(0), opt_args.request_time) < 3600.0) {
        alert_age(nbm);
    }

    if (opt_args.show_summary)
        show_daily_summary(nbm);

    if (opt_args.show_hourly)
        show_hourly(nbm);

    if (opt_args.show_temperature || opt_args.show_temperature_scenarios) {
        TempSum *tsum = temp_sum_build(nbm);

        if (opt_args.show_temperature) {
            show_temp_summary(tsum);
        }
        if (opt_args.show_temperature_scenarios) {
            show_temp_scenarios(tsum);
        }

        if (opt_args.save_dir) {
            temp_sum_save(tsum, opt_args.save_dir, opt_args.save_prefix);
        }

        temp_sum_free(&tsum);
    }

    for (int i = 0; i < sizeof(opt_args.accum_hours) && opt_args.accum_hours[i]; i++) {

        if (opt_args.show_rain || opt_args.show_precip_scenarios) {

            PrecipSum *psum = precip_sum_build(nbm, opt_args.accum_hours[i]);

            if (opt_args.show_rain) {
                show_precip_summary(psum);
            }

            if (opt_args.show_precip_scenarios) {
                show_precip_scenarios(psum);
            }

            if (opt_args.save_dir) {
                precip_sum_save(psum, opt_args.save_dir, opt_args.save_prefix);
            }

            precip_sum_free(&psum);
        }

        if (opt_args.show_snow || opt_args.show_snow_scenarios) {

            SnowSum *ssum = snow_sum_build(nbm, opt_args.accum_hours[i]);

            if (opt_args.show_snow) {
                show_snow_summary(ssum);
            }

            if (opt_args.show_snow_scenarios) {
                show_snow_scenarios(ssum);
            }

            if (opt_args.save_dir) {
                snow_sum_save(ssum, opt_args.save_dir, opt_args.save_prefix);
            }

            snow_sum_free(&ssum);
        }

        if (opt_args.show_ice) {
            show_ice_summary(nbm, opt_args.accum_hours[i]);
        }
    }

    if (opt_args.show_wind || opt_args.show_wind_scenarios) {
        WindSum *wsum = wind_sum_build(nbm);

        if (opt_args.show_wind) {
            show_wind_summary(wsum);
        }
        if (opt_args.show_wind_scenarios) {
            show_wind_scenarios(wsum);
        }

        if (opt_args.save_dir) {
            wind_sum_save(wsum, opt_args.save_dir, opt_args.save_prefix);
        }

        wind_sum_free(&wsum);
    }

    if (opt_args.show_gust || opt_args.show_gust_scenarios) {
        GustSum *gsum = gust_sum_build(nbm);

        if (opt_args.show_gust) {
            show_gust_summary(gsum);
        }
        if (opt_args.show_gust_scenarios) {
            show_gust_scenarios(gsum);
        }

        if (opt_args.save_dir) {
            gust_sum_save(gsum, opt_args.save_dir, opt_args.save_prefix);
        }

        gust_sum_free(&gsum);
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
    SiteValidation *validation = 0;
    NBMData *nbm_data = 0;

    program_initialization();

    struct OptArgs opt_args = parse_cmd_line(argc, argv);
    Stopif(opt_args.error_parsing_options, goto EXIT_ERR, "Error parsing command line.");

    validation = site_validation_create(opt_args.site, opt_args.request_time);
    if (site_validation_failed(validation)) {
        site_validation_print_failure_message(validation);
        goto EXIT_ERR;
    }

    nbm_data = retrieve_data(validation);
    Stopif(!nbm_data, goto EXIT_ERR, "Error retrieving data for %s.", opt_args.site);

    do_output(nbm_data, opt_args);

    exit_code = EXIT_SUCCESS;

EXIT_ERR:
    nbm_data_free(&nbm_data);
    site_validation_free(&validation);
    program_finalization();

    return exit_code;
}
