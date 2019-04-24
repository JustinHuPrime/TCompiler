// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Compiles code modules into assembly files, guided by decl modules

#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/options.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
  Report *report = reportCreate();

  // Read the given options, and validate them
  Options *options =
      parseOptions(report, (size_t)argc, (char const *const *)argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    optionsDestroy(options);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Sort the given files, and validate them
  FileList *files =
      parseFiles(report, options, (size_t)argc, (char const *const *)argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    fileListDestroy(files);
    optionsDestroy(options);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // clean up
  fileListDestroy(files);
  optionsDestroy(options);
  reportDestroy(report);

  return EXIT_SUCCESS;
}