// Copyright 2021 Justin Hu
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

#include "typechecker/typePredicates.h"

bool typeIsBoolean(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      return t->data.keyword.keyword == TK_BOOL;
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsBoolean(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsIntegral(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_UBYTE:
        case TK_BYTE:
        case TK_CHAR:
        case TK_USHORT:
        case TK_SHORT:
        case TK_UINT:
        case TK_INT:
        case TK_WCHAR:
        case TK_ULONG:
        case TK_LONG:
        case TK_BOOL: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsIntegral(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    case TK_REFERENCE: {
      return t->data.reference.entry->kind == SK_ENUMCONST;
    }
    default: {
      return false;
    }
  }
}

bool typeIsInitializable(Type const *to, Type const *from) {
  return false;  // TODO
}