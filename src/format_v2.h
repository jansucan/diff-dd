/* Copyright 2024 Ján Sučan <jan@jansucan.com>
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

#pragma once

#include "buffered_stream.h"

#include <endian.h>

#include <vector>

namespace FormatV2
{

const std::string FileSignature{"diff-dd image"};
const uint8_t FileVersion{2};
const size_t RecordHeaderSize{sizeof(uint64_t) + sizeof(uint32_t)};

struct RecordData {
    size_t size;
    std::shared_ptr<char[]> data;
};

class Error : public DiffddError
{
  public:
    explicit Error(const std::string &message) : DiffddError(message) {}
};

class Writer
{
  public:
    Writer(std::ostream &ostream, size_t buffer_size)
        : m_writer{BufferedStream::Writer{ostream, buffer_size}}
    {
        writeFileHeader();
    };

    void writeDiffRecord(
        uint64_t offset, size_t size,
        std::vector<RecordData> data) // cppcheck-suppress passedByValue
    {
        writeOffset(offset);
        writeSize(size);
        for (auto it = data.begin(); it != data.end(); ++it) {
            writeData(*it);
        }
    }

  private:
    BufferedStream::Writer m_writer;

    void writeFileHeader()
    {
        m_writer.write(FileSignature.data(), FileSignature.size());

        uint8_t val{FileVersion};
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };

    void writeOffset(uint64_t offset)
    {
        uint64_t val{htobe64(offset)};
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };

    void writeSize(size_t size)
    {
        uint32_t val{htobe32(size)};
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };

    void writeData(RecordData data)
    {
        m_writer.write(reinterpret_cast<char *>(data.data.get()), data.size);
    };
};

class Reader
{
  public:
    Reader(std::istream &istream, size_t buffer_size)
        : m_reader{BufferedStream::Reader{istream, buffer_size, 1}}, m_eof{
                                                                         false}
    {
        readFileHeader();
    };

    bool eof() { return m_eof; };

    void readFileHeader()
    {
        auto rawSignature{std::make_unique<char[]>(FileSignature.size())};
        size_t r{m_reader.read(FileSignature.size(), rawSignature.get())};
        if (r < FileSignature.size()) {
            throw new Error("cannot read file header signature");
        }
        const std::string signature{rawSignature.get(), FileSignature.size()};
        if (signature != FileSignature) {
            throw new Error("wrong file header signature");
        }

        uint8_t version;
        r = {
            m_reader.read(sizeof(version), reinterpret_cast<char *>(&version))};
        if (r < sizeof(version)) {
            throw new Error("cannot read file header version");
        }
        if (version != FileVersion) {
            throw new Error("wrong file header version");
        }
    };

    uint64_t readOffset()
    {
        uint64_t raw_offset;
        const size_t r{m_reader.read(sizeof(raw_offset),
                                     reinterpret_cast<char *>(&raw_offset))};
        if (r != sizeof(raw_offset)) {
            m_eof = true;
        }
        return be64toh(raw_offset);
    };

    size_t readSize()
    {
        uint32_t raw_size;
        const size_t r{m_reader.read(sizeof(raw_size),
                                     reinterpret_cast<char *>(&raw_size))};
        if (r != sizeof(raw_size)) {
            m_eof = true;
        }
        return be32toh(raw_size);
    };

    RecordData readRecordData(size_t size)
    {
        const BufferedStream::DataPart dp = m_reader.readMultipart(size);
        return RecordData{
            .size = dp.size,
            .data = dp.data,
        };
    };

  private:
    BufferedStream::Reader m_reader;
    bool m_eof;
};

} // namespace FormatV2
