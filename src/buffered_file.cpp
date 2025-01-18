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

#include "buffered_file.h"
#include "exception.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace BufferedFile
{

Reader::Reader(std::istream &istream, size_t buffer_capacity)
    : m_istream(istream), m_buffer_offset(0), m_buffer_size(0),
      m_buffer_capacity(buffer_capacity)
{
    try {
        m_buffer = std::make_unique<char[]>(m_buffer_capacity);
    } catch (const std::bad_alloc &e) {
        throw Error("cannot allocate buffer for input file data");
    }
};

size_t
Reader::read(char *data, size_t data_size)
{
    size_t retry_count{0};
    size_t offset{0};

    while ((data_size > 0) && (retry_count < 2)) {
        char *d;
        const size_t r{tryRead(data_size, &d)};
        if (r == 0) {
            ++retry_count;
            continue;
        }

        memcpy(data + offset, d, r);
        offset += r;
        data_size -= r;
    }

    return offset;
}

size_t
Reader::tryRead(size_t data_size, char **return_data)
{
    const size_t size_left{m_buffer_size - m_buffer_offset};
    if (size_left == 0) {
        refill_buffer();
        if (m_buffer_size == 0) {
            return 0;
        }
    }
    // There is at least one byte in the buffer
    const size_t size_read{read_buffer(data_size, return_data)};
    assert(size_read > 0);
    return size_read;
};

size_t
Reader::read_buffer(size_t data_size, char **return_data)
{
    *return_data = static_cast<char *>(m_buffer.get()) + m_buffer_offset;

    const size_t size_left{m_buffer_size - m_buffer_offset};
    const size_t size_read{std::min(data_size, size_left)};
    m_buffer_offset += size_read;
    return size_read;
};

void
Reader::refill_buffer()
{
    m_buffer_size = read_file(m_buffer.get(), m_buffer_capacity);
    m_buffer_offset = 0;
};

size_t
Reader::read_file(char *data, size_t data_size)
{
    m_istream.read(data, data_size);

    if (!m_istream.good() && !m_istream.eof()) {
        throw Error("cannot read from file");
    }

    return m_istream.gcount();
};

Writer::Writer(std::ostream &ostream, size_t buffer_capacity)
    : m_ostream(ostream), m_buffer_size(0), m_buffer_capacity(buffer_capacity)
{
    try {
        m_buffer = std::make_unique<char[]>(m_buffer_capacity);
    } catch (const std::bad_alloc &e) {
        throw Error("cannot allocate buffer for output file data");
    }
};
Writer::~Writer() { flush_buffer(); };

void
Writer::write(const char *data, size_t data_size)
{
    size_t free{m_buffer_capacity - m_buffer_size};
    if (data_size <= free) {
        // There is free space in the buffer
        write_buffer(data, data_size);
    } else {
        // No free space
        flush_buffer();
        if (data_size <= m_buffer_capacity) {
            // Data fits into the buffer
            write_buffer(data, data_size);
        } else {
            // Doesn't fit
            write_file(data, data_size);
        }
    }
};

void
Writer::write_buffer(const char *data, size_t data_size)
{
    memcpy(reinterpret_cast<char *>(m_buffer.get()) + m_buffer_size, data,
           data_size);
    m_buffer_size += data_size;
};

void
Writer::flush_buffer()
{
    write_file(m_buffer.get(), m_buffer_size);
    m_buffer_size = 0;
};

void
Writer::write_file(const char *data, size_t data_size)
{
    m_ostream.write(data, data_size);
    if (!m_ostream) {
        throw Error("cannot write to output file");
    }
};

} // namespace BufferedFile
