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

  // Report *, int, char ** -> Report *, FileList *
  FileList *files = sortFiles(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Report *, int, char ** -> Report *, Options *
  Options *options = parseArgs(report, (size_t)argc, argv);
  if (reportState(report) == RPT_ERR) {
    reportDisplay(report);

    optionsDestroy(options);
    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // FileList * -> FileNodeList *

  return EXIT_SUCCESS;
}