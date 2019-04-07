// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Calls all test modules and reports the stats

#include "engine.h"
#include "tests.h"

int main(int argc, char *argv[]) {
  TestStatus *status = testStatusCreate();

  constExpParseIntTest(status);
  constExpParseFloatTest(status);
  constExpParseStringTest(status);
  constExpParseCharTest(status);
  constExpParseWStringTest(status);
  constExpParseWCharTest(status);
  nodeListTest(status);
  nodeListPairTest(status);

  testStatusDisplay(status);

  int retVal = testStatusStatus(status);
  testStatusDestroy(status);
  return retVal;
}