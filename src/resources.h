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

#ifndef RESOURCES_H
#define RESOURCES_H

#include "operation_id.h"
#include "options.h"

#include <stdio.h>

typedef struct {
    FILE *in_file;
    FILE *ref_file;
    FILE *out_file;

    char *in_buffer;
    char *ref_buffer;
    char *out_buffer;

    size_t out_buffer_size;
} resources_backup_t;

typedef struct {
    FILE *in_file;
    FILE *out_file;

    char *in_buffer;
    char *out_buffer;

    size_t in_sector_size;
    size_t in_buffer_size;
} resources_restore_t;

typedef struct {
    operation_id_t operation_id;

    union {
        resources_backup_t backup;
        resources_restore_t restore;
    } res;
} resources_t;

int resources_allocate(const options_t *const opts, resources_t *const res);
const resources_backup_t *
resources_get_for_backup(const resources_t *const res);
const resources_restore_t *
resources_get_for_restore(const resources_t *const res);
void resources_free(resources_t *const res);

#endif /* RESOURCES_H */
