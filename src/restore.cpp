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
#include "format_v2.h"

#include <filesystem>
#include <fstream>
#include <vector>

void
restore(const OptionsRestore &opts)
{
    std::fstream diff_stream;
    diff_stream.open(opts.getDiffFilePath(),
                     std::ifstream::in | std::ifstream::binary);
    if (!diff_stream) {
        throw RestoreError("cannot open diff file");
    }

    FormatV2::Reader diff_reader(diff_stream, opts.getBufferSize());

    std::fstream out_file;
    out_file.open(opts.getOutFilePath(),
                  std::ios::in | std::ios::out | std::ios::binary);
    if (!out_file) {
        throw RestoreError("cannot open output file");
    }

    for (;;) {
        const uint64_t offset{diff_reader.readOffset()};
        if (diff_reader.eof()) {
            break;
        }

        if (!out_file.seekp(offset, std::ios_base::beg)) {
            throw RestoreError("cannot seek in output file");
        }

        uint64_t size{diff_reader.readSize()};

        while (size > 0) {
            const FormatV2::RecordData rd{diff_reader.readRecordData(size)};
            if (rd.size == 0) {
                break;
            }

            if (!out_file.write(rd.data.get(), rd.size)) {
                throw RestoreError("cannot write to output file");
            }

            size -= rd.size;
        }

        if (size > 0) {
            throw RestoreError("cannot read all the data of the record");
        }
    }
}
