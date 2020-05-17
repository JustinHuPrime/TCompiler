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
#include "internalError.h"
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

void parserBuildTopLevelStab(FileListEntry *entry) {
  Node *ast = entry->ast;

  // resolve imports
  Vector *imports = ast->data.file.imports;
  for (size_t idx = 0; idx < imports->size; idx++) {
    Node *import = imports->elements[idx];
    char *importName = stringifyId(import->data.import.id);
    FileListEntry const *referencedFile =
        hashMapGet(&fileList.moduleMap, importName);
    if (referencedFile == NULL) {
      // error - unresolved import
      fprintf(stderr, "%s:%zu:%zu: error: module '%s' not found",
              entry->inputFilename, import->data.import.id->line,
              import->data.import.id->character, importName);

      entry->errored = true;
    }
    // now record the stab reference
    import->data.import.referenced = &referencedFile->ast->data.file.stab;
    free(importName);
  }

  // process top-level
  Vector *topLevels = ast->data.file.bodies;
  for (size_t idx = 0; idx < topLevels->size; idx++) {
    Node *topLevel = topLevels->elements[idx];
    switch (topLevel->type) {
      case NT_FUNDEFN: {
        break;
      }
      case NT_VARDEFN: {
        break;
      }
      case NT_FUNDECL: {
        break;
      }
      case NT_VARDECL: {
        break;
      }
      case NT_OPAQUEDECL: {
        break;
      }
      case NT_STRUCTDECL: {
        break;
      }
      case NT_UNIONDECL: {
        break;
      }
      case NT_ENUMDECL: {
        break;
      }
      case NT_TYPEDEFDECL: {
        break;
      }
      default: { error(__FILE__, __LINE__, "non-top level form encountered"); }
    }
  }
}