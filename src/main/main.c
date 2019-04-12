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

  // Read the given options, and validate them
  Options *options = parseArgs(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    optionsDestroy(options);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Sort the given files, and validate them
  FileList *files = sortFiles(report, options, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    fileListDestroy(files);
    optionsDestroy(options);
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
  // Report *, ModuleNodeTablePair * -> Report *, ModuleNodeTablePair *,
  // SymbolTable *

  // translation into IR
  // Report *, ModuleNodeTablePair *, SymbolTable * -> Report *, IRList *

  // IR level optimizations
  // IRList * -> IRList *

  // translation into target language
  // IRList * -> SEPList * OR X86List *

  // target language optimizations
  // SEPList * OR X86List * -> SEPList * OR X86List *

  // write-out
  // SEPList * OR X86List * -> (void)

  // cleanup
  moduleInfoTableDestroy(moduleInfo);
  fileListDestroy(files);
  optionsDestroy(options);
  reportDestroy(report);

  return EXIT_SUCCESS;
}