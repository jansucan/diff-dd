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

#include "exception.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace BufferedStream
{

class Error : public DiffddError
{
  public:
    explicit Error(const std::string &message) : DiffddError(message) {}
};

struct DataPart {
    size_t size;
    std::shared_ptr<char[]> data;
};

class Reader
{
  public:
    Reader(std::istream &istream, size_t buffer_capacity, size_t buffer_count);
    virtual ~Reader() = default;

    size_t read(size_t data_size, char *dest_buf);
    DataPart readMultipart(size_t data_size);

  private:
    const size_t m_buffer_count;
    const size_t m_buffer_capacity;
    std::istream &m_istream;
    std::vector<std::shared_ptr<char[]>> m_buffers;
    size_t m_buffer_index;
    size_t m_buffer_offset;
    size_t m_buffer_size;

    DataPart read_current_buffer(size_t data_size);
    void refill_next_buffer();
    size_t read_stream(std::shared_ptr<char[]> data, size_t data_size);
};

class Writer
{
  public:
    Writer(std::ostream &ostream, size_t buffer_capacity);
    virtual ~Writer();

    void write(const char *data, size_t data_size);

  private:
    std::ostream &m_ostream;
    std::unique_ptr<char[]> m_buffer;
    size_t m_buffer_size;
    const size_t m_buffer_capacity;

    void write_buffer(const char *data, size_t data_size);
    void flush_buffer();
    void write_stream(const char *data, size_t data_size);
};

} // namespace BufferedStream
