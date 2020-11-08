#pragma once

#include <stdbool.h>

/** The command line options. */
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

/** Parse the command line and populate a \c struct \c OptArgs.
 *
 * This routine may also set some global configuration variables, like a verbose flage.
 */
struct OptArgs parse_cmd_line(int argc, char *argv[argc + 1]);
