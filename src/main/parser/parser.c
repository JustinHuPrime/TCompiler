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

#include "parser/parser.h"

#include "fileList.h"
#include "parser/buildStab.h"
#include "parser/topLevel.h"
#include "util/container/hashSet.h"
#include "util/container/vector.h"
#include "util/functional.h"

#include <stdio.h>
#include <string.h>

int parse(void) {
  int retval = 0;
  bool errored = false; /**< has any part of the whole thing errored */

  lexerInitMaps();

  // pass one - parse top level stuff, without populating symbol tables
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      errored = true;
      continue;
    }

    fileList.entries[idx].ast = parseFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;

    lexerStateUninit(&fileList.entries[idx]);
  }

  lexerUninitMaps();

  if (errored) return -1;

  // populate fileListEntries with moduleNames
  for (size_t idx = 0; idx < fileList.size; idx++)
    fileList.entries[idx].moduleName = stringifyId(
        fileList.entries[idx].ast->data.file.module->data.module.id);

  // check for duplciate decl modules
  HashSet processed;  // set of all processed modules
  hashSetInit(&processed);
  for (size_t idx = 0; idx < fileList.size; idx++) {
    char const *name = fileList.entries[idx].moduleName;
    if (!fileList.entries[idx].isCode && !hashSetContains(&processed, name)) {
      // for each declaration file, if its name hasn't been processed yet,
      // process it
      Vector duplicates;
      vectorInit(&duplicates);
      for (size_t compareIdx = idx + 1; compareIdx < fileList.size;
           compareIdx++) {
        // check it against each other file
        if (strcmp(name, fileList.entries[idx].moduleName) == 0) {
          // duplicate!
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
          vectorInsert(&duplicates,
                       (char *)fileList.entries[idx].inputFilename);
#pragma GCC diagnostic pop
        }
      }
      if (duplicates.size != 0) {
        fprintf(stderr,
                "%s: error: module %s redeclared in the following files:\n",
                fileList.entries[idx].inputFilename, name);
        for (size_t printIdx = 0; printIdx < duplicates.size; printIdx++)
          fprintf(stderr, "\t%s\n", (char const *)duplicates.elements[idx]);
      }
      vectorUninit(&duplicates, nullDtor);

      hashSetPut(&processed, name);
    }
  }
  hashSetUninit(&processed);

  if (errored) return -1;

  // build module tree
  hashMapInit(&fileList.moduleTree);
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (!fileList.entries[idx].isCode) {
      // for every declaration file, go through the tree, adding nodes if needed
      Node *moduleNameNode =
          fileList.entries[idx].ast->data.file.module->data.module.id;
      if (moduleNameNode->type == NT_ID) {
        // just foo
        ModuleTreeNode *node =
            hashMapGet(&fileList.moduleTree, moduleNameNode->data.id.id);
        if (node == NULL) {
          // node not in the tree - insert it
          node = moduleTreeNodeCreate(&fileList.entries[idx]);
          hashMapPut(&fileList.moduleTree, moduleNameNode->data.id.id, node);
        } else {
          // node is in the tree - already visited foo::bar...
          // guarenteed to be NULL
          node->entry = &fileList.entries[idx];
        }
      } else {
        // foo::bar...
        Vector *components = moduleNameNode->data.scopedId.components;
        HashMap *previous = &fileList.moduleTree;
        ModuleTreeNode *current = NULL;
        for (size_t idx = 0; idx < components->size; idx++) {
          Node *component = components->elements[idx];
          current = hashMapGet(previous, component->data.id.id);
          if (current == NULL) {
            // node not in the tree - insert it
            current = moduleTreeNodeCreate(NULL);
            hashMapPut(previous, component->data.id.id, current);
          }
          previous = &current->children;
        }
        current->entry = &fileList.entries[idx];
      }
    }
  }

  // pass two - populate symbol tables (but don't fill in entries)
  for (size_t idx = 0; idx < fileList.size; idx++)
    parserBuildTopLevelStab(&fileList.entries[idx]);

  // pass three - parse unparsed nodes, populating the symbol table as we go
  // (entries still not filled in)

  return 0;
}