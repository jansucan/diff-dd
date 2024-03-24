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

#include "options.h"

#include "print.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* This header file is automatically generated at build time from the Makefile
 */
#include "program_info.h"

#define OPTIONS_MAX_OPERATION_NAME_LENGTH 8
#define OPTIONS_DEFAULT_SECTOR_SIZE 512
#define OPTIONS_DEFAULT_BUFFER_SIZE (4 * 1024 * 1024)

struct common_options {
    uint32_t sector_size;
    uint32_t buffer_size;
};

static int options_parse_unsigned(const char *const arg, uint32_t *const value);
static void options_parse_operation(const char *const argv,
                                    options_t *const opts);
static bool options_parse_common(int *const argc, char ***const argv,
                                 struct common_options *const opts);
static bool options_parse_backup(int *const argc, char ***const argv,
                                 options_t *const opts);
static bool options_parse_restore(int *const argc, char ***const argv,
                                  options_t *const opts);
static const char *next_arg(char ***const argv);

bool
options_parse(int argc, char **argv, options_t *const opts)
{
    // Skip the executable name
    --argc;
    ++argv;

    options_parse_operation(argv[0], opts);

    if (options_is_operation(opts, OPERATION_ID_UNKNOWN)) {
        return false;
    }

    if (options_is_operation(opts, OPERATION_ID_BACKUP)) {
        if (!options_parse_backup(&argc, &argv, opts)) {
            return false;
        }
    } else if (options_is_operation(opts, OPERATION_ID_RESTORE)) {
        if (!options_parse_restore(&argc, &argv, opts)) {
            return false;
        }
    }
    return true;
}

void
options_usage(int exit_code)
{
    printf("Usage: %s backup [-s SECTOR_SIZE] [-b BUFFER_SIZE] INFILE REFFILE "
           "OUTFILE\n",
           PROGRAM_NAME_STR);
    printf("   Or: %s restore [-s SECTOR_SIZE] [-b BUFFER_SIZE] REFFILE "
           "OUTFILE\n",
           PROGRAM_NAME_STR);
    printf("   Or: %s help\n", PROGRAM_NAME_STR);
    exit(exit_code);
}

bool
options_is_operation(const options_t *const opts, operation_id_t operation_id)
{
    return (opts->operation_id == operation_id);
}

const options_backup_t *
options_get_for_backup(const options_t *const opts)
{
    return &(opts->op.backup);
}

const options_restore_t *
options_get_for_restore(const options_t *const opts)
{
    return &(opts->op.restore);
}

static int
options_parse_unsigned(const char *const arg, uint32_t *const value)
{
    char *end;

    errno = 0;

    *value = strtoul(arg, &end, 0);

    return ((*end != '\0') || (errno != 0)) ? -1 : 0;
}

static void
options_parse_operation(const char *const op_name, options_t *const opts)
{
    if (op_name == NULL) {
        opts->operation_id = OPERATION_ID_UNKNOWN;
    } else if (strncmp(op_name, "help", OPTIONS_MAX_OPERATION_NAME_LENGTH) ==
               0) {
        opts->operation_id = OPERATION_ID_HELP;
    } else if (strncmp(op_name, "backup", OPTIONS_MAX_OPERATION_NAME_LENGTH) ==
               0) {
        opts->operation_id = OPERATION_ID_BACKUP;
    } else if (strncmp(op_name, "restore", OPTIONS_MAX_OPERATION_NAME_LENGTH) ==
               0) {
        opts->operation_id = OPERATION_ID_RESTORE;
    } else {
        opts->operation_id = OPERATION_ID_UNKNOWN;
    }
}

static void
options_init_common(struct common_options *const opts)
{
    opts->sector_size = OPTIONS_DEFAULT_SECTOR_SIZE;
    opts->buffer_size = OPTIONS_DEFAULT_BUFFER_SIZE;
}

static bool
options_parse_common(int *const argc, char ***const argv,
                     struct common_options *const opts)
{
    int ch;
    char *arg_sector_size = NULL;
    char *arg_buffer_size = NULL;

    while ((ch = getopt(*argc, *argv, ":b:s:")) != -1) {
        switch (ch) {
        case 'b':
            arg_buffer_size = optarg;
            break;

        case 's':
            arg_sector_size = optarg;
            break;

        case ':':
            print_error("missing argument for option '-%c'", optopt);
            return false;
        default:
            print_error("unknown option '-%c'", optopt);
            return false;
        }
    }

    *argc -= optind;
    *argv += optind;

    /* Convert numbers in the arguments */
    if ((arg_sector_size != NULL) &&
        options_parse_unsigned(arg_sector_size, &(opts->sector_size))) {
        print_error("incorrect sector size");
        return false;
    } else if ((arg_buffer_size != NULL) &&
               options_parse_unsigned(arg_buffer_size, &(opts->buffer_size))) {
        print_error("incorrect buffer size");
        return false;
    } else if (opts->sector_size == 0) {
        print_error("sector size cannot be 0");
        return false;
    } else if (opts->buffer_size == 0) {
        print_error("buffer size cannot be 0");
        return false;
    } else if (opts->sector_size > opts->buffer_size) {
        print_error("sector size cannot larger than buffer size");
        return false;
    } else if ((opts->buffer_size % opts->sector_size) != 0) {
        print_error("buffer size is not multiple of sector size");
        return false;
    }

    return true;
}

static bool
options_parse_backup(int *const argc, char ***const argv, options_t *const opts)
{
    struct common_options common_opts;
    options_init_common(&common_opts);
    if (!options_parse_common(argc, argv, &common_opts)) {
        return false;
    }

    if (*argc < 3) {
        print_error("missing arguments");
        return false;
    } else if (*argc > 3) {
        print_error("too many arguments");
        return false;
    } else {
        opts->op.backup.sector_size = common_opts.sector_size;
        opts->op.backup.buffer_size = common_opts.buffer_size;
        opts->op.backup.in_file_path = next_arg(argv);
        opts->op.backup.ref_file_path = next_arg(argv);
        opts->op.backup.out_file_path = next_arg(argv);
    }

    return true;
}

static bool
options_parse_restore(int *const argc, char ***const argv,
                      options_t *const opts)
{
    struct common_options common_opts;
    options_init_common(&common_opts);
    if (!options_parse_common(argc, argv, &common_opts)) {
        return false;
    }

    if (*argc < 2) {
        print_error("missing arguments");
        return false;
    } else if (*argc > 2) {
        print_error("too many arguments");
        return false;
    } else {
        opts->op.restore.sector_size = common_opts.sector_size;
        opts->op.restore.buffer_size = common_opts.buffer_size;
        opts->op.restore.in_file_path = next_arg(argv);
        opts->op.restore.out_file_path = next_arg(argv);
    }

    return true;
}

static const char *
next_arg(char ***const argv)
{
    const char *arg = **argv;
    ++(*argv);
    return arg;
}
