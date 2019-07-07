// Copyright 2019 Justin Hu
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

#include "ast/printer.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "typecheck/buildSymbolTable.h"
#include "typecheck/typecheck.h"
#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/options.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool versionRequested(size_t argc, char *argv[]) {
  for (size_t idx = 1; idx < argc; idx++) {
    if (strcmp(argv[idx], "--version") == 0) {
      return true;
    }
  }
  return false;
}
static bool helpRequested(size_t argc, char *argv[]) {
  for (size_t idx = 1; idx < argc; idx++) {
    if (strcmp(argv[idx], "--help") == 0 || strcmp(argv[idx], "-h") == 0 ||
        strcmp(argv[idx], "-?") == 0) {
      return true;
    }
  }
  return false;
}

typedef enum {
  SUCCESS = EXIT_SUCCESS,
  OPTION_ERROR,
  FILE_ERROR,
  PARSE_ERROR,
  BUILD_STAB_ERROR,
  TYPECHECK_ERROR,
} ReturnValue;

int main(int argc, char *argv[]) {
  if (versionRequested((size_t)argc, argv)) {
    printf(
        "T Language Compiler (tlc) version 0.0.1\n"
        "Copyright 2019 Justin Hu\n"
        "This software is licensed under the Apache License, Version 2.0.\n"
        "See the \"LICENSE\" file for copying conditions.\n"
        "This software is distributed on an \"AS IS\" BASIS, WITHOUT "
        "WARRANTIES OR\n"
        "CONDITIONS OF ANY KIND\n");
    return SUCCESS;
  } else if (helpRequested((size_t)argc, argv)) {
    printf(
        "Usage: tlc [options] file...\n"
        "For more information, see the 'README.md' file.\n"
        "\n"
        "Options:\n"
        "  --help, -h, -?  Display this information, and stop\n"
        "  --version       Display version information, and stop\n"
        "  --arch          Set the target architecture\n"
        "  -W              Configure warning options\n"
        "  --debug-dump    Configure debug information\n"
        "\n"
        "Please report bugs at "
        "<https://github.com/JustinHuPrime/TCompiler/issues>\n");
    return SUCCESS;
  }

  Report report;
  reportInit(&report);

  // Read the given options, and validate them
  Options options;

  parseOptions(&options, &report, (size_t)argc, (char const *const *)argv);
  if (reportState(&report) == RPT_ERR) {
    optionsUninit(&options);
    reportUninit(&report);
    return OPTION_ERROR;
  }

  // Sort the given files, and validate them
  FileList files;
  parseFiles(&files, &report, &options, (size_t)argc,
             (char const *const *)argv);
  if (reportState(&report) == RPT_ERR) {
    fileListUninit(&files);
    optionsUninit(&options);
    reportUninit(&report);
    return FILE_ERROR;
  }

  // debug stop for lex
  if (optionsGet(&options, optionDebugDump) == O_DD_LEX) {
    Report dumpReport;
    reportInit(&dumpReport);
    lexDump(&dumpReport, &files);
    reportUninit(&dumpReport);
  }

  // parse the files
  ModuleAstMapPair asts;
  parse(&asts, &report, &options, &files);
  if (reportState(&report) == RPT_ERR) {
    moduleAstMapPairUninit(&asts);
    fileListUninit(&files);
    optionsUninit(&options);
    reportUninit(&report);
    return PARSE_ERROR;
  }

  fileListUninit(&files);

  // debug stop for parse
  if (optionsGet(&options, optionDebugDump) == O_DD_PARSE_PRETTY) {
    for (size_t idx = 0; idx < asts.decls.capacity; idx++) {
      if (asts.decls.keys[idx] != NULL) {
        nodePrint(asts.decls.values[idx]);
      }
    }
    for (size_t idx = 0; idx < asts.codes.capacity; idx++) {
      if (asts.codes.keys[idx] != NULL) {
        nodePrint(asts.codes.values[idx]);
      }
    }
  }
  if (optionsGet(&options, optionDebugDump) == O_DD_PARSE_STRUCTURE) {
    for (size_t idx = 0; idx < asts.decls.capacity; idx++) {
      if (asts.decls.keys[idx] != NULL) {
        nodePrintStructure(asts.decls.values[idx]);
      }
    }
    for (size_t idx = 0; idx < asts.codes.capacity; idx++) {
      if (asts.codes.keys[idx] != NULL) {
        nodePrintStructure(asts.codes.values[idx]);
      }
    }
  }

  buildSymbolTables(&report, &options, &asts);
  if (reportState(&report) == RPT_ERR) {
    moduleAstMapPairUninit(&asts);
    optionsUninit(&options);
    reportUninit(&report);
    return BUILD_STAB_ERROR;
  }

  typecheck(&report, &options, &asts);
  if (reportState(&report) == RPT_ERR) {
    moduleAstMapPairUninit(&asts);
    optionsUninit(&options);
    reportUninit(&report);
    return TYPECHECK_ERROR;
  }

  // clean up
  moduleAstMapPairUninit(&asts);
  optionsUninit(&options);
  reportUninit(&report);

  return SUCCESS;
}