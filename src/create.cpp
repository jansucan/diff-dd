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

#include "create.h"
#include "buffered_stream.h"
#include "format_v2.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

class Page
{
    friend bool operator==(const Page &lhs, const Page &rhs);

  public:
    Page() : m_start(0), m_end(0){};

    Page(std::shared_ptr<char[]> data, uint64_t start, uint64_t end)
        : m_data(data), m_start(start), m_end(end)
    {
        assert(m_start <= m_end);
    };

    std::shared_ptr<char[]> getData() const { return m_data; };
    uint64_t getStart() const { return m_start; };
    uint64_t getEnd() const { return m_end; };
    size_t getSize() const { return m_end - m_start; };
    bool isEmpty() const { return getSize() == 0; };

  private:
    std::shared_ptr<char[]> m_data;
    uint64_t m_start;
    uint64_t m_end;
};

bool
operator==(const Page &lhs, const Page &rhs)
{
    return (lhs.m_data == rhs.m_data) && (lhs.m_start == rhs.m_start) &&
           (lhs.m_end == rhs.m_end);
}

class PagedStreamReader
{
  public:
    PagedStreamReader(std::istream &istr, size_t page_size_bytes)
        : m_page_size_bytes(page_size_bytes),
          m_reader(istr, page_size_bytes, 2), m_stream_pos_bytes(0){};

    Page getNextPage()
    {
        const BufferedStream::DataPart dp{
            m_reader.readMultipart(m_page_size_bytes)};

        m_stream_pos_bytes += dp.size;

        return Page{dp.data, m_stream_pos_bytes - dp.size, m_stream_pos_bytes};
    }

  private:
    const size_t m_page_size_bytes;
    BufferedStream::Reader m_reader;
    uint64_t m_stream_pos_bytes;
};

enum class MergeState {
    Finished,
    Incomplete,
};

class Diff
{
    friend MergeState diffsTryMerge(Diff &diff_a, Diff &diff_b,
                                    size_t max_merge_gap, size_t max_size);

  public:
    explicit Diff(uint64_t start_end)
        : m_pages{}, m_start{start_end}, m_end{start_end}
    {
        assert(m_start <= m_end);
    };
    Diff(Page page, uint64_t start, uint64_t end)
        : m_pages{page}, m_start{start}, m_end{end}
    {
        assert(m_start <= m_end);
    };
    uint64_t getStart() const { return m_start; };
    uint64_t getEnd() const { return m_end; };
    size_t getSize() const { return m_end - m_start; };
    bool isEmpty() const { return getSize() == 0; };

    std::vector<FormatV2::RecordData> getData() const
    {
        std::vector<FormatV2::RecordData> data{};

        if (!m_pages[0].isEmpty() && m_pages[1].isEmpty()) {
            // Only the first page
            assert((m_start >= m_pages[0].getStart()) &&
                   (m_start <= m_pages[0].getEnd()) &&
                   (m_end >= m_pages[0].getStart()) &&
                   (m_end <= m_pages[0].getEnd()));

            const uint64_t offset{m_start - m_pages[0].getStart()};
            auto data_first{std::shared_ptr<char[]>{
                m_pages[0].getData(),
                static_cast<char *>(m_pages[0].getData().get()) + offset}};
            data.push_back(FormatV2::RecordData{getSize(), data_first});
        } else if (!m_pages[0].isEmpty() && !m_pages[1].isEmpty()) {
            // Both pages
            assert((m_start >= m_pages[0].getStart()) &&
                   (m_start <= m_pages[0].getEnd()) &&
                   (m_end >= m_pages[1].getStart()) &&
                   (m_end <= m_pages[1].getEnd()));

            size_t size{m_pages[0].getEnd() - m_start};
            const uint64_t offset{m_start - m_pages[0].getStart()};
            auto data_first{std::shared_ptr<char[]>{
                m_pages[0].getData(),
                static_cast<char *>(m_pages[0].getData().get()) + offset}};
            data.push_back(FormatV2::RecordData{size, data_first});

            size = m_end - m_pages[1].getStart();
            data.push_back(FormatV2::RecordData{size, m_pages[1].getData()});
        }

        return data;
    };

  private:
    std::array<Page, 2> m_pages;
    uint64_t m_start;
    uint64_t m_end;

    bool hasPage(size_t i) // cppcheck-suppress unusedPrivateFunction
    {
        return (i < m_pages.size()) && (m_pages[i].getData() != nullptr);
    };
};

MergeState
diffsTryMerge(Diff &diff_a, Diff &diff_b, size_t max_merge_gap, size_t max_size)
{
    if (diff_a.isEmpty()) {
        // Do not merge to an empty diff
        return MergeState::Finished;
    }

    if (diff_b.isEmpty()) {
        // Nothing to merge from an empty diff
        return MergeState::Finished;
    }

    assert(diff_a.getEnd() <= diff_b.getStart());
    const size_t gap{diff_b.getStart() - diff_a.getEnd()};
    if (gap > max_merge_gap) {
        // B is too far away
        return MergeState::Finished;
    }

    if ((diff_a.getSize() + gap) >= max_size) {
        // No space in A
        return MergeState::Finished;
    }

    // Can be merged

    // Adjust the diff start and end offsets

    // There is always at least 1 byte free in A here
    const size_t free{max_size - (diff_a.getSize() + gap)};
    const size_t to_merge{std::min(free, diff_b.getSize())};
    // There is always at least 1 byte to merge from B here

    // Enlarge A
    diff_a.m_end += gap + to_merge;
    // Shrink B
    diff_b.m_start += to_merge;

    // Add B's page to A if needed

    // Non-empty A must have only the first, or both pages
    assert(diff_a.hasPage(0));
    // Non-empty B must have only the first page
    assert(diff_b.hasPage(0) && !diff_b.hasPage(1));

    // If A has both pages, B's page must only be the same as A's second
    // page. No setting of pages in A is needed in this case
    assert(!diff_a.hasPage(1) || (diff_b.m_pages[0] == diff_a.m_pages[1]));
    if (!diff_a.hasPage(1)) {
        // If A has only the first page, B's page must only be the same as the
        // A's first page or following it
        const bool b_follows{
            (diff_b.m_pages[0].getData() != diff_a.m_pages[0].getData()) &&
            (diff_b.m_pages[0].getStart() == diff_a.m_pages[0].getEnd())};
        assert((diff_b.m_pages[0] == diff_a.m_pages[0]) || b_follows);
        if (b_follows) {
            diff_a.m_pages[1] = diff_b.m_pages[0];
        }
    }

    return (diff_a.getSize() >= max_size) ? MergeState::Finished
                                          : MergeState::Incomplete;
}

class DiffFinder
{
  public:
    DiffFinder(std::istream &old_stream, std::istream &new_stream,
               uint32_t buffer_size, size_t max_merge_gap)
        : m_old_page_reader(old_stream, buffer_size),
          m_new_page_reader(new_stream, buffer_size),
          m_diff_max_size(buffer_size), m_max_merge_gap(max_merge_gap),
          m_offset_in_stream(0), m_diff(0),
          m_search_state(SearchState::ReadPages){};

    Diff findNextDiff()
    {
        for (;;) {
            if (m_search_state == SearchState::ReadPages) {
                m_old_page = m_old_page_reader.getNextPage();
                m_new_page = m_new_page_reader.getNextPage();
                assert(m_old_page.getStart() == m_new_page.getStart());

                if (m_old_page.getSize() != m_new_page.getSize()) {
                    throw CreateError(
                        "cannot read the same amount of data from both files");
                }

                const bool end_of_stream{m_old_page.isEmpty() &&
                                         m_new_page.isEmpty()};
                if (end_of_stream) {
                    const Diff return_diff{m_diff};
                    m_diff = Diff{m_offset_in_stream};
                    return return_diff;
                }

                m_search_state = SearchState::FindDiff;

            } else if (m_search_state == SearchState::FindDiff) {
                Diff diff{findDiffInPages(m_old_page, m_new_page,
                                          m_offset_in_stream)};
                m_offset_in_stream = diff.getEnd();

                if (diff.isEmpty()) {
                    // End of pages. On the next call, read new pages.
                    m_old_page = Page{};
                    m_new_page = Page{};
                    m_search_state = SearchState::ReadPages;
                }

                const MergeState merge_state{diffsTryMerge(
                    m_diff, diff, m_max_merge_gap, m_diff_max_size)};

                if (merge_state == MergeState::Finished) {
                    const Diff return_diff{m_diff};
                    m_diff = diff;
                    if (!return_diff.isEmpty()) {
                        return return_diff;
                    }
                }

            } else {
                assert(false);
            }
        }
    };

  private:
    enum class SearchState { ReadPages, FindDiff };

    PagedStreamReader m_old_page_reader;
    PagedStreamReader m_new_page_reader;
    const size_t m_diff_max_size;
    const size_t m_max_merge_gap;
    Page m_old_page;
    Page m_new_page;
    uint64_t m_offset_in_stream;
    Diff m_diff;
    SearchState m_search_state;

    Diff findDiffInPages(Page old_page, Page new_page,
                         uint64_t offset_in_stream)
    {
        const char *old_data{old_page.getData().get()};
        const char *new_data{new_page.getData().get()};
        const uint64_t data_size_bytes{old_page.getSize()};

        assert(offset_in_stream >= new_page.getStart());
        size_t offset_in_pages{offset_in_stream - new_page.getStart()};

        // Find offset of the first different byte
        for (; offset_in_pages < data_size_bytes; ++offset_in_pages) {
            if (new_data[offset_in_pages] != old_data[offset_in_pages]) {
                break;
            }
        }
        const size_t start_in_pages{offset_in_pages};

        if (offset_in_pages < data_size_bytes) {
            // Different byte found. Start searching for a same byte immediately
            // after.
            ++offset_in_pages;
        }

        // Find offset of the first same byte
        for (; offset_in_pages < data_size_bytes; ++offset_in_pages) {
            if (new_data[offset_in_pages] == old_data[offset_in_pages]) {
                break;
            }
        }
        const size_t end_in_pages{offset_in_pages};

        // In the case when no different byte is found, the end offset will be
        // the same as the start offset

        const uint64_t start_in_stream{new_page.getStart() + start_in_pages};
        const uint64_t end_in_stream{new_page.getStart() + end_in_pages};
        if (start_in_stream == end_in_stream) {
            return Diff{start_in_stream};
        } else {
            return Diff{new_page, start_in_stream, end_in_stream};
        }
    }
};

void
create(const Options::Create &opts)
{
    std::ifstream in_istream{opts.getInFilePath(),
                             std::ifstream::in | std::ifstream::binary};
    if (!in_istream) {
        throw BufferedStream::Error("cannot open input file");
    }

    std::ifstream base_istream{opts.getBaseFilePath(),
                               std::ifstream::in | std::ifstream::binary};
    if (!base_istream) {
        throw BufferedStream::Error("cannot open base file");
    }

    // When backing up, the output file is truncated to hold the new data
    std::ofstream out_ostream{opts.getOutFilePath(), std::ofstream::out |
                                                         std::ofstream::trunc |
                                                         std::ofstream::binary};
    if (!out_ostream) {
        throw BufferedStream::Error("cannot open output file");
    }

    DiffFinder diff_finder(base_istream, in_istream, opts.getBufferSize(),
                           FormatV2::RecordHeaderSize);
    FormatV2::Writer diff_writer(out_ostream, opts.getBufferSize());

    for (;;) {
        const Diff diff{diff_finder.findNextDiff()};
        if (diff.isEmpty()) {
            break;
        }

        diff_writer.writeDiffRecord(diff.getStart(), diff.getSize(),
                                    diff.getData());

        // Here, the diff is destructed and page data reference counters
        // decremented
    }
}
