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

#include <iostream>

class Diff
{
  public:
    Diff() : m_start(0), m_end(0), m_page(nullptr), m_page_next(nullptr) {}
    Diff(uint64_t start, uint64_t end, Page *page)
        : m_start(start), m_end(end), m_page(page), m_page_next(nullptr)
    {
        ++(m_page->refcount);
    }

    virtual ~Diff() = default;

    uint64_t getEnd() const { return m_end; }

    void done()
    {
        m_start = 0;
        m_end = 0;

        --(m_page->refcount);
        m_page = nullptr;

        if (m_page_next != nullptr) {
            --(m_page_next->refcount);
            m_page_next = nullptr;
        }
    }

    uint64_t size() const { return (m_end - m_start); }

    bool is_empty() const { return (size() == 0); }

    bool merge_with(Diff &other, uint64_t max_gap, size_t max_size)
    {
        const uint64_t gap = gap_to(other);
        if ((gap > max_gap) || ((size() + gap) >= max_size)) {
            // Cannot be merged
            return false;
        }

        if (other.m_page == m_page) {
            // Both are in the first page
            m_end = other.m_end;
            other.done();
        } else if (m_page_next == nullptr) {
            // The other is a in different page. Create new one
            m_page_next = other.m_page;
            ++(m_page_next->refcount);
            // Extend this page
            const uint64_t free = max_size - (size() + gap);
            const uint64_t take = std::min(free, other.size());
            m_end += take;
            // Reduce the other page
            other.m_start += take;
            if (other.is_empty()) {
                other.done();
            }
        } else {
            // The other is a in different page. Add to existing one
            // Extend this page
            const uint64_t free = max_size - (size() + gap);
            const uint64_t take = std::min(free, other.size());
            m_end += take;
            // Reduce the other page
            other.m_start += take;
            if (other.is_empty()) {
                other.done();
            }
        }

        return true;
    }

    void get_data(char **data1, size_t *size1, char **data2, size_t *size2)
    {
        if (m_page_next == nullptr) {
            const size_t offset = m_start - m_page->start;
            *data1 = m_page->data.get() + offset;
            *size1 = m_end - m_start;

            *data2 = nullptr;
            *size2 = 0;
        } else {
            const size_t offset = m_start - m_page->start;
            *data1 = m_page->data.get() + offset;
            *size1 = m_page->end - m_start;

            *data2 = m_page_next->data.get();
            *size2 = m_end - m_page_next->start;
        }
    }

  private:
    uint64_t m_start;
    uint64_t m_end;
    Page *m_page;
    Page *m_page_next;

    uint64_t gap_to(const Diff &other) const { return other.m_start - m_end; }
};

class DiffWriter
{
  public:
    DiffWriter(size_t max_size, BufferedFileWriter &writer)
        : m_max_size(max_size), m_writer(writer)
    {
    }

    virtual ~DiffWriter()
    {
        if (!m_diff.is_empty()) {
            flush(m_diff);
        }
    }

    void write(Diff diff)
    {
        if (m_diff.is_empty()) {
            set(diff);
            return;
        }

        // There is a diff already in the buffer. Try to merge this one into it.
        const bool merged = m_diff.merge_with(diff, 12, m_max_size);
        if (!merged) {
            // Cannot be merged. Flush existing one as it is, and set a new one.
            flush(m_diff);
            set(diff);
        } else {
            // Merged
            if (is_full(m_diff)) {
                flush(m_diff);
                if (!diff.is_empty()) {
                    set(diff);
                }
            } else {
                // m_diff is not full, this diff must be empty
                ;
            }
        }
    }

  private:
    Diff m_diff;
    size_t m_max_size;
    BufferedFileWriter &m_writer;

    bool is_full(Diff &diff) const { return (diff.size() >= m_max_size); }

    void set(Diff diff)
    {
        if (is_full(diff)) {
            flush(diff);
        } else {
            m_diff = diff;
        }
    }

    void flush(Diff &diff)
    {
        char *data1;
        size_t size1;

        char *data2;
        size_t size2;

        diff.get_data(&data1, &size1, &data2, &size2);
        m_writer.write(data1, size1);

        if (data2 != nullptr) {
            m_writer.write(data1, size1);
        }

        diff.done();
    }
};

static Diff
find_diff(Page *page_ref, Page *page_in, size_t offset_in_file)
{
    const char *data_ref = page_ref->data.get();
    const char *data_in = page_in->data.get();
    const size_t size = page_in->size;

    size_t offset = offset_in_file - page_in->start;

    uint64_t diff_start{};

    // Najdi dalsi usek od offsetu v aktualnej stranke
    for (; offset < size; ++offset) {
        if (data_ref[offset] != data_in[offset]) {
            diff_start = page_in->start + offset;
            break;
        }
    }

    if (offset >= size) {
        // Ziadny rozdiel nenajdeny v aktualnej stranke
        return Diff{};
    }

    // Najdi koniec rozdielneho useku
    for (++offset; offset < size; ++offset) {
        if (data_ref[offset] == data_in[offset]) {
            break;
        }
    }
    uint64_t diff_end{page_in->start + offset};

    return Diff{diff_start, diff_end, page_in};
}

void
backup(const OptionsBackup &opts)
{
    PagedFileReader in_file(opts.getInFilePath(), opts.getBufferSize());
    PagedFileReader ref_file(opts.getRefFilePath(), opts.getBufferSize());
    BufferedFileWriter out_file(opts.getOutFilePath(), opts.getBufferSize());
    DiffWriter diff_writer(opts.getBufferSize(), out_file);

    size_t offset_in_file = 0;

    for (;;) {
        Page *const page_ref = ref_file.next_page();
        Page *const page_in = in_file.next_page();

        if ((page_ref->size == 0) && (page_in->size == 0)) {
            break;
        } else if ((page_ref->size == 0) || (page_in->size == 0)) {
            throw BackupError(
                "cannot read the same amount of data from both files");
        }

        for (;;) {
            Diff diff = find_diff(page_ref, page_in, offset_in_file);
            if (diff.is_empty()) {
                break;
            }
            offset_in_file = diff.getEnd();
            diff_writer.write(diff);
        }
    }
}
