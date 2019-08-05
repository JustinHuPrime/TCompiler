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

// Implementation of the symbol table

#include "typecheck/symbolTable.h"

#include "ast/ast.h"
#include "constants.h"
#include "internalError.h"
#include "util/format.h"
#include "util/functional.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

TypeVector *typeVectorCreate(void) { return vectorCreate(); }
void typeVectorInit(TypeVector *v) { vectorInit(v); }
TypeVector *typeVectorCopy(TypeVector *v) {
  return vectorCopy(v, (void *(*)(void *))typeCopy);
}
void typeVectorInsert(TypeVector *v, struct Type *t) { vectorInsert(v, t); }
char *typeVectorToString(TypeVector const *types) {
  char *argString =
      types->size == 0 ? strdup("") : typeToString(types->elements[0]);
  for (size_t idx = 1; idx < types->size; idx++) {
    argString = format("%s, %s", argString, typeToString(types->elements[idx]));
  }
  return argString;
}
void typeVectorUninit(TypeVector *v) {
  vectorUninit(v, (void (*)(void *))typeDestroy);
}
void typeVectorDestroy(TypeVector *v) {
  vectorDestroy(v, (void (*)(void *))typeDestroy);
}

static Type *typeCreate(TypeKind kind) {
  Type *t = malloc(sizeof(Type));
  t->kind = kind;
  return t;
}
static void typeInit(Type *t, TypeKind kind) { t->kind = kind; }
Type *keywordTypeCreate(TypeKind kind) { return typeCreate(kind); }
Type *referneceTypeCreate(TypeKind kind, SymbolInfo *referenced) {
  Type *t = typeCreate(kind);
  t->data.reference.referenced = referenced;
  return t;
}
Type *modifierTypeCreate(TypeKind kind, Type *target) {
  Type *t = typeCreate(kind);
  t->data.modifier.type = target;
  return t;
}
Type *arrayTypeCreate(Type *target, size_t size) {
  Type *t = typeCreate(K_ARRAY);
  t->data.array.type = target;
  t->data.array.size = size;
  return t;
}
Type *functionPtrTypeCreate(Type *returnType, TypeVector *argumentTypes) {
  Type *t = typeCreate(K_FUNCTION_PTR);
  t->data.functionPtr.returnType = returnType;
  t->data.functionPtr.argumentTypes = argumentTypes;
  return t;
}
Type *aggregateInitTypeCreate(TypeVector *elementTypes) {
  Type *t = typeCreate(K_AGGREGATE_INIT);
  t->data.aggregateInit.elementTypes = elementTypes;
  return t;
}
void keywordTypeInit(Type *t, TypeKind kind) { typeInit(t, kind); }
void referneceTypeInit(Type *t, TypeKind kind, struct SymbolInfo *referenced) {
  typeInit(t, kind);
  t->data.reference.referenced = referenced;
}
void modifierTypeInit(Type *t, TypeKind kind, Type *target) {
  typeInit(t, kind);
  t->data.modifier.type = target;
}
void arrayTypeInit(Type *t, Type *target, size_t size) {
  typeInit(t, K_ARRAY);
  t->data.array.type = target;
  t->data.array.size = size;
}
void functionPtrTypeInit(Type *t, Type *returnType, TypeVector *argumentTypes) {
  typeInit(t, K_FUNCTION_PTR);
  t->data.functionPtr.returnType = returnType;
  t->data.functionPtr.argumentTypes = argumentTypes;
}
void aggregateInitTypeInit(Type *t, TypeVector *elementTypes) {
  typeInit(t, K_AGGREGATE_INIT);
  t->data.aggregateInit.elementTypes = elementTypes;
}
Type *typeCopy(Type const *t) {
  switch (t->kind) {
    case K_VOID:
    case K_UBYTE:
    case K_BYTE:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_FLOAT:
    case K_DOUBLE:
    case K_BOOL: {
      return keywordTypeCreate(t->kind);
    }
    case K_CONST:
    case K_PTR: {
      return modifierTypeCreate(t->kind, typeCopy(t->data.modifier.type));
    }
    case K_FUNCTION_PTR: {
      TypeVector *argTypes = typeVectorCreate();
      for (size_t idx = 0; idx < t->data.functionPtr.argumentTypes->size;
           idx++) {
        typeVectorInsert(
            argTypes,
            typeCopy(t->data.functionPtr.argumentTypes->elements[idx]));
      }
      return functionPtrTypeCreate(typeCopy(t->data.functionPtr.returnType),
                                   argTypes);
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      return referneceTypeCreate(t->kind, t->data.reference.referenced);
    }
    case K_ARRAY: {
      return arrayTypeCreate(typeCopy(t->data.array.type), t->data.array.size);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
size_t typeSizeof(Type const *t) {
  switch (t->kind) {
    case K_VOID:
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL: {
      return BYTE_WIDTH;
    }
    case K_CHAR: {
      return CHAR_WIDTH;
    }
    case K_USHORT:
    case K_SHORT: {
      return SHORT_WIDTH;
    }
    case K_UINT:
    case K_INT: {
      return INT_WIDTH;
    }
    case K_WCHAR: {
      return WCHAR_WIDTH;
    }
    case K_ULONG:
    case K_LONG: {
      return LONG_WIDTH;
    }
    case K_PTR:
    case K_FUNCTION_PTR: {
      return POINTER_WIDTH;
    }
    case K_FLOAT: {
      return FLOAT_WIDTH;
    }
    case K_DOUBLE: {
      return DOUBLE_WIDTH;
    }
    case K_CONST: {
      return typeSizeof(t->data.modifier.type);
    }
    case K_STRUCT: {
      size_t acc = 0;
      TypeVector *fieldTypes =
          &t->data.reference.referenced->data.type.data.structType.fields;

      for (size_t idx = 0; idx < fieldTypes->size; idx++) {
        acc += typeSizeof(fieldTypes->elements[idx]);
      }

      return acc;
    }
    case K_UNION: {
      TypeVector *fieldTypes =
          &t->data.reference.referenced->data.type.data.structType.fields;
      size_t acc = typeSizeof(fieldTypes->elements[0]);

      for (size_t idx = 1; idx < fieldTypes->size; idx++) {
        size_t fieldSize = typeSizeof(fieldTypes->elements[idx]);
        if (fieldSize > acc) {
          acc = fieldSize;
        }
      }

      return acc;
    }
    case K_ENUM: {
      size_t numFields =
          t->data.reference.referenced->data.type.data.enumType.fields.size;
      if (numFields <= UBYTE_MAX) {
        return 1;  // fits in a byte
      } else if (numFields <= USHORT_MAX) {
        return 2;
      } else if (numFields <= UINT_MAX) {
        return 4;
      } else {
        return 8;  // might actually have more fields, but we crash anyways
      }
    }
    case K_TYPEDEF: {
      return typeSizeof(
          t->data.reference.referenced->data.type.data.typedefType.type);
    }
    case K_ARRAY: {
      return t->data.array.size * typeSizeof(t->data.array.type);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
bool typeIsIncomplete(Type const *t, Environment const *env) {
  switch (t->kind) {
    case K_VOID: {
      return true;
    }
    case K_UBYTE:
    case K_BYTE:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_FLOAT:
    case K_DOUBLE:
    case K_BOOL:
    case K_FUNCTION_PTR:
    case K_PTR: {
      return false;
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      SymbolInfo *info = t->data.reference.referenced;
      switch (info->data.type.kind) {
        case TDK_STRUCT: {
          return info->data.type.data.structType.incomplete;
        }
        case TDK_UNION: {
          return info->data.type.data.unionType.incomplete;
        }
        case TDK_ENUM: {
          return info->data.type.data.enumType.incomplete;
        }
        case TDK_TYPEDEF: {
          return typeIsIncomplete(info->data.type.data.typedefType.type, env);
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid TypeDefinitionKind enum constant");
        }
      }
    }
    case K_CONST: {
      return typeIsIncomplete(t->data.modifier.type, env);
    }
    case K_ARRAY: {
      return typeIsIncomplete(t->data.array.type, env);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
bool typeEqual(Type const *t1, Type const *t2) {
  if (t1->kind != t2->kind) {
    return false;
  } else {
    switch (t1->kind) {
      case K_VOID:
      case K_UBYTE:
      case K_BYTE:
      case K_CHAR:
      case K_USHORT:
      case K_SHORT:
      case K_UINT:
      case K_INT:
      case K_WCHAR:
      case K_ULONG:
      case K_LONG:
      case K_FLOAT:
      case K_DOUBLE:
      case K_BOOL: {
        return true;
      }
      case K_FUNCTION_PTR: {
        if (t1->data.functionPtr.argumentTypes->size !=
            t2->data.functionPtr.argumentTypes->size) {
          return false;
        }
        for (size_t idx = 0; idx < t1->data.functionPtr.argumentTypes->size;
             idx++) {
          if (!typeEqual(t1->data.functionPtr.argumentTypes->elements[idx],
                         t2->data.functionPtr.argumentTypes->elements[idx])) {
            return false;
          }
        }

        return typeEqual(t1->data.functionPtr.returnType,
                         t2->data.functionPtr.returnType);
      }
      case K_PTR:
      case K_CONST: {
        return typeEqual(t1->data.modifier.type, t2->data.modifier.type);
      }
      case K_ARRAY: {
        return t1->data.array.size == t2->data.array.size &&
               typeEqual(t1->data.array.type, t2->data.array.type);
      }
      case K_STRUCT:
      case K_UNION:
      case K_ENUM:
      case K_TYPEDEF: {
        return t1->data.reference.referenced == t2->data.reference.referenced;
        // this works because references reference the symbol table entry
      }
      default: {
        error(__FILE__, __LINE__,
              "encountered an invalid TypeKind enum constant");
      }
    }
  }
}
static bool pointerAssignable(Type const *pointedTo, Type const *pointedFrom) {
  if (pointedFrom->kind == K_CONST) {
    return pointedTo->kind == K_CONST &&
           pointerAssignable(pointedTo->data.modifier.type,
                             pointedFrom->data.modifier.type);
  } else if (pointedTo->kind == K_CONST) {
    return pointerAssignable(pointedTo->data.modifier.type, pointedFrom);
  } else if (pointedFrom->kind == K_VOID || pointedTo->kind == K_VOID) {
    return true;
  } else {
    return typeEqual(pointedTo, pointedFrom);
  }
}
bool typeAssignable(Type const *to, Type const *from) {
  switch (to->kind) {
    case K_VOID: {
      return false;
    }
    case K_UBYTE: {
      switch (from->kind) {
        case K_UBYTE: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_BYTE: {
      switch (from->kind) {
        case K_BYTE: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_CHAR: {
      switch (from->kind) {
        case K_CHAR: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_USHORT: {
      switch (from->kind) {
        case K_UBYTE:
        case K_USHORT: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_SHORT: {
      switch (from->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_SHORT: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_UINT: {
      switch (from->kind) {
        case K_UBYTE:
        case K_USHORT:
        case K_UINT: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_INT: {
      switch (from->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_INT: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_WCHAR: {
      switch (from->kind) {
        case K_CHAR:
        case K_WCHAR: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_ULONG: {
      switch (from->kind) {
        case K_UBYTE:
        case K_USHORT:
        case K_UINT:
        case K_ULONG: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_LONG: {
      switch (from->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_LONG: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_FLOAT: {
      switch (from->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_DOUBLE: {
      switch (from->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_BOOL: {
      switch (from->kind) {
        case K_BOOL: {
          return true;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_STRUCT: {
      switch (from->kind) {
        case K_STRUCT: {
          return from->data.reference.referenced ==
                 to->data.reference.referenced;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        case K_AGGREGATE_INIT: {
          TypeVector *fields =
              &to->data.reference.referenced->data.type.data.structType.fields;
          TypeVector *elements = from->data.aggregateInit.elementTypes;
          if (fields->size == elements->size) {
            for (size_t idx = 0; idx < fields->size; idx++) {
              if (!typeAssignable(fields->elements[idx],
                                  elements->elements[idx])) {
                return false;
              }
            }
            return true;
          } else {
            return false;
          }
        }
        default: { return false; }
      }
    }
    case K_UNION: {
      switch (from->kind) {
        case K_UNION: {
          return from->data.reference.referenced ==
                 to->data.reference.referenced;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_ENUM: {
      switch (from->kind) {
        case K_ENUM: {
          return from->data.reference.referenced ==
                 to->data.reference.referenced;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_TYPEDEF: {
      switch (from->kind) {
        case K_TYPEDEF: {
          return from->data.reference.referenced ==
                 to->data.reference.referenced;
        }
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_CONST: {
      return false;
    }
    case K_ARRAY: {
      switch (from->kind) {
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        case K_ARRAY: {
          return from->data.array.size == to->data.array.size &&
                 typeAssignable(to->data.array.type, from->data.array.type);
        }
        case K_AGGREGATE_INIT: {
          if (from->data.aggregateInit.elementTypes->size ==
              to->data.array.size) {
            for (size_t idx = 0; idx < to->data.array.size; idx++) {
              if (!typeAssignable(
                      to->data.array.type,
                      from->data.aggregateInit.elementTypes->elements[idx])) {
                return false;
              }
            }
            return true;
          } else {
            return false;
          }
        }
        default: { return false; }
      }
    }
    case K_PTR: {
      switch (from->kind) {
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        case K_PTR: {
          return pointerAssignable(to->data.modifier.type,
                                   from->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_FUNCTION_PTR: {
      switch (from->kind) {
        case K_CONST: {
          return typeAssignable(to, from->data.modifier.type);
        }
        case K_FUNCTION_PTR: {
          return typeEqual(to, from);
        }
        default: { return false; }
      }
    }
    case K_AGGREGATE_INIT: {
      return false;
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
bool typeComparable(Type const *a, Type const *b) {
  if (a->kind == K_VOID || b->kind == K_VOID || a->kind == K_AGGREGATE_INIT ||
      b->kind == K_AGGREGATE_INIT) {
    return false;
  }
  switch (a->kind) {
    case K_UBYTE: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_BYTE: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_CHAR: {
      switch (b->kind) {
        case K_CHAR:
        case K_WCHAR: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_USHORT: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_SHORT: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_UINT: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_INT: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_WCHAR: {
      switch (b->kind) {
        case K_CHAR:
        case K_WCHAR: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_ULONG: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_LONG: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_FLOAT: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_DOUBLE: {
      switch (b->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_BOOL: {
      switch (b->kind) {
        case K_BOOL: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_STRUCT: {
      switch (b->kind) {
        case K_STRUCT: {
          return a->data.reference.referenced == b->data.reference.referenced;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_UNION: {
      switch (b->kind) {
        case K_UNION: {
          return a->data.reference.referenced == b->data.reference.referenced;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_ENUM: {
      switch (b->kind) {
        case K_ENUM: {
          return a->data.reference.referenced == b->data.reference.referenced;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_TYPEDEF: {
      switch (b->kind) {
        case K_TYPEDEF: {
          return a->data.reference.referenced == b->data.reference.referenced;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_CONST: {
      return typeComparable(a->data.modifier.type, b);
    }
    case K_ARRAY: {
      switch (b->kind) {
        case K_ARRAY: {
          return typeEqual(a, b);
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_PTR: {
      switch (b->kind) {
        case K_PTR: {
          return true;
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    case K_FUNCTION_PTR: {
      switch (b->kind) {
        case K_FUNCTION_PTR: {
          return typeEqual(a, b);
        }
        case K_CONST: {
          return typeComparable(a, b->data.modifier.type);
        }
        default: { return false; }
      }
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
bool typeCastable(Type const *to, Type const *from) {
  if (to->kind == K_AGGREGATE_INIT || to->kind == K_VOID ||
      from->kind == K_VOID) {
    return false;
  }
  switch (from->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_ULONG:
    case K_LONG: {
      switch (to->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_CHAR:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_WCHAR:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE:
        case K_BOOL:
        case K_ENUM: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_CHAR:
    case K_WCHAR: {
      switch (to->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_CHAR:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_WCHAR:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE:
        case K_BOOL: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_FLOAT:
    case K_DOUBLE: {
      switch (to->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_FLOAT:
        case K_DOUBLE:
        case K_BOOL: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_BOOL: {
      switch (to->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_ULONG:
        case K_LONG:
        case K_BOOL: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_STRUCT: {
      switch (to->kind) {
        case K_STRUCT: {
          return to->data.reference.referenced ==
                 from->data.reference.referenced;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_UNION: {
      TypeVector *myTypes =
          &from->data.reference.referenced->data.type.data.unionType.fields;
      for (size_t idx = 0; idx < myTypes->size; idx++) {
        if (typeCastable(to, myTypes->elements[idx])) {
          return true;
        }
      }
      switch (to->kind) {
        case K_UNION: {
          if (to->data.reference.referenced ==
              from->data.reference.referenced) {
            return true;
          }
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_ENUM: {
      switch (to->kind) {
        case K_UBYTE: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= UBYTE_MAX;
        }
        case K_BYTE: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= BYTE_MAX;
        }
        case K_USHORT: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= USHORT_MAX;
        }
        case K_SHORT: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= SHORT_MAX;
        }
        case K_UINT: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= UINT_MAX;
        }
        case K_INT: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= INT_MAX;
        }
        case K_ULONG: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= ULONG_MAX;
        }
        case K_LONG: {
          return from->data.reference.referenced->data.type.data.enumType.fields
                     .size <= LONG_MAX;
        }
        case K_ENUM: {
          return to->data.reference.referenced ==
                 from->data.reference.referenced;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return typeCastable(
              to->data.reference.referenced->data.type.data.typedefType.type,
              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_TYPEDEF: {
      if (typeCastable(to, from->data.reference.referenced->data.type.data
                               .typedefType.type)) {
        return true;
      }
      switch (to->kind) {
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return to->data.reference.referenced ==
                     from->data.reference.referenced ||
                 typeCastable(to->data.reference.referenced->data.type.data
                                  .typedefType.type,
                              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_CONST: {
      switch (to->kind) {
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return to->data.reference.referenced ==
                     from->data.reference.referenced ||
                 typeCastable(to->data.reference.referenced->data.type.data
                                  .typedefType.type,
                              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from) ||
                 typeCastable(to, from->data.modifier.type);
        }
        default: { return typeCastable(to, from->data.modifier.type); }
      }
    }
    case K_ARRAY: {
      switch (to->kind) {
        case K_ARRAY: {
          return to->data.array.size == from->data.array.size &&
                 typeCastable(to->data.array.type, from->data.array.type);
        }
        case K_PTR: {
          return pointerAssignable(to->data.modifier.type,
                                   from->data.array.type);
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return to->data.reference.referenced ==
                     from->data.reference.referenced ||
                 typeCastable(to->data.reference.referenced->data.type.data
                                  .typedefType.type,
                              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_PTR: {
      switch (to->kind) {
        case K_PTR: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return to->data.reference.referenced ==
                     from->data.reference.referenced ||
                 typeCastable(to->data.reference.referenced->data.type.data
                                  .typedefType.type,
                              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_FUNCTION_PTR: {
      switch (to->kind) {
        case K_FUNCTION_PTR: {
          return true;
        }
        case K_UNION: {
          TypeVector *possibleTypes =
              &to->data.reference.referenced->data.type.data.unionType.fields;
          for (size_t idx = 0; idx < possibleTypes->size; idx++) {
            if (typeCastable(possibleTypes->elements[idx], from)) {
              return true;
            }
          }
          return false;
        }
        case K_TYPEDEF: {
          return to->data.reference.referenced ==
                     from->data.reference.referenced ||
                 typeCastable(to->data.reference.referenced->data.type.data
                                  .typedefType.type,
                              from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        default: { return false; }
      }
    }
    case K_AGGREGATE_INIT: {
      switch (to->kind) {
        case K_STRUCT: {
          return typeAssignable(to, from);
        }
        case K_CONST: {
          return typeCastable(to->data.modifier.type, from);
        }
        case K_ARRAY: {
          return typeAssignable(to, from);
        }
        default: { return false; }
      }
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}
Type *typeExpMerge(Type const *lhs, Type const *rhs) {
  if (lhs->kind == K_VOID || rhs->kind == K_VOID) {
    return NULL;
  }
  switch (lhs->kind) {
    case K_UBYTE: {
      switch (rhs->kind) {
        case K_UBYTE: {
          return keywordTypeCreate(K_UBYTE);
        }
        case K_BYTE:
        case K_SHORT: {
          return keywordTypeCreate(K_SHORT);
        }
        case K_USHORT: {
          return keywordTypeCreate(K_USHORT);
        }
        case K_UINT: {
          return keywordTypeCreate(K_UINT);
        }
        case K_INT: {
          return keywordTypeCreate(K_INT);
        }
        case K_ULONG: {
          return keywordTypeCreate(K_ULONG);
        }
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_BYTE: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_SHORT: {
          return keywordTypeCreate(K_SHORT);
        }
        case K_BYTE: {
          return keywordTypeCreate(K_BYTE);
        }
        case K_USHORT:
        case K_INT: {
          return keywordTypeCreate(K_INT);
        }
        case K_UINT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_CHAR: {
      switch (rhs->kind) {
        case K_CHAR: {
          return keywordTypeCreate(K_CHAR);
        }
        case K_WCHAR: {
          return keywordTypeCreate(K_WCHAR);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_USHORT: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_USHORT: {
          return keywordTypeCreate(K_USHORT);
        }
        case K_BYTE:
        case K_SHORT: {
          return keywordTypeCreate(K_INT);
        }
        case K_UINT: {
          return keywordTypeCreate(K_UINT);
        }
        case K_INT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_ULONG: {
          return keywordTypeCreate(K_ULONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_SHORT: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_SHORT: {
          return keywordTypeCreate(K_SHORT);
        }
        case K_USHORT:
        case K_INT: {
          return keywordTypeCreate(K_INT);
        }
        case K_UINT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_UINT: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_USHORT:
        case K_UINT: {
          return keywordTypeCreate(K_UINT);
        }
        case K_BYTE:
        case K_SHORT:
        case K_INT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_ULONG: {
          return keywordTypeCreate(K_ULONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_INT: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_INT: {
          return keywordTypeCreate(K_INT);
        }
        case K_UINT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_WCHAR: {
      switch (rhs->kind) {
        case K_CHAR:
        case K_WCHAR: {
          return keywordTypeCreate(K_WCHAR);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_ULONG: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_USHORT:
        case K_UINT:
        case K_ULONG: {
          return keywordTypeCreate(K_ULONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_LONG: {
      switch (rhs->kind) {
        case K_UBYTE:
        case K_BYTE:
        case K_USHORT:
        case K_SHORT:
        case K_UINT:
        case K_INT:
        case K_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_FLOAT: {
      switch (rhs->kind) {
        case K_BYTE:
        case K_UBYTE:
        case K_SHORT:
        case K_USHORT:
        case K_INT:
        case K_UINT:
        case K_LONG:
        case K_ULONG:
        case K_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_DOUBLE: {
      switch (rhs->kind) {
        case K_BYTE:
        case K_UBYTE:
        case K_SHORT:
        case K_USHORT:
        case K_INT:
        case K_UINT:
        case K_LONG:
        case K_ULONG:
        case K_FLOAT:
        case K_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_BOOL: {
      switch (rhs->kind) {
        case K_BOOL: {
          return keywordTypeCreate(K_BOOL);
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_STRUCT: {
      switch (rhs->kind) {
        case K_STRUCT: {
          return rhs->data.reference.referenced ==
                         lhs->data.reference.referenced
                     ? referneceTypeCreate(K_STRUCT,
                                           lhs->data.reference.referenced)
                     : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_UNION: {
      switch (rhs->kind) {
        case K_UNION: {
          return rhs->data.reference.referenced ==
                         lhs->data.reference.referenced
                     ? referneceTypeCreate(K_UNION,
                                           lhs->data.reference.referenced)
                     : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_ENUM: {
      switch (rhs->kind) {
        case K_ENUM: {
          return rhs->data.reference.referenced ==
                         lhs->data.reference.referenced
                     ? referneceTypeCreate(K_ENUM,
                                           lhs->data.reference.referenced)
                     : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_TYPEDEF: {
      switch (rhs->kind) {
        case K_TYPEDEF: {
          return rhs->data.reference.referenced ==
                         lhs->data.reference.referenced
                     ? referneceTypeCreate(K_TYPEDEF,
                                           lhs->data.reference.referenced)
                     : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_CONST: {
      switch (rhs->kind) {
        case K_CONST: {
          return modifierTypeCreate(
              K_CONST,
              typeExpMerge(lhs->data.modifier.type, rhs->data.modifier.type));
        }
        default: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs->data.modifier.type, rhs));
        }
      }
    }
    case K_ARRAY: {
      switch (rhs->kind) {
        case K_ARRAY: {
          return typeEqual(lhs, rhs) ? typeCopy(lhs) : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_PTR: {
      switch (rhs->kind) {
        case K_PTR: {
          if (typeEqual(lhs, rhs)) {
            return typeCopy(lhs);
          } else if (typeEqual(typeGetNonConst(lhs->data.modifier.type),
                               typeGetNonConst(rhs->data.modifier.type))) {
            return modifierTypeCreate(
                K_PTR, modifierTypeCreate(
                           K_CONST,
                           typeCopy(typeGetNonConst(lhs->data.modifier.type))));
          } else {
            return NULL;
          }
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_FUNCTION_PTR: {
      switch (rhs->kind) {
        case K_FUNCTION_PTR: {
          return typeEqual(lhs, rhs) ? typeCopy(lhs) : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    case K_AGGREGATE_INIT: {
      switch (rhs->kind) {
        case K_AGGREGATE_INIT: {
          return typeEqual(lhs, rhs) ? typeCopy(lhs) : NULL;
        }
        case K_CONST: {
          return modifierTypeCreate(K_CONST,
                                    typeExpMerge(lhs, rhs->data.modifier.type));
        }
        default: { return NULL; }
      }
    }
    default: {
      return NULL;  // invalid enum
    }
  }
}
char *typeToString(Type const *type) {
  switch (type->kind) {
    case K_VOID: {
      return strdup("void");
    }
    case K_UBYTE: {
      return strdup("ubyte");
    }
    case K_BYTE: {
      return strdup("byte");
    }
    case K_CHAR: {
      return strdup("char");
    }
    case K_USHORT: {
      return strdup("ushort");
    }
    case K_SHORT: {
      return strdup("short");
    }
    case K_UINT: {
      return strdup("uint");
    }
    case K_INT: {
      return strdup("int");
    }
    case K_WCHAR: {
      return strdup("wchar");
    }
    case K_ULONG: {
      return strdup("ulong");
    }
    case K_LONG: {
      return strdup("long");
    }
    case K_FLOAT: {
      return strdup("float");
    }
    case K_DOUBLE: {
      return strdup("double");
    }
    case K_BOOL: {
      return strdup("bool");
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      return strdup(type->data.reference.referenced->data.type.name);
    }
    case K_CONST: {
      return format("%s const", typeToString(type->data.modifier.type));
    }
    case K_ARRAY: {
      return format("%s[%zu]", typeToString(type->data.array.type),
                    type->data.array.size);
    }
    case K_PTR: {
      return format("%s *", typeToString(type->data.modifier.type));
    }
    case K_FUNCTION_PTR: {
      return format("%s(%s)", typeToString(type->data.functionPtr.returnType),
                    typeVectorToString(type->data.functionPtr.argumentTypes));
    }
    case K_AGGREGATE_INIT: {
      return format("{%s}",
                    typeVectorToString(type->data.functionPtr.argumentTypes));
    }
    default: { return NULL; }
  }
}
static bool typeIsX(Type const *type, TypeKind x) {
  return type->kind == x ||
         (type->kind == K_CONST && typeIsX(type->data.modifier.type, x));
}
static bool typeIsXOrY(Type const *type, TypeKind x, TypeKind y) {
  return type->kind == x || type->kind == y ||
         (type->kind == K_CONST && typeIsXOrY(type->data.modifier.type, x, y));
}
bool typeIsBoolean(Type const *type) { return typeIsX(type, K_BOOL); }
bool typeIsIntegral(Type const *type) {
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_ULONG:
    case K_LONG: {
      return true;
    }
    case K_CONST: {
      return typeIsIntegral(type->data.modifier.type);
    }
    default: { return false; }
  }
}
bool typeIsSignedIntegral(Type const *type) {
  return type->kind == K_BYTE || type->kind == K_SHORT || type->kind == K_INT ||
         type->kind == K_LONG ||
         (type->kind == K_CONST &&
          typeIsSignedIntegral(type->data.modifier.type));
}
bool typeIsFloat(Type const *type) {
  return typeIsXOrY(type, K_FLOAT, K_DOUBLE);
}
bool typeIsNumeric(Type const *type) {
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_ULONG:
    case K_LONG:
    case K_FLOAT:
    case K_DOUBLE: {
      return true;
    }
    case K_CONST: {
      return typeIsIntegral(type->data.modifier.type);
    }
    default: { return false; }
  }
}
bool typeIsValuePointer(Type const *type) { return typeIsX(type, K_PTR); }
bool typeIsFunctionPointer(Type const *type) {
  return typeIsX(type, K_FUNCTION_PTR);
}
bool typeIsPointer(Type const *type) {
  return typeIsXOrY(type, K_PTR, K_FUNCTION_PTR);
}
bool typeIsCompound(Type const *type) {
  return typeIsXOrY(type, K_STRUCT, K_UNION);
}
bool typeIsComposite(Type const *type) {
  switch (type->kind) {
    case K_ARRAY:
    case K_STRUCT: {
      return true;
    }
    case K_UNION: {
      TypeVector *fields =
          &type->data.reference.referenced->data.type.data.unionType.fields;
      for (size_t idx = 0; idx < fields->size; idx++) {
        if (typeIsComposite(fields->elements[idx])) {
          return true;
        }
      }
      return false;
    }
    case K_CONST: {
      return typeIsComposite(type->data.modifier.type);
    }
    default: { return false; }
  }
}
bool typeIsArray(Type const *type) { return typeIsX(type, K_ARRAY); }
Type *typeGetNonConst(Type *type) {
  return type->kind == K_CONST ? typeGetNonConst(type->data.modifier.type)
                               : type;
}
Type *typeGetDereferenced(Type *type) {
  if (type->kind == K_PTR) {
    return typeCopy(type->data.modifier.type);
  } else if (type->kind == K_CONST) {
    return typeGetDereferenced(type->data.modifier.type);
  } else {
    return NULL;
  }
}
Type *typeGetArrayElement(Type *type) {
  if (type->kind == K_ARRAY) {
    return typeCopy(type->data.array.type);
  } else if (type->kind == K_CONST) {
    return typeGetArrayElement(type->data.modifier.type);
  } else {
    return NULL;
  }
}
void typeUninit(Type *t) {
  switch (t->kind) {
    case K_VOID:
    case K_UBYTE:
    case K_BYTE:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_FLOAT:
    case K_DOUBLE:
    case K_BOOL:
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      break;
    }
    case K_CONST:
    case K_PTR: {
      typeDestroy(t->data.modifier.type);
      break;
    }
    case K_ARRAY: {
      typeDestroy(t->data.array.type);
      break;
    }
    case K_FUNCTION_PTR: {
      typeDestroy(t->data.functionPtr.returnType);
      typeVectorDestroy(t->data.functionPtr.argumentTypes);
      break;
    }
    case K_AGGREGATE_INIT: {
      typeVectorDestroy(t->data.aggregateInit.elementTypes);
      break;
    }
  }
}
void typeDestroy(Type *t) {
  typeUninit(t);
  free(t);
}

OverloadSetElement *overloadSetElementCreate(void) {
  OverloadSetElement *elm = malloc(sizeof(OverloadSetElement));
  typeVectorInit(&elm->argumentTypes);
  elm->returnType = NULL;
  return elm;
}
OverloadSetElement *overloadSetElementCopy(OverloadSetElement const *from) {
  OverloadSetElement *to = malloc(sizeof(OverloadSetElement));

  to->defined = from->defined;
  to->numOptional = from->numOptional;
  to->returnType = typeCopy(from->returnType);
  to->argumentTypes.capacity = from->argumentTypes.capacity;
  to->argumentTypes.size = from->argumentTypes.size;
  to->argumentTypes.elements =
      malloc(to->argumentTypes.capacity * sizeof(void *));
  for (size_t idx = 0; idx < to->argumentTypes.size; idx++) {
    to->argumentTypes.elements[idx] =
        typeCopy(from->argumentTypes.elements[idx]);
  }

  return to;
}
void overloadSetElementDestroy(OverloadSetElement *elm) {
  if (elm->returnType != NULL) {
    typeDestroy(elm->returnType);
  }
  typeVectorUninit(&elm->argumentTypes);
  free(elm);
}

void overloadSetInit(OverloadSet *set) { vectorInit(set); }
void overloadSetInsert(OverloadSet *set, OverloadSetElement *elm) {
  vectorInsert(set, elm);
}
OverloadSetElement *overloadSetLookupCollision(OverloadSet const *set,
                                               TypeVector const *argTypes,
                                               size_t numOptional) {
  for (size_t candidateIdx = 0; candidateIdx < set->size; candidateIdx++) {
    // select n args, where n is the larger of the required argument sizes.
    // if that minimum is within both the possible number of args for the
    // candidate and the lookup, try to match types if they all match, return
    // match, else, keep looking
    OverloadSetElement *candidate = set->elements[candidateIdx];
    size_t candidateRequired =
        candidate->argumentTypes.size - candidate->numOptional;
    size_t thisRequired = argTypes->size - numOptional;
    size_t maxRequired =
        candidateRequired > thisRequired ? candidateRequired : thisRequired;
    bool candidateLonger = candidateRequired > thisRequired;
    if (candidateLonger ? argTypes->size >= maxRequired
                        : candidate->argumentTypes.size >= maxRequired) {
      bool match = true;
      for (size_t idx = 0; idx < maxRequired; idx++) {
        if (!typeEqual(
                (candidateLonger ? candidate->argumentTypes.elements
                                 : argTypes->elements)[idx],
                (candidateLonger ? argTypes->elements
                                 : candidate->argumentTypes.elements)[idx])) {
          match = false;
          break;
        }
      }
      if (match) {
        return candidate;
      }
    }
  }

  return NULL;
}
OverloadSetElement *overloadSetLookupDefinition(OverloadSet const *set,
                                                TypeVector const *argTypes) {
  for (size_t candidateIdx = 0; candidateIdx < set->size; candidateIdx++) {
    OverloadSetElement *candidate = set->elements[candidateIdx];
    if (candidate->argumentTypes.size == argTypes->size) {
      bool match = true;
      for (size_t argIdx = 0; argIdx < argTypes->size; argIdx++) {
        if (!typeEqual(candidate->argumentTypes.elements[argIdx],
                       argTypes->elements[argIdx])) {
          match = false;
          break;
        }
      }
      if (match) {
        return candidate;
      }
    }
  }

  return NULL;
}
Vector *overloadSetLookupCall(OverloadSet const *set,
                              TypeVector const *argTypes) {
  Vector *candidates = vectorCreate();

  for (size_t maxCasted = 0; maxCasted < argTypes->size; maxCasted++) {
    for (size_t candidateIdx = 0; candidateIdx < set->size; candidateIdx++) {
      OverloadSetElement *candidate = set->elements[candidateIdx];
      if (candidate->argumentTypes.size <= argTypes->size &&
          candidate->argumentTypes.size - argTypes->size <=
              candidate->numOptional) {
        size_t numCasted = 0;
        for (size_t idx = 0; idx < argTypes->size; idx++) {
          if (!typeAssignable(
                  typeGetNonConst(candidate->argumentTypes.elements[idx]),
                  argTypes->elements[idx])) {
            numCasted = maxCasted + 1;
            break;
          } else if (!typeEqual(typeGetNonConst(
                                    candidate->argumentTypes.elements[idx]),
                                typeGetNonConst(argTypes->elements[idx]))) {
            numCasted++;
            if (numCasted > maxCasted) {
              break;
            }
          }
        }
        if (numCasted <= maxCasted) {
          vectorInsert(candidates, candidate);
        }
      }
    }
    if (candidates->size != 0) {
      return candidates;
    }
  }

  return candidates;
}
void overloadSetUninit(OverloadSet *set) {
  vectorUninit(set, (void (*)(void *))overloadSetElementDestroy);
}

char const *typeDefinitionKindToString(TypeDefinitionKind kind) {
  switch (kind) {
    case TDK_STRUCT: {
      return "a struct";
    }
    case TDK_UNION: {
      return "a union";
    }
    case TDK_ENUM: {
      return "an enumeration";
    }
    case TDK_TYPEDEF: {
      return "a type alias";
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeDefinitionKind enum constant");
    }
  }
}

static SymbolInfo *symbolInfoCreate(SymbolKind kind, char const *moduleName) {
  SymbolInfo *si = malloc(sizeof(SymbolInfo));
  si->kind = kind;
  si->module = moduleName;
  return si;
}
SymbolInfo *varSymbolInfoCreate(char const *moduleName, Type *type, bool bound,
                                bool escapes) {
  SymbolInfo *si = symbolInfoCreate(SK_VAR, moduleName);
  si->data.var.type = type;
  si->data.var.access = NULL;
  si->data.var.bound = bound;
  si->data.var.escapes = escapes;
  return si;
}
SymbolInfo *structSymbolInfoCreate(char const *moduleName, char const *name) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE, moduleName);
  si->data.type.kind = TDK_STRUCT;
  si->data.type.name = name;
  si->data.type.data.structType.incomplete = true;
  typeVectorInit(&si->data.type.data.structType.fields);
  stringVectorInit(&si->data.type.data.structType.names);
  return si;
}
SymbolInfo *unionSymbolInfoCreate(char const *moduleName, char const *name) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE, moduleName);
  si->data.type.kind = TDK_UNION;
  si->data.type.name = name;
  si->data.type.data.unionType.incomplete = true;
  typeVectorInit(&si->data.type.data.unionType.fields);
  stringVectorInit(&si->data.type.data.unionType.names);
  return si;
}
SymbolInfo *enumSymbolInfoCreate(char const *moduleName, char const *name) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE, moduleName);
  si->data.type.kind = TDK_ENUM;
  si->data.type.name = name;
  si->data.type.data.enumType.incomplete = true;
  stringVectorInit(&si->data.type.data.enumType.fields);
  return si;
}
SymbolInfo *typedefSymbolInfoCreate(char const *moduleName, Type *what,
                                    char const *name) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE, moduleName);
  si->data.type.kind = TDK_TYPEDEF;
  si->data.type.name = name;
  si->data.type.data.typedefType.type = what;
  return si;
}
SymbolInfo *functionSymbolInfoCreate(char const *moduleName) {
  SymbolInfo *si = symbolInfoCreate(SK_FUNCTION, moduleName);
  overloadSetInit(&si->data.function.overloadSet);
  return si;
}
SymbolInfo *symbolInfoCopy(SymbolInfo const *from) {
  SymbolInfo *to = malloc(sizeof(SymbolInfo));

  to->kind = from->kind;
  switch (to->kind) {
    case SK_VAR: {
      to->data.var.type = typeCopy(from->data.var.type);
      break;
    }
    case SK_TYPE: {
      to->data.type.kind = from->data.type.kind;
      switch (to->data.type.kind) {
        case TDK_STRUCT: {
          TypeVector *toFields = &to->data.type.data.structType.fields;
          TypeVector const *fromFields =
              &from->data.type.data.structType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          TypeVector *toNames = &to->data.type.data.structType.names;
          TypeVector const *fromNames = &from->data.type.data.structType.names;
          toNames->capacity = fromNames->capacity;
          toNames->size = fromNames->size;
          toNames->elements = malloc(toNames->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toNames->size; idx++) {
            toNames->elements[idx] = typeCopy(fromNames->elements[idx]);
          }
          to->data.type.data.structType.incomplete =
              from->data.type.data.structType.incomplete;
          break;
        }
        case TDK_UNION: {
          TypeVector *toFields = &to->data.type.data.unionType.fields;
          TypeVector const *fromFields = &from->data.type.data.unionType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          StringVector *toNames = &to->data.type.data.unionType.names;
          StringVector const *fromNames = &from->data.type.data.unionType.names;
          toNames->capacity = fromNames->capacity;
          toNames->size = fromNames->size;
          toNames->elements = malloc(toNames->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toNames->size; idx++) {
            toNames->elements[idx] = typeCopy(fromNames->elements[idx]);
          }
          to->data.type.data.unionType.incomplete =
              from->data.type.data.unionType.incomplete;
          break;
        }
        case TDK_ENUM: {
          StringVector *toFields = &to->data.type.data.enumType.fields;
          StringVector const *fromFields =
              &from->data.type.data.enumType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          to->data.type.data.enumType.incomplete =
              from->data.type.data.enumType.incomplete;
          break;
        }
        case TDK_TYPEDEF: {
          to->data.type.data.typedefType.type =
              typeCopy(from->data.type.data.typedefType.type);
          break;
        }
      }
      break;
    }
    case SK_FUNCTION: {
      OverloadSet *toOverloads = &to->data.function.overloadSet;
      OverloadSet const *fromOverloads = &from->data.function.overloadSet;
      toOverloads->capacity = fromOverloads->capacity;
      toOverloads->size = fromOverloads->size;
      toOverloads->elements = malloc(toOverloads->size * sizeof(void *));
      for (size_t idx = 0; idx < toOverloads->size; idx++) {
        toOverloads->elements[idx] =
            overloadSetElementCopy(fromOverloads->elements[idx]);
      }
      break;
    }
  }

  return to;
}
char const *symbolInfoToKindString(SymbolInfo const *si) {
  switch (si->kind) {
    case SK_VAR: {
      return "a variable";
    }
    case SK_TYPE: {
      switch (si->data.type.kind) {
        case TDK_STRUCT: {
          return "a struct";
        }
        case TDK_UNION: {
          return "a union";
        }
        case TDK_ENUM: {
          return "an enum";
        }
        case TDK_TYPEDEF: {
          return "a typedef";
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid TypeDefinitionKind enum constant");
        }
      }
    }
    case SK_FUNCTION: {
      return "a function";
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid SymbolKind enum constant");
    }
  }
}
void symbolInfoDestroy(SymbolInfo *si) {
  switch (si->kind) {
    case SK_VAR: {
      typeDestroy(si->data.var.type);
      if (si->data.var.access != NULL) {
        (si->data.var.access->vtable->dtor)(si->data.var.access);
      }
      break;
    }
    case SK_TYPE: {
      switch (si->data.type.kind) {
        case TDK_STRUCT: {
          typeVectorUninit(&si->data.type.data.structType.fields);
          stringVectorUninit(&si->data.type.data.structType.names, false);
          break;
        }
        case TDK_UNION: {
          typeVectorUninit(&si->data.type.data.unionType.fields);
          stringVectorUninit(&si->data.type.data.unionType.names, false);
          break;
        }
        case TDK_ENUM: {
          stringVectorUninit(&si->data.type.data.unionType.fields, false);
          break;
        }
        case TDK_TYPEDEF: {
          typeDestroy(si->data.type.data.typedefType.type);
          break;
        }
      }
      break;
    }
    case SK_FUNCTION: {
      overloadSetUninit(&si->data.function.overloadSet);
      break;
    }
  }
  free(si);
}

SymbolTable *symbolTableCreate(void) { return hashMapCreate(); }
SymbolTable *symbolTableCopy(SymbolTable const *from) {
  SymbolTable *to = malloc(sizeof(SymbolTable));
  to->capacity = from->capacity;
  to->size = from->size;
  to->keys = calloc(to->capacity, sizeof(char const *));
  to->values = malloc(to->capacity * sizeof(void *));
  memcpy(to->keys, from->keys, to->capacity * sizeof(char const *));

  for (size_t idx = 0; idx < to->size; idx++) {
    if (to->keys[idx] != NULL) {
      to->values[idx] = symbolInfoCopy(from->values[idx]);
    }
  }

  return to;
}
SymbolInfo *symbolTableGet(SymbolTable const *table, char const *key) {
  return hashMapGet(table, key);
}
int symbolTablePut(SymbolTable *table, char const *key, SymbolInfo *value) {
  return hashMapPut(table, key, value, (void (*)(void *))symbolInfoDestroy);
}
void symbolTableDestroy(SymbolTable *table) {
  hashMapDestroy(table, (void (*)(void *))symbolInfoDestroy);
}

ModuleTableMap *moduleTableMapCreate(void) { return hashMapCreate(); }
void moduleTableMapInit(ModuleTableMap *map) { hashMapInit(map); }
SymbolTable *moduleTableMapGet(ModuleTableMap const *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleTableMapPut(ModuleTableMap *table, char const *key,
                      SymbolTable *value) {
  return hashMapPut(table, key, value, nullDtor);
}
void moduleTableMapUninit(ModuleTableMap *map) { hashMapUninit(map, nullDtor); }
void moduleTableMapDestroy(ModuleTableMap *table) {
  hashMapDestroy(table, nullDtor);
}

Environment *environmentCreate(SymbolTable *currentModule,
                               char const *currentModuleName) {
  Environment *env = malloc(sizeof(Environment));
  environmentInit(env, currentModule, currentModuleName);
  return env;
}
void environmentInit(Environment *env, SymbolTable *currentModule,
                     char const *currentModuleName) {
  env->currentModule = currentModule;
  env->currentModuleName = currentModuleName;
  moduleTableMapInit(&env->imports);
  stackInit(&env->scopes);
}
static SymbolInfo *environmentLookupInternal(Environment const *env,
                                             Report *report, char const *id,
                                             size_t line, size_t character,
                                             char const *filename,
                                             bool reportErrors) {
  if (isScoped(id)) {
    char *moduleName;
    char *shortName;
    splitName(id, &moduleName, &shortName);
    if (strcmp(moduleName, env->currentModuleName) == 0) {
      return symbolTableGet(env->currentModule, shortName);
    } else {
      for (size_t idx = 0; idx < env->imports.capacity; idx++) {
        if (strcmp(moduleName, env->imports.keys[idx]) == 0) {
          SymbolInfo *info =
              symbolTableGet(env->imports.values[idx], shortName);
          if (info != NULL) {
            free(moduleName);
            free(shortName);
            return info;
          }
        }
      }
    }

    if (isScoped(moduleName)) {
      char *enumModuleName;
      char *enumName;
      splitName(moduleName, &enumModuleName, &enumName);

      SymbolInfo *enumType = environmentLookupInternal(
          env, report, enumModuleName, line, character, filename, false);
      free(enumName);
      free(enumModuleName);

      if (enumType != NULL && enumType->kind == SK_TYPE) {
        free(moduleName);
        free(shortName);
        return enumType;
      }
    }

    free(moduleName);
    free(shortName);
    if (reportErrors) {
      reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                  filename, line, character, id);
    }
    return NULL;
  } else {
    for (size_t idx = env->scopes.size; idx-- > 0;) {
      SymbolInfo *info = symbolTableGet(env->scopes.elements[idx], id);
      if (info != NULL) return info;
    }
    SymbolInfo *info = symbolTableGet(env->currentModule, id);
    if (info != NULL) return info;

    char const *foundModule = NULL;
    for (size_t idx = 0; idx < env->imports.capacity; idx++) {
      if (env->imports.keys[idx] != NULL) {
        SymbolInfo *current = symbolTableGet(env->imports.values[idx], id);
        if (current != NULL) {
          if (info == NULL) {
            info = current;
            foundModule = env->imports.keys[idx];
          } else {
            if (reportErrors) {
              reportError(report,
                          "%s:%zu:%zu: error: identifier '%s' is ambiguous",
                          filename, line, character, id);
              reportMessage(report, "\tcandidate module: %s",
                            env->imports.keys[idx]);
              reportMessage(report, "\tcandidate module: %s", foundModule);
            }
            return NULL;
          }
        }
      }
    }
    if (info != NULL) {
      return info;
    } else {
      if (reportErrors) {
        reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                    filename, line, character, id);
      }
      return NULL;
    }
  }
}
SymbolInfo *environmentLookup(Environment const *env, Report *report,
                              char const *id, size_t line, size_t character,
                              char const *filename) {
  return environmentLookupInternal(env, report, id, line, character, filename,
                                   true);
}
SymbolTable *environmentTop(Environment const *env) {
  return env->scopes.size == 0 ? env->currentModule : stackPeek(&env->scopes);
}
void environmentPush(Environment *env) {
  stackPush(&env->scopes, symbolTableCreate());
}
SymbolTable *environmentPop(Environment *env) { return stackPop(&env->scopes); }
void environmentUninit(Environment *env) {
  moduleTableMapUninit(&env->imports);
  stackUninit(&env->scopes, (void (*)(void *))symbolTableDestroy);
}
void environmentDestroy(Environment *env) {
  environmentUninit(env);
  free(env);
}