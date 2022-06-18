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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "operation_id.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t sector_size;
    uint32_t buffer_size;
    const char *in_file_path;
    const char *ref_file_path;
    const char *out_file_path;
} options_backup_t;

typedef struct {
    uint32_t sector_size;
    uint32_t buffer_size;
    const char *in_file_path;
    const char *out_file_path;
} options_restore_t;

typedef struct {
    operation_id_t operation_id;

    union {
        options_backup_t backup;
        options_restore_t restore;
    } op;
} options_t;

bool options_parse(int argc, char **argv, options_t *const opts);
void options_usage(int exit_code);
bool options_is_operation(const options_t *const opts,
                          operation_id_t operation_id);
const options_backup_t *options_get_for_backup(const options_t *const opts);
const options_restore_t *options_get_for_restore(const options_t *const opts);

#endif /* OPTIONS_H */
