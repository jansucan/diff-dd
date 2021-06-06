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

#include "file.h"

#include "print.h"

#include <errno.h>
#include <string.h>

long
file_size(FILE *const file)
{
    fpos_t p;

    if ((fgetpos(file, &p) != 0) || (fseek(file, 0L, SEEK_END) != 0)) {
        return -1;
    }

    const long size = ftell(file);

    if (fsetpos(file, &p) != 0) {
        return -1;
    }

    return size;
}

size_t
file_read_sectors(FILE *const file, char *const buffer, uint32_t buffer_size,
                  uint32_t sector_size)
{
    const size_t bytes_read = fread(buffer, 1U, buffer_size, file);

    if (ferror(file)) {
        print_error("cannot read from file: %s", strerror(errno));
        return 0;
    } else if ((bytes_read % sector_size) != 0) {
        print_error("data read from input file is not multiple of sector size");
        return 0;
    } else {
        return (bytes_read / sector_size);
    }
}
