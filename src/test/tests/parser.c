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
#include "ir/ir.h"
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

static void testModuleParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/moduleWithId.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/moduleWithId.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/moduleWithScopedId.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/moduleWithScopedId.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testImportParser(void) {
  FileListEntry entries[3];
  fileList.entries = &entries[0];
  fileList.size = 2;

  fileListEntryInit(&entries[0], "testFiles/parser/input/importWithId.tc",
                    true);
  fileListEntryInit(&entries[1], "testFiles/parser/input/target.td", false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/importWithId.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/importWithScopedId.tc",
                    true);
  fileListEntryInit(&entries[1], "testFiles/parser/input/targetWithScope.td",
                    false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/importWithScopedId.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);

  fileList.size = 3;
  fileListEntryInit(&entries[0], "testFiles/parser/input/multipleImports.tc",
                    true);
  fileListEntryInit(&entries[1], "testFiles/parser/input/target.td", false);
  fileListEntryInit(&entries[2], "testFiles/parser/input/targetWithScope.td",
                    false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/multipleImports.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[2].ast);
  vectorUninit(&entries[2].irFrags, (void (*)(void *))irFragFree);
}

static void testFunDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/funDefnNoBodyNoArgs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyNoArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/funDefnNoBodyOneArg.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyOneArg.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/funDefnNoBodyManyArgs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnNoBodyManyArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/funDefnOneBodyNoArgs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/funDefnOneBodyNoArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/funDefnManyBodiesNoArgs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/funDefnManyBodiesNoArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testVarDefnParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/varDefnOneIdNoInit.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnOneIdNoInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/varDefnOneIdWithInit.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnOneIdWithInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/varDefnMany.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDefnMany.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testFunDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  fileListEntryInit(&entries[0], "testFiles/parser/input/funDeclNoArgs.td",
                    false);
  fileListEntryInit(&entries[1], "testFiles/parser/input/empty.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclNoArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/funDeclOneArg.td",
                    false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclOneArg.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);

  fileListEntryInit(&entries[0], "testFiles/parser/input/funDeclManyArgs.td",
                    false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/funDeclManyArgs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
}

static void testVarDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  fileListEntryInit(&entries[0], "testFiles/parser/input/varDeclOneId.td",
                    false);
  fileListEntryInit(&entries[1], "testFiles/parser/input/empty.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDeclOneId.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/varDeclManyIds.td",
                    false);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/varDeclManyIds.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
}

static void testOpaqueDeclParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  fileListEntryInit(&entries[0], "testFiles/parser/input/opaqueNoDefn.td",
                    false);
  fileListEntryInit(&entries[1], "testFiles/parser/input/empty.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/opaqueNoDefn.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/opaqueWithDefn.td",
                    false);
  fileListEntryInit(&entries[1], "testFiles/parser/input/opaqueWithDefn.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/opaqueWithDefn.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
  nodeFree(entries[1].ast);
  vectorUninit(&entries[1].irFrags, (void (*)(void *))irFragFree);
}

static void testStructDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/structOneField.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/structOneField.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/structManyFields.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/structManyFields.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testUnionDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/unionOneOption.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/unionOneOption.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/unionManyOptions.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/unionManyOptions.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testEnumDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/enumOneConstant.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumOneConstant.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/enumManyConstants.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/enumManyConstants.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/enumLiteralInit.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumLiteralInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/enumEnumInit.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/enumEnumInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testTypedefDeclParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/typedef.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/typedef.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testCompoundStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/compoundStmtOneStmt.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/compoundStmtOneStmt.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/compoundStmtManyStmts.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/compoundStmtManyStmts.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/compoundStmtNestedStmts.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/compoundStmtNestedStmts.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testIfStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/ifNoElse.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/ifNoElse.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/ifWithElse.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/ifWithElse.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/ifWithCompoundStmts.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/ifWithCompoundStmts.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testWhileStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/while.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/while.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testDoWhileStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/doWhile.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/doWhile.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testForStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/forNoInitNoIncrement.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/forNoInitNoIncrement.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/forWithInitNoIncrement.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forWithInitNoIncrement.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/forNoInitWithIncrement.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forNoInitWithIncrement.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/forWithInitWithIncrement.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/forWithInitWithIncrement.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testSwitchStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/switchStmtOneCase.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchStmtOneCase.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/switchStmtManyCases.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchStmtManyCases.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testBreakStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/break.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/break.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testContinueStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/continue.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/continue.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testReturnStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/returnVoid.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/returnVoid.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/returnValue.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/returnValue.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testAsmStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/asm.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/asm.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testVariableDefinitionStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/varDefnStmtOneVar.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtOneVar.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/varDefnStmtManyVars.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtManyVars.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/varDefnStmtExprInit.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtExprInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/varDefnStmtMultiInit.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/varDefnStmtMultiInit.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testExpressionStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/expression.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/expression.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testOpaqueDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/opaqueDeclStmtNoDefn.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/opaqueDeclStmtNoDefn.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/opaqueDeclStmtWithDefn.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/opaqueDeclStmtWithDefn.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testStructDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/structDeclStmtOneField.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/structDeclStmtOneField.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/structDeclStmtManyFields.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/structDeclStmtManyFields.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testUnionDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/unionDeclStmtOneOption.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/unionDeclStmtOneOption.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/unionDeclStmtManyOptions.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/unionDeclStmtManyOptions.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testEnumDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/enumDeclStmtOneConstant.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/enumDeclStmtOneConstant.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(
      &entries[0], "testFiles/parser/input/enumDeclStmtManyConstants.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0],
                "testFiles/parser/expected/enumDeclStmtManyConstants.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testTypedefDeclStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/typedefDeclStmt.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/typedefDeclStmt.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testNullStmtParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/nullStmt.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/nullStmt.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testSwitchCaseParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/switchCaseOneValue.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchCaseOneValue.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/switchCaseManyValues.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/switchCaseManyValues.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testSwitchDefaultParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/switchDefault.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/switchDefault.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testSeqExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/seqExprOne.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/seqExprOne.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);

  fileListEntryInit(&entries[0], "testFiles/parser/input/seqExprMany.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/seqExprMany.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testAssignmentExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/assignmentExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/assignmentExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testTernaryExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/ternaryExpr.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/ternaryExpr.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testLogicalExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/logicalExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/logicalExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testBitwiseExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/bitwiseExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/bitwiseExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testEqualityExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/equalityExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/equalityExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testComparisonExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/comparisonExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/comparisonExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testSpaceshipExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/spaceshipExpr.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/spaceshipExpr.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testShiftExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/shiftExprs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/shiftExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testAdditionExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/additionExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/additionExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testMultiplicationExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0],
                    "testFiles/parser/input/multiplicationExprs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0],
                        "testFiles/parser/expected/multiplicationExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testPrefixExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/prefixExprs.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/prefixExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testPostfixExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/postfixExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/postfixExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testPrimaryExprParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/primaryExprs.tc",
                    true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(
      format("ast of %s is correct", entries[0].inputFilename),
      dumpEqual(&entries[0], "testFiles/parser/expected/primaryExprs.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
}

static void testTypeParser(void) {
  FileListEntry entries[1];
  fileList.entries = &entries[0];
  fileList.size = 1;

  fileListEntryInit(&entries[0], "testFiles/parser/input/types.tc", true);
  testDynamic(format("parser accepts %s", entries[0].inputFilename),
              parse() == 0);
  testDynamic(format("no errors in %s", entries[0].inputFilename),
              entries[0].errored == false);
  testDynamic(format("ast of %s is correct", entries[0].inputFilename),
              dumpEqual(&entries[0], "testFiles/parser/expected/types.txt"));
  nodeFree(entries[0].ast);
  vectorUninit(&entries[0].irFrags, (void (*)(void *))irFragFree);
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