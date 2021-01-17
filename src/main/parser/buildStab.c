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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"
#include "ast/environment.h"
#include "common.h"
#include "fileList.h"
#include "internalError.h"
#include "numericSizing.h"
#include "options.h"
#include "util/container/hashMap.h"
#include "util/container/hashSet.h"
#include "util/container/vector.h"
#include "util/format.h"
#include "util/functional.h"

static bool fileListEntryArrayContains(FileListEntry **arry, size_t size,
                                       FileListEntry *f) {
  for (size_t idx = 0; idx < size; ++idx)
    if (arry[idx] == f) return true;
  return false;
}
static bool nameArrayContains(Node **arry, size_t size, Node *n) {
  for (size_t idx = 0; idx < size; ++idx)
    if (nameNodeEqual(arry[idx], n)) return true;
  return false;
}
int resolveImports(void) {
  bool errored = false;

  // check for duplciate decl modules
  FileListEntry **processed = malloc(sizeof(FileListEntry *) * fileList.size);
  size_t numProcessed = 0;
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    if (!fileList.entries[fileIdx].isCode &&
        !fileListEntryArrayContains(processed, numProcessed,
                                    &fileList.entries[fileIdx])) {
      // for each declaration file, if its name hasn't been processed yet,
      // process it
      FileListEntry **duplicateEntries =
          malloc(sizeof(FileListEntry *) * fileList.size);
      size_t numDuplicates = 0;
      for (size_t compareIdx = fileIdx + 1; compareIdx < fileList.size;
           ++compareIdx) {
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
        for (size_t printIdx = 0; printIdx < numDuplicates; ++printIdx)
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
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    Node *ast = fileList.entries[fileIdx].ast;
    Vector *imports = ast->data.file.imports;

    Node **processed = malloc(sizeof(Node *) * imports->size);
    size_t numProcessed = 0;
    for (size_t importIdx = 0; importIdx < imports->size; ++importIdx) {
      Node *import = imports->elements[importIdx];

      // note - we don't always abort after a duplicate, so this prevents
      // double-importing
      if (!nameArrayContains(processed, numProcessed, import->data.import.id)) {
        // check for upcoming duplicates
        Node **colliding =
            malloc(sizeof(Node *) * (imports->size - importIdx - 1));
        size_t numColliding = 0;
        for (size_t checkIdx = importIdx + 1; checkIdx < imports->size;
             ++checkIdx) {
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
              for (size_t idx = 0; idx < numColliding; ++idx)
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
              for (size_t idx = 0; idx < numColliding; ++idx)
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
        ++numProcessed;
      }
    }
    free(processed);
  }

  if (errored)
    return -1;
  else
    return 0;
}

void startTopLevelStab(FileListEntry *entry) {
  Vector *bodies = entry->ast->data.file.bodies;
  HashMap *stab = entry->ast->data.file.stab;
  HashMap *implicitStab = NULL;
  if (entry->isCode) {
    FileListEntry *declEntry =
        fileListFindDeclName(entry->ast->data.file.module->data.module.id);
    if (declEntry != NULL) implicitStab = declEntry->ast->data.file.stab;
  }

  // for each top level thing
  for (size_t bodyIdx = 0; bodyIdx < bodies->size; ++bodyIdx) {
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
          }
        } else {
          parentEnum = body->data.enumDecl.name->data.id.entry =
              enumStabEntryCreate(entry, body->line, body->character);
          hashMapPut(stab, name, body->data.enumDecl.name->data.id.entry);
        }

        if (parentEnum != NULL) {
          Vector *constantNames = body->data.enumDecl.constantNames;

          // for each of the constants
          for (size_t idx = 0; idx < constantNames->size; ++idx) {
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
        for (size_t idx = 0; idx < names->size; ++idx) {
          Node *name = names->elements[idx];
          char const *nameString = name->data.id.id;
          SymbolTableEntry *existing = hashMapGet(stab, nameString);
          // can't possibly be from an implicit - this is in a decl module

          // must not exist
          if (existing != NULL) {
            errorRedeclaration(entry, name->line, name->character, nameString,
                               existing->file, existing->line,
                               existing->character);
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
        for (size_t idx = 0; idx < names->size; ++idx) {
          Node *name = names->elements[idx];
          char const *nameString = name->data.id.id;
          SymbolTableEntry *existing = hashMapGet(stab, nameString);
          bool fromImplicit = false;
          if (existing == NULL && implicitStab != NULL) {
            existing = hashMapGet(implicitStab, nameString);
            fromImplicit = true;
          }

          if (existing != NULL) {
            // may only exist as a varDecl (which means it must be from the
            // implicit)
            if (existing->kind == SK_VARIABLE && fromImplicit) {
              name->data.id.entry =
                  variableStabEntryCreate(entry, name->line, name->character);
              hashMapPut(stab, nameString, name->data.id.entry);
            } else {
              errorRedeclaration(entry, name->line, name->character, nameString,
                                 existing->file, existing->line,
                                 existing->character);
            }
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
        // can't possibly be from an implicit - this is a decl module

        // must not exist
        if (existing != NULL) {
          errorRedeclaration(entry, body->line, body->character, name,
                             existing->file, existing->line,
                             existing->character);
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
        bool fromImplicit = false;
        if (existing == NULL && implicitStab != NULL) {
          existing = hashMapGet(implicitStab, name);
          fromImplicit = true;
        }

        if (existing != NULL) {
          // may only exist as a funDecl (must be from the implicit)
          if (existing->kind == SK_FUNCTION && fromImplicit) {
            body->data.funDefn.name->data.id.entry =
                functionStabEntryCreate(entry, body->line, body->character);
            hashMapPut(stab, name, body->data.funDefn.name->data.id.entry);
          } else {
            errorRedeclaration(entry, body->line, body->character, name,
                               existing->file, existing->line,
                               existing->character);
          }
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

/**
 * find the index of e in enumConstants
 *
 * e must be in enumConstants
 */
static size_t constantEntryFind(Vector *enumConstants, SymbolTableEntry *e) {
  for (size_t idx = 0; idx < enumConstants->size; ++idx) {
    if (enumConstants->elements[idx] == e) return idx;
  }
  error(__FILE__, __LINE__,
        "constantEntryFind called with an e not in enumConstants");
}
int buildTopLevelEnumStab(void) {
  Vector enumConstants;  // vector of SymbolTableEntry, non-owning
  Vector dependencies;   // vector of SymbolTableEntry, non-owning, nullable
  Vector enumValues;  // vector of extended int literals, non-owning, nullable
  bool errored = false;
  vectorInit(&enumConstants);
  vectorInit(&dependencies);
  vectorInit(&enumValues);

  // for each enum in each file, create the enumConstant entries
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;

    // for each top level
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; ++bodyIdx) {
      Node *body = bodies->elements[bodyIdx];
      // if it's an enum
      if (body->type == NT_ENUMDECL) {
        SymbolTableEntry *thisEnum = body->data.enumDecl.name->data.id.entry;
        Vector *constantSymbols = &thisEnum->data.enumType.constantValues;
        // for each constant, record it in the graph
        for (size_t constantIdx = 0; constantIdx < constantSymbols->size;
             ++constantIdx) {
          vectorInsert(&enumConstants, constantSymbols->elements[constantIdx]);
          vectorInsert(&dependencies, NULL);
          vectorInsert(
              &enumValues,
              body->data.enumDecl.constantValues->elements[constantIdx]);
        }
      }
    }
  }

  // for each enum in each file
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *entry = &fileList.entries[fileIdx];
    Vector *bodies = entry->ast->data.file.bodies;
    Environment env;
    environmentInit(&env, entry);

    // for each enum in the file
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; ++bodyIdx) {
      Node *body = bodies->elements[bodyIdx];
      if (body->type == NT_ENUMDECL) {
        SymbolTableEntry *thisEnum = body->data.enumDecl.name->data.id.entry;
        Vector *constantValues = &thisEnum->data.enumType.constantValues;

        // for each constant
        for (size_t constantIdx = 0; constantIdx < constantValues->size;
             ++constantIdx) {
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
    vectorUninit(&enumValues, nullDtor);
    return -1;
  }

  bool *processed = calloc(enumConstants.size, sizeof(bool));
  for (size_t startIdx = 0; startIdx < enumConstants.size; ++startIdx) {
    if (!processed[startIdx]) {
      typedef struct PathNode {
        size_t curr;
        struct PathNode *prev;
      } PathNode;

      PathNode *path = malloc(sizeof(PathNode));
      path->curr = startIdx;
      path->prev = NULL;

      processed[startIdx] = true;
      size_t curr =
          constantEntryFind(&enumConstants, dependencies.elements[startIdx]);
      while (true) {
        // loop that's my problem detected - complain
        if (curr == startIdx) {
          errored = true;
          SymbolTableEntry *start = enumConstants.elements[startIdx];
          fprintf(stderr,
                  "%s:%zu:%zu: error: circular reference in enumeration "
                  "constants\n",
                  start->file->inputFilename, start->line, start->character);
          PathNode *currPathNode = path;
          while (currPathNode != NULL) {
            currPathNode = currPathNode->prev;
            SymbolTableEntry *currEntry =
                enumConstants.elements[currPathNode->curr];
            fprintf(stderr, "%s:%zu:%zu: note: references above\n",
                    currEntry->file->inputFilename, currEntry->line,
                    currEntry->character);
          }
          break;
        }

        // loop that's not my problem detected - stop early
        PathNode *currPathNode = path;
        bool willBreak = false;
        while (currPathNode != NULL) {
          if (currPathNode->curr == curr) {
            // is in a loop - break
            willBreak = true;
            break;
          }
          currPathNode = path->prev;
        }
        if (willBreak) break;

        PathNode *newPath = malloc(sizeof(PathNode));
        newPath->curr = curr;
        newPath->prev = path;
        path = newPath;

        SymbolTableEntry *dependency = dependencies.elements[startIdx];
        if (dependency == NULL) break;
        curr = constantEntryFind(&enumConstants, dependency);
      }

      while (path != NULL) {
        PathNode *prev = path->prev;
        free(path);
        path = prev;
      }
    }
  }

  if (errored) {
    free(processed);
    vectorUninit(&enumConstants, nullDtor);
    vectorUninit(&dependencies, nullDtor);
    vectorUninit(&enumValues, nullDtor);
    return -1;
  }

  // build the enum values
  size_t numProcessed = 0;
  memset(processed, 0, sizeof(bool) * enumConstants.size);
  while (numProcessed < enumConstants.size && !errored) {
    for (size_t idx = 0; idx < enumConstants.size; ++idx) {
      // for each unprocessed enum
      if (!processed[idx]) {
        SymbolTableEntry *current = enumConstants.elements[idx];
        SymbolTableEntry *dependency = dependencies.elements[idx];
        if (dependency == NULL) {
          // no dependency
          Node *literal = enumValues.elements[idx];
          if (literal == NULL) {
            // has no literal value - must be equal to zero at the start of an
            // enum
            current->data.enumConst.signedness = false;
            current->data.enumConst.data.unsignedValue = 0;
          } else {
            // must be a plain (int, char, etc.) literal
            switch (literal->data.literal.type) {
              case LT_UBYTE: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.ubyteVal;
                break;
              }
              case LT_BYTE: {
                current->data.enumConst.signedness =
                    literal->data.literal.data.byteVal < 0;
                current->data.enumConst.data.signedValue =
                    literal->data.literal.data.byteVal;
                break;
              }
              case LT_USHORT: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.ushortVal;
                break;
              }
              case LT_SHORT: {
                current->data.enumConst.signedness =
                    literal->data.literal.data.shortVal < 0;
                current->data.enumConst.data.signedValue =
                    literal->data.literal.data.shortVal;
                break;
              }
              case LT_UINT: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.uintVal;
                break;
              }
              case LT_INT: {
                current->data.enumConst.signedness =
                    literal->data.literal.data.intVal < 0;
                current->data.enumConst.data.signedValue =
                    literal->data.literal.data.intVal;
                break;
              }
              case LT_ULONG: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.ulongVal;
                break;
              }
              case LT_LONG: {
                current->data.enumConst.signedness =
                    literal->data.literal.data.longVal < 0;
                current->data.enumConst.data.signedValue =
                    literal->data.literal.data.longVal;
                break;
              }
              case LT_CHAR: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.charVal;
                break;
              }
              case LT_WCHAR: {
                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    literal->data.literal.data.wcharVal;
                break;
              }
              default: {
                error(__FILE__, __LINE__,
                      "invalid extended int literal used to initialize enum "
                      "constant");
              }
            }
          }
          processed[idx] = true;
          ++numProcessed;
        } else {
          // depends on something
          size_t dependencyIdx = constantEntryFind(&enumConstants, dependency);
          if (processed[dependencyIdx]) {
            // and dependency is satisfied
            Node *literal = enumValues.elements[idx];
            if (literal == NULL) {
              // is previous plus one
              if (dependency->data.enumConst.signedness) {
                if (dependency->data.enumConst.data.signedValue == -1) {
                  // next becomes unsigned
                  current->data.enumConst.signedness = false;
                  current->data.enumConst.data.unsignedValue = 0;
                } else {
                  current->data.enumConst.signedness = true;
                  current->data.enumConst.data.signedValue =
                      dependency->data.enumConst.data.signedValue + 1;
                }
              } else {
                if (current->data.enumConst.data.unsignedValue == ULONG_MAX) {
                  errored = true;
                  fprintf(stderr,
                          "%s:%zu:%zu: error: unrepresentable enumeration "
                          "constant value - value would overflow a ulong",
                          current->file->inputFilename, current->line,
                          current->character);
                  break;
                }

                current->data.enumConst.signedness = false;
                current->data.enumConst.data.unsignedValue =
                    dependency->data.enumConst.data.unsignedValue + 1;
              }
            } else {
              // is equal to the literal
              current->data.enumConst.signedness =
                  dependency->data.enumConst.signedness;
              if (current->data.enumConst.signedness)
                current->data.enumConst.data.signedValue =
                    dependency->data.enumConst.data.signedValue;
              else
                current->data.enumConst.data.unsignedValue =
                    dependency->data.enumConst.data.unsignedValue;
            }
            processed[idx] = true;
            ++numProcessed;
          }
        }
      }
    }
  }
  free(processed);

  vectorUninit(&enumConstants, nullDtor);
  vectorUninit(&dependencies, nullDtor);
  vectorUninit(&enumValues, nullDtor);

  if (errored) return -1;

  // make all enums be all signed or all unsigned, checking for unrepresentable
  // enums
  // for each enum in each file
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *fileEntry = &fileList.entries[fileIdx];
    Vector *bodies = fileEntry->ast->data.file.bodies;

    // for each top level
    for (size_t bodyIdx = 0; bodyIdx < bodies->size; ++bodyIdx) {
      Node *body = bodies->elements[bodyIdx];
      // if it's an enum
      if (body->type == NT_ENUMDECL) {
        SymbolTableEntry *thisEnum = body->data.enumDecl.name->data.id.entry;
        Vector *constantValues = &thisEnum->data.enumType.constantValues;

        // 0 = no particular signedness required
        // 1 = must be unsigned (there's something larger than LONG_MAX)
        // -1 = must be signed (there's a negative)
        // -2 = conflicting requirements
        int requiredSign = 0;
        for (size_t constIdx = 0; constIdx < constantValues->size; ++constIdx) {
          SymbolTableEntry *constEntry = constantValues->elements[constIdx];
          if (constEntry->data.enumConst.signedness) {
            // must be signed - this is a negative
            if (requiredSign == 1) {
              // unrepresentable enum
              fprintf(stderr,
                      "%s:%zu:%zu: error: unrepresentable enumeration - "
                      "enumeration values must be signed, but are large enough "
                      "to overflow a long",
                      thisEnum->file->inputFilename, thisEnum->line,
                      thisEnum->character);
              errored = true;
              requiredSign = -2;
              break;
            }

            requiredSign = -1;
          } else {
            if (constEntry->data.enumConst.data.unsignedValue > LONG_MAX) {
              // must be unsigned - this is greater than LONG_MAX
              if (requiredSign == -1) {
                // unrepresentable enum
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: unrepresentable enumeration - "
                    "enumeration values must be signed, but are large enough "
                    "to overflow a long",
                    thisEnum->file->inputFilename, thisEnum->line,
                    thisEnum->character);
                errored = true;
                requiredSign = -2;
                break;
              }

              requiredSign = 1;
            }
          }
        }

        // nothing to be done if sign is unsigned - already enforced above
        if (requiredSign == -1) {
          for (size_t constIdx = 0; constIdx < constantValues->size;
               ++constIdx) {
            SymbolTableEntry *constEntry = constantValues->elements[constIdx];
            if (!constEntry->data.enumConst.signedness) {
              constEntry->data.enumConst.signedness = true;
              constEntry->data.enumConst.data.signedValue =
                  (int64_t)constEntry->data.enumConst.data.unsignedValue;
            }
          }
        }
      }
    }
  }

  return 0;
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
        hashMapGet(shortFile->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the shorter
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; ++enumIdx) {
        // for each enum constant, is it in the longFile?
        SymbolTableEntry *colliding = hashMapGet(
            longFile->ast->data.file.stab,
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
        hashMapGet(importFile->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the import
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; ++enumIdx) {
        // for each enum constant, is it in the current module?
        SymbolTableEntry *colliding = hashMapGet(
            entry->ast->data.file.stab,
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
        hashMapGet(entry->ast->data.file.stab, lastNameNode->data.id.id);
    if (nameMatch != NULL && nameMatch->kind == SK_ENUM) {
      // FIRSTELM is an enum within the current module
      for (size_t enumIdx = 0;
           enumIdx < nameMatch->data.enumType.constantNames.size; ++enumIdx) {
        // for each enum constant, is it in the import?
        SymbolTableEntry *colliding = hashMapGet(
            importFile->ast->data.file.stab,
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
  for (size_t longIdx = 0; longIdx < imports->size; ++longIdx) {
    Node *longImport = imports->elements[longIdx];
    // search the rest of the list for imports that have all but the last
    // element matching
    for (size_t shortIdx = 0; shortIdx < imports->size; ++shortIdx) {
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

void finishStructStab(FileListEntry *entry, Node *body,
                      SymbolTableEntry *stabEntry, Environment *env) {
  Vector *fields = body->data.structDecl.fields;
  for (size_t fieldIdx = 0; fieldIdx < fields->size; ++fieldIdx) {
    Node *field = fields->elements[fieldIdx];
    Type *type = nodeToType(field->data.varDecl.type, env);
    if (type == NULL) {
      entry->errored = true;
      // process next field
      continue;
    }
    Vector *names = field->data.varDecl.names;
    for (size_t nameIdx = 0; nameIdx < names->size; ++nameIdx) {
      Node *name = names->elements[nameIdx];
      vectorInsert(&stabEntry->data.structType.fieldNames, name->data.id.id);
      vectorInsert(&stabEntry->data.structType.fieldTypes, typeCopy(type));
    }
    typeFree(type);
  }
}

void finishUnionStab(FileListEntry *entry, Node *body,
                     SymbolTableEntry *stabEntry, Environment *env) {
  Vector *options = body->data.unionDecl.options;
  for (size_t optionIdx = 0; optionIdx < options->size; ++optionIdx) {
    Node *option = options->elements[optionIdx];
    Type *type = nodeToType(option->data.varDecl.type, env);
    if (type == NULL) {
      entry->errored = true;
      // process next option
      continue;
    }
    Vector *names = option->data.varDecl.names;
    for (size_t nameIdx = 0; nameIdx < names->size; ++nameIdx) {
      Node *name = names->elements[nameIdx];
      vectorInsert(&stabEntry->data.unionType.optionNames, name->data.id.id);
      vectorInsert(&stabEntry->data.unionType.optionTypes, typeCopy(type));
    }
    typeFree(type);
  }
}

void finishTopLevelStab(FileListEntry *entry) {
  Environment env;
  environmentInit(&env, entry);
  HashMap *implicitStab = env.implicitImport;

  for (size_t bodyIdx = 0; bodyIdx < entry->ast->data.file.bodies->size;
       ++bodyIdx) {
    Node *body = entry->ast->data.file.bodies->elements[bodyIdx];
    switch (body->type) {
      case NT_STRUCTDECL: {
        finishStructStab(entry, body, body->data.structDecl.name->data.id.entry,
                         &env);
        break;
      }
      case NT_UNIONDECL: {
        finishUnionStab(entry, body, body->data.unionDecl.name->data.id.entry,
                        &env);
        break;
      }
      case NT_TYPEDEFDECL: {
        SymbolTableEntry *stabEntry =
            body->data.typedefDecl.name->data.id.entry;
        stabEntry->data.typedefType.actual =
            nodeToType(body->data.typedefDecl.originalType, &env);
        if (stabEntry->data.typedefType.actual == NULL) entry->errored = true;
        break;
      }
      case NT_VARDECL: {
        Vector *names = body->data.varDecl.names;
        Type *type = nodeToType(body->data.varDecl.type, &env);
        if (type == NULL) {
          entry->errored = true;
          break;
        }

        for (size_t nameIdx = 0; nameIdx < names->size; ++nameIdx) {
          Node *name = names->elements[nameIdx];
          name->data.id.entry->data.variable.type = typeCopy(type);
        }
        typeFree(type);

        break;
      }
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        Type *type = nodeToType(body->data.varDefn.type, &env);
        if (type == NULL) {
          entry->errored = true;
          break;
        }

        for (size_t nameIdx = 0; nameIdx < names->size; ++nameIdx) {
          Node *name = names->elements[nameIdx];
          char const *nameString = name->data.id.id;
          SymbolTableEntry *existing = hashMapGet(implicitStab, nameString);
          if (existing != NULL && existing->data.variable.type != NULL &&
              !typeEqual(existing->data.variable.type, type)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: redeclaration of %s as a variable of a "
                    "different type\n",
                    entry->inputFilename, name->line, name->character,
                    nameString);
            fprintf(stderr, "%s:%zu:%zu: note: previously declared here\n",
                    existing->file->inputFilename, existing->line,
                    existing->character);
            entry->errored = true;
          }

          name->data.id.entry->data.variable.type = typeCopy(type);
        }

        typeFree(type);

        break;
      }
      case NT_FUNDECL: {
        Type *returnType = nodeToType(body->data.funDecl.returnType, &env);
        if (returnType == NULL) {
          entry->errored = true;
          break;
        }
        body->data.funDecl.name->data.id.entry->data.function.returnType =
            returnType;

        for (size_t argIdx = 0; argIdx < body->data.funDecl.argTypes->size;
             ++argIdx) {
          Type *argType =
              nodeToType(body->data.funDecl.argTypes->elements[argIdx], &env);
          if (argType == NULL) {
            entry->errored = true;
            break;
          }
          vectorInsert(&body->data.funDecl.name->data.id.entry->data.function
                            .argumentTypes,
                       argType);
        }

        break;
      }
      case NT_FUNDEFN: {
        char const *name = body->data.funDefn.name->data.id.id;
        SymbolTableEntry *existing = hashMapGet(implicitStab, name);
        bool mismatch = false;

        Type *returnType = nodeToType(body->data.funDefn.returnType, &env);
        if (returnType == NULL) {
          entry->errored = true;
          break;
        }
        if (existing != NULL && existing->data.function.returnType != NULL &&
            !typeEqual(existing->data.function.returnType, returnType)) {
          // redeclaration of function with different type
          fprintf(stderr,
                  "%s:%zu:%zu: error: redeclaration of %s as a function of a "
                  "different type\n",
                  entry->inputFilename, body->line, body->character, name);
          fprintf(stderr, "%s:%zu:%zu: note: previously declared here\n",
                  existing->file->inputFilename, existing->line,
                  existing->character);
          entry->errored = true;
          mismatch = true;
        }
        body->data.funDefn.name->data.id.entry->data.function.returnType =
            returnType;

        for (size_t argIdx = 0; argIdx < body->data.funDefn.argTypes->size;
             ++argIdx) {
          Type *argType =
              nodeToType(body->data.funDefn.argTypes->elements[argIdx], &env);
          if (argType == NULL) {
            entry->errored = true;
            break;
          }

          if (existing != NULL &&
              existing->data.function.argumentTypes.elements[argIdx] != NULL &&
              !typeEqual(existing->data.function.argumentTypes.elements[argIdx],
                         argType) &&
              !mismatch) {
            // redeclaration of function with different type
            fprintf(stderr,
                    "%s:%zu:%zu: error: redeclaration of %s as a function of a "
                    "different type\n",
                    entry->inputFilename, body->line, body->character, name);
            fprintf(stderr, "%s:%zu:%zu: note: previously declared here\n",
                    existing->file->inputFilename, existing->line,
                    existing->character);
            entry->errored = true;
            mismatch = true;
          }
          vectorInsert(&body->data.funDefn.name->data.id.entry->data.function
                            .argumentTypes,
                       argType);
        }

        break;
      }
      default: {
        // nothing more to add
        break;
      }
    }
  }

  environmentUninit(&env);
}