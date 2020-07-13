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

#include "parser/buildStab.h"

#include "ast/ast.h"
#include "ast/environment.h"
#include "internalError.h"
#include "util/container/hashMap.h"
#include "util/container/hashSet.h"
#include "util/container/vector.h"
#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int buildModuleMap(void) {
  bool errored = false;

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
        errored = true;
      }
      vectorUninit(&duplicates, nullDtor);

      hashSetPut(&processed, name);
    }
  }
  hashSetUninit(&processed);

  if (errored) return -1;

  // build module map
  hashMapInit(&fileList.moduleMap);
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (!fileList.entries[idx].isCode)
      hashMapPut(&fileList.moduleMap, fileList.entries[idx].moduleName,
                 &fileList.entries[idx]);
  }

  return 0;
}

/**
 * complain about a redeclaration
 */
static void errorRedeclaration(char const *file, size_t line, size_t character,
                               char const *name, char const *collidingFile,
                               size_t collidingLine, size_t collidingChar) {
  fprintf(stderr,
          "%s:%zu:%zu: error: redeclaration of %s as a different "
          "kind of identifier\n",
          file, line, character, name);
  fprintf(stderr, "%s:%zu:%zu: note: previously declared here\n", collidingFile,
          collidingLine, collidingChar);
}

void buildTopLevelStab(FileListEntry *entry) {}

void completeTopLevelStab(FileListEntry *entry) {}