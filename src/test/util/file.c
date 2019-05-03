// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for file streams

#include "util/file.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void fileTest(TestStatus *status) {
  File *f;

  // ctor
  f = fOpen("testFiles/fileStreamTestFileGoodNonempty.txt");
  test(status,
       "[util] [fileStream] [ctor] good, non-empty file does not produce error",
       f != NULL);
  fClose(f);

  f = fOpen("testFiles/fileStreamTestFileGoodEmpty.txt");
  test(status,
       "[util] [fileStream] [ctor] good, empty file does not produce error",
       f != NULL);
  fClose(f);

  f = fOpen("testFiles/fileStreamTestFileDNE.txt");
  test(status, "[util] [fileStream] [ctor] nonexistent file produces error",
       f == NULL);

  // fGet
  f = fOpen("testFiles/fileStreamTestFileTwoChar.txt");
  test(status, "[util] [fileStream] [fGet] getting a character works",
       fGet(f) == 'a');
  test(status,
       "[util] [fileStream] [fGet] getting more than one character works",
       fGet(f) == 'b');
  test(status, "[util] [fileStream] [fGet] get at end of file produces EOF",
       fGet(f) == F_EOF);

  // unGet
  fUnget(f);
  test(status, "[util] [fileStream] [fUnget] unget at EOF behaves properly",
       fGet(f) == 'b');
  fUnget(f);
  fUnget(f);
  test(status, "[util] [fileStream] [fUnget] unget not at EOF behaves properly",
       fGet(f) == 'a');
  fClose(f);

  // buffer boundary behaviour
  test(status,
       "[util] [fileStream] [buffer boundary] expect buffer size to be 4096",
       F_BUFFER_SIZE == 4096);
  f = fOpen("testFiles/fileStreamTestFile4097Char.txt");
  for (size_t n = 0; n < F_BUFFER_SIZE; n++) fGet(f);
  test(status,
       "[util] [fileStream] [buffer boundary] get after buffer boundary "
       "produces correct value",
       fGet(f) == '$');
  test(status,
       "[util] [fileStream] [buffer boundary] get after buffer boundary "
       "produces correct EOF",
       fGet(f) == F_EOF);
  fUnget(f);
  fUnget(f);
  fUnget(f);
  test(status,
       "[util] [fileStream] [buffer boundary] unget to before buffer "
       "boundary produces correct offset",
       fGet(f) == '\n');
  fClose(f);
}