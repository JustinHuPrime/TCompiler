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
 * tests for the translator
 */

#include "translation/translation.h"

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "fileList.h"
#include "ir/dump.h"
#include "ir/ir.h"
#include "options.h"
#include "parser/parser.h"
#include "tests.h"
#include "typechecker/typechecker.h"

static bool dumpEqual(FileListEntry *entry, char const *expectedFilename) {
  FILE *actualFile = tmpfile();
  irDump(actualFile, entry);
  fflush(actualFile);

  long actualLen = ftell(actualFile);
  assert("couldn't get length of actual" && actualLen >= 0);

  rewind(actualFile);
  char *actualBuffer = malloc((unsigned long)actualLen + 1);
  actualBuffer[actualLen] = '\0';
  unsigned long readLen =
      fread(actualBuffer, sizeof(char), (unsigned long)actualLen, actualFile);
  assert("couldn't read actual" && readLen == (unsigned long)actualLen);

  if (status.bless) {
    FILE *expectedFile = fopen(expectedFilename, "wb");
    assert("couldn't open expected for blessing" && expectedFile != NULL);

    size_t writtenLen = fwrite(actualBuffer, sizeof(char),
                               (unsigned long)actualLen, expectedFile);
    assert("couldn't write to expected for blessing" &&
           writtenLen == (unsigned long)actualLen);

    fclose(expectedFile);
    free(actualBuffer);
    fclose(actualFile);
    return true;
  } else {
    FILE *expectedFile = fopen(expectedFilename, "rb");
    assert("couldn't read expected" && expectedFile != NULL);

    fseek(expectedFile, 0, SEEK_END);
    long expectedLen = ftell(expectedFile);
    assert("couldn't get length of expected" && expectedLen >= 0);

    rewind(expectedFile);
    char *expectedBuffer = malloc((unsigned long)expectedLen + 1);
    expectedBuffer[expectedLen] = '\0';
    readLen = fread(expectedBuffer, sizeof(char), (unsigned long)expectedLen,
                    expectedFile);
    assert("couldn't read expected" && readLen == (unsigned long)expectedLen);

    bool retval = strcmp(actualBuffer, expectedBuffer) == 0;

    free(expectedBuffer);
    fclose(expectedFile);
    free(actualBuffer);
    fclose(actualFile);
    return retval;
  }
}

static int noHiddenFilter(struct dirent const *entry) {
  return strncmp(entry->d_name, ".", 1) != 0;
}
void testTranslation(void) {
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
    char *expectedFolder =
        format("testFiles/translation/%s/expectedUnoptimized", arch->d_name);

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

      char *expectedName =
          format("testFiles/translation/%s/expectedUnoptimized/%s",
                 arch->d_name, expectedEntry->d_name);

      testDynamic(format("ir of %s is correct", entries[0].inputFilename),
                  dumpEqual(&entries[0], expectedName));

      testDynamic(format("ir of %s is valid", entries[0].inputFilename),
                  validateIr("translation") == 0);

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