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

#include "ast/ast.h"
#include "fileList.h"
#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void environmentInit(Environment *env, FileListEntry *currentModuleFile) {
  vectorInit(&env->importFiles);
  vectorInit(&env->importTables);
  env->currentModuleFile = currentModuleFile;
  env->currentModule = &currentModuleFile->ast->data.file.stab;
  env->implicitImport = NULL;
  if (currentModuleFile->isCode) {
    FileListEntry *declEntry =
        hashMapGet(&fileList.moduleMap, currentModuleFile->moduleName);
    if (declEntry != NULL)
      env->implicitImport = &declEntry->ast->data.file.stab;
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
                                                   Node *nameNode) {
  char const *name = nameNode->data.id.id;
  SymbolTableEntry *matched;
  Vector *scopes = &env->scopes;
  for (size_t idx = 0; idx < scopes->size; idx++) {
    // look at local scopes from last to first
    HashMap *scope = scopes->elements[scopes->size - idx - 1];
    matched = hashMapGet(scope, name);
    if (matched != NULL) return matched;
  }
  // check the current module then implicit import
  matched = hashMapGet(env->currentModule, name);
  if (matched != NULL) return matched;
  if (env->implicitImport != NULL) {
    matched = hashMapGet(env->implicitImport, name);
    if (matched != NULL) return matched;
  }

  // search in the imports
  Vector *imports = &env->importTables;
  SymbolTableEntry **matches =
      malloc(sizeof(SymbolTableEntry *) * imports->size);
  size_t *matchIndices = malloc(sizeof(size_t) * imports->size);
  size_t numMatches = 0;
  for (size_t idx = 0; idx < imports->size; idx++) {
    matched = hashMapGet(imports->elements[idx], name);
    if (matched != NULL) {
      matches[numMatches] = matched;
      matchIndices[numMatches] = idx;
      numMatches++;
    }
  }

  if (numMatches == 0) {
    errorNoDecl(env->currentModuleFile, nameNode);
    free(matches);
    free(matchIndices);
    return NULL;
  } else if (numMatches > 1) {
    fprintf(stderr,
            "%s:%zu:%zu: error: '%s' declared in mutliple imported modules\n",
            env->currentModuleFile->inputFilename, nameNode->line,
            nameNode->character, name);
    for (size_t idx = 0; idx < numMatches; idx++)
      fprintf(stderr, "%s:%zu:%zu: note: declared here",
              matches[idx]->file->inputFilename, matches[idx]->line,
              matches[idx]->character);
    free(matches);
    free(matchIndices);
    return NULL;
  } else {
    matched = matches[0];
    free(matches);
    free(matchIndices);
    return matched;
  }
}
static SymbolTableEntry *environmentLookupScoped(Environment *env, Node *name) {
}
SymbolTableEntry *environmentLookup(Environment *env, Node *name) {
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
  // If the
  // name is scoped: There are two possibilities: the name is an enum constant,
  // or it isn't. To cover these cases, first, the name, with the last id
  // removed, is recursively looked up (excepting that it does not resolve to an
  // enum constant), and if it resolves to an enum, and if the last id is an
  // element of the enum, that match is saved as a potential match. Second, the
  // name, with the last id removed, is searched for as a module name, and if a
  // module is found, a element exported by the module (or present in the file
  // scope of the current module, if the name refers to the current module) with
  // the last id is looked up. If only one of these searches ends in a valid
  // result, that result is produced, otherwise there is an ambiguity (which
  // ought to have been caught prior to this). If no valid results are produced,
  // the identifier so named is undefined.

  if (name->type == NT_ID) {
    // is unscoped
    return environmentLookupUnscoped(env, name);
  } else {
    // is scoped
    return environmentLookupScoped(env, name);
  }
}

static void stabFree(HashMap *stab) {
  stabUninit(stab);
  free(stab);
}
void environmentUninit(Environment *env) {
  vectorUninit(&env->importFiles, nullDtor);
  vectorUninit(&env->importTables, nullDtor);
  vectorUninit(&env->scopes, (void (*)(void *))stabFree);
}