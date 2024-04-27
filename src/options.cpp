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

#include "options.h"

#include <iostream>

#include <cstring>
#include <unistd.h>

/* This header file is automatically generated at build time from the Makefile
 */
#include "program_info.h"

Options::Options()
    : sector_size{Options::DEFAULT_SECTOR_SIZE},
      buffer_size{Options::DEFAULT_BUFFER_SIZE}
{
}

uint32_t
Options::getSectorSize() const
{
    return sector_size;
}

uint32_t
Options::getBufferSize() const
{
    return buffer_size;
}

std::filesystem::path
OptionsBackup::getInFilePath() const
{
    return in_file_path;
}

std::filesystem::path
OptionsBackup::getRefFilePath() const
{
    return ref_file_path;
}

std::filesystem::path
OptionsBackup::getOutFilePath() const
{
    return out_file_path;
}

std::filesystem::path
OptionsRestore::getInFilePath() const
{
    return in_file_path;
}

std::filesystem::path
OptionsRestore::getOutFilePath() const
{
    return out_file_path;
}

void
OptionParser::printUsage()
{
    std::cout << "Usage: " << PROGRAM_NAME_STR << " backup [-s SECTOR_SIZE]";
    std::cout << " [-b BUFFER_SIZE] INFILE REFFILE OUTFILE" << std::endl;

    std::cout << "   Or: " << PROGRAM_NAME_STR << " restore [-s SECTOR_SIZE]";
    std::cout << "[-b BUFFER_SIZE] REFFILE OUTFILE" << std::endl;

    std::cout << "   Or: " << PROGRAM_NAME_STR << " help" << std::endl;
}

bool
OptionParser::isHelp(int argc, char **argv)
{
    return isOperation(argc, argv, "help");
}

bool
OptionParser::isBackup(int argc, char **argv)
{
    return isOperation(argc, argv, "backup");
}

bool
OptionParser::isRestore(int argc, char **argv)
{
    return isOperation(argc, argv, "restore");
}

OptionsBackup
OptionParser::parseBackup(int argc, char **argv)
{
    OptionsBackup opts;

    parse_common(&argc, &argv, opts);

    if (argc < 3) {
        throw OptionError("missing arguments");
    } else if (argc > 3) {
        throw OptionError("too many arguments");
    } else {
        opts.in_file_path = next_arg(&argv);
        opts.ref_file_path = next_arg(&argv);
        opts.out_file_path = next_arg(&argv);
    }

    return opts;
}

OptionsRestore
OptionParser::parseRestore(int argc, char **argv)
{
    OptionsRestore opts;

    parse_common(&argc, &argv, opts);

    if (argc < 2) {
        throw OptionError("missing arguments");
    } else if (argc > 2) {
        throw OptionError("too many arguments");
    } else {
        opts.in_file_path = next_arg(&argv);
        opts.out_file_path = next_arg(&argv);
    }

    return opts;
}

bool
OptionParser::isOperation(int argc, char **argv, std::string_view operationName)
{
    return ((argc >= 2) &&
            (strncmp(argv[1], operationName.data(),
                     OptionParser::MAX_OPERATION_NAME_LENGTH) == 0));
}

int
OptionParser::parse_unsigned(const char *const arg, uint32_t *const value)
{
    char *end;

    errno = 0;

    *value = strtoul(arg, &end, 0);

    return ((*end != '\0') || (errno != 0)) ? -1 : 0;
}

void
OptionParser::parse_common(int *const argc, char ***const argv, Options &opts)
{
    // Skip the executable name. Do not skip the operation name. getopt expects
    // to start at an argument immediately preceding the possible options.
    *argc -= 1;
    *argv += 1;

    int ch;
    const char *arg_sector_size = NULL;
    const char *arg_buffer_size = NULL;

    while ((ch = getopt(*argc, *argv, ":b:s:")) != -1) {
        switch (ch) {
        case 'b':
            arg_buffer_size = optarg;
            break;

        case 's':
            arg_sector_size = optarg;
            break;

        case ':':
            throw OptionError("missing argument for option '-" +
                              std::string(1, optopt) + "'");
        default:
            throw OptionError("unknown option '-" + std::string(1, optopt) +
                              "'");
        }
    }

    *argc -= optind;
    *argv += optind;

    /* Convert numbers in the arguments */
    if ((arg_sector_size != NULL) &&
        parse_unsigned(arg_sector_size, &(opts.sector_size))) {
        throw OptionError("incorrect sector size");
    } else if ((arg_buffer_size != NULL) &&
               parse_unsigned(arg_buffer_size, &(opts.buffer_size))) {
        throw OptionError("incorrect buffer size");
    } else if (opts.sector_size == 0) {
        throw OptionError("sector size cannot be 0");
    } else if (opts.buffer_size == 0) {
        throw OptionError("buffer size cannot be 0");
    } else if (opts.sector_size > opts.buffer_size) {
        throw OptionError("sector size cannot larger than buffer size");
    } else if ((opts.buffer_size % opts.sector_size) != 0) {
        throw OptionError("buffer size is not multiple of sector size");
    }
}

const char *
OptionParser::next_arg(char ***const argv)
{
    const char *arg = **argv;
    ++(*argv);
    return arg;
}
