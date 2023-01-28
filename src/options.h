#pragma once

#include <stdbool.h>
#include <time.h>

/** The command line options. */
struct OptArgs {
    char *site;

    char *save_dir;
    char *save_prefix;

    time_t request_time;

    int num_accum_periods;
    int accum_hours[4];

    bool show_summary;
    bool show_hourly;
    bool show_rain;
    bool show_snow;
    bool show_ice;
    bool show_temperature;
    bool show_wind;

    bool show_wind_scenarios;
    bool show_temperature_scenarios;
    bool show_precip_scenarios;
    bool show_snow_scenarios;

    bool error_parsing_options;
};

/** Parse the command line and populate a \c struct \c OptArgs.
 *
 * This routine may also set some global configuration variables, like a verbose flag.
 */
struct OptArgs parse_cmd_line(int argc, char *argv[argc + 1]);
