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
#include "buffered_file.h"

#include <filesystem>
#include <fstream>

static void
check_input_file(const OptionsRestore &opts)
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

    std::ifstream in_file;
    in_file.open(opts.getInFilePath(), std::ios::in | std::ios::binary);
    if (!in_file) {
        throw RestoreError("cannot open input file");
    }

    uint64_t prev_out_offset = 0;
    bool is_first_reading = true;

    /* Scan the input file and check  */
    for (;;) {
        uint64_t out_offset;
        /* Read the next offset */
        in_file.read(reinterpret_cast<char *>(&out_offset), sizeof(out_offset));

        if (in_file.eof() && in_file.fail() && !in_file.bad()) {
            break;
        } else if (!in_file.good() && !in_file.eof()) {
            throw RestoreError("cannot read from file");
        }
        out_offset = le64toh(out_offset);

        if (!is_first_reading && (out_offset <= prev_out_offset)) {
            throw RestoreError(
                "a sector offset points behind the previous offset");
        } else if ((out_offset + opts.getSectorSize()) > out_size) {
            throw RestoreError(
                "a sector offset points past the end of the output file");
        } else if (!in_file.seekg(opts.getSectorSize(), std::ios_base::cur)) {
            throw RestoreError("cannot seek in input file");
        }

        is_first_reading = false;
        prev_out_offset = out_offset;
    }

    /* The input file must be read completely */
    char c;
    in_file.read(&c, 1);
    if (in_file.gcount() != 0) {
        throw RestoreError("input file is not valid");
    }
    in_file.clear();

    in_file.close();
}

void
restore(const OptionsRestore &opts)
{
    check_input_file(opts);

    BufferedFileReader in_file(opts.getInFilePath(), opts.getBufferSize());

    std::fstream out_file;
    out_file.open(opts.getOutFilePath(),
                  std::ios::in | std::ios::out | std::ios::binary);
    if (!out_file) {
        throw RestoreError("cannot open output file");
    }

    const size_t in_buffer_size = sizeof(uint64_t) + opts.getSectorSize();
    std::unique_ptr<char[]> in_buffer;
    try {
        in_buffer = std::make_unique<char[]>(in_buffer_size);
    } catch (const std::bad_alloc &e) {
        throw RestoreError("cannot allocate sector buffer for input file data");
    }

    /* Restore data from the differential image */
    size_t in_read_size = {0};
    for (;;) {
        in_read_size = in_file.read(in_buffer.get(), in_buffer_size);

        if (in_read_size == 0) {
            break;
        } else if (in_read_size != in_buffer_size) {
            throw RestoreError("cannot read from input file");
        }

        const uint64_t out_offset =
            le64toh(*reinterpret_cast<uint64_t *>(in_buffer.get()));

        if (!out_file.seekp(out_offset, std::ios_base::beg)) {
            throw RestoreError("cannot seek in output file");
        }

        if (!out_file.write(reinterpret_cast<char *>(in_buffer.get()) +
                                sizeof(uint64_t),
                            opts.getSectorSize())) {
            throw RestoreError("cannot write to output file");
        }
    }

    out_file.close();

    /* The input file must be read completely */
    char c;
    in_read_size = in_file.read(&c, 1);
    if (in_read_size != 0) {
        throw RestoreError("input file is not valid");
    }
}
