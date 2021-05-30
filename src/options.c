/* Copyright 2019 Ján Sučan <jan@jansucan.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "print.h"

/* This header file is automatically generated at build time from the Makefile
 */
#include "program_info.h"

#define OPTIONS_DEFAULT_SECTOR_SIZE 512
#define OPTIONS_DEFAULT_BUFFER_SIZE (4 * 1024 * 1024)

static void options_init(options_t *const opts);
static int options_parse_unsigned(const char *const arg, uint32_t *const value);

int
options_parse(int argc, char **argv, options_t *const opts)
{
    options_init(opts);

    int ch;
    char *arg_sector_size = NULL;
    char *arg_buffer_size = NULL;

    while ((ch = getopt(argc, argv, ":b:hs:")) != -1) {
        switch (ch) {
        case 'b':
            arg_buffer_size = optarg;
            break;

        case 'h':
            opts->help = true;
            break;

        case 's':
            arg_sector_size = optarg;
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
options_usage(int exit_code)
{
    printf("Usage: %s [-s SECTOR_SIZE] [-b BUFFER_SIZE] [INFILE] REFFILE "
           "OUTFILE\n",
           PROGRAM_NAME_STR);
    exit(exit_code);
}

static void
options_init(options_t *const opts)
{
    opts->help = false;
    opts->sector_size = OPTIONS_DEFAULT_SECTOR_SIZE;
    opts->buffer_size = OPTIONS_DEFAULT_BUFFER_SIZE;

    opts->in_file_path = NULL;
    opts->ref_file_path = NULL;
    opts->out_file_path = NULL;
}

static int
options_parse_unsigned(const char *const arg, uint32_t *const value)
{
    char *end;

    errno = 0;

    *value = strtoul(arg, &end, 0);

    return ((*end != '\0') || (errno != 0)) ? -1 : 0;
}
