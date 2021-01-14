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

#include "ast/environment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"
#include "fileList.h"
#include "util/functional.h"

void environmentInit(Environment *env, FileListEntry *currentModuleFile) {
  vectorInit(&env->importFiles);
  for (size_t idx = 0; idx < currentModuleFile->ast->data.file.imports->size;
       ++idx) {
    Node *import = currentModuleFile->ast->data.file.imports->elements[idx];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    vectorInsert(&env->importFiles, import->data.import.referenced);
#pragma GCC diagnostic pop
  }
  env->currentModuleFile = currentModuleFile;
  env->implicitImport = NULL;
  if (currentModuleFile->isCode) {
    FileListEntry *declEntry = fileListFindDeclName(
        currentModuleFile->ast->data.file.module->data.module.id);
    if (declEntry != NULL) env->implicitImport = declEntry->ast->data.file.stab;
  }
  vectorInit(&env->scopes);
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

static SymbolTableEntry *environmentLookupUnscoped(Environment *env,
                                                   Node *nameNode, bool quiet) {
  char const *name = nameNode->data.id.id;
  SymbolTableEntry *matched;
  Vector *scopes = &env->scopes;
  for (size_t idx = 0; idx < scopes->size; ++idx) {
    // look at local scopes from last to first
    HashMap *scope = scopes->elements[scopes->size - idx - 1];
    matched = hashMapGet(scope, name);
    if (matched != NULL) return matched;
  }
  // check the current module then implicit import
  matched = hashMapGet(env->currentModuleFile->ast->data.file.stab, name);
  if (matched != NULL) return matched;
  if (env->implicitImport != NULL) {
    matched = hashMapGet(env->implicitImport, name);
    if (matched != NULL) return matched;
  }

  // search in the imports
  Vector *imports = &env->importFiles;
  SymbolTableEntry **matches =
      malloc(sizeof(SymbolTableEntry *) * imports->size);
  size_t numMatches = 0;
  for (size_t idx = 0; idx < imports->size; ++idx) {
    FileListEntry *import = imports->elements[idx];
    matched = hashMapGet(import->ast->data.file.stab, name);
    if (matched != NULL) matches[numMatches++] = matched;
  }

  if (numMatches == 0 && !quiet) {
    errorNoDecl(env->currentModuleFile, nameNode);
    free(matches);
    return NULL;
  } else if (numMatches > 1 && !quiet) {
    fprintf(stderr,
            "%s:%zu:%zu: error: '%s' declared in mutliple imported modules\n",
            env->currentModuleFile->inputFilename, nameNode->line,
            nameNode->character, name);
    for (size_t idx = 0; idx < numMatches; ++idx)
      fprintf(stderr, "%s:%zu:%zu: note: declared here\n",
              matches[idx]->file->inputFilename, matches[idx]->line,
              matches[idx]->character);
    free(matches);
    return NULL;
  } else {
    matched = matches[0];
    free(matches);
    return matched;
  }
}
static FileListEntry *environmentFindModule(Environment *env, Node *name,
                                            size_t dropCount) {
  for (size_t idx = 0; idx < env->importFiles.size; ++idx) {
    FileListEntry *file = env->importFiles.elements[idx];
    Node *moduleName = file->ast->data.file.module->data.module.id;
    if (nameNodeEqualWithDrop(moduleName, name, dropCount)) return file;
  }
  return NULL;
}
static SymbolTableEntry *environmentLookupScoped(Environment *env, Node *name,
                                                 bool quiet) {
  // try to match as an enum constant
  if (name->data.scopedId.components->size == 2) {
    SymbolTableEntry *parentEnum = environmentLookupUnscoped(
        env, name->data.scopedId.components->elements[0], true);
    if (parentEnum != NULL && parentEnum->kind == SK_ENUM) {
      // parent was found as an enum - look for the current thing
      Node *last = name->data.scopedId.components->elements[1];
      SymbolTableEntry *enumConst =
          enumLookupEnumConst(parentEnum, last->data.id.id);
      if (enumConst != NULL) return enumConst;
    }
  } else {
    FileListEntry *import = environmentFindModule(env, name, 2);
    if (import != NULL) {
      Node *secondLast =
          name->data.scopedId.components
              ->elements[name->data.scopedId.components->size - 2];
      SymbolTableEntry *parentEnum =
          hashMapGet(import->ast->data.file.stab, secondLast->data.id.id);
      if (parentEnum != NULL && parentEnum->kind == SK_ENUM) {
        Node *last = name->data.scopedId.components
                         ->elements[name->data.scopedId.components->size - 1];
        SymbolTableEntry *enumConst =
            enumLookupEnumConst(parentEnum, last->data.id.id);
        if (enumConst != NULL) return enumConst;
      }
    }
  }

  // try to match as a non-enum-constant
  FileListEntry *import = environmentFindModule(env, name, 1);
  if (import != NULL) {
    Node *last = name->data.scopedId.components
                     ->elements[name->data.scopedId.components->size - 1];
    SymbolTableEntry *entry =
        hashMapGet(import->ast->data.file.stab, last->data.id.id);
    if (entry != NULL) return entry;
  }

  if (!quiet) errorNoDecl(env->currentModuleFile, name);
  return NULL;
}
SymbolTableEntry *environmentLookup(Environment *env, Node *name, bool quiet) {
  // IMPLEMENTATION NOTES: the lookup algorithm
  // If the name is unscoped:
  // The name is looked up in the local scopes from first to
  // last, then in the file scope. A match is produced as soon as it is found
  // If it isn't found yet, it's looked up in the current module, then the
  // implicit import, preferring any found in the current module over those in
  // the implicit import. If it's still not found, it's looked up in each of the
  // imports and produced only if it's found in only one. If it's found in
  // multiple imports, it's declared as ambiguous, and complained about. If it
  // still isn't found, it's declared as missing and complained about.
  //
  // If the name is scoped:
  // There are two possibilities: the name is an enum constant, or it isn't. To
  // cover these cases, first, the name, with the last id removed, is
  // recursively looked up (excepting that it does not resolve to an enum
  // constant), and if it resolves to an enum, and if the last id is an element
  // of the enum, that match is saved as a potential match. Second, the name,
  // with the last id removed, is searched for as a module name, and if a module
  // is found, a element exported by the module (or present in the file scope of
  // the current module, if the name refers to the current module) with the last
  // id is looked up. If only one of these searches ends in a valid result, that
  // result is produced, otherwise there is an ambiguity (which ought to have
  // been caught prior to this). If no valid results are produced, the
  // identifier so named is undefined.

  if (name->type == NT_ID)
    return environmentLookupUnscoped(env, name, quiet);  // is unscoped
  else
    return environmentLookupScoped(env, name, quiet);  // is scoped
}

void environmentPush(Environment *env, HashMap *map) {
  vectorInsert(&env->scopes, map);
}

HashMap *environmentTop(Environment *env) {
  if (env->scopes.size == 0) {
    // no scope - return the main scope
    return env->currentModuleFile->ast->data.file.stab;
  } else {
    return env->scopes.elements[env->scopes.size - 1];
  }
}

HashMap *environmentPop(Environment *env) {
  return env->scopes.elements[--env->scopes.size];
}

void environmentUninit(Environment *env) {
  vectorUninit(&env->importFiles, nullDtor);
  vectorUninit(&env->scopes, (void (*)(void *))stabFree);
}