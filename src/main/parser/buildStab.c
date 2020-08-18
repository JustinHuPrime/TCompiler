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
#include "fileList.h"
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

  // link imports
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    Node *ast = fileList.entries[fileIdx].ast;
    Vector *imports = ast->data.file.imports;
    for (size_t importIdx = 0; importIdx < imports->size; importIdx++) {
      Node *import = imports->elements[importIdx];
      char *name = stringifyId(import->data.import.id);
      HashMap *importStab = hashMapGet(&fileList.moduleMap, name);

      if (importStab != NULL) {
        import->data.import.referenced = importStab;
      } else {
        fprintf(stderr, "%s:%zu:%zu error: cannot find module '%s':\n",
                fileList.entries[fileIdx].inputFilename, import->line,
                import->character, name);
        errored = true;
      }

      free(name);
    }
  }

  if (errored)
    return -1;
  else
    return 0;
}

/**
 * complain about a redeclaration
 */
static void errorRedeclaration(FileListEntry *file, size_t line,
                               size_t character, char const *name,
                               FileListEntry *collidingFile,
                               size_t collidingLine, size_t collidingChar) {
  fprintf(stderr, "%s:%zu:%zu: error: redeclaration of %s\n",
          file->inputFilename, line, character, name);
  fprintf(stderr, "%s:%zu:%zu: note: previously declared here\n",
          collidingFile->inputFilename, collidingLine, collidingChar);
}

/**
 * complain about missing declaration
 *
 * @param file file to attribute error to
 * @param node node to attribute error to - must be an id or scoped id
 */
static void errorNoDecl(FileListEntry *file, Node *node) {
  if (node->type == NT_ID) {
    fprintf(stderr, "%s:%zu:%zu: error: '%s' was not declared\n",
            file->inputFilename, node->line, node->character, node->data.id.id);
  } else {
    char *str = stringifyId(node);
    fprintf(stderr, "%s:%zu:%zu: error: '%s' was not declared\n",
            file->inputFilename, node->line, node->character, str);
    free(str);
  }
}

void startTopLevelStab(FileListEntry *entry) {
  Vector *bodies = entry->ast->data.file.bodies;
  HashMap *stab = &entry->ast->data.file.stab;
  HashMap *implicitStab = NULL;
  if (entry->isCode) {
    FileListEntry *declEntry =
        hashMapGet(&fileList.moduleMap, entry->moduleName);
    if (declEntry != NULL) implicitStab = &declEntry->ast->data.file.stab;
  }

  // for each top level thing
  for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
    Node *body = bodies->elements[bodyIdx];
    switch (body->type) {
      case NT_OPAQUEDECL: {
        char const *name = body->data.opaqueDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        if (existing == NULL && implicitStab != NULL)
          existing = hashMapGet(implicitStab, name);

        if (existing != NULL) {
          // must not exist
          errorRedeclaration(entry, body->line, body->character, name,
                             existing->file, existing->line,
                             existing->character);
          entry->errored = true;
        } else {
          // create an entry if it doesn't exist
          body->data.opaqueDecl.name->data.id.entry =
              opaqueStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.opaqueDecl.name->data.id.entry);
        }
        break;
      }
      case NT_STRUCTDECL: {
        char const *name = body->data.structDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        bool fromImplicit = false;
        if (existing == NULL && implicitStab != NULL) {
          existing = hashMapGet(implicitStab, name);
          fromImplicit = true;
        }

        if (existing != NULL) {
          // may exist only as an opaque from the implicit import
          if (existing->kind == SK_OPAQUE && fromImplicit) {
            existing->data.opaqueType.definition =
                body->data.structDecl.name->data.id.entry =
                    structStabEntryCreate(entry, body->line, body->character);
            hashMapPut(stab, name, body->data.structDecl.name->data.id.entry);
          } else {
            errorRedeclaration(entry, body->line, body->character, name,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          }
        } else {
          body->data.structDecl.name->data.id.entry =
              structStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.structDecl.name->data.id.entry);
        }
        break;
      }
      case NT_UNIONDECL: {
        char const *name = body->data.unionDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        bool fromImplicit = false;
        if (existing == NULL && implicitStab != NULL) {
          existing = hashMapGet(implicitStab, name);
          fromImplicit = true;
        }

        if (existing != NULL) {
          // may exist only as an opaque from the implicit import
          if (existing->kind == SK_OPAQUE && fromImplicit) {
            existing->data.opaqueType.definition =
                body->data.unionDecl.name->data.id.entry =
                    unionStabEntryCreate(entry, body->line, body->character);
            hashMapPut(stab, name, body->data.unionDecl.name->data.id.entry);
          } else {
            errorRedeclaration(entry, body->line, body->character, name,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          }
        } else {
          body->data.unionDecl.name->data.id.entry =
              unionStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.unionDecl.name->data.id.entry);
        }
        break;
      }
      case NT_ENUMDECL: {
        char const *name = body->data.enumDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        bool fromImplicit = false;
        if (existing == NULL && implicitStab != NULL) {
          existing = hashMapGet(implicitStab, name);
          fromImplicit = true;
        }

        SymbolTableEntry *parentEnum = NULL;
        if (existing != NULL) {
          // may exist only as an opaque from the implicit import
          if (existing->kind == SK_OPAQUE && fromImplicit) {
            parentEnum = existing->data.opaqueType.definition =
                body->data.enumDecl.name->data.id.entry =
                    enumStabEntryCreate(entry, body->line, body->character);
            hashMapPut(stab, name, body->data.enumDecl.name->data.id.entry);
          } else {
            errorRedeclaration(entry, body->line, body->character, name,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          }
        } else {
          parentEnum = body->data.enumDecl.name->data.id.entry =
              enumStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.enumDecl.name->data.id.entry);
        }

        if (parentEnum != NULL) {
          Vector *constantNames = body->data.enumDecl.constantNames;

          // for each of the constants
          for (size_t idx = 0; idx < constantNames->size; idx++) {
            Node *constantName = constantNames->elements[idx];
            vectorInsert(&parentEnum->data.enumType.constantNames,
                         constantName->data.id.id);

            constantName->data.id.entry = enumConstStabEntryCreate(
                entry, constantName->line, constantName->character, parentEnum);
            vectorInsert(&parentEnum->data.enumType.constantValues,
                         constantName->data.id.entry);
          }
        }
        break;
      }
      case NT_TYPEDEFDECL: {
        char const *name = body->data.typedefDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        bool fromImplicit = false;
        if (existing == NULL && implicitStab != NULL) {
          existing = hashMapGet(implicitStab, name);
          fromImplicit = true;
        }

        if (existing != NULL) {
          // may exist only as an opaque from the implicit import
          if (existing->kind == SK_OPAQUE && fromImplicit) {
            existing->data.opaqueType.definition =
                body->data.typedefDecl.name->data.id.entry =
                    typedefStabEntryCreate(entry, body->line, body->character);
            hashMapPut(stab, name, body->data.typedefDecl.name->data.id.entry);
          } else {
            errorRedeclaration(entry, body->line, body->character, name,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          }
        } else {
          body->data.typedefDecl.name->data.id.entry =
              typedefStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.typedefDecl.name->data.id.entry);
        }
        break;
      }
      case NT_VARDECL: {
        Vector *names = body->data.varDecl.names;
        for (size_t idx = 0; idx < names->size; idx++) {
          Node *name = names->elements[idx];
          char const *nameString = name->data.id.id;
          SymbolTableEntry *existing = hashMapGet(stab, nameString);
          // can't possibly be from an implicit - this is in a decl module
          if (existing != NULL) {
            errorRedeclaration(entry, name->line, name->character, nameString,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          } else {
            name->data.id.entry =
                variableStabEntryCreate(entry, name->line, name->character);
            hashMapPut(stab, nameString, name->data.id.entry);
          }
        }
        break;
      }
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        for (size_t idx = 0; idx < names->size; idx++) {
          Node *name = names->elements[idx];
          char const *nameString = name->data.id.id;
          SymbolTableEntry *existing = hashMapGet(stab, nameString);
          // don't care about implcit - this either declares itself or doesn't
          // yet collide - check for collision when building the stab
          if (existing != NULL) {
            errorRedeclaration(entry, name->line, name->character, nameString,
                               existing->file, existing->line,
                               existing->character);
            entry->errored = true;
          } else {
            name->data.id.entry =
                variableStabEntryCreate(entry, name->line, name->character);
            hashMapPut(stab, nameString, name->data.id.entry);
          }
        }
        break;
      }
      case NT_FUNDECL: {
        char const *name = body->data.funDecl.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        // don't care about other declarations - overload sets resolved later
        if (existing != NULL) {
          body->data.funDecl.name->data.id.entry = existing;
        } else {
          body->data.funDecl.name->data.id.entry =
              functionStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.funDecl.name->data.id.entry);
        }
        break;
      }
      case NT_FUNDEFN: {
        char const *name = body->data.funDefn.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(stab, name);
        // don't care about other declarations - overload sets resolved later
        if (existing != NULL) {
          body->data.funDefn.name->data.id.entry = existing;
        } else {
          body->data.funDefn.name->data.id.entry =
              functionStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.funDefn.name->data.id.entry);
        }
        break;
      }
      default: {
        error(
            __FILE__, __LINE__,
            "non-top-level form encountered at top level in startTopLevelStab");
      }
    }
  }
}

typedef struct {
  SymbolTableEntry *parentEnum;
  char const *name;
} EnumConstant;

static size_t constantEntryFind(Vector *enumConstants,
                                SymbolTableEntry *parentEnum,
                                char const *name) {
  for (size_t idx = 0; idx < enumConstants->size; idx++) {
    EnumConstant *c = enumConstants->elements[idx];
    if (c->parentEnum == parentEnum && c->name == name) return idx;
  }

  error(__FILE__, __LINE__, "attempted to look up non-existent enum constant");
}

static void initEnvForEntry(Environment *env, FileListEntry *entry,
                            HashMap *stab, HashMap *implicitStab) {
  environmentInit(env, entry->ast->data.file.module->data.module.id, stab,
                  implicitStab);

  Vector *imports = entry->ast->data.file.imports;
  for (size_t idx = 0; idx < imports->size; idx++) {
    Node *import = imports->elements[idx];
    vectorInsert(&env->importNames, import->data.import.id);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    vectorInsert(&env->importTables, (HashMap *)import->data.import.referenced);
#pragma GCC diagnostic push
  }
}

int buildTopLevelEnumStab(void) {
  Vector enumConstants;  // vector of EnumConstant
  Vector dependencies;   // vector of EnumConstant, non-owning
  bool errored = false;

  // for each enum in each file, create the enumConstant entries
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;
    HashMap *stab = &entry->ast->data.file.stab;

    // for each top level
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
      Node *body = bodies->elements[bodyIdx];
      // if it's an enum
      if (body->type == NT_ENUMDECL) {
        Vector *constantNames = body->data.enumDecl.constantNames;
        SymbolTableEntry *thisEnum =
            hashMapGet(stab, body->data.enumDecl.name->data.id.id);
        // for each constant, construct the enumConstant entry
        for (size_t constantIdx = 0; constantIdx < constantNames->size;
             constantIdx++) {
          Node *constantName = constantNames->elements[constantIdx];

          EnumConstant *constant = malloc(sizeof(EnumConstant));
          constant->parentEnum = thisEnum;
          constant->name = constantName->data.id.id;
          vectorInsert(&enumConstants, constant);
          vectorInsert(&dependencies, NULL);
        }
      }
    }
  }

  // for each file
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;
    HashMap *stab = &entry->ast->data.file.stab;
    HashMap *implicitStab = NULL;
    if (entry->isCode) {
      FileListEntry *declEntry =
          hashMapGet(&fileList.moduleMap, entry->moduleName);
      if (declEntry != NULL) implicitStab = &declEntry->ast->data.file.stab;
    }
    Environment env;
    initEnvForEntry(&env, entry, stab, implicitStab);

    // for each enum in the file
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
      Node *body = bodies->elements[bodyIdx];
      if (body->type == NT_ENUMDECL) {
        Vector *constantNames = body->data.enumDecl.constantNames;
        Vector *constantValues = body->data.enumDecl.constantValues;
        SymbolTableEntry *thisEnum =
            hashMapGet(stab, body->data.enumDecl.name->data.id.id);

        // for each constant
        for (size_t constantIdx = 0; constantIdx < constantNames->size;
             constantIdx++) {
          Node *constantName = constantNames->elements[constantIdx];
          Node *constantValue = constantValues->elements[constantIdx];

          size_t constantEntryIdx = constantEntryFind(&enumConstants, thisEnum,
                                                      constantName->data.id.id);
          if (constantValue == NULL) {
            // depends on previous
            if (constantIdx == 0) {
              // no previous entry - depends on nothing
              dependencies.elements[constantEntryIdx] = NULL;
            } else {
              // depends on previous entry (is always at current - 1)
              dependencies.elements[constantEntryIdx] =
                  &dependencies.elements[constantEntryIdx - 1];
            }
          } else {
            // entry = ...
            if (constantValue->type == NT_LITERAL) {
              // must be int, char, wchar literal
              // depends on nothing
              dependencies.elements[constantEntryIdx] = NULL;
            } else {
              // depends on another enum - constantValue is a SCOPED_ID
              // find the enum
              SymbolTableEntry
                  *stabEntry;  // TODO: how do we get and validate this?
              char const *name =
                  constantValue->data.scopedId.components
                      ->elements[constantValue->data.scopedId.components->size -
                                 1];
              if (stabEntry == NULL || stabEntry->kind != SK_ENUM) {
                // error - no such enum
                errorNoDecl(entry, constantValue);
              } else {
                dependencies.elements[constantEntryIdx] =
                    &dependencies.elements[constantEntryFind(&enumConstants,
                                                             stabEntry, name)];
              }
            }
          }
        }
      }
    }

    environmentUninit(&env);
  }

  if (errored)
    return -1;
  else
    return 0;

  // TODO: traverse enum dependency graph and build enum constant values,
  // checking for loops
}