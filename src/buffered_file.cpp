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

#include <cstring>
#include <filesystem>
#include <fstream>

BufferedFileReader::BufferedFileReader(std::filesystem::path path,
                                       size_t buffer_capacity)
    : m_buffer_offset(0), m_buffer_capacity(buffer_capacity)
{
    m_file.open(path, std::ifstream::in | std::ifstream::binary);
    if (!m_file) {
        throw BufferedFileError("cannot open input file");
    }

    try {
        m_buffer = std::make_unique<char[]>(m_buffer_capacity);
    } catch (const std::bad_alloc &e) {
        throw BufferedFileError("cannot allocate buffer for input file data");
    }

    refill_buffer();
};

size_t
BufferedFileReader::read(char *data, size_t data_size)
{
    const size_t size_left = m_buffer_size - m_buffer_offset;
    if (data_size <= size_left) {
        return read_buffer(data, data_size);
    } else {
        const size_t size_outside_buffer = data_size - size_left;
        read_buffer(data, size_left);
        const size_t read_outside =
            read_file(data + size_left, size_outside_buffer);
        refill_buffer();
        return size_left + read_outside;
    }
};

size_t
BufferedFileReader::read_buffer(char *data, size_t data_size)
{
    // Assumes that the caller makes sure there is enough data in the buffer
    // to read
    memcpy(data, reinterpret_cast<char *>(m_buffer.get()) + m_buffer_offset,
           data_size);
    m_buffer_offset += data_size;
    return data_size;
};

void
BufferedFileReader::refill_buffer()
{
    m_buffer_size = read_file(m_buffer.get(), m_buffer_capacity);
    m_buffer_offset = 0;
};

size_t
BufferedFileReader::read_file(char *data, size_t data_size)
{
    m_file.read(data, data_size);

    if (!m_file.good() && !m_file.eof()) {
        throw BufferedFileError("cannot read from file");
    }

    return m_file.gcount();
};

BufferedFileWriter::BufferedFileWriter(std::filesystem::path path,
                                       size_t buffer_capacity)
    : m_buffer_size(0), m_buffer_capacity(buffer_capacity)
{
    /* When backing up, the output file is truncated to hold the
     * new data
     */
    m_file.open(path, std::ifstream::out | std::ifstream::trunc |
                          std::ifstream::binary);
    if (!m_file) {
        throw BufferedFileError("cannot open output file");
    }

    try {
        m_buffer = std::make_unique<char[]>(m_buffer_capacity);
    } catch (const std::bad_alloc &e) {
        throw BufferedFileError("cannot allocate buffer for output file data");
    }
};
BufferedFileWriter::~BufferedFileWriter() { flush_buffer(); };

void
BufferedFileWriter::write(const char *data, size_t data_size)
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
BufferedFileWriter::write_buffer(const char *data, size_t data_size)
{
    memcpy(reinterpret_cast<char *>(m_buffer.get()) + m_buffer_size, data,
           data_size);
    m_buffer_size += data_size;
};

void
BufferedFileWriter::flush_buffer()
{
    write_file(m_buffer.get(), m_buffer_size);
    m_buffer_size = 0;
};

void
BufferedFileWriter::write_file(const char *data, size_t data_size)
{
    m_file.write(data, data_size);
    if (!m_file) {
        throw BufferedFileError("cannot write to output file");
    }
};
