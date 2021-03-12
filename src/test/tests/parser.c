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

static bool dumpEqual(FileListEntry *entry, char const *expected) {
  FILE *temp = tmpfile();
  astDump(temp, entry);
  fflush(temp);

  long len = ftell(temp);
  if (len < 0) {
    fclose(temp);
    return false;
  }

  rewind(temp);
  char *buffer = malloc((unsigned long)len + 1);
  buffer[len] = '\0';
  if (fread(buffer, sizeof(char), (unsigned long)len, temp) !=
      (unsigned long)len) {
    fclose(temp);
    return false;
  }

  bool retval = strcmp(buffer, expected) == 0;
  fclose(temp);
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
       dumpEqual(&entries[0],
                 "testFiles/parser/moduleWithId.tc (code):\n"
                 "FILE(1, 1, STAB(), MODULE(1, 1, ID(1, 8, foo)))\n"));
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/moduleWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test(
      "ast is correct",
      dumpEqual(
          &entries[0],
          "testFiles/parser/moduleWithScopedId.tc (code):\n"
          "FILE(1, 1, STAB(), MODULE(1, 1, SCOPEDID(1, 8, foo::bar::baz)))\n"));
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
  // TODO: test ast dump equality
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
  // TODO: test ast dump equality
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
  // TODO: test ast dump equality
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
  // TODO: test ast dump equality
  nodeFree(entries[0].ast);
}

static void testVarDefnParser(void) {
  // TODO
}

static void testFunDeclParser(void) {
  // TODO
}

static void testVarDeclParser(void) {
  // TODO
}

static void testOpaqueDeclParser(void) {
  // TODO
}

static void testStructDeclParser(void) {
  // TODO
}

static void testUnionDeclParser(void) {
  // TODO
}

static void testEnumDeclParser(void) {
  // TODO
}

static void testTypedefDeclParser(void) {
  // TODO
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
}