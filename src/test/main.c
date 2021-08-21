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

static bool containsString(size_t argc, char **argv, char const *string) {
  for (size_t idx = 1; idx < argc; ++idx) {
    if (strcmp(argv[idx], string) == 0) return true;
  }
  return false;
}

int main(int argc, char *argv[]) {
  bool blessing = containsString((size_t)argc, argv, "--bless");
  testStatusInit(blessing);

  if (blessing && argc <= 1) {
    printf("Warning: no tests specified to bless\n");
    return -1;
  }

  if (argc <= 1 || containsString((size_t)argc, argv, "bigInteger"))
    testBigInteger();
  if (argc <= 1 || containsString((size_t)argc, argv, "conversions"))
    testConversions();

  if (argc <= 1 || containsString((size_t)argc, argv, "commandLineArgs"))
    testCommandLineArgs();
  if (argc <= 1 || containsString((size_t)argc, argv, "lexer")) testLexer();
  if (argc <= 1 || containsString((size_t)argc, argv, "parser")) testParser();
  if (argc <= 1 || containsString((size_t)argc, argv, "typechecker"))
    testTypechecker();
  if (argc <= 1 || containsString((size_t)argc, argv, "translation"))
    testTranslation();
  if (argc <= 1 || containsString((size_t)argc, argv, "blockedOptimization"))
    testBlockedOptimization();
  if (argc <= 1 || containsString((size_t)argc, argv, "traceScheduling"))
    testTraceScheduling();
  if (argc <= 1 || containsString((size_t)argc, argv, "scheduledOptimization"))
    testScheduledOptimization();

  return testStatusStatus();
}