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

class OptionError : public DiffddError
{
  public:
    explicit OptionError(const std::string &message) : DiffddError(message) {}
};

class Options
{
    friend class OptionParser;

  public:
    static const int DEFAULT_BUFFER_SIZE{4 * 1024 * 1024};

    Options();
    virtual ~Options() = default;

    uint32_t getBufferSize() const;

  private:
    uint32_t buffer_size;
};

class OptionsBackup : public Options
{
    friend class OptionParser;

  public:
    virtual ~OptionsBackup() override = default;

    std::filesystem::path getInFilePath() const;
    std::filesystem::path getBaseFilePath() const;
    std::filesystem::path getOutFilePath() const;

  private:
    std::filesystem::path in_file_path;
    std::filesystem::path base_file_path;
    std::filesystem::path out_file_path;
};

class OptionsRestore : public Options
{
    friend class OptionParser;

  public:
    virtual ~OptionsRestore() override = default;

    std::filesystem::path getDiffFilePath() const;
    std::filesystem::path getOutFilePath() const;

  private:
    std::filesystem::path diff_file_path;
    std::filesystem::path out_file_path;
};

class OptionParser
{
  public:
    static void printUsage();
    static bool isHelp(int argc, char **argv);
    static bool isBackup(int argc, char **argv);
    static bool isRestore(int argc, char **argv);
    ;
    static OptionsBackup parseBackup(int argc, char **argv);
    static OptionsRestore parseRestore(int argc, char **argv);

  private:
    static const size_t MAX_OPERATION_NAME_LENGTH{8};

    static bool isOperation(int argc, char **argv,
                            std::string_view operationName);
    static int parse_unsigned(const char *const arg, uint32_t *const value);
    static void parse_common(int *const argc, char ***const argv,
                             Options &opts);
    static const char *next_arg(char ***const argv);
};
