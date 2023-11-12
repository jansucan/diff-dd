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

#include "resources.h"

#include "print.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
resources_init_for_backup(resources_backup_t *const res)
{
    res->in_file = NULL;
    res->ref_file = NULL;
    res->out_file = NULL;

    res->in_buffer = NULL;
    res->ref_buffer = NULL;
    res->out_buffer = NULL;
}

static int
resources_allocate_for_backup(const options_backup_t *const opts,
                              resources_backup_t *const res)
{
    if ((res->in_file = fopen(opts->in_file_path, "r")) == NULL) {
        print_error("cannot open input file: %s", strerror(errno));
        return 1;
    }

    if ((res->ref_file = fopen(opts->ref_file_path, "r")) == NULL) {
        print_error("cannot open reference file: %s", strerror(errno));
        return 1;
    }

    /* When backing up, the output file is truncated to hold the
     * new data
     */
    if ((res->out_file = fopen(opts->out_file_path, "w+")) == NULL) {
        print_error("cannot open output file: %s", strerror(errno));
        return 1;
    }

    /* The output buffer contains also the offsets */
    res->out_buffer_size =
        ((opts->buffer_size / opts->sector_size) * sizeof(uint64_t)) +
        opts->buffer_size;

    // TODO: separate function
    if ((res->in_buffer = (char *)malloc(opts->buffer_size)) == NULL) {
        print_error("cannot allocate buffer for input file data");
        return 1;
    } else if ((res->ref_buffer = (char *)malloc(opts->buffer_size)) == NULL) {
        print_error("cannot allocate buffer for reference file data");
        return 1;
    } else if ((res->out_buffer = (char *)malloc(res->out_buffer_size)) ==
               NULL) {
        print_error("cannot allocate buffer for output file data");
        return 1;
    }

    return 0;
}

static void
resources_init_for_restore(resources_restore_t *const res)
{
    res->in_file = NULL;
    res->out_file = NULL;

    res->in_buffer = NULL;
    res->out_buffer = NULL;
}

static int
resources_allocate_for_restore(const options_restore_t *const opts,
                               resources_restore_t *const res)
{
    if ((res->in_file = fopen(opts->in_file_path, "r")) == NULL) {
        print_error("cannot open input file: %s", strerror(errno));
        return 1;
    }

    /* When restoring, the file must be opened for writing and not
     * truncated
     */
    if ((res->out_file = fopen(opts->out_file_path, "r+")) == NULL) {
        print_error("cannot open output file: %s", strerror(errno));
        return 1;
    }

    /* Allocate the buffer for data from the input file */
    /* The input buffer contains also the offsets */
    res->in_sector_size = sizeof(uint64_t) + opts->sector_size;
    const size_t in_buffer_sector_count =
        opts->buffer_size / res->in_sector_size;
    res->in_buffer_size = in_buffer_sector_count * res->in_sector_size;

    if ((res->in_buffer = (char *)malloc(res->in_buffer_size)) == NULL) {
        print_error("cannot allocate buffer for input file data");
        return 1;
    }

    return 0;
}

static void
resources_close_file(FILE **const file)
{
    if (*file != NULL) {
        fclose(*file);
        *file = NULL;
    }
}

static void
resources_close_buffer(char **const buffer)
{
    if (*buffer != NULL) {
        free(*buffer);
        *buffer = NULL;
    }
}

static void
resources_free_backup(resources_backup_t *const res)
{
    resources_close_file(&(res->in_file));
    resources_close_file(&(res->ref_file));
    resources_close_file(&(res->out_file));

    resources_close_buffer(&(res->in_buffer));
    resources_close_buffer(&(res->ref_buffer));
    resources_close_buffer(&(res->out_buffer));
}

static void
resources_free_restore(resources_restore_t *const res)
{
    resources_close_file(&(res->in_file));
    resources_close_file(&(res->out_file));

    resources_close_buffer(&(res->in_buffer));
    resources_close_buffer(&(res->out_buffer));
}

int
resources_allocate(const options_t *const opts, resources_t *const res)
{
    res->operation_id = opts->operation_id;

    if (res->operation_id == OPERATION_ID_BACKUP) {
        resources_init_for_backup(&(res->res.backup));
        return resources_allocate_for_backup(&(opts->op.backup),
                                             &(res->res.backup));
    } else if (res->operation_id == OPERATION_ID_RESTORE) {
        resources_init_for_restore(&(res->res.restore));
        return resources_allocate_for_restore(&(opts->op.restore),
                                              &(res->res.restore));
    }

    return 0;
}

const resources_backup_t *
resources_get_for_backup(const resources_t *const res)
{
    return &(res->res.backup);
}

const resources_restore_t *
resources_get_for_restore(const resources_t *const res)
{
    return &(res->res.restore);
}

void
resources_free(resources_t *const res)
{
    if (res->operation_id == OPERATION_ID_BACKUP) {
        resources_free_backup(&(res->res.backup));
    } else if (res->operation_id == OPERATION_ID_RESTORE) {
        resources_free_restore(&(res->res.restore));
    }

    res->operation_id = OPERATION_ID_UNKNOWN;
}
