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
#include "options.h"
#include "util/container/hashMap.h"
#include "util/container/hashSet.h"
#include "util/container/vector.h"
#include "util/format.h"
#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool fileListEntryArrayContains(FileListEntry **arry, size_t size,
                                       FileListEntry *f) {
  for (size_t idx = 0; idx < size; idx++)
    if (arry[idx] == f) return true;
  return false;
}
static bool nameArrayContains(Node **arry, size_t size, Node *n) {
  for (size_t idx = 0; idx < size; idx++)
    if (nameNodeEqual(arry[idx], n)) return true;
  return false;
}
int resolveImports(void) {
  bool errored = false;

  // check for duplciate decl modules
  FileListEntry **processed = malloc(sizeof(FileListEntry *) * fileList.size);
  size_t numProcessed = 0;
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    if (!fileList.entries[fileIdx].isCode &&
        !fileListEntryArrayContains(processed, numProcessed,
                                    &fileList.entries[fileIdx])) {
      // for each declaration file, if its name hasn't been processed yet,
      // process it
      FileListEntry **duplicateEntries =
          malloc(sizeof(FileListEntry *) * fileList.size);
      size_t numDuplicates = 0;
      for (size_t compareIdx = fileIdx + 1; compareIdx < fileList.size;
           compareIdx++) {
        // check it against each other file
        if (nameNodeEqual(
                fileList.entries[fileIdx].ast->data.file.module->data.module.id,
                fileList.entries[compareIdx]
                    .ast->data.file.module->data.module.id)) {
          duplicateEntries[numDuplicates++] = &fileList.entries[compareIdx];
        }
      }
      if (numDuplicates != 0) {
        char *nameString = stringifyId(
            fileList.entries[fileIdx].ast->data.file.module->data.module.id);
        fprintf(stderr,
                "%s:%zu:%zu: error: module '%s' declared in multiple "
                "declaration modules\n",
                fileList.entries[fileIdx].inputFilename,
                fileList.entries[fileIdx].ast->line,
                fileList.entries[fileIdx].ast->character, nameString);
        free(nameString);
        for (size_t printIdx = 0; printIdx < numDuplicates; printIdx++)
          fprintf(stderr, "%s:%zu:%zu: note: declared here\n",
                  duplicateEntries[printIdx]->inputFilename,
                  duplicateEntries[printIdx]->ast->line,
                  duplicateEntries[printIdx]->ast->character);
        errored = true;
      }
      free(duplicateEntries);

      processed[numProcessed++] = &fileList.entries[fileIdx];
    }
  }
  free(processed);

  if (errored) return -1;

  // link imports
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    Node *ast = fileList.entries[fileIdx].ast;
    Vector *imports = ast->data.file.imports;

    Node **processed = malloc(sizeof(Node *) * imports->size);
    size_t numProcessed = 0;
    for (size_t importIdx = 0; importIdx < imports->size; importIdx++) {
      Node *import = imports->elements[importIdx];

      if (!nameArrayContains(processed, numProcessed, import->data.import.id)) {
        // check for upcoming duplicates
        Node **colliding =
            malloc(sizeof(Node *) * (imports->size - importIdx - 1));
        size_t numColliding = 0;
        for (size_t checkIdx = importIdx + 1; checkIdx < imports->size;
             checkIdx++) {
          Node *toCheck = imports->elements[checkIdx];
          if (nameNodeEqual(import->data.import.id, toCheck->data.import.id))
            colliding[numColliding++] = toCheck;
        }
        if (numColliding != 0) {
          switch (options.duplicateImport) {
            case OPTION_W_ERROR: {
              char *nameString = stringifyId(import->data.import.id);
              fprintf(stderr,
                      "%s:%zu:%zu: error: '%s' imported multiple times\n",
                      fileList.entries[fileIdx].inputFilename, import->line,
                      import->character, nameString);
              free(nameString);
              for (size_t idx = 0; idx < numColliding; idx++)
                fprintf(stderr, "%s:%zu:%zu: note: imported here\n",
                        fileList.entries[fileIdx].inputFilename,
                        colliding[idx]->line, colliding[idx]->character);
              fileList.entries[fileIdx].errored = true;
              break;
            }
            case OPTION_W_WARN: {
              char *nameString = stringifyId(import->data.import.id);
              fprintf(stderr,
                      "%s:%zu:%zu: warning: '%s' imported multiple times\n",
                      fileList.entries[fileIdx].inputFilename, import->line,
                      import->character, nameString);
              free(nameString);
              for (size_t idx = 0; idx < numColliding; idx++)
                fprintf(stderr, "%s:%zu:%zu: note: imported here\n",
                        fileList.entries[fileIdx].inputFilename,
                        colliding[idx]->line, colliding[idx]->character);
              break;
            }
            case OPTION_W_IGNORE: {
              break;
            }
          }
        }
        free(colliding);

        import->data.import.referenced =
            fileListFindDeclName(import->data.import.id);

        if (import->data.import.referenced == NULL) {
          char *name = stringifyId(import->data.import.id);
          fprintf(stderr, "%s:%zu:%zu error: cannot find module '%s'\n",
                  fileList.entries[fileIdx].inputFilename, import->line,
                  import->character, name);
          free(name);
          errored = true;
        }

        processed[numProcessed] = import->data.import.id;
        numProcessed++;
      }
    }
    free(processed);
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

void startTopLevelStab(FileListEntry *entry) {
  Vector *bodies = entry->ast->data.file.bodies;
  HashMap *stab = &entry->ast->data.file.stab;
  HashMap *implicitStab = NULL;
  if (entry->isCode) {
    FileListEntry *declEntry =
        fileListFindDeclName(entry->ast->data.file.module->data.module.id);
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

static bool checkScopedIdCollisionsBetween(Node *longImport, Node *shortImport,
                                           char const *currentFilename) {
  Node *longName = longImport->data.import.id;
  Node *shortName = shortImport->data.import.id;
  FileListEntry const *longFile = longImport->data.import.referenced;
  FileListEntry const *shortFile = shortImport->data.import.referenced;
  if (longName->type == NT_SCOPEDID &&
      nameNodeEqualWithDrop(shortName, longName, 1)) {
    // possible collision if shortFile has an enum matching the last
    // element of the longName
    Node *lastNameNode =
        longName->data.scopedId.components
            ->elements[longName->data.scopedId.components->size - 1];
    SymbolTableEntry *nameMatch =
        hashMapGet(&shortFile->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the shorter
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; enumIdx++) {
        // for each enum constant, is it in the longFile?
        SymbolTableEntry *colliding = hashMapGet(
            &longFile->ast->data.file.stab,
            nameMatch->data.enumType.constantNames.elements[enumIdx]);
        if (colliding != NULL) {
          char *longNameString = stringifyId(longName);
          char *collidingName = format(
              "%s::%s", longNameString,
              (char *)nameMatch->data.enumType.constantNames.elements[enumIdx]);
          fprintf(stderr, "%s:%zu:%zu: error: '%s' introduced multiple times\n",
                  currentFilename, longImport->line, longImport->character,
                  collidingName);
          fprintf(stderr, "%s:%zu:%zu: note: also introduced here\n",
                  currentFilename, shortImport->line, shortImport->character);
          free(longNameString);
          free(collidingName);
          return true;
        }
      }
    }
  }
  return false;
}
static bool checkScopedIdCollisionsWithCurrent(Node *import,
                                               FileListEntry *entry) {
  Node *importName = import->data.import.id;
  FileListEntry const *importFile = import->data.import.referenced;
  Node *fileName = entry->ast->data.file.module->data.module.id;
  if (fileName->type == NT_SCOPEDID &&
      nameNodeEqualWithDrop(importName, fileName, 1)) {
    // import is shorter than current module
    Node *lastNameNode =
        fileName->data.scopedId.components
            ->elements[fileName->data.scopedId.components->size - 1];
    SymbolTableEntry *nameMatch =
        hashMapGet(&importFile->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the import
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; enumIdx++) {
        // for each enum constant, is it in the current module?
        SymbolTableEntry *colliding = hashMapGet(
            &entry->ast->data.file.stab,
            nameMatch->data.enumType.constantNames.elements[enumIdx]);
        if (colliding != NULL) {
          fprintf(
              stderr,
              "%s:%zu:%zu: error: '%s' collides with imported scoped "
              "identifier\n",
              entry->inputFilename, colliding->line, colliding->character,
              (char *)nameMatch->data.enumType.constantNames.elements[enumIdx]);
          fprintf(stderr, "%s:%zu:%zu: note: also introduced here\n",
                  entry->inputFilename, import->line, import->character);
          return true;
        }
      }
    }
  } else if (importName->type == NT_SCOPEDID &&
             nameNodeEqualWithDrop(fileName, importName, 1)) {
    // current module is shorter than import
    Node *lastNameNode =
        importName->data.scopedId.components
            ->elements[importName->data.scopedId.components->size - 1];
    SymbolTableEntry *nameMatch =
        hashMapGet(&entry->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the current module
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; enumIdx++) {
        // for each enum constant, is it in the import?
        SymbolTableEntry *colliding = hashMapGet(
            &importFile->ast->data.file.stab,
            nameMatch->data.enumType.constantNames.elements[enumIdx]);
        if (colliding != NULL) {
          SymbolTableEntry *collidingEntry =
              nameMatch->data.enumType.constantValues.elements[enumIdx];
          fprintf(
              stderr,
              "%s:%zu:%zu: error: '%s' collides with imported scoped "
              "identifier\n",
              entry->inputFilename, collidingEntry->line,
              collidingEntry->character,
              (char *)nameMatch->data.enumType.constantNames.elements[enumIdx]);
          fprintf(stderr, "%s:%zu:%zu: note: also introduced here\n",
                  entry->inputFilename, import->line, import->character);
          return true;
        }
      }
    }
  }
  return false;
}
void checkScopedIdCollisions(FileListEntry *entry) {
  // is a scoped ID collision when two identifiers look like
  // PREFIX::FIRSTELM::SECONDELM
  // where PREFIX::FIRSTELM describes a module, and SECONDELM is an element of
  // that module
  // and PREFIX describes a module, and FIRSTELM describes an enum within that
  // module and SECONDELM is an element of that module

  // for each import
  Vector *imports = entry->ast->data.file.imports;
  for (size_t longIdx = 0; longIdx < imports->size; longIdx++) {
    Node *longImport = imports->elements[longIdx];
    // search the rest of the list for imports that have all but the last
    // element matching
    for (size_t shortIdx = 0; shortIdx < imports->size; shortIdx++) {
      Node *shortImport = imports->elements[shortIdx];
      entry->errored =
          entry->errored || checkScopedIdCollisionsBetween(
                                longImport, shortImport, entry->inputFilename);
    }

    // check for problems with current module
    entry->errored =
        entry->errored || checkScopedIdCollisionsWithCurrent(longImport, entry);
  }
}

/**
 * find the index of e in enumConstants
 *
 * e must be in enumConstants
 */
static size_t constantEntryFind(Vector *enumConstants, SymbolTableEntry *e) {
  for (size_t idx = 0; idx < enumConstants->size; idx++) {
    if (enumConstants->elements[idx] == e) return idx;
  }
  error(__FILE__, __LINE__,
        "constantEntryFind called with an e not in enumConstants");
}
int buildTopLevelEnumStab(void) {
  Vector enumConstants;  // vector of SymbolTableEntry, non-owning
  Vector dependencies;   // vector of SymbolTableEntry, non-owning, nullable
  bool errored = false;

  // for each enum in each file, create the enumConstant entries
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;

    // for each top level
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
      Node *body = bodies->elements[bodyIdx];
      // if it's an enum
      if (body->type == NT_ENUMDECL) {
        SymbolTableEntry *thisEnum = body->data.enumDecl.name->data.id.entry;
        Vector *constantValues = &thisEnum->data.enumType.constantValues;
        // for each constant, record it in the graph
        for (size_t constantIdx = 0; constantIdx < constantValues->size;
             constantIdx++) {
          SymbolTableEntry *constantValue =
              constantValues->elements[constantIdx];
          vectorInsert(&enumConstants, constantValue);
          vectorInsert(&dependencies, NULL);
        }
      }
    }
  }

  // for each enum in each file
  for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;
    Environment env;
    environmentInit(&env, entry);

    // for each enum in the file
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
      Node *body = bodies->elements[bodyIdx];
      if (body->type == NT_ENUMDECL) {
        SymbolTableEntry *thisEnum = body->data.enumDecl.name->data.id.entry;
        Vector *constantValues = &thisEnum->data.enumType.constantValues;

        // for each constant
        for (size_t constantIdx = 0; constantIdx < constantValues->size;
             constantIdx++) {
          Node *constantValueNode =
              body->data.enumDecl.constantValues->elements[constantIdx];
          size_t constantEntryIdx = constantEntryFind(
              &enumConstants, constantValues->elements[constantIdx]);
          if (constantValueNode == NULL) {
            // depends on previous
            if (constantIdx == 0) {
              // no previous entry in this enum - depends on nothing
              dependencies.elements[constantEntryIdx] = NULL;
            } else {
              // depends on previous entry (is always at current - 1)
              dependencies.elements[constantEntryIdx] =
                  enumConstants.elements[constantEntryIdx - 1];
            }
          } else {
            // entry = ...
            if (constantValueNode->type == NT_LITERAL) {
              // must be int, char, wchar literal
              // depends on nothing
              dependencies.elements[constantEntryIdx] = NULL;
            } else {
              // depends on another enum - constantValue is a SCOPED_ID
              // find the enum
              SymbolTableEntry *stabEntry =
                  environmentLookup(&env, constantValueNode, false);
              if (stabEntry == NULL) {
                // error - no such enum
                errored = true;
              } else if (stabEntry->kind != SK_ENUMCONST) {
                fprintf(stderr,
                        "%s:%zu:%zu: error: expected an extended integer "
                        "literal, found %s\n",
                        entry->inputFilename, constantValueNode->line,
                        constantValueNode->character,
                        symbolKindToString(stabEntry->kind));
                errored = true;
              } else {
                dependencies.elements[constantEntryIdx] = stabEntry;
              }
            }
          }
        }
      }
    }

    environmentUninit(&env);
  }

  if (errored) {
    vectorUninit(&enumConstants, nullDtor);
    vectorUninit(&dependencies, nullDtor);
    return -1;
  }

  // TODO: traverse enum dependency graph and build enum constant values,
  // checking for loops
}