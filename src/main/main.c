// Copyright 2020-2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Compiles code modules into assembly files, guided by decl modules

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/dump.h"
#include "fileList.h"
#include "internalError.h"
#include "lexer/dump.h"
#include "lexer/lexer.h"
#include "options.h"
#include "parser/parser.h"
#include "typechecker/typechecker.h"
#include "version.h"

/**
 * determines if any argument in argv is "--version"
 *
 * @param argc number of arugments (including name of program)
 * @param argv list of arguments (including name of program)
 * @returns whether any argument in argv is "--version"
 */
static bool versionRequested(size_t argc, char **argv) {
  for (size_t idx = 1; idx < argc; ++idx) {
    if (strcmp(argv[idx], "--version") == 0) {
      return true;
    }
  }
  return false;
}
/**
 * determines if any argument in argv is "--help", "-h", or "-?"
 *
 * @param argc number of arguments (including name of program)
 * @param argv list of arguments (including name of program)
 * @returns whether any argument in argv is asking for help
 */
static bool helpRequested(size_t argc, char **argv) {
  for (size_t idx = 1; idx < argc; ++idx) {
    if (strcmp(argv[idx], "--help") == 0 || strcmp(argv[idx], "-h") == 0 ||
        strcmp(argv[idx], "-?") == 0) {
      return true;
    }
  }
  return false;
}

/** possible return values for main */
enum {
  CODE_SUCCESS = EXIT_SUCCESS,
  CODE_OPTION_ERROR,
  CODE_FILE_ERROR,
  CODE_PARSE_ERROR,
  CODE_TYPECHECK_ERROR,
};

// compile the given declaration and code files into one assembly file per code
// file, given the flags
int main(int argc, char **argv) {
  // handle overriding command line arguments
  if (helpRequested((size_t)argc, argv)) {
    printf(
        "Usage: tlc [options] file...\n"
        "For more information, see the 'README.md' file.\n"
        "\n"
        "Options:\n"
        "  --help, -h, -?    Display this information, and stop\n"
        "  --version         Display version information, and stop\n"
        "  --arch=...        Set the target architecture\n"
        "  -W...=...         Configure warning options\n"
        "  --debug-dump=...  Configure debug information\n"
        "\n"
        "Please report bugs at "
        "<https://github.com/JustinHuPrime/TCompiler/issues>\n");
    return CODE_SUCCESS;
  } else if (versionRequested((size_t)argc, argv)) {
    printf(
        "%s\n"
        "Copyright 2020 Justin Hu\n"
        "This is free software; see the source for copying conditions. There "
        "is NO\n"
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR "
        "PURPOSE.\n",
        VERSION_STRING);
    return CODE_SUCCESS;
  }

  // parse options, get number of files
  size_t numFiles;
  if (parseArgs((size_t)argc, (char const *const *)argv, &numFiles) != 0)
    return CODE_OPTION_ERROR;

  // fill in global file list
  if (parseFiles((size_t)argc, (char const *const *)argv, numFiles) != 0)
    return CODE_FILE_ERROR;

  // debug-dump stop for lexing
  if (options.dump == OPTION_DD_LEX) {
    lexerInitMaps();
    for (size_t idx = 0; idx < fileList.size; ++idx)
      lexDump(&fileList.entries[idx]);
    lexerUninitMaps();
  }

  // front-end

  // parse
  if (parse() != 0) return CODE_PARSE_ERROR;

  // debug-dump stop for parsing
  if (options.dump == OPTION_DD_PARSE) {
    for (size_t idx = 0; idx < fileList.size; ++idx)
      astDump(stderr, &fileList.entries[idx]);
  }

  // typecheck
  if (typecheck() != 0) return CODE_TYPECHECK_ERROR;

  // source code optimization
  // TODO: write this

  // translate to IR
  // TODO: write this

  // middle-end

  // ir optimization
  // TODO: write this

  // back-end
  // switch (options.arch) {
  //   case OPTION_A_X86_64_LINUX: {
  //     // assembly generation

  //     // assembly optimization part 1
  //     // TODO: write this

  //     // register allocation

  //     // assembly optimization part 2
  //     // TODO: write this

  //     // write out
  //     break;
  //   }
  // }

  return CODE_SUCCESS;
}