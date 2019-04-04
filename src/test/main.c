// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#include "engine.h"

#include "ast/constExpParseTest.h"

int main(int argc, char *argv[]) {
  TestStatus *status = testStatusCreate();

  constExpParseTest(status);

  testStatusDisplay(status, stdout);

  int retVal = testStatusStatus(status);
  testStatusDestroy(status);
  return retVal;
}