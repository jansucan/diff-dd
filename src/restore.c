/* Copyright 2021 Ján Sučan <jan@jansucan.com>
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

#include "restore.h"

#include "file.h"
#include "print.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
is_input_file_valid(const resources_restore_t *const res, uint32_t sector_size)
{
    bool in_size_ok = false;
    const size_t in_size = file_size(res->in_file, &in_size_ok);

    if (!in_size_ok) {
        print_error("cannot get size of input file: %s", strerror(errno));
        return false;
    } else if (in_size == 0) {
        print_error("input file is empty");
        return false;
    } else if ((in_size % (sizeof(uint64_t) + sector_size)) != 0) {
        /* The input file must hold equally sized sectors and the
         * offset of each of them
         */
        print_error("input file has size that cannot contain valid diff data");
        return false;
    }

    bool out_size_ok = false;
    const size_t out_size = file_size(res->out_file, &out_size_ok);

    if (!out_size_ok) {
        print_error("cannot get size of output file: %s", strerror(errno));
        return 1;
    }

    uint64_t prev_out_offset = 0;
    bool is_first_reading = true;

    /* Scan the input file and check  */
    for (;;) {
        uint64_t out_offset;
        /* Read the next offset */
        const size_t in_read =
            fread(&out_offset, sizeof(out_offset), 1U, res->in_file);
        out_offset = le64toh(out_offset);

        if (feof(res->in_file)) {
            break;
        } else if ((in_read != 1U) || ferror(res->in_file)) {
            print_error("cannot read from input file: %s", strerror(errno));
            return false;
        } else if (!is_first_reading && (out_offset <= prev_out_offset)) {
            print_error("a sector offset points behind the previous offset");
            return false;
        } else if ((out_offset + sector_size) > out_size) {
            print_error(
                "a sector offset points past the end of the output file");
            return false;
        } else if (fseek(res->in_file, sector_size, SEEK_CUR) != 0) {
            print_error("cannot seek in input file: %s", strerror(errno));
            return false;
        }

        is_first_reading = false;
        prev_out_offset = out_offset;
    }

    bool pos_ok = false;
    const size_t pos = file_tell(res->in_file, &pos_ok);

    if (!pos_ok) {
        print_error("cannot get position in the input file");
        return false;
    } else if (pos != in_size) {
        /* The input file must be read completely */
        print_error("input file is not valid");
        return false;
    } else if (fseek(res->in_file, 0L, SEEK_SET) != 0) {
        /* The file must be prepared for the restoring */
        print_error("cannot seek in input file: %s", strerror(errno));
        return false;
    }

    return true;
}

int
restore(const options_restore_t *const opts,
        const resources_restore_t *const res)
{
    /* Check validity of the input file */
    if (!is_input_file_valid(res, opts->sector_size)) {
        return 1;
    }

    bool in_size_ok = false;
    const size_t in_size = file_size(res->in_file, &in_size_ok);

    if (!in_size_ok) {
        print_error("cannot get size of input file: %s", strerror(errno));
        return 1;
    }

    /* Restore data from the differential image */
    for (;;) {

        /* Read data of the offset and the next sector */
        const size_t in_sectors_read =
            file_read_sectors(res->in_file, res->in_buffer, res->in_buffer_size,
                              res->in_sector_size);

        if (in_sectors_read == 0) {
            break;
        }

        char *in_buffer = res->in_buffer;

        for (size_t s = 0; s < in_sectors_read; ++s) {
            const uint64_t out_offset = le64toh(*((uint64_t *)in_buffer));
            in_buffer += sizeof(uint64_t);

            if (fseek(res->out_file, out_offset, SEEK_SET) != 0) {
                print_error("cannot seek in output file: %s", strerror(errno));
                return 1;
            }

            const size_t out_written =
                fwrite(in_buffer, opts->sector_size, 1U, res->out_file);
            in_buffer += opts->sector_size;

            if (out_written != 1U) {
                print_error("cannot write to output file: %s", strerror(errno));
                return 1;
            }
        }
    }

    /* The input file must be read completely */
    bool pos_ok = false;
    const size_t pos = file_tell(res->in_file, &pos_ok);

    if (!pos_ok) {
        print_error("cannot get position in the input file");
        return 1;
    } else if (pos != in_size) {
        print_error("input file is not valid");
        return 1;
    }

    return 0;
}
