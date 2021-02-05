// Copyright 2020 Justin Hu
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

#include "options.h"

#include <stdio.h>
#include <string.h>

Options options = {
    OPTION_W_ERROR,
    OPTION_W_ERROR,
    OPTION_W_ERROR,
    OPTION_DD_NONE,
};

int parseArgs(size_t argc, char const *const *argv, size_t *numFilesOut) {
  size_t numFiles = 0;

  for (size_t idx = 1; idx < argc; ++idx) {
    if (argv[idx][0] != '-') {
      // not an option
      ++numFiles;
    } else if (strcmp(argv[idx], "--") == 0) {
      // remaining options are all files
      numFiles += argc - idx - 1;
      break;
    } else if (strcmp(argv[idx], "-Wduplicate-file=error") == 0) {
      options.duplicateFile = OPTION_W_ERROR;
    } else if (strcmp(argv[idx], "-Wduplicate-file=warn") == 0) {
      options.duplicateFile = OPTION_W_WARN;
    } else if (strcmp(argv[idx], "-Wduplicate-file=ignore") == 0) {
      options.duplicateFile = OPTION_W_IGNORE;
    } else if (strcmp(argv[idx], "-Wduplicate-import=error") == 0) {
      options.duplicateImport = OPTION_W_ERROR;
    } else if (strcmp(argv[idx], "-Wduplicate-import=warn") == 0) {
      options.duplicateImport = OPTION_W_WARN;
    } else if (strcmp(argv[idx], "-Wduplicate-import=ignore") == 0) {
      options.duplicateImport = OPTION_W_IGNORE;
    } else if (strcmp(argv[idx], "-Wunrecognized-file=error") == 0) {
      options.unrecognizedFile = OPTION_W_ERROR;
    } else if (strcmp(argv[idx], "-Wunrecognized-file=warn") == 0) {
      options.unrecognizedFile = OPTION_W_WARN;
    } else if (strcmp(argv[idx], "-Wunrecognized-file=ignore") == 0) {
      options.unrecognizedFile = OPTION_W_IGNORE;
    } else if (strcmp(argv[idx], "--debug-dump=none") == 0) {
      options.dump = OPTION_DD_NONE;
    } else if (strcmp(argv[idx], "--debug-dump=lex") == 0) {
      options.dump = OPTION_DD_LEX;
    } else if (strcmp(argv[idx], "--debug-dump=parse") == 0) {
      options.dump = OPTION_DD_PARSE;
    } else {
      fprintf(stderr, "tlc: error: options '%s' not recognized\n", argv[idx]);
      return -1;
    }
  }

  *numFilesOut = numFiles;

  return 0;
}