// Copyright 2020 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// Compiles code modules into assembly files, guided by decl modules

#include "constants.h"
#include "fileList.h"
#include "util/options.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// determines if any argument in argv is "--version"
static bool versionRequested(size_t argc, char *argv[]) {
  for (size_t idx = 1; idx < argc; idx++) {
    if (strcmp(argv[idx], "--version") == 0) {
      return true;
    }
  }
  return false;
}
// determines if any argument in argv is "--help", "-h", or "-?"
static bool helpRequested(size_t argc, char *argv[]) {
  for (size_t idx = 1; idx < argc; idx++) {
    if (strcmp(argv[idx], "--help") == 0 || strcmp(argv[idx], "-h") == 0 ||
        strcmp(argv[idx], "-?") == 0) {
      return true;
    }
  }
  return false;
}

// possible return values for main
enum {
  SUCCESS = EXIT_SUCCESS,
  OPTION_ERROR,
  FILE_ERROR,
  PARSE_ERROR,
  BUILD_STAB_ERROR,
  TYPECHECK_ERROR,
};

// compile the given declaration and code files into one assembly file per code
// file, given the flags
int main(int argc, char *argv[]) {
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
    return SUCCESS;
  } else if (versionRequested((size_t)argc, argv)) {
    printf(
        "%s\n"
        "Copyright 2020 Justin Hu\n"
        "This software is licensed under the Apache License, Version 2.0.\n"
        "See the \"LICENSE\" file for copying conditions.\n"
        "This software is distributed on an \"AS IS\" BASIS, WITHOUT "
        "WARRANTIES OR\n"
        "CONDITIONS OF ANY KIND\n",
        VERSION_STRING);
    return SUCCESS;
  }

  // parse options, get number of files
  size_t numFiles;
  if (parseArgs((size_t)argc, (char const *const *)argv, &numFiles) != 0)
    return OPTION_ERROR;

  // fill in global file list
  if (parseFiles((size_t)argc, (char const *const *)argv, numFiles) != 0)
    return FILE_ERROR;

  return SUCCESS;
}