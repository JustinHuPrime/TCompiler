// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

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

  testStatusDisplay(status);

  int retVal = testStatusStatus(status);
  testStatusDestroy(status);
  return retVal;
}