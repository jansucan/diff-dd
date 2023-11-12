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

#include "backup.h"
#include "options.h"
#include "resources.h"
#include "restore.h"

#include <stdlib.h>

static void
clean_exit(resources_t *const res, int exit_code)
{
    resources_free(res);
    exit(exit_code);
}

int
main(int argc, char **argv)
{
    options_t opts;

    if (!options_parse(argc, argv, &opts)) {
        options_usage(1);
    } else if (options_is_operation(&opts, OPERATION_ID_HELP)) {
        options_usage(0);
    }

    resources_t res;

    if (resources_allocate(&opts, &res)) {
        clean_exit(&res, 1);
    } else if (options_is_operation(&opts, OPERATION_ID_BACKUP)) {
        clean_exit(&res, backup(options_get_for_backup(&opts),
                                resources_get_for_backup(&res)));
    } else {
        clean_exit(&res, restore(options_get_for_restore(&opts),
                                 resources_get_for_restore(&res)));
    }
}
