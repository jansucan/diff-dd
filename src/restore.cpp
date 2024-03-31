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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <filesystem>
#include <fstream>
#include <iostream>

typedef struct {
    std::ifstream in_file;
    std::fstream out_file;

    std::unique_ptr<char[]> in_buffer;
    std::unique_ptr<char[]> out_buffer;

    size_t in_sector_size;
    size_t in_buffer_size;
} resources_restore_t;

static resources_restore_t
resources_allocate_for_restore(const OptionsRestore &opts)
{
    resources_restore_t res;

    res.in_file.open(opts.getInFilePath(), std::ios::in | std::ios::binary);
    if (!res.in_file) {
        throw RestoreError("cannot open input file");
    }

    /* When restoring, the file must be opened for writing and not
     * truncated
     */
    res.out_file.open(opts.getOutFilePath(),
                      std::ios::in | std::ios::out | std::ios::binary);
    if (!res.out_file) {
        throw RestoreError("cannot open output file");
    }

    /* Allocate the buffer for data from the input file */
    /* The input buffer contains also the offsets */
    res.in_sector_size = sizeof(uint64_t) + opts.getSectorSize();
    const size_t in_buffer_sector_count =
        opts.getBufferSize() / res.in_sector_size;
    res.in_buffer_size = in_buffer_sector_count * res.in_sector_size;

    try {
        res.in_buffer = std::make_unique<char[]>(res.in_buffer_size);
    } catch (const std::bad_alloc &e) {
        throw RestoreError("cannot allocate buffer for input file data");
    }

    return res;
}

static void
check_input_file(resources_restore_t &res, const OptionsRestore &opts)
{
    size_t in_size{0};
    try {
        in_size = std::filesystem::file_size(opts.getInFilePath());
    } catch (const std::exception &e) {
        throw RestoreError("cannot get size of input file: " +
                           std::string(e.what()));
    }

    if (in_size == 0) {
        throw RestoreError("input file is empty");
    } else if ((in_size % (sizeof(uint64_t) + opts.getSectorSize())) != 0) {
        /* The input file must hold equally sized sectors and the
         * offset of each of them
         */
        throw RestoreError(
            "input file has size that cannot contain valid diff data");
    }

    size_t out_size{0};
    try {
        out_size = std::filesystem::file_size(opts.getOutFilePath());
    } catch (const std::exception &e) {
        throw RestoreError("cannot get size of output file: " +
                           std::string(e.what()));
    }

    uint64_t prev_out_offset = 0;
    bool is_first_reading = true;

    /* Scan the input file and check  */
    for (;;) {
        uint64_t out_offset;
        /* Read the next offset */
        res.in_file.read(reinterpret_cast<char *>(&out_offset),
                         sizeof(out_offset));

        if (res.in_file.eof() && res.in_file.fail() && !res.in_file.bad()) {
            break;
        } else if (!res.in_file.good() && !res.in_file.eof()) {
            throw RestoreError("cannot read from file");
        }
        out_offset = le64toh(out_offset);

        if (!is_first_reading && (out_offset <= prev_out_offset)) {
            throw RestoreError(
                "a sector offset points behind the previous offset");
        } else if ((out_offset + opts.getSectorSize()) > out_size) {
            throw RestoreError(
                "a sector offset points past the end of the output file");
        } else if (!res.in_file.seekg(opts.getSectorSize(),
                                      std::ios_base::cur)) {
            throw RestoreError("cannot seek in input file");
        }

        is_first_reading = false;
        prev_out_offset = out_offset;
    }

    /* The input file must be read completely */
    char c;
    res.in_file.read(&c, 1);
    if (res.in_file.gcount() != 0) {
        throw RestoreError("input file is not valid");
    }
    res.in_file.clear();

    /* The file must be prepared for the restoring */
    if (!res.in_file.seekg(0, std::ios_base::beg)) {
        throw RestoreError("cannot seek in input file");
    }
}

static size_t
read_sectors(std::ifstream &file, char *const buffer, uint32_t buffer_size,
             uint32_t sector_size)
{
    file.read(buffer, buffer_size);
    const size_t bytes_read = file.gcount();

    if (!file.good() && !file.eof()) {
        throw RestoreError("cannot read from file");
    } else if ((bytes_read % sector_size) != 0) {
        throw RestoreError(
            "data read from input file is not multiple of sector size");
    } else {
        return (bytes_read / sector_size);
    }
}

void
restore(const OptionsRestore &opts)
{
    resources_restore_t res{resources_allocate_for_restore(opts)};

    check_input_file(res, opts);

    /* Restore data from the differential image */
    for (;;) {
        /* Read data of the offset and the next sector */
        const size_t in_sectors_read =
            read_sectors(res.in_file, res.in_buffer.get(), res.in_buffer_size,
                         res.in_sector_size);

        if (in_sectors_read == 0) {
            break;
        }

        char *in_buffer = res.in_buffer.get();

        for (size_t s = 0; s < in_sectors_read; ++s) {
            const uint64_t out_offset = le64toh(*((uint64_t *)in_buffer));
            in_buffer += sizeof(uint64_t);

            if (!res.out_file.seekp(out_offset, std::ios_base::beg)) {
                throw RestoreError("cannot seek in output file");
            }

            if (!res.out_file.write(in_buffer, opts.getSectorSize())) {
                throw RestoreError("cannot write to output file");
            }

            in_buffer += opts.getSectorSize();
        }
    }

    /* The input file must be read completely */
    char c;
    res.in_file.read(&c, 1);
    if (res.in_file.gcount() != 0) {
        throw RestoreError("input file is not valid");
    }
    res.in_file.clear();
}
