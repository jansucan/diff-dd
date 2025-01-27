/* Copyright 2019 Ján Sučan <jan@jansucan.com>
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

#include <cstdint>
#include <filesystem>

namespace Options
{

class Error : public DiffddError
{
  public:
    explicit Error(const std::string &message) : DiffddError(message) {}
};

const inline int DEFAULT_BUFFER_SIZE{4 * 1024 * 1024};

void printUsage();

class Create
{
    friend class Parser;

  public:
    Create();

    uint32_t getBufferSize() const;
    std::filesystem::path getInFilePath() const;
    std::filesystem::path getBaseFilePath() const;
    std::filesystem::path getOutFilePath() const;

  private:
    uint32_t buffer_size;
    std::filesystem::path in_file_path;
    std::filesystem::path base_file_path;
    std::filesystem::path out_file_path;
};

class Restore
{
    friend class Parser;

  public:
    Restore();

    uint32_t getBufferSize() const;
    std::filesystem::path getDiffFilePath() const;
    std::filesystem::path getOutFilePath() const;

  private:
    uint32_t buffer_size;
    std::filesystem::path diff_file_path;
    std::filesystem::path out_file_path;
};

class Parser
{
  public:
    static bool isHelp(int argc, char **argv);
    static bool isVersion(int argc, char **argv);
    static bool isCreate(int argc, char **argv);
    static bool isRestore(int argc, char **argv);

    static Create parseCreate(int argc, char **argv);
    static Restore parseRestore(int argc, char **argv);

  private:
    static const size_t MAX_OPERATION_NAME_LENGTH{8};

    static bool isOperation(int argc, char **argv,
                            std::string_view operationName);
    static int parse_unsigned(const char *const arg, uint32_t *const value);
};

} // namespace Options
