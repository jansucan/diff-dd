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
#include <optional>
#include <queue>

class BufferedFileError : public DiffddError
{
  public:
    explicit BufferedFileError(const std::string &message)
        : DiffddError(message)
    {
    }
};

class BufferedFileReader
{
  public:
    BufferedFileReader(std::filesystem::path path, size_t buffer_capacity);
    virtual ~BufferedFileReader() = default;

    size_t read(char *data, size_t data_size);

  private:
    std::ifstream m_file;
    std::unique_ptr<char[]> m_buffer;
    size_t m_buffer_offset;
    size_t m_buffer_size;
    const size_t m_buffer_capacity;

    size_t read_buffer(char *data, size_t data_size);
    void refill_buffer();
    size_t read_file(char *data, size_t data_size);
};

class BufferedFileWriter
{
  public:
    BufferedFileWriter(std::filesystem::path path, size_t buffer_capacity);
    virtual ~BufferedFileWriter();

    void write(const char *data, size_t data_size);

  private:
    std::fstream m_file;
    std::unique_ptr<char[]> m_buffer;
    size_t m_buffer_size;
    const size_t m_buffer_capacity;

    void write_buffer(const char *data, size_t data_size);
    void flush_buffer();
    void write_file(const char *data, size_t data_size);
};

struct Page {
    std::unique_ptr<char[]> data;
    size_t size;
    uint64_t start;
    uint64_t end;
    size_t refcount;

    Page() : size(0), start(0), end(0), refcount(0){};
};

class PagedFileReader
{
  public:
    PagedFileReader(std::filesystem::path path, size_t page_size)
        : m_page_size(page_size), m_file_pos(0), m_eof(false)
    {
        m_file.open(path, std::ifstream::in | std::ifstream::binary);
        if (!m_file) {
            throw BufferedFileError("cannot open input file");
        }

        try {
            m_pages[0].data = std::make_unique<char[]>(m_page_size);
            m_pages[1].data = std::make_unique<char[]>(m_page_size);
        } catch (const std::bad_alloc &e) {
            throw BufferedFileError(
                "cannot allocate pages for input file data");
        }
    };

    virtual ~PagedFileReader() = default;

    Page *next_page()
    {
        if (m_eof) {
            return nullptr;
        }

        ++m_page_index;

        Page *const p{&m_pages[m_page_index % 2]};
        if (p->refcount != 0) {
            throw BufferedFileError("page is not finished yet");
        }
        fill_page(p);

        if (p->size == 0) {
            m_eof = true;
        }

        p->start = m_file_pos;
        p->end = p->start + p->size;

        m_file_pos += p->size;

        return p;
    }

  private:
    const size_t m_page_size;
    std::ifstream m_file;
    uint64_t m_file_pos;
    bool m_eof;
    Page m_pages[2];
    unsigned m_page_index;

    void fill_page(Page *page)
    {
        m_file.read(page->data.get(), m_page_size);

        if (!m_file.good() && !m_file.eof()) {
            throw BufferedFileError("cannot read from file");
        }

        page->size = m_file.gcount();
    }
};
