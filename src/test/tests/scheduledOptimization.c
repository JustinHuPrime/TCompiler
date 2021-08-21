// Copyright 2021 Justin Hu
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

/**
 * @file
 * tests for the scheduled IR optimizer
 */

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "fileList.h"
#include "ir/dump.h"
#include "ir/ir.h"
#include "optimization/optimization.h"
#include "options.h"
#include "parser/parser.h"
#include "tests.h"
#include "translation/traceSchedule.h"
#include "translation/translation.h"
#include "typechecker/typechecker.h"
#include "util/dump.h"
#include "util/filesystem.h"

void testScheduledOptimization(void) {
  Options original;
  memcpy(&original, &options, sizeof(Options));

  DIR *archs = opendir("testFiles/translation");
  assert("couldn't open arch dir" && archs != NULL);

  for (struct dirent *arch = readdir(archs); arch != NULL;
       arch = readdir(archs)) {
    if (strncmp(arch->d_name, ".", 1) == 0) continue;

    if (strcmp(arch->d_name, "x86_64-linux") == 0) {
      options.arch = OPTION_A_X86_64_LINUX;
    } else {
      assert("unrecognized arch folder name" && false);
    }

    char *inputFolder = format("testFiles/translation/%s/input", arch->d_name);
    char *expectedFolder = format(
        "testFiles/translation/%s/expectedScheduledOptimized", arch->d_name);

    struct dirent **input;
    int inputLen = scandir(inputFolder, &input, noHiddenFilter, alphasort);
    assert("couldn't open input files dir" && inputLen != -1);

    struct dirent **expected;
    int expectedLen =
        scandir(expectedFolder, &expected, noHiddenFilter, alphasort);
    assert("couldn't open expected files dir" && expectedLen != -1);
    assert("different numbers of files in input and expected dirs" &&
           inputLen == expectedLen);

    for (int idx = 0; idx < inputLen; ++idx) {
      struct dirent *entry = input[idx];
      struct dirent *expectedEntry = expected[idx];
      FileListEntry entries[1];
      fileList.entries = &entries[0];
      fileList.size = 1;

      if (strncmp(entry->d_name, ".", 1) == 0) continue;

      char *name = format("testFiles/translation/%s/input/%s", arch->d_name,
                          entry->d_name);
      fileListEntryInit(&entries[0], name, true);

      int parseStatus = parse();
      assert("couldn't parse file in testTranslation's accepted file list" &&
             parseStatus == 0);
      int typecheckStatus = typecheck();
      assert(
          "couldn't typecheck file in testTranslation's accepted file list" &&
          typecheckStatus == 0);
      translate();

      assert("translation produced invalid ir" &&
             validateBlockedIr("translation") == 0);

      optimizeBlockedIr();

      assert("optimization produced invalid ir" &&
             validateBlockedIr("optimization before trace scheduling") == 0);

      traceSchedule();

      assert("trace scheduling produced invalid ir" &&
             validateScheduledIr("trace scheduling") == 0);

      optimizeScheduledIr();

      char *expectedName =
          format("testFiles/translation/%s/expectedScheduledOptimized/%s",
                 arch->d_name, expectedEntry->d_name);

      testDynamic(format("scheduled, optimized ir of %s is correct",
                         entries[0].inputFilename),
                  dumpEqual(&entries[0], irDump, expectedName));

      testDynamic(
          format("scheduled, optimized ir of %s is valid",
                 entries[0].inputFilename),
          validateScheduledIr("optimization after trace scheduling") == 0);

      free(name);
      free(expectedName);
      irFragVectorUninit(&entries[0].irFrags);
      nodeFree(entries[0].ast);
      free(entry);
      free(expectedEntry);
    }
    free(input);
    free(expected);
    free(inputFolder);
    free(expectedFolder);
  }
  closedir(archs);

  memcpy(&options, &original, sizeof(Options));
}