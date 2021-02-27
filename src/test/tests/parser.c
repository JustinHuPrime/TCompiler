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

static void testModuleParser(FileListEntry *entry) {
  entry->inputFilename = "testFiles/parser/moduleWithId.tc";
  entry->isCode = true;
  entry->errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entry->errored == false);
  test("there are no imports", entry->ast->data.file.imports->size == 0);
  test("there are no bodies", entry->ast->data.file.bodies->size == 0);
  test("the file stab is empty", entry->ast->data.file.stab->size == 0);
  test("the module is a module",
       entry->ast->data.file.module->type == NT_MODULE);
  test("the module's id is an id",
       entry->ast->data.file.module->data.module.id->type == NT_ID);
  test("the module's id is 'foo'",
       strcmp(entry->ast->data.file.module->data.module.id->data.id.id,
              "foo") == 0);
  test("the module's id has no stab entry listed",
       entry->ast->data.file.module->data.module.id->data.id.entry == NULL);
  nodeFree(entry->ast);

  entry->inputFilename = "testFiles/parser/moduleWithScopedId.tc";
  entry->isCode = true;
  entry->errored = false;
  test("parser accepts the file", parse() == 0);
  test("file has not errored", entry->errored == false);
  test("there are no imports", entry->ast->data.file.imports->size == 0);
  test("there are no bodies", entry->ast->data.file.bodies->size == 0);
  test("the file stab is empty", entry->ast->data.file.stab->size == 0);
  test("the module is a module",
       entry->ast->data.file.module->type == NT_MODULE);
  test("the module's id is a scoped id",
       entry->ast->data.file.module->data.module.id->type == NT_SCOPEDID);
  test("the module's scoped id has three elements",
       entry->ast->data.file.module->data.module.id->data.scopedId.components
               ->size == 3);
  test("the module's scoped id's first element is an id",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[0])
               ->type == NT_ID);
  test("the module's scoped id's first element is 'foo'",
       strcmp(((Node *)entry->ast->data.file.module->data.module.id->data
                   .scopedId.components->elements[0])
                  ->data.id.id,
              "foo") == 0);
  test("the module's scoped id's first element has no stab entry listed",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[0])
               ->data.id.entry == NULL);
  test("the module's scoped id's second element is an id",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[1])
               ->type == NT_ID);
  test("the module's scoped id's second element is 'bar'",
       strcmp(((Node *)entry->ast->data.file.module->data.module.id->data
                   .scopedId.components->elements[1])
                  ->data.id.id,
              "bar") == 0);
  test("the module's scoped id's second element has no stab entry listed",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[1])
               ->data.id.entry == NULL);
  test("the module's scoped id's third element is an id",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[2])
               ->type == NT_ID);
  test("the module's scoped id's third element is 'baz'",
       strcmp(((Node *)entry->ast->data.file.module->data.module.id->data
                   .scopedId.components->elements[2])
                  ->data.id.id,
              "baz") == 0);
  test("the module's scoped id's third element has no stab entry listed",
       ((Node *)entry->ast->data.file.module->data.module.id->data.scopedId
            .components->elements[2])
               ->data.id.entry == NULL);
  test("the module's id has no stab entry listed",
       entry->ast->data.file.module->data.module.id->data.scopedId.entry ==
           NULL);
  nodeFree(entry->ast);
}

void testParser(void) {
  FileListEntry entries[2];
  fileList.entries = &entries[0];
  fileList.size = 2;

  testModuleParser(&entries[0]);
}