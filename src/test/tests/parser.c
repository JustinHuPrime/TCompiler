// Copyright 2020-2021 Justin Hu
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

#include <assert.h>
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
  assert("couldn't get length of actual" && actualLen >= 0);

  rewind(actualFile);
  char *actualBuffer = malloc((unsigned long)actualLen + 1);
  actualBuffer[actualLen] = '\0';
  unsigned long readLen =
      fread(actualBuffer, sizeof(char), (unsigned long)actualLen, actualFile);
  assert("couldn't read actual" && readLen == (unsigned long)actualLen);

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

static void testModuleParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/moduleWithId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/moduleWithId.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/moduleWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/moduleWithScopedId.txt"));
  nodeFree(entries[0].ast);
}

static void testImportParser(void) {
  FileListEntry entries[3];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/input/importWithId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/target.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/importWithId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/input/importWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/targetWithScope.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/importWithScopedId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  fileList.size = 3;
  entries[0].inputFilename = "testFiles/parser/input/multipleImports.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/target.td";
  entries[1].isCode = false;
  entries[1].errored = false;
  entries[2].inputFilename = "testFiles/parser/input/targetWithScope.td";
  entries[2].isCode = false;
  entries[2].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/multipleImports.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
  nodeFree(entries[2].ast);
}

static void testFunDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/funDefnNoBodyNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyNoArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/funDefnNoBodyOneArg.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyOneArg.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/funDefnNoBodyManyArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyManyArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/funDefnOneBodyNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnOneBodyNoArgs.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/funDefnManyBodiesNoArgs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/funDefnManyBodiesNoArgs.txt"));
  nodeFree(entries[0].ast);
}

static void testVarDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/varDefnOneIdNoInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnOneIdNoInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDefnOneIdWithInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnOneIdWithInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDefnMany.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDefnMany.txt"));
  nodeFree(entries[0].ast);
}

static void testFunDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/input/funDeclNoArgs.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclNoArgs.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/input/funDeclOneArg.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclOneArg.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/input/funDeclManyArgs.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclManyArgs.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testVarDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/input/varDeclOneId.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDeclOneId.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDeclManyIds.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDeclManyIds.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testOpaqueDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  entries[0].inputFilename = "testFiles/parser/input/opaqueNoDefn.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/empty.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/opaqueNoDefn.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);

  entries[0].inputFilename = "testFiles/parser/input/opaqueWithDefn.td";
  entries[0].isCode = false;
  entries[0].errored = false;
  entries[1].inputFilename = "testFiles/parser/input/opaqueWithDefn.tc";
  entries[1].isCode = true;
  entries[1].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/opaqueWithDefn.txt"));
  nodeFree(entries[0].ast);
  nodeFree(entries[1].ast);
}

static void testStructDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/structOneField.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/structOneField.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/structManyFields.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/structManyFields.txt"));
  nodeFree(entries[0].ast);
}

static void testUnionDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/unionOneOption.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/unionOneOption.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/unionManyOptions.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/unionManyOptions.txt"));
  nodeFree(entries[0].ast);
}

static void testEnumDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/enumOneConstant.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumOneConstant.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/enumManyConstants.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/enumManyConstants.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/enumLiteralInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumLiteralInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/enumEnumInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumEnumInit.txt"));
  nodeFree(entries[0].ast);
}

static void testTypedefDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/typedef.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/typedef.txt"));
  nodeFree(entries[0].ast);
}

static void testCompoundStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/compoundStmtOneStmt.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/compoundStmtOneStmt.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/compoundStmtManyStmts.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/compoundStmtManyStmts.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/compoundStmtNestedStmts.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/compoundStmtNestedStmts.txt"));
  nodeFree(entries[0].ast);
}

static void testIfStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/ifNoElse.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/ifNoElse.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/ifWithElse.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/ifWithElse.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/ifWithCompoundStmts.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/ifWithCompoundStmts.txt"));
  nodeFree(entries[0].ast);
}

static void testWhileStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/while.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/while.txt"));
  nodeFree(entries[0].ast);
}

static void testDoWhileStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/doWhile.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/doWhile.txt"));
  nodeFree(entries[0].ast);
}

static void testForStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/forNoInitNoIncrement.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/forNoInitNoIncrement.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/forWithInitNoIncrement.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forWithInitNoIncrement.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/forNoInitWithIncrement.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forNoInitWithIncrement.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/forWithInitWithIncrement.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forWithInitWithIncrement.txt"));
  nodeFree(entries[0].ast);
}

static void testSwitchStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/switchStmtOneCase.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchStmtOneCase.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/switchStmtManyCases.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchStmtManyCases.txt"));
  nodeFree(entries[0].ast);
}

static void testBreakStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/break.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/break.txt"));
  nodeFree(entries[0].ast);
}

static void testContinueStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/continue.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/continue.txt"));
  nodeFree(entries[0].ast);
}

static void testReturnStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/returnVoid.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/returnVoid.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/returnValue.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/returnValue.txt"));
  nodeFree(entries[0].ast);
}

static void testAsmStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/asm.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/asm.txt"));
  nodeFree(entries[0].ast);
}

static void testVariableDefinitionStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/varDefnStmtOneVar.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtOneVar.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDefnStmtManyVars.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtManyVars.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDefnStmtExprInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtExprInit.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/varDefnStmtMultiInit.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtMultiInit.txt"));
  nodeFree(entries[0].ast);
}

static void testExpressionStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/expression.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/expression.txt"));
  nodeFree(entries[0].ast);
}

static void testOpaqueDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/opaqueDeclStmtNoDefn.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/opaqueDeclStmtNoDefn.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/opaqueDeclStmtWithDefn.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/opaqueDeclStmtWithDefn.txt"));
  nodeFree(entries[0].ast);
}

static void testStructDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/structDeclStmtOneField.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/structDeclStmtOneField.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/structDeclStmtManyFields.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/structDeclStmtManyFields.txt"));
  nodeFree(entries[0].ast);
}

static void testUnionDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/unionDeclStmtOneOption.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/unionDeclStmtOneOption.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/unionDeclStmtManyOptions.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/unionDeclStmtManyOptions.txt"));
  nodeFree(entries[0].ast);
}

static void testEnumDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename =
      "testFiles/parser/input/enumDeclStmtOneConstant.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/enumDeclStmtOneConstant.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename =
      "testFiles/parser/input/enumDeclStmtManyConstants.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/enumDeclStmtManyConstants.txt"));
  nodeFree(entries[0].ast);
}

static void testTypedefDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/typedefDeclStmt.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/typedefDeclStmt.txt"));
  nodeFree(entries[0].ast);
}

static void testNullStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/nullStmt.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/nullStmt.txt"));
  nodeFree(entries[0].ast);
}

static void testSwitchCaseParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/switchCaseOneValue.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchCaseOneValue.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/switchCaseManyValues.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchCaseManyValues.txt"));
  nodeFree(entries[0].ast);
}

static void testSwitchDefaultParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/switchDefault.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/switchDefault.txt"));
  nodeFree(entries[0].ast);
}

static void testSeqExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/seqExprOne.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/seqExprOne.txt"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/input/seqExprMany.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/seqExprMany.txt"));
  nodeFree(entries[0].ast);
}

static void testAssignmentExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/assignmentExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/assignmentExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testTernaryExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/ternaryExpr.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/ternaryExpr.txt"));
  nodeFree(entries[0].ast);
}

static void testLogicalExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/logicalExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/logicalExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testBitwiseExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/bitwiseExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/bitwiseExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testEqualityExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/equalityExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/equalityExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testComparisonExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/comparisonExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/comparisonExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testSpaceshipExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/spaceshipExpr.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/spaceshipExpr.txt"));
  nodeFree(entries[0].ast);
}

static void testShiftExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/shiftExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/shiftExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testAdditionExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/additionExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/additionExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testMultiplicationExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/multiplicationExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/multiplicationExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testPrefixExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/prefixExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/prefixExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testPostfixExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/postfixExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/postfixExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testPrimaryExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/primaryExprs.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/primaryExprs.txt"));
  nodeFree(entries[0].ast);
}

static void testTypeParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  entries[0].inputFilename = "testFiles/parser/input/types.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/types.txt"));
  nodeFree(entries[0].ast);
}

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

  testCompoundStmtParser();
  testIfStmtParser();
  testWhileStmtParser();
  testDoWhileStmtParser();
  testForStmtParser();
  testSwitchStmtParser();
  testBreakStmtParser();
  testContinueStmtParser();
  testReturnStmtParser();
  testAsmStmtParser();
  testVariableDefinitionStmtParser();
  testExpressionStmtParser();
  testOpaqueDeclStmtParser();
  testStructDeclStmtParser();
  testUnionDeclStmtParser();
  testEnumDeclStmtParser();
  testTypedefDeclStmtParser();
  testNullStmtParser();

  testSwitchCaseParser();
  testSwitchDefaultParser();

  testSeqExprParser();
  testAssignmentExprParser();
  testTernaryExprParser();
  testLogicalExprParser();
  testBitwiseExprParser();
  testEqualityExprParser();
  testComparisonExprParser();
  testSpaceshipExprParser();
  testShiftExprParser();
  testAdditionExprParser();
  testMultiplicationExprParser();
  testPrefixExprParser();
  testPostfixExprParser();
  testPrimaryExprParser();

  testTypeParser();
}