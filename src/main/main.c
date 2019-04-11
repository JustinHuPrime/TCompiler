// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// The primary driver for the TLC.

#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/options.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
  Report *report = reportCreate();

  // Sort the given files, and validate them
  // Report *, int, char ** -> Report *, FileList *
  FileList *files = sortFiles(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Read the given options, and validate them
  // Report *, int, char ** -> Report *, Options *
  Options *options = parseArgs(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    optionsDestroy(options);
    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // lex+parse phase (also includes circular dependency check)
  // Report *, FileList * -> Report *, ModuleNodeTablePair *
  // ModuleNodeTablePair *asts = parseFiles(report, fileList);

  // check that decls are decls and codes are codes
  // Report *, ModuleNodeTablePair * -> Report *, ModuleNodeTablePair *

  // symbol table building + type check phase
  // Report *, ModuleNodeTablePair * -> Report *, ModuleNodeTablePair *

  return EXIT_SUCCESS;
}