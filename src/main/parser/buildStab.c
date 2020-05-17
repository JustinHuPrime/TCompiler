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

#include <stdio.h>
#include <stdlib.h>

/**
 * looks up a module in the moduleTree
 *
 * @param id id to look up
 * @returns reference to the FileListEntry, or NULL if no such module exists
 */
static FileListEntry const *lookupModule(Node *id) {
  if (id->type == NT_ID) {
    // just foo
    ModuleTreeNode const *found =
        hashMapGet(&fileList.moduleTree, id->data.id.id);
    if (found == NULL)
      return NULL;
    else
      return found->entry;
  } else {
    // foo::bar...
    Vector *components = id->data.scopedId.components;
    HashMap const *previous = &fileList.moduleTree;
    ModuleTreeNode const *current = NULL;
    for (size_t idx = 0; idx < components->size; idx++) {
      Node *component = components->elements[idx];
      current = hashMapGet(previous, component->data.id.id);
      if (current == NULL) return NULL;
      previous = &current->children;
    }
    return current->entry;
  }
}

void parserBuildTopLevelStab(FileListEntry *entry) {
  Node *ast = entry->ast;

  // resolve imports
  Vector *imports = ast->data.file.imports;
  for (size_t idx = 0; idx < imports->size; idx++) {
    Node *import = imports->elements[idx];
    FileListEntry const *referencedFile = lookupModule(import->data.import.id);
    if (referencedFile == NULL) {
      // error - unresolved import
      char *importName = stringifyId(import->data.import.id);
      fprintf(stderr, "%s:%zu:%zu: error: module '%s' not found",
              entry->inputFilename, import->data.import.id->line,
              import->data.import.id->character, importName);

      free(importName);
      return;
    }
    // now record the stab reference
    import->data.import.referenced = &referencedFile->ast->data.file.stab;
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