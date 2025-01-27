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

void
OptionsPrintUsage()
{
    std::cout << "Usage: " << PROGRAM_NAME_STR << " create";
    std::cout << " [-B BUFFER_SIZE] -i INFILE -b BASEFILE -o OUTFILE"
              << std::endl;

    std::cout << "   Or: " << PROGRAM_NAME_STR << " restore";
    std::cout << " [-B BUFFER_SIZE] -d DIFFFILE -o OUTFILE" << std::endl;

    std::cout << "   Or: " << PROGRAM_NAME_STR << " version" << std::endl;

    std::cout << "   Or: " << PROGRAM_NAME_STR << " help" << std::endl;
}

OptionsCreate::OptionsCreate() : buffer_size{OPTIONS_DEFAULT_BUFFER_SIZE} {}

uint32_t
OptionsCreate::getBufferSize() const
{
    return buffer_size;
}

std::filesystem::path
OptionsCreate::getInFilePath() const
{
    return in_file_path;
}

std::filesystem::path
OptionsCreate::getBaseFilePath() const
{
    return base_file_path;
}

std::filesystem::path
OptionsCreate::getOutFilePath() const
{
    return out_file_path;
}

OptionsRestore::OptionsRestore() : buffer_size{OPTIONS_DEFAULT_BUFFER_SIZE} {}

uint32_t
OptionsRestore::getBufferSize() const
{
    return buffer_size;
}

std::filesystem::path
OptionsRestore::getDiffFilePath() const
{
    return diff_file_path;
}

std::filesystem::path
OptionsRestore::getOutFilePath() const
{
    return out_file_path;
}

bool
OptionParser::isHelp(int argc, char **argv)
{
    return isOperation(argc, argv, "help");
}

bool
OptionParser::isVersion(int argc, char **argv)
{
    return isOperation(argc, argv, "version");
}

bool
OptionParser::isCreate(int argc, char **argv)
{
    return isOperation(argc, argv, "create");
}

bool
OptionParser::isRestore(int argc, char **argv)
{
    return isOperation(argc, argv, "restore");
}

OptionsCreate
OptionParser::parseCreate(int argc, char **argv)
{
    OptionsCreate opts;

    // Skip the executable name. Do not skip the operation name. getopt expects
    // to start at an argument immediately preceding the possible options.
    argc -= 1;
    argv += 1;

    int ch;
    const char *arg_buffer_size = NULL;
    const char *arg_input_file = NULL;
    const char *arg_base_file = NULL;
    const char *arg_output_file = NULL;

    while ((ch = getopt(argc, argv, ":B:i:b:o:")) != -1) {
        switch (ch) {
        case 'B':
            arg_buffer_size = optarg;
            break;

        case 'i':
            arg_input_file = optarg;
            break;

        case 'b':
            arg_base_file = optarg;
            break;

        case 'o':
            arg_output_file = optarg;
            break;

        case ':':
            throw OptionError("missing argument for option '-" +
                              std::string(1, optopt) + "'");
        default:
            throw OptionError("unknown option '-" + std::string(1, optopt) +
                              "'");
        }
    }

    argc -= optind;

    /* Convert numbers in the arguments */
    if ((arg_buffer_size != NULL) &&
        parse_unsigned(arg_buffer_size, &(opts.buffer_size))) {
        throw OptionError("incorrect buffer size");
    } else if (opts.buffer_size == 0) {
        throw OptionError("buffer size cannot be 0");
    }

    if (arg_input_file == NULL) {
        throw OptionError("missing input file");
    } else if (arg_base_file == NULL) {
        throw OptionError("missing base file");
    } else if (arg_output_file == NULL) {
        throw OptionError("missing output file");
    } else if (argc != 0) {
        throw OptionError("too many arguments");
    }

    opts.in_file_path = arg_input_file;
    opts.base_file_path = arg_base_file;
    opts.out_file_path = arg_output_file;

    return opts;
}

OptionsRestore
OptionParser::parseRestore(int argc, char **argv)
{
    OptionsRestore opts;

    argc -= 1;
    argv += 1;

    int ch;
    const char *arg_buffer_size = NULL;
    const char *arg_diff_file = NULL;
    const char *arg_output_file = NULL;

    while ((ch = getopt(argc, argv, ":B:d:o:")) != -1) {
        switch (ch) {
        case 'B':
            arg_buffer_size = optarg;
            break;

        case 'd':
            arg_diff_file = optarg;
            break;

        case 'o':
            arg_output_file = optarg;
            break;

        case ':':
            throw OptionError("missing argument for option '-" +
                              std::string(1, optopt) + "'");
        default:
            throw OptionError("unknown option '-" + std::string(1, optopt) +
                              "'");
        }
    }

    argc -= optind;

    /* Convert numbers in the arguments */
    if ((arg_buffer_size != NULL) &&
        parse_unsigned(arg_buffer_size, &(opts.buffer_size))) {
        throw OptionError("incorrect buffer size");
    } else if (opts.buffer_size == 0) {
        throw OptionError("buffer size cannot be 0");
    }

    if (arg_diff_file == NULL) {
        throw OptionError("missing diff file");
    } else if (arg_output_file == NULL) {
        throw OptionError("missing output file");
    } else if (argc != 0) {
        throw OptionError("too many arguments");
    }

    opts.diff_file_path = arg_diff_file;
    opts.out_file_path = arg_output_file;

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
