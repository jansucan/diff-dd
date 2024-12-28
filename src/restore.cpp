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
check_diff_file(const OptionsRestore &opts)
{
    size_t diff_size{0};
    try {
        diff_size = std::filesystem::file_size(opts.getDiffFilePath());
    } catch (const std::exception &e) {
        throw RestoreError("cannot get size of diff file: " +
                           std::string(e.what()));
    }

    if (diff_size == 0) {
        throw RestoreError("diff file is empty");
    } else if ((diff_size % (sizeof(uint64_t) + opts.getSectorSize())) != 0) {
        /* The diff file must hold equally sized sectors and the
         * offset of each of them
         */
        throw RestoreError(
            "diff file has size that cannot contain valid diff data");
    }

    size_t out_size{0};
    try {
        out_size = std::filesystem::file_size(opts.getOutFilePath());
    } catch (const std::exception &e) {
        throw RestoreError("cannot get size of output file: " +
                           std::string(e.what()));
    }

    std::ifstream diff_file;
    diff_file.open(opts.getDiffFilePath(), std::ios::in | std::ios::binary);
    if (!diff_file) {
        throw RestoreError("cannot open diff file");
    }

    uint64_t prev_out_offset = 0;
    bool is_first_reading = true;

    /* Scan the diff file and check  */
    for (;;) {
        uint64_t out_offset;
        /* Read the next offset */
        diff_file.read(reinterpret_cast<char *>(&out_offset),
                       sizeof(out_offset));

        if (diff_file.eof() && diff_file.fail() && !diff_file.bad()) {
            break;
        } else if (!diff_file.good() && !diff_file.eof()) {
            throw RestoreError("cannot read from file");
        }
        out_offset = le64toh(out_offset);

        if (!is_first_reading && (out_offset <= prev_out_offset)) {
            throw RestoreError(
                "a sector offset points behind the previous offset");
        } else if ((out_offset + opts.getSectorSize()) > out_size) {
            throw RestoreError(
                "a sector offset points past the end of the output file");
        } else if (!diff_file.seekg(opts.getSectorSize(), std::ios_base::cur)) {
            throw RestoreError("cannot seek in diff file");
        }

        is_first_reading = false;
        prev_out_offset = out_offset;
    }

    /* The diff file must be read completely */
    char c;
    diff_file.read(&c, 1);
    if (diff_file.gcount() != 0) {
        throw RestoreError("diff file is not valid");
    }
    diff_file.clear();

    diff_file.close();
}

void
restore(const OptionsRestore &opts)
{
    check_diff_file(opts);

    BufferedFileReader diff_file(opts.getDiffFilePath(), opts.getBufferSize());

    std::fstream out_file;
    out_file.open(opts.getOutFilePath(),
                  std::ios::in | std::ios::out | std::ios::binary);
    if (!out_file) {
        throw RestoreError("cannot open output file");
    }

    const size_t diff_buffer_size = sizeof(uint64_t) + opts.getSectorSize();
    std::unique_ptr<char[]> diff_buffer;
    try {
        diff_buffer = std::make_unique<char[]>(diff_buffer_size);
    } catch (const std::bad_alloc &e) {
        throw RestoreError("cannot allocate sector buffer for diff file data");
    }

    /* Restore data from the differential image */
    size_t diff_read_size = {0};
    for (;;) {
        diff_read_size = diff_file.read(diff_buffer.get(), diff_buffer_size);

        if (diff_read_size == 0) {
            break;
        } else if (diff_read_size != diff_buffer_size) {
            throw RestoreError("cannot read from diff file");
        }

        const uint64_t out_offset =
            le64toh(*reinterpret_cast<uint64_t *>(diff_buffer.get()));

        if (!out_file.seekp(out_offset, std::ios_base::beg)) {
            throw RestoreError("cannot seek in output file");
        }

        if (!out_file.write(reinterpret_cast<char *>(diff_buffer.get()) +
                                sizeof(uint64_t),
                            opts.getSectorSize())) {
            throw RestoreError("cannot write to output file");
        }
    }

    out_file.close();

    /* The diff file must be read completely */
    char c;
    diff_read_size = diff_file.read(&c, 1);
    if (diff_read_size != 0) {
        throw RestoreError("diff file is not valid");
    }
}
