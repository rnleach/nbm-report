// standard lib
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Third party libs
#include <curl/curl.h>

// Program developed headers
#include "daily_summary.h"
#include "download.h"
#include "nbm_data.h"
#include "raw_nbm_data.h"
#include "utils.h"

static void
print_usage(char **argv)
{
    char *progname = argv[0];

    printf("Usage: %s site_id\n", progname);
    printf("e.g. : %s kmso\n\n", progname);
    printf("Options:\n");
    printf("   -s show summary of snow forecast.\n");
    printf("   -r show summary of rain / liquid equivalent forecast.\n");
    printf("   -n do not show main summary.\n");
}

struct OptArgs {
    char *site;
    bool show_summary;
    bool show_rain;
    bool show_snow;
};

static struct OptArgs
parse_cmd_line(int argc, char *argv[argc + 1])
{
    Stopif(argc < 2, print_usage(argv); exit(EXIT_FAILURE), "Not enough arguments.\n");
    Stopif(argc > 5, print_usage(argv); exit(EXIT_FAILURE), "Too many arguments.\n");

    struct OptArgs result = {
        .site = 0, .show_summary = true, .show_rain = false, .show_snow = false};

    int opt;
    while ((opt = getopt(argc, argv, "srn")) != -1) {
        switch (opt) {
        case 's':
            result.show_snow = true;
            break;
        case 'r':
            result.show_rain = true;
            break;
        case 'n':
            result.show_summary = false;
            break;
        default:
            fprintf(stderr, "Unknown argument.\n");
            print_usage(argv);
            exit(EXIT_FAILURE);
            break;
        }
    }

    Stopif(optind >= argc, print_usage(argv); exit(EXIT_FAILURE), "Not enough arguments.");

    result.site = argv[optind];

    return result;
}

static void
program_initialization()
{
    CURLcode err = curl_global_init(CURL_GLOBAL_DEFAULT);
    Stopif(err, exit(EXIT_FAILURE), "Failed to initialize curl");
}

static void
program_finalization()
{
    curl_global_cleanup();
}

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

    if (opt_args.show_summary)
        show_daily_summary(parsed_nbm_data);

    nbm_data_free(&parsed_nbm_data);
    program_finalization();
}
