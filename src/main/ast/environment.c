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
#include "util/functional.h"

#include <stdlib.h>
#include <string.h>

void environmentInit(Environment *env, Node *currentModuleName,
                     HashMap *currentModule, HashMap *implicitImport) {
  vectorInit(&env->importNames);
  vectorInit(&env->importTables);
  env->currentModuleName = currentModuleName;
  env->currentModule = currentModule;
  env->implicitImport = implicitImport;
  vectorInit(&env->scopes);
}

static SymbolTableEntry *environmentLookupUnscoped(Environment *env,
                                                   Node *name) {
  Vector *scopes = &env->scopes;
  for (size_t idx = 0; idx < scopes->size; idx++) {
    // look at scopes from last to first
    HashMap *scope = scopes->elements[scopes->size - idx - 1];
    
  }
}
static SymbolTableEntry *environmentLookupScoped(Environment *env, Node *name) {
}
SymbolTableEntry *environmentLookup(Environment *env, Node *name) {
  // IMPLEMENTATION NOTES: the lookup algorithm
  // If the name is unscoped:
  // The name is looked up in the local scopes from first to
  // last, then in the file scope. A match is produced as soon as it is found
  // If it isn't found yet, it's looked up in each of the imports and produced
  // only if it's found in only one. If it's found in multiple imports, it's
  // declared as ambiguous, and complained about
  // If the name is scoped:
  // There are two possibilities: the name is an enum constant, or it isn't.
  // To cover these cases, first, the name, with the last id removed, is
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
  vectorUninit(&env->importNames, nullDtor);
  vectorUninit(&env->importTables, nullDtor);
  vectorUninit(&env->scopes, (void (*)(void *))stabFree);
}