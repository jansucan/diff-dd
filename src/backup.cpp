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

#include <fstream>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    std::ifstream in_file;
    std::ifstream ref_file;
    std::ofstream out_file;

    std::unique_ptr<char[]> in_buffer;
    std::unique_ptr<char[]> ref_buffer;
    std::unique_ptr<char[]> out_buffer;

    size_t out_buffer_size;
} resources_backup_t;

static void check_files(const OptionsBackup &opts);
static void write_out_buffer(const char *const buffer, size_t size,
                             std::ofstream &file);

static resources_backup_t
resources_allocate_for_backup(const OptionsBackup &opts)
{
    resources_backup_t res;

    res.in_file.open(opts.getInFilePath(),
                     std::ifstream::in | std::ifstream::binary);
    if (!res.in_file) {
        throw BackupError("cannot open input file");
    }

    res.ref_file.open(opts.getRefFilePath(),
                      std::ifstream::in | std::ifstream::binary);
    if (!res.ref_file) {
        throw BackupError("cannot open reference file");
    }

    /* When backing up, the output file is truncated to hold the
     * new data
     */
    res.out_file.open(opts.getOutFilePath(), std::ifstream::out |
                                                 std::ifstream::trunc |
                                                 std::ifstream::binary);
    if (!res.out_file) {
        throw BackupError("cannot open output file");
    }

    /* The output buffer contains also the offsets */
    res.out_buffer_size =
        ((opts.getBufferSize() / opts.getSectorSize()) * sizeof(uint64_t)) +
        opts.getBufferSize();

    // TODO: separate function
    try {
        res.in_buffer = std::make_unique<char[]>(opts.getBufferSize());
    } catch (const std::bad_alloc &e) {
        throw BackupError("cannot allocate buffer for input file data");
    }

    try {
        res.ref_buffer = std::make_unique<char[]>(opts.getBufferSize());
    } catch (const std::bad_alloc &e) {
        throw BackupError("cannot allocate buffer for reference file data");
    }

    try {
        res.out_buffer = std::make_unique<char[]>(res.out_buffer_size);
    } catch (const std::bad_alloc &e) {
        throw BackupError("cannot allocate buffer for output file data");
    }

    return res;
}

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

static void
write_out_buffer(const char *const buffer, size_t size, std::ofstream &file)
{
    file.write(buffer, size);

    if (!file) {
        throw BackupError("cannot write to output file");
    }
}

static size_t
read_sectors(std::ifstream &file, char *const buffer, uint32_t buffer_size,
             uint32_t sector_size)
{
    file.read(buffer, buffer_size);
    const size_t bytes_read = file.gcount();

    if (!file.good() && !file.eof()) {
        throw BackupError("cannot read from file");
    } else if ((bytes_read % sector_size) != 0) {
        throw BackupError(
            "data read from input file is not multiple of sector size");
    } else {
        return (bytes_read / sector_size);
    }
}

void
backup(const OptionsBackup &opts)
{
    resources_backup_t res{resources_allocate_for_backup(opts)};
    check_files(opts);

    size_t out_buffer_index = 0;
    uint64_t input_file_offset = 0;

    for (;;) {
        /* Read the sectors from the input and reference files into the buffers
         */
        const size_t in_sectors_read =
            read_sectors(res.in_file, res.in_buffer.get(), opts.getBufferSize(),
                         opts.getSectorSize());
        const size_t ref_sectors_read =
            read_sectors(res.ref_file, res.ref_buffer.get(),
                         opts.getBufferSize(), opts.getSectorSize());

        if ((in_sectors_read == 0) || (ref_sectors_read == 0)) {
            break;
        } else if (in_sectors_read != ref_sectors_read) {
            throw BackupError(
                "cannot read equal amount of sectors from the input files");
        }

        /* Process the sectors in the buffers */
        for (size_t sector = 0; sector < in_sectors_read; ++sector) {
            const size_t buffer_offset = sector * opts.getSectorSize();

            if (memcmp(res.in_buffer.get() + buffer_offset,
                       res.ref_buffer.get() + buffer_offset,
                       opts.getSectorSize()) != 0) {
                /* Backup the changed sector */
                if (out_buffer_index >= res.out_buffer_size) {
                    /* The output buffer is full. Write it to the output file */
                    write_out_buffer(res.out_buffer.get(), out_buffer_index,
                                     res.out_file);
                    out_buffer_index = 0;
                }
                /* Write the next backup record */
                const uint64_t o = htole64(input_file_offset);
                memcpy(res.out_buffer.get() + out_buffer_index, (void *)&o,
                       sizeof(o));
                out_buffer_index += sizeof(o);

                memcpy(res.out_buffer.get() + out_buffer_index,
                       res.in_buffer.get() + buffer_offset,
                       opts.getSectorSize());
                out_buffer_index += opts.getSectorSize();
            }

            input_file_offset += opts.getSectorSize();
        }
    }

    /* Write out the output buffer */
    if (out_buffer_index > 0) {
        write_out_buffer(res.out_buffer.get(), out_buffer_index, res.out_file);
    }
}
