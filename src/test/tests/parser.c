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

/**
 * @file
 * tests for the parser
 */

#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/dump.h"
#include "engine.h"
#include "fileList.h"
#include "tests.h"

static bool dumpEqual(FileListEntry *entry, char const *expectedFilename) {
  FILE *actualFile = tmpfile();
  astDump(actualFile, entry);
  fflush(actualFile);

  long actualLen = ftell(actualFile);
  if (actualLen < 0) {
    fprintf(stderr, "couldn't get length of actual for %s\n", expectedFilename);

    fclose(actualFile);
    return false;
  }

  rewind(actualFile);
  char *actualBuffer = malloc((unsigned long)actualLen + 1);
  actualBuffer[actualLen] = '\0';
  if (fread(actualBuffer, sizeof(char), (unsigned long)actualLen, actualFile) !=
      (unsigned long)actualLen) {
    fprintf(stderr, "couldn't read actual for %s\n", expectedFilename);

    free(actualBuffer);
    fclose(actualFile);
    return false;
  }

  FILE *expectedFile = fopen(expectedFilename, "rb");
  if (expectedFile == NULL) {
    fprintf(stderr, "couldn't read expected for %s\n", expectedFilename);

    free(actualBuffer);
    fclose(actualFile);
    return false;
  }

  fseek(expectedFile, 0, SEEK_END);
  long expectedLen = ftell(expectedFile);
  if (expectedLen < 0) {
    fprintf(stderr, "couldn't get length of expected for %s\n",
            expectedFilename);

    fclose(expectedFile);
    free(actualBuffer);
    fclose(actualFile);
    return false;
  }

  rewind(expectedFile);
  char *expectedBuffer = malloc((unsigned long)expectedLen + 1);
  expectedBuffer[expectedLen] = '\0';
  if (fread(expectedBuffer, sizeof(char), (unsigned long)expectedLen,
            expectedFile) != (unsigned long)expectedLen) {
    fprintf(stderr, "couldn't read expected for %s\n", expectedFilename);

    free(expectedBuffer);
    fclose(expectedFile);
    free(actualBuffer);
    fclose(actualFile);
    return false;
  }

  bool retval = strcmp(actualBuffer, expectedBuffer) == 0;

  free(expectedBuffer);
  fclose(expectedFile);
  free(actualBuffer);
  fclose(actualFile);
  return retval;
}

static void testModuleParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/moduleWithId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/moduleWithId.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/moduleWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/moduleWithScopedId.txt"));
  nodeFree(entries[0].ast);
}

static void testImportParser(void) {
  FileListEntry entries[3];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/importWithId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/target.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/importWithId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/importWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/targetWithScope.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/importWithScopedId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  fileList.size = 3;
  entries[0].inputFilename = "testFiles/parser/multipleImports.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/target.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  entries[2].inputFilename = "testFiles/parser/targetWithScope.td";
  entries[2].isCode = false;
  entries[2].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/multipleImports.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
  nodeFree(entries[2].ast);
}

static void testFunDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/funDefnNoBodyNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/funDefnNoBodyNoArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/funDefnNoBodyOneArg.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/funDefnNoBodyOneArg.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/funDefnNoBodyManyArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/funDefnNoBodyManyArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/funDefnOneBodyNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/funDefnOneBodyNoArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/funDefnManyBodiesNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/funDefnManyBodiesNoArgs.txt"));
  nodeFree(entries[0].ast);
}

static void testVarDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/varDefnOneIdNoInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/varDefnOneIdNoInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/varDefnOneIdWithInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/varDefnOneIdWithInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/varDefnMany.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/varDefnMany.txt"));
  nodeFree(entries[0].ast);
}

static void testFunDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/funDeclNoArgs.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/funDeclNoArgs.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/funDeclOneArg.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/funDeclOneArg.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/funDeclManyArgs.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/funDeclManyArgs.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testVarDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/varDeclOneId.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/varDeclOneId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/varDeclManyIds.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/varDeclManyIds.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testOpaqueDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/opaqueNoDefn.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/opaqueNoDefn.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/opaqueWithDefn.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/opaqueWithDefn.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/opaqueWithDefn.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testStructDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/structOneField.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/structOneField.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/structManyFields.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test(
      "ast is correct",
      dumpEqual(&entries[0], "testFiles/parser/expected/structManyFields.txt"));
  nodeFree(entries[0].ast);
}

static void testUnionDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/unionOneOption.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/unionOneOption.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/unionManyOptions.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test(
      "ast is correct",
      dumpEqual(&entries[0], "testFiles/parser/expected/unionManyOptions.txt"));
  nodeFree(entries[0].ast);
}

static void testEnumDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/enumOneConstant.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/enumOneConstant.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/enumManyConstants.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0],
                 "testFiles/parser/expected/enumManyConstants.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/enumLiteralInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/enumLiteralInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/enumEnumInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/enumEnumInit.txt"));
  nodeFree(entries[0].ast);
}

static void testTypedefDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/typedef.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("ast is correct",
       dumpEqual(&entries[0], "testFiles/parser/expected/typedef.txt"));
  nodeFree(entries[0].ast);
}

// TODO: statement parser tests

// TODO: expression parser tests

// TODO: type parser tests

void testParser(void) {
  testModuleParser();
  testImportParser();

  testFunDefnParser();
  testVarDefnParser();

  testFunDeclParser();
  testVarDeclParser();
  testOpaqueDeclParser();
  testStructDeclParser();
  testUnionDeclParser();
  testEnumDeclParser();
  testTypedefDeclParser();
}