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

#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "fileList.h"
#include "tests.h"

static bool idEqual(Node *id, char const *compare) {
  char *stringified = stringifyId(id);
  bool retval = strcmp(stringified, compare) == 0;
  free(stringified);

  if (id->type == NT_SCOPEDID) {
    for (size_t idx = 0; idx < id->data.scopedId.components->size; ++idx) {
      Node *component = id->data.scopedId.components->elements[idx];
      if (component->data.id.entry != NULL) retval = false;
    }
  }
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
  test("there are no imports", entries[0].ast->data.file.imports->size == 0);
  test("there are no bodies", entries[0].ast->data.file.bodies->size == 0);
  test("the file stab is empty", entries[0].ast->data.file.stab->size == 0);
  test("the module is a module",
       entries[0].ast->data.file.module->type == NT_MODULE);
  test("the module's id is 'foo'",
       idEqual(entries[0].ast->data.file.module->data.module.id, "foo"));
  test("the module's id has no stab entry listed",
       entries[0].ast->data.file.module->data.module.id->data.id.entry == NULL);
  nodeFree(entries[0].ast);

  entries[0].inputFilename = "testFiles/parser/moduleWithScopedId.tc";
  entries[0].isCode = true;
  entries[0].errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entries[0].errored == false);
  test("there are no imports", entries[0].ast->data.file.imports->size == 0);
  test("there are no bodies", entries[0].ast->data.file.bodies->size == 0);
  test("the file stab is empty", entries[0].ast->data.file.stab->size == 0);
  test("the module is a module",
       entries[0].ast->data.file.module->type == NT_MODULE);
  test("the module's id is foo::bar::baz",
       idEqual(entries[0].ast->data.file.module->data.module.id,
               "foo::bar::baz"));
  test("the module's id has no stab entry listed",
       entries[0].ast->data.file.module->data.module.id->data.scopedId.entry ==
           NULL);
  nodeFree(entries[0].ast);
}

static void testImportParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;
}

void testParser(void) {
  testModuleParser();
  testImportParser();
}