// standard lib
#include <locale.h>
#include <math.h>

#include <glib.h>

// Program developed headers
#include "cache.h"
#include "daily_summary.h"
#include "download.h"
#include "ice_summary.h"
#include "nbm_data.h"
#include "options.h"
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
    alert_age(nbm);

    if (opt_args.show_summary)
        show_daily_summary(nbm);

    if (opt_args.show_temperature)
        show_temperature_summary(nbm);

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

        if (opt_args.show_snow) {
            show_snow_summary(nbm, opt_args.accum_hours[i]);
        }

        if (opt_args.show_snow_scenarios) {
            show_snow_scenarios(nbm, opt_args.accum_hours[i]);
        }

        if (opt_args.show_ice) {
            show_ice_summary(nbm, opt_args.accum_hours[i]);
        }
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
    RawNbmData *raw_nbm_data = 0;
    NBMData *parsed_nbm_data = 0;

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

    do_output(parsed_nbm_data, opt_args);

    exit_code = EXIT_SUCCESS;

EXIT_ERR:
    nbm_data_free(&parsed_nbm_data);
    raw_nbm_data_free(&raw_nbm_data);
    site_validation_free(&validation);
    program_finalization();

    return exit_code;
}
