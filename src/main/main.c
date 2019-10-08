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

#include "architecture/x86_64/frame.h"
#include "ast/printer.h"
#include "ir/frame.h"
#include "ir/ir.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "translate/translate.h"
#include "typecheck/buildSymbolTable.h"
#include "typecheck/typecheck.h"
#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/internalError.h"
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
typedef enum {
  SUCCESS = EXIT_SUCCESS,
  OPTION_ERROR,
  FILE_ERROR,
  PARSE_ERROR,
  BUILD_STAB_ERROR,
  TYPECHECK_ERROR,
} ReturnValue;

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
        "  --help, -h, -?  Display this information, and stop\n"
        "  --version       Display version information, and stop\n"
        "  --arch          Set the target architecture\n"
        "  -W              Configure warning options\n"
        "  --debug-dump    Configure debug information\n"
        "\n"
        "Please report bugs at "
        "<https://github.com/JustinHuPrime/TCompiler/issues>\n");
    return SUCCESS;
  } else if (versionRequested((size_t)argc, argv)) {
    printf(
        "T Language Compiler (tlc) version 0.1.0\n"
        "Copyright 2019 Justin Hu and Bronwyn Damm\n"
        "This software is licensed under the Apache License, Version 2.0.\n"
        "See the \"LICENSE\" file for copying conditions.\n"
        "This software is distributed on an \"AS IS\" BASIS, WITHOUT "
        "WARRANTIES OR\n"
        "CONDITIONS OF ANY KIND\n");
    return SUCCESS;
  }

  // Global error report - tracks the error level
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

  // debug stop for lex - displays the tokens in all given files
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

  // no longer need to track file names - file name strings are owned by main
  fileListUninit(&files);

  // debug stop for parse - displays the parse tree as text or in constructor
  // style
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

  // semantic analysis
  // associate a symbol table with each scope, and a symbol table entry with
  // each identifier
  buildSymbolTables(&report, &options, &asts);
  if (reportState(&report) == RPT_ERR) {
    moduleAstMapPairUninit(&asts);
    optionsUninit(&options);
    reportUninit(&report);
    return BUILD_STAB_ERROR;
  }

  // check for type consistency
  typecheck(&report, &options, &asts);
  if (reportState(&report) == RPT_ERR) {
    moduleAstMapPairUninit(&asts);
    optionsUninit(&options);
    reportUninit(&report);
    return TYPECHECK_ERROR;
  }

  // no more user errors - program is well formed, and further errors are all
  // internal compiler errors
  reportUninit(&report);

  // translation into IR
  FileIRFileMap fileMap;

  // architecture specific data
  LabelGeneratorCtor labelGeneratorCtor;
  FrameCtor frameCtor;
  GlobalAccessCtor globalAccessCtor;
  FunctionAccessCtor functionAccessCtor;

  switch (optionsGet(&options, optionArch)) {
    case O_AT_X86: {
      labelGeneratorCtor = x86_64LabelGeneratorCtor;
      frameCtor = x86_64FrameCtor;
      globalAccessCtor = x86_64GlobalAccessCtor;
      functionAccessCtor = x86_64FunctionAccessCtor;
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid architecture specified"); }
  }

  translate(&fileMap, &asts, labelGeneratorCtor, frameCtor, globalAccessCtor,
            functionAccessCtor);

  // debug stop for translate - displays the IR fragments for each file
  if (optionsGet(&options, optionDebugDump) == O_DD_IR) {
    error(__FILE__, __LINE__, "not yet implemented");
  }

  // post translate cleanup
  moduleAstMapPairUninit(&asts);  // no longer using asts

  // optimize
  // TODO: write optimizations

  // backend - everything is architecture specific
  switch (optionsGet(&options, optionArch)) {
    case O_AT_X86: {
      // instruction selection

      // register alloc

      // write-out

      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid architecture specified, furthermore, architecture was "
            "valid during translate (possible memory corruption?)");
    }
  }

  // clean up
  fileIRFileMapUninit(&fileMap);
  optionsUninit(&options);

  return SUCCESS;
}