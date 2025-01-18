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

namespace BufferedStream
{

class Error : public DiffddError
{
  public:
    explicit Error(const std::string &message) : DiffddError(message) {}
};

class Reader
{
  public:
    Reader(std::istream &istream, size_t buffer_capacity);
    virtual ~Reader() = default;

    size_t read(char *data, size_t data_size);
    size_t tryRead(size_t data_size, char **return_data);

  private:
    std::istream &m_istream;
    std::unique_ptr<char[]> m_buffer;
    size_t m_buffer_offset;
    size_t m_buffer_size;
    const size_t m_buffer_capacity;

    size_t read_buffer(size_t data_size, char **return_data);
    void refill_buffer();
    size_t read_stream(char *data, size_t data_size);
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
