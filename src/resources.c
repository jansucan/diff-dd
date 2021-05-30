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

#include <stdio.h>
#include <stdlib.h>

void
resources_init(resources_t *const res)
{
    res->in_file = NULL;
    res->ref_file = NULL;
    res->out_file = NULL;
    res->in_buffer = NULL;
    res->ref_buffer = NULL;
    res->out_buffer = NULL;
}

void
resources_free(resources_t *const res)
{
    if (res->in_file != NULL) {
        fclose(res->in_file);
        res->in_file = NULL;
    }
    if (res->ref_file != NULL) {
        fclose(res->ref_file);
        res->ref_file = NULL;
    }
    if (res->out_file != NULL) {
        fclose(res->out_file);
        res->out_file = NULL;
    }
    if (res->in_buffer != NULL) {
        free(res->in_buffer);
        res->in_buffer = NULL;
    }
    if (res->ref_buffer != NULL) {
        free(res->ref_buffer);
        res->ref_buffer = NULL;
    }
    if (res->out_buffer != NULL) {
        free(res->out_buffer);
        res->out_buffer = NULL;
    }
}
