// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// implementation of register allocation hints

#include "ir/allocHint.h"

#include "typecheck/symbolTable.h"
#include "util/internalError.h"

AllocHint typeKindof(Type const *type) {
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_PTR:
    case K_FUNCTION_PTR:
    case K_ENUM: {
      return AH_GP;
    }
    case K_FLOAT:
    case K_DOUBLE: {
      return AH_SSE;
    }
    case K_STRUCT:
    case K_UNION:
    case K_ARRAY:
    case K_AGGREGATE_INIT: {
      return AH_MEM;
    }
    case K_CONST: {
      return typeKindof(type->data.modifier.type);
    }
    case K_TYPEDEF: {
      return typeKindof(
          type->data.reference.referenced->data.type.data.typedefType.type);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}