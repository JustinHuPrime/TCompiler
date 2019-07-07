// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// Calls all test modules and reports the stats

#include "engine.h"
#include "unitTests/tests.h"

int main(void) {
  TestStatus status;
  testStatusInit(&status);

  constExpParseIntTest(&status);
  constExpParseFloatTest(&status);
  constExpParseStringTest(&status);
  constExpParseCharTest(&status);
  constExpParseWStringTest(&status);
  constExpParseWCharTest(&status);
  keywordMapTest(&status);
  lexerTest(&status);
  errorReportTest(&status);
  fileListTest(&status);
  fileTest(&status);
  hashTest(&status);
  hashMapTest(&status);
  stringBuilderTest(&status);
  vectorTest(&status);

  testStatusDisplay(&status);

  int retVal = testStatusStatus(&status);
  testStatusUninit(&status);
  return retVal;
}