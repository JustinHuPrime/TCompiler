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

// symbol table implementation

#include "ast/symbolTable.h"

#include <stdlib.h>

/**
 * deinitialize and free a type
 *
 * @param t type to destroy
 */
static void typeDestroy(Type *t) {
  typeUninit(t);
  free(t);
}

void typeUninit(Type *t) {
  switch (t->type) {
    case K_POINTER: {
      typeDestroy(t->data.pointer.base);
      break;
    }
    case K_ARRAY: {
      typeDestroy(t->data.array.base);
      break;
    }
    case K_FUNPTR: {
      typeDestroy(t->data.funPtr.returnType);
      vectorUninit(&t->data.funPtr.argumentTypes,
                   (void (*)(void *))typeDestroy);
      break;
    }
    default: {
      break;  // nothing to destroy
    }
  }
}

void overloadSetElementDestroy(OverloadSetElement *e) {
  typeUninit(&e->returnType);
  vectorUninit(&e->argumentTypes, (void (*)(void *))typeDestroy);
}

void stabEntryUninit(SymbolTableEntry *e) {
  // TODO: fill this in once there's data in the entry
  // switch (e->type) {
  //   case ST_TYPE: {
  //     break;
  //   }
  //   case ST_FUNCTION: {
  //     break;
  //   }
  //   case ST_VARIABLE: {
  //     break;
  //   }
  // }
}