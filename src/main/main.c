// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// The primary driver for the TLC.

#include "dependencyGraph/grapher.h"
#include "parser/moduleNodeTable.h"
#include "parser/parser.h"
#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/options.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
  Report *report = reportCreate();

  // Sort the given files, and validate them
  FileList *files = sortFiles(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Read the given options, and validate them
  Options *options = parseArgs(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    optionsDestroy(options);
    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Generate dependency graph
  ModuleInfoTable *moduleInfo = moduleInfoTableCreate(report, options, files);

  // lex+parse phase
  // ModuleNodeTablePair *asts = parseFiles(report, files);

  // check that decls are decls and codes are codes
  // Report *, ModuleNodeTablePair * -> Report *, ModuleNodeTablePair *

  // symbol table building + type check phase
  // Report *, ModuleNodeTablePair * -> Report *, ModuleNodeTablePair *

  // translation into IR

  // IR level optimizations

  // translation into target language

  // target language optimizations

  // write-out

  // cleanup
  moduleInfoTableDestroy(moduleInfo);
  optionsDestroy(options);
  fileListDestroy(files);
  reportDestroy(report);

  return EXIT_SUCCESS;
}