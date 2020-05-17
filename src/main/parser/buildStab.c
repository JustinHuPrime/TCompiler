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

void parserBuildTopLevelStab(FileListEntry *entry) {
  Node *ast = entry->ast;
  HashMap *stab = &ast->data.file.stab;

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

  // resolve implicit import
  if (entry->isCode) {
    Node *module = ast->data.file.module;
    char *moduleName = stringifyId(module->data.module.id);
    FileListEntry const *implicit = hashMapGet(&fileList.moduleMap, moduleName);
    free(moduleName);
    if (implicit != NULL) {
      // get the implicit import
      HashMap const *implicitStab = &implicit->ast->data.file.stab;
      for (size_t idx = 0; idx < implicitStab->size; idx++) {
        if (implicitStab->keys[idx] != NULL) {
          SymbolTableEntry *original = implicitStab->values[idx];
          SymbolTableEntry *copy =
              stabEntryCreate(original->type, original->file, original->line,
                              original->character);
          hashMapPut(stab, implicitStab->keys[idx], copy);
        }
      }
    }
  }

  // process top-level
  Vector *topLevels = ast->data.file.bodies;
  for (size_t idx = 0; idx < topLevels->size; idx++) {
    Node *topLevel = topLevels->elements[idx];
    switch (topLevel->type) {
      case NT_FUNDEFN: {
        char const *name = topLevel->data.funDefn.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_FUNCTION, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with a function declaration
          if (colliding->type != ST_FUNCTION) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      case NT_VARDEFN: {
        Vector *names = topLevel->data.varDefn.names;
        for (size_t idx = 0; idx < names->size; idx++) {
          Node *nameNode = names->elements[idx];
          char const *name = nameNode->data.id.id;
          SymbolTableEntry *e =
              stabEntryCreate(ST_VARIABLE, entry->inputFilename, nameNode->line,
                              nameNode->character);
          int retval = hashMapPut(stab, name, e);
          if (retval != 0) {
            stabEntryDestroy(e);
            SymbolTableEntry const *colliding = hashMapGet(stab, name);
            // allowed to collide with another variable
            if (colliding->type != ST_VARIABLE) {
              errorRedeclaration(entry->inputFilename, nameNode->line,
                                 nameNode->character, name, colliding->file,
                                 colliding->line, colliding->character);
              entry->errored = true;
            }
          }
        }
        break;
      }
      case NT_FUNDECL: {
        char const *name = topLevel->data.funDecl.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_FUNCTION, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with a function declaration
          if (colliding->type != ST_FUNCTION) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      case NT_VARDECL: {
        Vector *names = topLevel->data.varDecl.names;
        for (size_t idx = 0; idx < names->size; idx++) {
          Node *nameNode = names->elements[idx];
          char const *name = nameNode->data.id.id;
          SymbolTableEntry *e =
              stabEntryCreate(ST_VARIABLE, entry->inputFilename, nameNode->line,
                              nameNode->character);
          int retval = hashMapPut(stab, name, e);
          if (retval != 0) {
            stabEntryDestroy(e);
            SymbolTableEntry const *colliding = hashMapGet(stab, name);
            // allowed to collide with another variable
            if (colliding->type != ST_VARIABLE) {
              errorRedeclaration(entry->inputFilename, nameNode->line,
                                 nameNode->character, name, colliding->file,
                                 colliding->line, colliding->character);
              entry->errored = true;
            }
          }
        }
        break;
      }
      case NT_OPAQUEDECL: {
        char const *name = topLevel->data.opaqueDecl.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_OPAQUE, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // not allowed to collide with anything
          errorRedeclaration(entry->inputFilename, topLevel->line,
                             topLevel->character, name, colliding->file,
                             colliding->line, colliding->character);
          entry->errored = true;
        }
        break;
      }
      case NT_STRUCTDECL: {
        char const *name = topLevel->data.structDecl.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_STRUCT, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with an opaque
          if (colliding->type != ST_OPAQUE) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      case NT_UNIONDECL: {
        char const *name = topLevel->data.unionDecl.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_UNION, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with an opaque
          if (colliding->type != ST_OPAQUE) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      case NT_ENUMDECL: {
        char const *name = topLevel->data.enumDecl.name->data.id.id;
        SymbolTableEntry *e = stabEntryCreate(
            ST_ENUM, entry->inputFilename, topLevel->line, topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with an opaque
          if (colliding->type != ST_OPAQUE) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      case NT_TYPEDEFDECL: {
        char const *name = topLevel->data.typedefDecl.name->data.id.id;
        SymbolTableEntry *e =
            stabEntryCreate(ST_TYPEDEF, entry->inputFilename, topLevel->line,
                            topLevel->character);
        int retval = hashMapPut(stab, name, e);
        if (retval != 0) {
          stabEntryDestroy(e);
          SymbolTableEntry const *colliding = hashMapGet(stab, name);
          // allowed to collide with an opaque
          if (colliding->type != ST_OPAQUE) {
            errorRedeclaration(entry->inputFilename, topLevel->line,
                               topLevel->character, name, colliding->file,
                               colliding->line, colliding->character);
            entry->errored = true;
          }
        }
        break;
      }
      default: { error(__FILE__, __LINE__, "non-top level form encountered"); }
    }
  }
}