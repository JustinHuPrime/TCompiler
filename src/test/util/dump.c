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

#include "util/dump.h"

#include <assert.h>

#include "engine.h"

bool dumpEqual(FileListEntry *entry, void (*dump)(FILE *, FileListEntry *),
               char const *expectedFilename) {
  FILE *actualFile = tmpfile();
  dump(actualFile, entry);
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