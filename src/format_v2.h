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

#include <endian.h>

#include <vector>

class FormatV2Writer
{
  public:
    FormatV2Writer(std::ostream &ostream, size_t buffer_size)
        : m_writer{BufferedFileWriter{ostream, buffer_size}}, m_header_written{
                                                                  false} {};
    void writeDiffRecord(uint64_t offset, size_t size,
                         std::vector<std::shared_ptr<char[]>> data)
    // TODO: S datami potrebujem aj velkost jednotlivych casti
    {
        if (!m_header_written) {
            writeHeader();
            m_header_written = true;
        }

        writeOffset(offset);
        writeSize(size);
        for (auto it = data.begin(); it != data.end(); ++it) {
            writeData(*it);
        }
    }

  private:
    BufferedFileWriter m_writer;
    bool m_header_written;

    void writeHeader(){
        // TODO
    };

    void writeOffset(uint64_t offset)
    {
        uint64_t val{htobe64(offset)};
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };

    void writeSize(size_t size)
    {
        uint32_t val{htobe64(size)};
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };

    void writeData(std::shared_ptr<char[]> data)
    {
        m_writer.write(reinterpret_cast<char *>(&val), sizeof(val));
    };
};

class FormatV2Reader
{
  public:
    FormatV2Reader();
};
