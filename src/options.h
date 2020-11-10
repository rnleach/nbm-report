#pragma once

#include <stdbool.h>

/** The command line options. */
struct OptArgs {
    char *site;
    bool show_summary;
    bool show_rain;
    bool show_snow;
    bool show_ice;
    bool show_temperature;
    bool show_temperature_scenarios;
    bool show_precip_scenarios;
    bool show_snow_scenarios;
    int num_accum_periods;
    int accum_hours[4];
    bool error_parsing_options;
    char *save_dir;
    char *save_prefix;
};

/** Parse the command line and populate a \c struct \c OptArgs.
 *
 * This routine may also set some global configuration variables, like a verbose flage.
 */
struct OptArgs parse_cmd_line(int argc, char *argv[argc + 1]);
