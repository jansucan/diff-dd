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

#include "backup.h"
#include "buffered_file.h"
#include "format_v2.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

// TODO: Rozlisovat medzi offsetom v streame a offsetom v stranke v nazvoch
// TODO: Vztah medzi max_merge_gap a page_size
// TODO: Page a Diff assert metoda

class Page
{
    friend bool operator!=(const Page &lhs, const Page &rhs);

  public:
    Page() = default;
    Page(std::shared_ptr<char[]> data, uint64_t start, uint64_t end)
        : m_data(data), m_start(start), m_end(end)
    {
        assert(m_start <= m_end);
    };

    std::shared_ptr<char[]> getData() const { return m_data; };
    uint64_t getStart() const { return m_start; };
    uint64_t getEnd() const { return m_end; };
    size_t getSize() const { return m_end - m_start; };
    bool followsPage(const Page &other) const
    {
        return m_start == other.m_end;
    };

  private:
    std::shared_ptr<char[]> m_data;
    uint64_t m_start;
    uint64_t m_end;
};

bool
operator!=(const Page &lhs, const Page &rhs)
{
    return (lhs.m_data != rhs.m_data) || (lhs.m_start != rhs.m_start) ||
           (lhs.m_end != rhs.m_end);
}

class PagedStreamReader
{
  public:
    PagedStreamReader(std::istream &istr, size_t page_size_bytes)
        : m_page_size_bytes(page_size_bytes), m_istream(istr),
          m_stream_pos_bytes(0)
    {
        try {
            m_buffers[0] = std::shared_ptr<char[]>(new char[m_page_size_bytes]);
            m_buffers[1] = std::shared_ptr<char[]>(new char[m_page_size_bytes]);
        } catch (const std::bad_alloc &e) {
            throw BufferedFileError(
                "cannot allocate pages for input stream data");
        }
        assert(istr.good());
    };

    Page getNextPage()
    {
        m_buffer_index = (m_buffer_index + 1) % 2;
        auto buf = m_buffers[m_buffer_index];
        // Buffer for new data must not in use
        assert(buf.use_count() == 2);

        const size_t bytes_read{readFromStream(buf.get())};
        m_stream_pos_bytes += bytes_read;

        if (bytes_read == 0) {
            buf = std::shared_ptr<char[]>();
        }
        return Page{buf, m_stream_pos_bytes - bytes_read, m_stream_pos_bytes};
    }

  private:
    const size_t m_page_size_bytes;
    std::istream &m_istream;
    uint64_t m_stream_pos_bytes;
    std::array<std::shared_ptr<char[]>, 2> m_buffers;
    unsigned m_buffer_index;

    size_t readFromStream(char *const data)
    {
        if (m_istream.eof()) {
            return 0;
        }

        m_istream.read(data, m_page_size_bytes);

        if (!m_istream.good() && !m_istream.eof()) {
            throw BufferedFileError("cannot read from stream");
        }

        return m_istream.gcount();
    }
};

// Assert: empty diff has no pages
class Diff
{
    friend bool diffsTryMerge(Diff &diff_a, Diff &diff_b, size_t max_merge_gap);

  public:
    Diff(uint64_t start, uint64_t end)
        : m_pages{}, m_start{start}, m_end{end}, m_max_size{}
    {
        assert(m_start <= m_end);
    };
    Diff(Page page, uint64_t start, uint64_t end, size_t max_size)
        : m_pages{page}, m_start{start}, m_end{end}, m_max_size{max_size}
    {
        assert(m_start <= m_end);
    };
    uint64_t getStart() const { return m_start; };
    uint64_t getEnd() const { return m_end; };
    size_t getSize() const { return m_end - m_start; };
    bool isEmpty() const { return getSize() == 0; };
    std::vector<std::shared_ptr<char[]>> getData() const
    {
        std::vector<std::shared_ptr<char[]>> data{m_pages[0].getData(),
                                                  m_pages[1].getData()};
        return data;
    };
    // TODO: skryt metody ktore netreba

  private:
    std::array<Page, 2> m_pages;
    uint64_t m_start;
    uint64_t m_end;
    size_t m_max_size;

    // TODO: Mozno nie je treba
    const Page &getPage(size_t i) const { return m_pages[i]; };

    bool hasPage(size_t i)
    {
        return (i < m_pages.size()) && (m_pages[i].getData() != nullptr);
    };
};

// Retval: finished
bool
diffsTryMerge(Diff &diff_a, Diff &diff_b, size_t max_merge_gap)
{
    // TODO: Je toto spravne miesto kde verifikovat stranky?
    // diff_a must have either no, or the first, or both pages
    assert(diff_a.hasPage(0) || !diff_a.hasPage(1));
    // diff_b must have exactly one page and it has to be its page
    // TODO: Len prva alebo ziadna stranka
    // assert(diff_b.hasPage(0) && !diff_b.hasPage(1));

    // Do not merge to an empty diff
    if (diff_a.isEmpty()) {
        return true;
    }

    // B stranka musi nadvazovat na prvu alebo druhu v A
    // TODO: Stranky nemozu byt od seba, inak by bol assert v paged reader
    // diff_b moze byt v tej istej stranke
    // assert((!diff_a.hasPage(0) && !diff_a.hasPage(1)) ||
    //     (diff_a.hasPage(1) &&
    //         diff_b.getPage(0).followsPage(diff_a.getPage(1))) ||
    //        (!diff_a.hasPage(1) &&
    //         diff_b.getPage(0).followsPage(diff_a.getPage(0))));

    assert(diff_a.getEnd() <= diff_b.getStart());
    const size_t gap{diff_b.getStart() - diff_a.getEnd()};
    if (gap > max_merge_gap) {
        // diff_b is too far away
        return true;
    }

    if ((diff_a.getSize() + gap) >= diff_a.m_max_size) {
        // No space in diff_a
        return true;
    }

    if (diff_b.isEmpty()) {
        // Nothing to merge from diff_b (end of page)
        return true;
    }

    const size_t free{diff_a.m_max_size - (diff_a.getSize() + gap)};
    const size_t to_merge{std::min(free, diff_b.getSize())};

    assert(free > 0);
    assert(to_merge > 0);

    // TODO: Asserts
    // Vybavit stranky
    if (diff_b.m_pages[0] != diff_a.m_pages[0]) {
        diff_a.m_pages[1] = diff_b.m_pages[0];
    }

    // Vybavit adresy
    diff_a.m_end += to_merge;
    diff_b.m_start += to_merge;

    // TODO: Presunut do diffsTryMerge?
    // When m_diff is not finished, diff must be empty
    // assert(finished || diff.isEmpty());

    return diff_a.getSize() == diff_a.m_max_size;
}

class DiffFinder
{
  public:
    DiffFinder(std::istream &old_stream, std::istream &new_stream,
               uint32_t buffer_size, size_t max_merge_gap)
        : m_old_page_reader(old_stream, buffer_size),
          m_new_page_reader(new_stream, buffer_size),
          m_buffer_size(buffer_size), m_max_merge_gap(max_merge_gap),
          m_offset_in_stream(0), m_diff(0, 0),
          m_search_state(SearchState::ReadPages){};

    Diff findNextDiff()
    {
        for (;;) {
            if (m_search_state == SearchState::ReadPages) {
                m_old_page = m_old_page_reader.getNextPage();
                m_new_page = m_new_page_reader.getNextPage();
                assert(m_old_page.getStart() == m_new_page.getStart());

                if (m_old_page.getSize() != m_new_page.getSize()) {
                    throw BackupError(
                        "cannot read the same amount of data from both files");
                }

                const bool end_of_stream{(m_old_page.getSize() == 0) &&
                                         (m_new_page.getSize() == 0)};
                if (end_of_stream) {
                    const Diff return_diff{m_diff};
                    // TODO: Prazdny ale reflektuje koniec streamu, nie zaciatok
                    m_diff = Diff{0, 0};
                    return return_diff;
                }

                m_search_state = SearchState::FindDiff;

            } else if (m_search_state == SearchState::FindDiff) {
                Diff diff{findDiffInPages(m_old_page, m_new_page,
                                          m_offset_in_stream)};
                m_offset_in_stream = diff.getEnd();

                if (diff.isEmpty()) {
                    // End of pages, read next ones
                    m_old_page = Page{};
                    m_new_page = Page{};
                    m_search_state = SearchState::ReadPages;
                }

                const bool finished{
                    diffsTryMerge(m_diff, diff, m_max_merge_gap)};

                if (finished) {
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
    size_t m_buffer_size;
    size_t m_max_merge_gap;
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

        // Najdi dalsi usek od offsetu v aktualnej stranke
        for (; offset_in_pages < data_size_bytes; ++offset_in_pages) {
            if (new_data[offset_in_pages] != old_data[offset_in_pages]) {
                break;
            }
        }
        const size_t start_in_pages{offset_in_pages};

        size_t end_in_pages{offset_in_pages};
        if (start_in_pages < data_size_bytes) {
            // Najdi koniec rozdielneho useku
            for (++offset_in_pages; offset_in_pages < data_size_bytes;
                 ++offset_in_pages) {
                if (new_data[offset_in_pages] == old_data[offset_in_pages]) {
                    break;
                }
            }
            end_in_pages = offset_in_pages;
        }

        const uint64_t start_in_stream{new_page.getStart() + start_in_pages};
        const uint64_t end_in_stream{new_page.getStart() + end_in_pages};
        if (start_in_stream == end_in_stream) {
            return Diff{start_in_stream, end_in_stream};
        } else {
            return Diff{new_page, start_in_stream, end_in_stream,
                        m_buffer_size};
        }
    }
};

void
backup(const OptionsBackup &opts)
{
    std::ifstream in_istream{opts.getInFilePath(),
                             std::ifstream::in | std::ifstream::binary};
    if (!in_istream) {
        throw BufferedFileError("cannot open input file");
    }

    std::ifstream ref_istream{opts.getRefFilePath(),
                              std::ifstream::in | std::ifstream::binary};
    if (!ref_istream) {
        throw BufferedFileError("cannot open reference file");
    }

    // When backing up, the output file is truncated to hold the new data
    std::ofstream out_ostream{opts.getOutFilePath(), std::ofstream::out |
                                                         std::ofstream::trunc |
                                                         std::ofstream::binary};
    if (!out_ostream) {
        throw BufferedFileError("cannot open output file");
    }

    // TODO: gap = format record head
    DiffFinder diff_finder(ref_istream, in_istream, opts.getBufferSize(), 12);
    FormatV2Writer diff_writer(out_ostream, opts.getBufferSize());

    for (;;) {
        const Diff diff{diff_finder.findNextDiff()};
        if (diff.isEmpty()) {
            break;
        }

        // TODO: Remove before commit
        std::cout << diff.getStart() << " " << diff.getEnd() << " "
                  << diff.getSize() << std::endl;

        diff_writer.writeDiffRecord(diff.getStart(), diff.getSize(),
                                    diff.getData());

        // Here, the diff is destructed and page data reference counters
        // decremented
    }
}
