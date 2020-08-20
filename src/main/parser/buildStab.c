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
            &fileListFindDeclName(import->data.import.id)->ast->data.file.stab;

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

int checkScopedIdCollisions(FileListEntry *entry) {
  // TODO: write this
}

int buildTopLevelEnumStab(void) {
  // TODO: write this
}

// typedef struct {
//   SymbolTableEntry *parentEnum;
//   char const *name;
// } EnumConstant;
//
// static size_t constantEntryFind(Vector *enumConstants,
//                                 SymbolTableEntry *parentEnum,
//                                 char const *name) {
//   for (size_t idx = 0; idx < enumConstants->size; idx++) {
//     EnumConstant *c = enumConstants->elements[idx];
//     if (c->parentEnum == parentEnum && c->name == name) return idx;
//   }
//
//   error(__FILE__, __LINE__, "attempted to look up non-existent enum
//   constant");
// }
//
// static void initEnvForEntry(Environment *env, FileListEntry *entry,
//                             HashMap *stab, HashMap *implicitStab) {
//   environmentInit(env, entry->ast->data.file.module->data.module.id, stab,
//                   implicitStab);
//
//   Vector *imports = entry->ast->data.file.imports;
//   for (size_t idx = 0; idx < imports->size; idx++) {
//     Node *import = imports->elements[idx];
//     vectorInsert(&env->importNames, import->data.import.id);
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wcast-qual"
//     vectorInsert(&env->importTables, (HashMap
//     *)import->data.import.referenced);
// #pragma GCC diagnostic push
//   }
// }
//
// int buildTopLevelEnumStab(void) {
//   Vector enumConstants;  // vector of EnumConstant
//   Vector dependencies;   // vector of EnumConstant, non-owning
//   bool errored = false;

//   // for each enum in each file, create the enumConstant entries
//   for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
//     FileListEntry *entry = &fileList.entries[fileIdx];
//     Vector *bodies = entry->ast->data.file.bodies;
//     HashMap *stab = &entry->ast->data.file.stab;

//     // for each top level
//     for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
//       Node *body = bodies->elements[bodyIdx];
//       // if it's an enum
//       if (body->type == NT_ENUMDECL) {
//         Vector *constantNames = body->data.enumDecl.constantNames;
//         SymbolTableEntry *thisEnum =
//             hashMapGet(stab, body->data.enumDecl.name->data.id.id);
//         // for each constant, construct the enumConstant entry
//         for (size_t constantIdx = 0; constantIdx < constantNames->size;
//              constantIdx++) {
//           Node *constantName = constantNames->elements[constantIdx];

//           EnumConstant *constant = malloc(sizeof(EnumConstant));
//           constant->parentEnum = thisEnum;
//           constant->name = constantName->data.id.id;
//           vectorInsert(&enumConstants, constant);
//           vectorInsert(&dependencies, NULL);
//         }
//       }
//     }
//   }

//   // for each file
//   for (size_t fileIdx = 0; fileIdx < fileList.size; fileIdx++) {
//     FileListEntry *entry = &fileList.entries[fileIdx];
//     Vector *bodies = entry->ast->data.file.bodies;
//     HashMap *stab = &entry->ast->data.file.stab;
//     HashMap *implicitStab = NULL;
//     if (entry->isCode) {
//       FileListEntry *declEntry =
//           hashMapGet(&fileList.moduleMap, entry->moduleName);
//       if (declEntry != NULL) implicitStab = &declEntry->ast->data.file.stab;
//     }
//     Environment env;
//     initEnvForEntry(&env, entry, stab, implicitStab);

//     // for each enum in the file
//     for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
//       Node *body = bodies->elements[bodyIdx];
//       if (body->type == NT_ENUMDECL) {
//         Vector *constantNames = body->data.enumDecl.constantNames;
//         Vector *constantValues = body->data.enumDecl.constantValues;
//         SymbolTableEntry *thisEnum =
//             hashMapGet(stab, body->data.enumDecl.name->data.id.id);

//         // for each constant
//         for (size_t constantIdx = 0; constantIdx < constantNames->size;
//              constantIdx++) {
//           Node *constantName = constantNames->elements[constantIdx];
//           Node *constantValue = constantValues->elements[constantIdx];

//           size_t constantEntryIdx = constantEntryFind(&enumConstants,
//           thisEnum,
//                                                       constantName->data.id.id);
//           if (constantValue == NULL) {
//             // depends on previous
//             if (constantIdx == 0) {
//               // no previous entry - depends on nothing
//               dependencies.elements[constantEntryIdx] = NULL;
//             } else {
//               // depends on previous entry (is always at current - 1)
//               dependencies.elements[constantEntryIdx] =
//                   &dependencies.elements[constantEntryIdx - 1];
//             }
//           } else {
//             // entry = ...
//             if (constantValue->type == NT_LITERAL) {
//               // must be int, char, wchar literal
//               // depends on nothing
//               dependencies.elements[constantEntryIdx] = NULL;
//             } else {
//               // depends on another enum - constantValue is a SCOPED_ID
//               // find the enum
//               SymbolTableEntry
//                   *stabEntry;  // TODO: how do we get and validate this?
//               char const *name =
//                   constantValue->data.scopedId.components
//                       ->elements[constantValue->data.scopedId.components->size
//                       -
//                                  1];
//               if (stabEntry == NULL || stabEntry->kind != SK_ENUM) {
//                 // error - no such enum
//               } else {
//                 dependencies.elements[constantEntryIdx] =
//                     &dependencies.elements[constantEntryFind(&enumConstants,
//                                                              stabEntry,
//                                                              name)];
//               }
//             }
//           }
//         }
//       }
//     }

//     environmentUninit(&env);
//   }

//   if (errored)
//     return -1;
//   else
//     return 0;

//   // TODO: traverse enum dependency graph and build enum constant values,
//   // checking for loops
// }