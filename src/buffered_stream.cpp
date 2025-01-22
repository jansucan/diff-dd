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

#include "buffered_stream.h"
#include "exception.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace BufferedStream
{

Reader::Reader(std::istream &istream, size_t buffer_capacity,
               size_t buffer_count)
    : m_buffer_count(buffer_count), m_buffer_capacity(buffer_capacity),
      m_istream(istream), m_buffers(buffer_count),
      m_buffer_index(buffer_count - 1), m_buffer_offset(buffer_capacity),
      m_buffer_size(buffer_capacity)
{
    for (size_t i = 0; i < m_buffer_count; ++i) {
        try {
            m_buffers[i] = std::shared_ptr<char[]>(new char[m_buffer_capacity]);
        } catch (const std::bad_alloc &e) {
            throw Error("cannot allocate buffer for input stream data");
        }
    }

    refill_next_buffer();
};

size_t
Reader::read(size_t data_size, char *dest_buf)
{
    size_t retry_count{0};
    size_t offset{0};
    size_t to_read{data_size};

    while ((to_read > 0) && (retry_count < 2)) {
        const DataPart dp{readMultipart(to_read)};
        if (dp.size == 0) {
            ++retry_count;
            continue;
        }

        retry_count = 0;
        memcpy(dest_buf + offset, dp.data.get(), dp.size);
        offset += dp.size;
        to_read -= dp.size;
    }

    assert(offset <= data_size);
    return offset;
}

DataPart
Reader::readMultipart(size_t data_size)
{
    const size_t size_left{m_buffer_size - m_buffer_offset};
    if (size_left == 0) {
        refill_next_buffer();
        if (m_buffer_size == 0) {
            return DataPart{.size = 0, .data = std::shared_ptr<char[]>()};
        }
    }
    // There is at least one byte in the buffer
    const DataPart dp{read_current_buffer(data_size)};
    assert(dp.size > 0);
    return dp;
}

DataPart
Reader::read_current_buffer(size_t data_size)
{
    DataPart dp;

    // Set data
    if (m_buffer_offset == 0) {
        dp.data = std::shared_ptr<char[]>{m_buffers[m_buffer_index]};
    } else {
        dp.data = std::shared_ptr<char[]>{
            m_buffers[m_buffer_index],
            static_cast<char *>(m_buffers[m_buffer_index].get()) +
                m_buffer_offset};
    }

    const size_t size_left{m_buffer_size - m_buffer_offset};
    dp.size = std::min(data_size, size_left);
    m_buffer_offset += dp.size;
    return dp;
};

void
Reader::refill_next_buffer()
{
    if (m_buffer_size == 0) {
        // Current buffer is the last one. Don't fill the next one.
        return;
    }

    // Current buffer must be completely read before filling the next one
    assert(m_buffer_offset == m_buffer_size);

    m_buffer_index = (m_buffer_index + 1) % m_buffer_count;
    auto buf = m_buffers[m_buffer_index];
    // Buffer for new data must not in use
    assert(buf.use_count() == 2);

    m_buffer_size = read_stream(buf, m_buffer_capacity);
    m_buffer_offset = 0;
};

size_t
Reader::read_stream(std::shared_ptr<char[]> data, size_t data_size)
{
    m_istream.read(data.get(), data_size);

    if (!m_istream.good() && !m_istream.eof()) {
        throw Error("cannot read from stream");
    }

    return m_istream.gcount();
};

Writer::Writer(std::ostream &ostream, size_t buffer_capacity)
    : m_ostream(ostream), m_buffer_size(0), m_buffer_capacity(buffer_capacity)
{
    try {
        m_buffer = std::make_unique<char[]>(m_buffer_capacity);
    } catch (const std::bad_alloc &e) {
        throw Error("cannot allocate buffer for output stream data");
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
            write_stream(data, data_size);
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
    write_stream(m_buffer.get(), m_buffer_size);
    m_buffer_size = 0;
};

void
Writer::write_stream(const char *data, size_t data_size)
{
    m_ostream.write(data, data_size);
    if (!m_ostream) {
        throw Error("cannot write to output stream");
    }
};

} // namespace BufferedStream
