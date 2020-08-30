// standard lib
#include <stdio.h>
#include <stdlib.h>

// Program developed headers
#include "utils.h"

static void print_usage(char **argv){
    char *progname = argv[0];

    printf("Usage: %s site_id\n\n", progname);
    printf("e.g. : %s kmso\n", progname);
}

struct OptArgs {
    char *site;
};

static struct OptArgs
parse_cmd_line(int argc, char *argv[argc + 1]) {
    Stopif(argc < 2, print_usage(argv); exit(EXIT_FAILURE), "Not enough arguments.\n");
    Stopif(argc > 2, print_usage(argv); exit(EXIT_FAILURE), "Too many arguments.\n");

    struct OptArgs result = {0};

    result.site = argv[1];

    return result;
}

int
main(int argc, char *argv[argc + 1])
{
    struct OptArgs opt_args = parse_cmd_line(argc, argv);

    printf("Hello world, we're going to try to get NBM data for site %s.\n", opt_args.site);
}
