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

#include "backup.h"
#include "buffered_file.h"

#include <cstring>
#include <fstream>

static void check_files(const OptionsBackup &opts);

static void
check_files(const OptionsBackup &opts)
{
    size_t in_size{0};
    try {
        in_size = std::filesystem::file_size(opts.getInFilePath());
    } catch (const std::exception &e) {
        throw BackupError("cannot get size of input file: " +
                          std::string(e.what()));
    }

    size_t ref_size{0};
    try {
        ref_size = std::filesystem::file_size(opts.getRefFilePath());
    } catch (const std::exception &e) {
        throw BackupError("cannot get size of reference file: " +
                          std::string(e.what()));
    }

    /* Check sizes of the input file and the reference file */
    if (in_size != ref_size) {
        throw BackupError("input file and reference file differ in size");
    } else if ((in_size % opts.getSectorSize()) != 0) {
        throw BackupError(
            "size of input file and reference file is not multiple of " +
            std::to_string(opts.getSectorSize()));
    }
}

void
backup(const OptionsBackup &opts)
{
    check_files(opts);

    BufferedFileReader in_file(opts.getInFilePath(), opts.getBufferSize());
    BufferedFileReader ref_file(opts.getRefFilePath(), opts.getBufferSize());
    BufferedFileWriter out_file(opts.getOutFilePath(), opts.getBufferSize());

    std::unique_ptr<char[]> in_buffer;
    try {
        in_buffer = std::make_unique<char[]>(opts.getSectorSize());
    } catch (const std::bad_alloc &e) {
        throw BackupError("cannot allocate sector buffer for input file data");
    }

    std::unique_ptr<char[]> ref_buffer;
    try {
        ref_buffer = std::make_unique<char[]>(opts.getSectorSize());
    } catch (const std::bad_alloc &e) {
        throw BackupError(
            "cannot allocate sector buffer for reference file data");
    }

    uint64_t input_file_offset{0};
    for (;;) {
        // Read sectors
        const size_t in_read_size =
            in_file.read(in_buffer.get(), opts.getSectorSize());
        const size_t ref_read_size =
            ref_file.read(ref_buffer.get(), opts.getSectorSize());

        if (in_read_size != ref_read_size) {
            throw BackupError(
                "cannot read equal amount of bytes from the input files");
        } else if (in_read_size == 0) {
            break;
        } else if (in_read_size != opts.getSectorSize()) {
            throw BackupError("cannot read full sectors from the input files");
        }

        // Check for difference
        const bool differ = (memcmp(in_buffer.get(), ref_buffer.get(),
                                    opts.getSectorSize()) != 0);
        if (differ) {
            // Backup sector
            uint64_t o = htole64(input_file_offset);
            out_file.write(reinterpret_cast<char *>(&o), sizeof(o));
            out_file.write(in_buffer.get(), opts.getSectorSize());
        }

        input_file_offset += opts.getSectorSize();
    }
}
