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

#include "util/functional.h"

void environmentInit(Environment *env, HashMap *currentModule,
                     HashMap *implicitImport) {
  vectorInit(&env->importNames);
  vectorInit(&env->importTables);
  env->currentModule = currentModule;
  env->implicitImport = implicitImport;
  vectorInit(&env->scopes);
}

void environmentUninit(Environment *env) {
  vectorUninit(&env->importNames, nullDtor);
  vectorUninit(&env->importTables, nullDtor);
  vectorUninit(&env->scopes, nullDtor);
}