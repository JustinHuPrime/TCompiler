// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// The primary driver for the TLC.

#include "util/errorReport.h"
#include "util/fileList.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
  Report *report = reportCreate();

  // Report *, int, char ** -> Report *, FileList *
  FileList *files = sortFiles(report, argc, argv);

  reportDisplay(report);
  if (reportState(report) == RPT_ERR) {
    fileListDestroy(files);
    reportDestroy(report);
    return EXIT_FAILURE;
  }

  // Report *, int, char ** -> Report *, Options *

  // FileList * -> FileNodeList *

  return EXIT_SUCCESS;
}