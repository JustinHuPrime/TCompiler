// Copyright 2019-2021 Justin Hu
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

// Calls all test modules and reports the stats

#include <string.h>

#include "engine.h"
#include "tests.h"

int main(int argc, char *argv[]) {
  if (argc > 2) {
    fprintf(stderr, "usage: %s [suite]\n", argv[0]);
    return -1;
  }

  testStatusInit();

  if (argc < 2 || strcmp(argv[1], "bigInteger") == 0) testBigInteger();
  if (argc < 2 || strcmp(argv[1], "conversions") == 0) testConversions();

  if (argc < 2 || strcmp(argv[1], "commandLineArgs") == 0)
    testCommandLineArgs();
  if (argc < 2 || strcmp(argv[1], "lexer") == 0) testLexer();
  if (argc < 2 || strcmp(argv[1], "parser") == 0) testParser();

  return testStatusStatus();
}