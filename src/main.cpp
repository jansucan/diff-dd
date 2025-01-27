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

#include "create.h"
#include "options.h"
#include "restore.h"

#include "program_info.h"

#include <iostream>

void
print_version()
{
    std::cout << PROGRAM_NAME_STR << " " << PROGRAM_VERSION_STR << std::endl;
}

int
main(int argc, char **argv)
{
    try {
        if (Options::Parser::isHelp(argc, argv)) {
            Options::printUsage();
        } else if (Options::Parser::isVersion(argc, argv)) {
            print_version();
        } else if (Options::Parser::isCreate(argc, argv)) {
            create(Options::Parser::parseCreate(argc, argv));
        } else if (Options::Parser::isRestore(argc, argv)) {
            restore(Options::Parser::parseRestore(argc, argv));
        } else {
            Options::printUsage();
            exit(1);
        }
    } catch (const Options::Error &e) {
        Options::printUsage();
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    } catch (const DiffddError &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }

    exit(0);
}
