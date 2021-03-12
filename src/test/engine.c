// Copyright 2020 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Implementation of the test engine status object

#include "engine.h"

#include <stdlib.h>

TestStatus status;

void testStatusInit(void) {
  status.numTests = 0;
  status.numPassed = 0;
}
int testStatusStatus(void) {
  if (status.numTests == status.numPassed)
    printf("\x1B[1;92mAll %zu tests passed!\x1B[m\n", status.numTests);
  else
    printf("\x1B[1;91m%zu out of %zu tests failed!\x1B[m\n",
           status.numTests - status.numPassed, status.numTests);
  return status.numTests == status.numPassed ? 0 : -1;
}
void test(char const *name, bool condition) {
  if (condition) {
    ++status.numTests;
    ++status.numPassed;
  } else {
    printf("\x1B[1;91mFAILED: %s\x1B[m\n", name);
    ++status.numTests;
  }
}
void dropLine(void) { fprintf(stderr, "\x1B[1A\x1B[2K"); }