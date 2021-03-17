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
 * tests for the type checker
 */

#include "typechecker/typechecker.h"

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "engine.h"
#include "fileList.h"
#include "parser/parser.h"
#include "tests.h"
#include "util/format.h"

// static void testVarInitialization(void) {}
// static void testAssignmentExprs(void) {}
// static void testTernaryExprs(void) {}
// static void testLogicalExprs(void) {}
// static void testBitwiseExprs(void) {}
// static void testEqualityExprs(void) {}
// static void testComparisonExprs(void) {}
// static void testSpaceshipExprs(void) {}
// static void testShiftExprs(void) {}
// static void testAdditionExprs(void) {}
// static void testMultiplicationExprs(void) {}
// static void testPrefixExprs(void) {}
// static void testPostfixExprs(void) {}
// static void testPrimaryExprs(void) {}

void testTypechecker(void) {
  DIR *accepted = opendir("testFiles/typechecker/accepted");
  assert("couldn't open accepted files dir" && accepted != NULL);

  for (struct dirent *entry = readdir(accepted); entry != NULL;
       entry = readdir(accepted)) {
    FileListEntry entries[1];
    fileList.entries = &entries[0];
    fileList.size = 1;

    if (strncmp(entry->d_name, ".", 1) == 0) continue;

    char *name = format("testFiles/typechecker/accepted/%s", entry->d_name);
    entries[0].inputFilename = name;
    entries[0].isCode = true;
    entries[0].errored = false;

    assert("couldn't parse file in testTypechecker's accepted file list" &&
           parse() == 0);
    test("type checker accepts the file", typecheck() == 0);
    nodeFree(entries[0].ast);
    free(name);
  }

  DIR *rejected = opendir("testFiles/typechecker/rejected");
  assert("couldn't open rejected files dir" && rejected != NULL);
  for (struct dirent *entry = readdir(accepted); entry != NULL;
       entry = readdir(accepted)) {
    FileListEntry entries[1];
    fileList.entries = &entries[0];
    fileList.size = 1;

    char *name = format("testFiles/typechecker/rejected/%s", entry->d_name);
    entries[0].inputFilename = name;
    entries[0].isCode = true;
    entries[0].errored = false;

    assert("couldn't parse file in testTypechecker's rejected file list" &&
           parse() == 0);
    test("type checker rejects the file", typecheck() != 0);
    nodeFree(entries[0].ast);
    free(name);
  }
}