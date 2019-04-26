// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for file streams

#include "util/fileStream.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void fileStreamTest(TestStatus *status) {
  FileStream *fs;

  // ctor
  fs = fsOpen("testFiles/fileStreamTestFileGoodNonempty.txt");
  test(status,
       "[util] [fileStream] [ctor] good, non-empty file does not produce error",
       fs != NULL);
  fsClose(fs);

  fs = fsOpen("testFiles/fileStreamTestFileGoodEmpty.txt");
  test(status,
       "[util] [fileStream] [ctor] good, empty file does not produce error",
       fs != NULL);
  fsClose(fs);

  fs = fsOpen("testFiles/fileStreamTestFileDNE.txt");
  test(status, "[util] [fileStream] [ctor] nonexistent file produces error",
       fs == NULL);

  // fsGet
  fs = fsOpen("testFiles/fileStreamTestFileTwoChar.txt");
  test(status, "[util] [fileStream] [fsGet] getting a character works",
       fsGet(fs) == 'a');
  test(status,
       "[util] [fileStream] [fsGet] getting more than one character works",
       fsGet(fs) == 'b');
  test(status, "[util] [fileStream] [fsGet] get at end of file produces EOF",
       fsGet(fs) == FS_EOF);

  // unGet
  fsUnget(fs);
  test(status, "[util] [fileStream] [fsUnget] unget at EOF behaves properly",
       fsGet(fs) == 'b');
  fsUnget(fs);
  fsUnget(fs);
  test(status,
       "[util] [fileStream] [fsUnget] unget not at EOF behaves properly",
       fsGet(fs) == 'a');
  fsClose(fs);

  // buffer boundary behaviour
  test(status,
       "[util] [fileStream] [buffer boundary] expect buffer size to be 4096",
       FS_BUFFER_SIZE == 4096);
  fs = fsOpen("testFiles/fileStreamTestFile4097Char.txt");
  for (size_t n = 0; n < FS_BUFFER_SIZE; n++) fsGet(fs);
  test(status,
       "[util] [fileStream] [buffer boundary] get after buffer boundary "
       "produces correct value",
       fsGet(fs) == '$');
  test(status,
       "[util] [fileStream] [buffer boundary] get after buffer boundary "
       "produces correct EOF",
       fsGet(fs) == FS_EOF);
  fsUnget(fs);
  fsUnget(fs);
  fsUnget(fs);
  test(status,
       "[util] [fileStream] [buffer boundary] unget to before buffer "
       "boundary produces correct offset",
       fsGet(fs) == '\n');
}