#include "options.h"

#include <stdio.h>

#include <glib.h>

#include "utils.h"

/*-------------------------------------------------------------------------------------------------
 *                                    Forward Declarations
 *-----------------------------------------------------------------------------------------------*/
static gboolean option_callback(const char *name, const char *value, void *data, GError **unused);

/*-------------------------------------------------------------------------------------------------
 *                                      Global Options
 *-----------------------------------------------------------------------------------------------*/
bool global_verbose = false;

/*-------------------------------------------------------------------------------------------------
 *                            Command line options configuration
 *-----------------------------------------------------------------------------------------------*/
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
    {.long_name = "verbose",
     .short_name = 'v',
     .flags = G_OPTION_FLAG_NONE,
     .arg = G_OPTION_ARG_NONE,
     .arg_data = &global_verbose,
     .description = "show verbose output.",
     .arg_description = 0},
    {0},
};

/*-------------------------------------------------------------------------------------------------
 *                                    Argument Parsing
 *-----------------------------------------------------------------------------------------------*/
static bool
is_valid_accum_period(int hours)
{
    return hours == 6 || hours == 12 || hours == 24 || hours == 48 || hours == 72;
}

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

struct OptArgs
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
