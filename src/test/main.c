// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Calls all test modules and reports the stats

#include "engine.h"
#include "tests.h"

int main(void) {
  TestStatus *status = testStatusCreate();

  errorReportTest(status);
  fileListTest(status);
  fileTest(status);

  testStatusDisplay(status);

  int retVal = testStatusStatus(status);
  testStatusDestroy(status);
  return retVal;
}