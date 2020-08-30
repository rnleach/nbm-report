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

    char *string_data = retrieve_data_for_site(opt_args.site);
    Stopif(!string_data, exit(EXIT_FAILURE), "Null data retrieved.");

    printf("Hello world, we're going to try to get NBM data for site %s.\n", opt_args.site);
    printf("We got:\n%s\n", string_data);

    free(string_data);
    program_finalization();
}
