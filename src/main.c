// standard lib
#include <stdio.h>
#include <stdlib.h>

// Third party libs
#include <curl/curl.h>

// Program developed headers
#include "download/download.h"
#include "utils.h"

static void
print_usage(char **argv)
{
    char *progname = argv[0];

    printf("Usage: %s site_id\n\n", progname);
    printf("e.g. : %s kmso\n", progname);
}

struct OptArgs {
    char *site;
};

static struct OptArgs
parse_cmd_line(int argc, char *argv[argc + 1])
{
    Stopif(argc < 2, print_usage(argv); exit(EXIT_FAILURE), "Not enough arguments.\n");
    Stopif(argc > 2, print_usage(argv); exit(EXIT_FAILURE), "Too many arguments.\n");

    struct OptArgs result = {0};

    result.site = argv[1];

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

    struct tm init_time = raw_nbm_data_init_time(raw_nbm_data);
    printf(
        "Hello world, we downloaded NBM point data for site %s (%s) at initialization time %s.\n",
        opt_args.site, raw_nbm_data_site(raw_nbm_data), asctime(&init_time));

    free_raw_nbm_data(&raw_nbm_data);
    program_finalization();
}
