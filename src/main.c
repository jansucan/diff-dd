#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backup.h"
#include "file.h"
#include "options.h"
#include "print.h"
#include "resources.h"
#include "restore.h"

static void
clean_exit(resources_t *const res, int exit_code)
{
    resources_free(res);
    exit(exit_code);
}

static int
open_files(const options_t *const opts, resources_t *const res,
           bool is_action_backup)
{
    /* Open the input file */
    if ((opts->in_file_path != NULL) &&
        ((res->in_file = fopen(opts->in_file_path, "r")) == NULL)) {
        print_error("cannot open input file: %s", strerror(errno));
        return 1;
    }

    /* Open the reference file */
    if ((res->ref_file = fopen(opts->ref_file_path, "r")) == NULL) {
        print_error("cannot open reference file: %s", strerror(errno));
        return 1;
    }

    /* Open the output file
     *
     * When restoring, the file must be opened for writing and not
     * truncated
     */
    char out_mode[] = "r+";

    if (is_action_backup) {
        /* When backing up, the output file is truncated to hold the
         * new data
         */
        out_mode[0] = 'w';
        out_mode[1] = '\0';
    }

    if ((res->out_file = fopen(opts->out_file_path, out_mode)) == NULL) {
        print_error("cannot open output file: %s", strerror(errno));
        return 1;
    }

    return 0;
}

int
main(int argc, char **argv)
{
    options_t opts;

    if (options_parse(argc, argv, &opts) || opts.help) {
        options_usage(1);
    }

    const bool is_action_backup = (opts.in_file_path != NULL);
    resources_t res;

    resources_init(&res);

    if (open_files(&opts, &res, is_action_backup) != 0) {
        clean_exit(&res, 1);
    } else if (is_action_backup) {
        clean_exit(&res, backup(&opts, &res));
    } else {
        clean_exit(&res, restore(&opts, &res));
    }
}
