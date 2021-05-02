#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "print.h"

#define OPTIONS_DEFAULT_SECTOR_SIZE 512
#define OPTIONS_DEFAULT_BUFFER_SIZE (4 * 1024 * 1024)

void options_init(options_t *const opts);
int options_parse_unsigned(const char *const arg, uint32_t *const value);

int
options_parse(int argc, char **argv, options_t *const opts)
{
    options_init(opts);

    int ch;
    char *arg_sector_size = NULL;
    char *arg_buffer_size = NULL;

    while ((ch = getopt(argc, argv, ":s:b:h")) != -1) {
        switch (ch) {
        case 's':
            arg_sector_size = optarg;
            break;

        case 'b':
            arg_buffer_size = optarg;
            break;

        case 'h':
            opts->help = true;
            break;

        case ':':
            print_error("missing argument for option '-%c'", optopt);
            return 1;

        default:
            print_error("unknown option '-%c'", optopt);
            return 1;
        }
    }

    argc -= optind;
    argv += optind;

    if (opts->help) {
        return 0;
    } else if (argc < 2) {
        print_error("missing arguments");
        return 1;
    } else if (argc > 3) {
        print_error("too many arguments");
        return 1;
    }

    /* Convert numbers in the arguments */
    if ((arg_sector_size != NULL) &&
        options_parse_unsigned(arg_sector_size, &(opts->sector_size))) {
        print_error("incorrect sector size");
        return 1;
    } else if ((arg_buffer_size != NULL) &&
               options_parse_unsigned(arg_buffer_size, &(opts->buffer_size))) {
        print_error("incorrect buffer size");
        return 1;
    } else if (opts->sector_size == 0) {
        print_error("sector size cannot be 0");
        return 1;
    } else if (opts->buffer_size == 0) {
        print_error("buffer size cannot be 0");
        return 1;
    } else if (opts->sector_size > opts->buffer_size) {
        print_error("sector size cannot larger than buffer size");
        return 1;
    } else if ((opts->buffer_size % opts->sector_size) != 0) {
        print_error("buffer size is not multiple of sector size");
        return 1;
    }

    /* Pick the file paths */
    int last_path_index = argc - 1;
    opts->out_file_path = argv[last_path_index--];
    opts->ref_file_path = argv[last_path_index--];
    if (last_path_index >= 0) {
        opts->in_file_path = argv[last_path_index];
    }

    return 0;
}

void
options_init(options_t *const opts)
{
    opts->help = false;
    opts->sector_size = OPTIONS_DEFAULT_SECTOR_SIZE;
    opts->buffer_size = OPTIONS_DEFAULT_BUFFER_SIZE;

    opts->in_file_path = NULL;
    opts->ref_file_path = NULL;
    opts->out_file_path = NULL;
}

int
options_parse_unsigned(const char *const arg, uint32_t *const value)
{
    char *end;

    errno = 0;

    *value = strtoul(arg, &end, 0);

    return ((*end != '\0') || (errno != 0)) ? -1 : 0;
}
