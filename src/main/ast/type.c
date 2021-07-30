// Copyright 2019-2021 Justin Hu, Kol Crooks
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

// type implementation

#include "ast/type.h"

#include "ast/symbolTable.h"
#include "util/internalError.h"
#include "util/numericSizing.h"

static Type *typeCreate(TypeKind kind) {
  Type *t = malloc(sizeof(Type));
  t->kind = kind;
  return t;
}
Type *keywordTypeCreate(TypeKeyword keyword) {
  Type *t = typeCreate(TK_KEYWORD);
  t->data.keyword.keyword = keyword;
  return t;
}
Type *qualifiedTypeCreate(Type *base, bool constQual, bool volatileQual) {
  Type *t = typeCreate(TK_QUALIFIED);
  t->data.qualified.constQual = constQual;
  t->data.qualified.volatileQual = volatileQual;
  t->data.qualified.base = base;
  return t;
}
Type *pointerTypeCreate(Type *base) {
  Type *t = typeCreate(TK_POINTER);
  t->data.pointer.base = base;
  return t;
}
Type *arrayTypeCreate(uint64_t length, Type *type) {
  Type *t = typeCreate(TK_ARRAY);
  t->data.array.length = length;
  t->data.array.type = type;
  return t;
}
Type *funPtrTypeCreate(Type *returnType) {
  Type *t = typeCreate(TK_FUNPTR);
  t->data.funPtr.returnType = returnType;
  vectorInit(&t->data.funPtr.argTypes);
  return t;
}
Type *aggregateTypeCreate(void) {
  Type *t = typeCreate(TK_AGGREGATE);
  vectorInit(&t->data.aggregate.types);
  return t;
}
Type *referenceTypeCreate(SymbolTableEntry *entry) {
  Type *t = typeCreate(TK_REFERENCE);
  t->data.reference.entry = entry;
  return t;
}
Type *typeCopy(Type const *t) {
  if (t == NULL) return NULL;
  switch (t->kind) {
    case TK_KEYWORD: {
      return keywordTypeCreate(t->data.keyword.keyword);
    }
    case TK_QUALIFIED: {
      return qualifiedTypeCreate(typeCopy(t->data.qualified.base),
                                 t->data.qualified.constQual,
                                 t->data.qualified.volatileQual);
    }
    case TK_POINTER: {
      return pointerTypeCreate(typeCopy(t->data.pointer.base));
    }
    case TK_ARRAY: {
      return arrayTypeCreate(t->data.array.length,
                             typeCopy(t->data.array.type));
    }
    case TK_FUNPTR: {
      Type *copy = funPtrTypeCreate(typeCopy(t->data.funPtr.returnType));
      for (size_t idx = 0; idx < t->data.funPtr.argTypes.size; ++idx)
        vectorInsert(&copy->data.funPtr.argTypes,
                     typeCopy(t->data.funPtr.argTypes.elements[idx]));
      return copy;
    }
    case TK_AGGREGATE: {
      Type *copy = aggregateTypeCreate();
      for (size_t idx = 0; idx < t->data.aggregate.types.size; ++idx)
        vectorInsert(&copy->data.funPtr.argTypes,
                     typeCopy(t->data.aggregate.types.elements[idx]));
      return copy;
    }
    case TK_REFERENCE: {
      return referenceTypeCreate(t->data.reference.entry);
    }
    default: {
      error(__FILE__, __LINE__, "bad type given to typeCopy");
    }
  }
}
bool typeEqual(Type const *a, Type const *b) {
  if (a->kind != b->kind) return false;

  switch (a->kind) {
    case TK_KEYWORD: {
      return a->data.keyword.keyword == b->data.keyword.keyword;
    }
    case TK_QUALIFIED: {
      return a->data.qualified.constQual == b->data.qualified.constQual &&
             a->data.qualified.volatileQual == b->data.qualified.volatileQual &&
             typeEqual(a->data.qualified.base, b->data.qualified.base);
    }
    case TK_POINTER: {
      return typeEqual(a->data.pointer.base, b->data.pointer.base);
    }
    case TK_ARRAY: {
      return a->data.array.length == b->data.array.length &&
             typeEqual(a->data.array.type, b->data.array.type);
    }
    case TK_FUNPTR: {
      if (!typeEqual(a->data.funPtr.returnType, b->data.funPtr.returnType))
        return false;
      if (a->data.funPtr.argTypes.size != b->data.funPtr.argTypes.size)
        return false;
      for (size_t idx = 0; idx < a->data.funPtr.argTypes.size; ++idx) {
        if (!typeEqual(a->data.funPtr.argTypes.elements[idx],
                       b->data.funPtr.argTypes.elements[idx]))
          return false;
      }
      return true;
    }
    case TK_AGGREGATE: {
      if (a->data.aggregate.types.size != b->data.aggregate.types.size)
        return false;
      for (size_t idx = 0; idx < a->data.aggregate.types.size; ++idx) {
        if (!typeEqual(a->data.aggregate.types.elements[idx],
                       b->data.aggregate.types.elements[idx]))
          return false;
      }
      return true;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *aEntry = a->data.reference.entry;
      SymbolTableEntry *bEntry = b->data.reference.entry;
      return aEntry == bEntry ||
             (aEntry->kind == SK_OPAQUE &&
              aEntry->data.opaqueType.definition == bEntry) ||
             (bEntry->kind == SK_OPAQUE &&
              aEntry == bEntry->data.opaqueType.definition) ||
             (aEntry->kind == SK_OPAQUE && bEntry->kind == SK_OPAQUE &&
              aEntry->data.opaqueType.definition ==
                  bEntry->data.opaqueType.definition);
    }
    default: {
      error(__FILE__, __LINE__, "bad type given to typeEqual");
    }
  }
}
Type const *stripCV(Type const *t) {
  return t->kind == TK_QUALIFIED ? t->data.qualified.base : t;
}
/**
 * is lhs at least as CV-qualfied as RHS?
 */
static bool atLeastAsCVQualified(Type const *lhs, Type const *rhs) {
  if (rhs->kind != TK_QUALIFIED)
    return true;  // none -> anything
  else if (lhs->kind != TK_QUALIFIED)
    return false;  // !(something -> none)
  else
    return (!rhs->data.qualified.constQual || lhs->data.qualified.constQual) &&
           (!rhs->data.qualified.volatileQual ||
            lhs->data.qualified.volatileQual);
}
/**
 * implements pointer base implicit convertability @see
 * typeImplicitlyConvertable
 *
 * [2]: pointer type conversion (5.4.1.9) =
 *        at least as CV-qualified && (
 *          from is void ||
 *          to is void ||
 *          same ||
 *          (both pointers && recurse)
 *        )
 *
 * @param from base type of the converted-from pointer type
 * @param to base type of the converted-to pointer type
 * @returns whether the converted-from type can be implicitly converted to the
 * converted-to type
 */
static bool pointerBaseImplicitlyConvertable(Type const *from, Type const *to) {
  Type const *fromBase = stripCV(from);
  Type const *toBase = stripCV(to);
  return atLeastAsCVQualified(to, from) &&
         ((fromBase->kind == TK_KEYWORD &&
           fromBase->data.keyword.keyword == TK_VOID) ||
          (toBase->kind == TK_KEYWORD &&
           toBase->data.keyword.keyword == TK_VOID) ||
          typeEqual(fromBase, toBase) ||
          (fromBase->kind == TK_POINTER && toBase->kind == TK_POINTER &&
           pointerBaseImplicitlyConvertable(fromBase->data.pointer.base,
                                            toBase->data.pointer.base)));
}
bool typeImplicitlyConvertable(Type const *from, Type const *to) {
  // strip CV qualification
  from = stripCV(from);
  to = stripCV(to);
  // 2-one-of table - this implements 5.4.1
  // to \ from + kwd | ptr | arry | funPtr | aggregate | ref
  // kwd       | [1] | -----------------no------------------
  // ptr       | no  | [2] | [3]  | -----------no-----------
  // arry      | ---no---  | same | no     | [4]       | no
  // funPtr    | -------no------- | same   | ------no-------
  // ref       | -----------no------------ | [5]       | same

  if (from->kind == TK_KEYWORD && to->kind == TK_KEYWORD) {
    // [1]: keyword type conversions (5.4.1.1-6)
    // to \ from + ub | b | c | us | s | ui | i | wc | ul | l | f | d | b
    // ubyte     | y  | ------------------------no------------------------
    // byte      | no | y | ----------------------no----------------------
    // char      | --no-- | y | --------------------no--------------------
    // ushort    | y  | -no-- | y  | -----------------no------------------
    // short     | -yes-- | --no-- | y | ---------------no----------------
    // uint      | y  | -no-- | y  | n | y  | -------------no-------------
    // int       | -yes-- | n | -yes-- | n  | y | -----------no-----------
    // wchar     | --no-- | y | ------no------- | y  | --------no---------
    // ulong     | y  | -no-- | y  | n | y  | --no-- | y  | ------no------
    // long      | -yes-- | n | ------yes------ | --no--- | y | ----no----
    // float     | -yes-- | n | ------yes------ | no | ---yes---- | --no--
    // double    | -yes-- | n | ------yes------ | no | -----yes------ | n
    // bool      | ------------------------no------------------------ | y
    switch (to->data.keyword.keyword) {
      case TK_UBYTE:
        return from->data.keyword.keyword == TK_UBYTE;
      case TK_BYTE:
        return from->data.keyword.keyword == TK_BYTE;
      case TK_CHAR:
        return from->data.keyword.keyword == TK_CHAR;
      case TK_USHORT:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_USHORT;
      case TK_SHORT:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_BYTE ||
               from->data.keyword.keyword == TK_SHORT;
      case TK_UINT:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_UINT;
      case TK_INT:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_BYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_SHORT ||
               from->data.keyword.keyword == TK_INT;
      case TK_WCHAR:
        return from->data.keyword.keyword == TK_CHAR ||
               from->data.keyword.keyword == TK_WCHAR;
      case TK_ULONG:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_UINT ||
               from->data.keyword.keyword == TK_ULONG;
      case TK_LONG:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_BYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_SHORT ||
               from->data.keyword.keyword == TK_UINT ||
               from->data.keyword.keyword == TK_INT ||
               from->data.keyword.keyword == TK_LONG;
      case TK_FLOAT:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_BYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_SHORT ||
               from->data.keyword.keyword == TK_UINT ||
               from->data.keyword.keyword == TK_INT ||
               from->data.keyword.keyword == TK_ULONG ||
               from->data.keyword.keyword == TK_LONG ||
               from->data.keyword.keyword == TK_FLOAT;
      case TK_DOUBLE:
        return from->data.keyword.keyword == TK_UBYTE ||
               from->data.keyword.keyword == TK_BYTE ||
               from->data.keyword.keyword == TK_USHORT ||
               from->data.keyword.keyword == TK_SHORT ||
               from->data.keyword.keyword == TK_UINT ||
               from->data.keyword.keyword == TK_INT ||
               from->data.keyword.keyword == TK_ULONG ||
               from->data.keyword.keyword == TK_LONG ||
               from->data.keyword.keyword == TK_FLOAT ||
               from->data.keyword.keyword == TK_DOUBLE;
      case TK_BOOL:
        return from->data.keyword.keyword == TK_BOOL;
      default:
        error(__FILE__, __LINE__, "invalid keyword type encountered");
    }
  } else if (from->kind == TK_POINTER && to->kind == TK_POINTER) {
    return pointerBaseImplicitlyConvertable(from->data.pointer.base,
                                            to->data.pointer.base);
  } else if (from->kind == TK_ARRAY && to->kind == TK_POINTER) {
    // [3]: array to pointer decay (5.4.1.10) =
    //          at least as CV-qualified && (
    //            same ||
    //            pointer is void
    //          )
    Type const *toBase = stripCV(to->data.pointer.base);
    Type const *fromBase = stripCV(from->data.array.type);
    return atLeastAsCVQualified(to->data.pointer.base, from->data.array.type) &&
           (typeEqual(fromBase, toBase) ||
            (toBase->kind == TK_KEYWORD &&
             toBase->data.keyword.keyword == TK_VOID));
  } else if (from->kind == TK_AGGREGATE && to->kind == TK_ARRAY) {
    // [4]: aggregate initialization of arrays (5.4.1.8) =
    //          aggregate.length == array.length &&
    //          aggregate's types can implicit convert to array elements
    if (from->data.aggregate.types.size != to->data.array.length) return false;
    for (size_t idx = 0; idx < from->data.aggregate.types.size; ++idx) {
      if (!typeImplicitlyConvertable(from->data.aggregate.types.elements[idx],
                                     to->data.array.type))
        return false;
    }
    return true;
  } else if (from->kind == TK_AGGREGATE && to->kind == TK_REFERENCE) {
    // [5]: aggregate initialization of struct (5.4.1.7) =
    //          ref is a struct &&
    //          aggregate.length = struct.length
    //          aggregate's types can implicit convert to struct fields
    SymbolTableEntry *entry = to->data.reference.entry;
    if (entry->kind != SK_STRUCT || from->data.aggregate.types.size !=
                                        entry->data.structType.fieldTypes.size)
      return false;
    for (size_t idx = 0; idx < from->data.aggregate.types.size; ++idx) {
      if (!typeImplicitlyConvertable(
              from->data.aggregate.types.elements[idx],
              entry->data.structType.fieldTypes.elements[idx]))
        return false;
    }
    return true;
  } else {
    return typeEqual(from, to);
  }
}
bool typeExplicitlyConvertable(Type const *from, Type const *to) {
  from = stripCV(from);
  to = stripCV(to);

  if (typeImplicitlyConvertable(from, to)) {
    return true;
  } else if ((typeNumeric(from) || typeCharacter(from)) &&
             (typeNumeric(to) || typeCharacter(to))) {
    return true;
  } else if ((typeIntegral(from) || typeAnyPointer(from)) &&
             (typeIntegral(to) || typeAnyPointer(to))) {
    return true;
  } else if ((to->kind == TK_REFERENCE &&
              to->data.reference.entry->kind == SK_TYPEDEF &&
              typeEqual(to->data.reference.entry->data.typedefType.actual,
                        from)) ||
             (from->kind == TK_REFERENCE &&
              from->data.reference.entry->kind == SK_TYPEDEF &&
              typeEqual(from->data.reference.entry->data.typedefType.actual,
                        to))) {
    return true;
  } else if ((typeBoolean(from) && typeNumeric(to)) ||
             (typeNumeric(from) && typeBoolean(to))) {
    return true;
  } else if ((typeNumeric(from) && typeEnum(to)) ||
             (typeEnum(from) && typeNumeric(to))) {
    return true;
  } else {
    return false;
  }
}
bool typeSignedIntegral(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_KEYWORD && (t->data.keyword.keyword == TK_BYTE ||
                                   t->data.keyword.keyword == TK_SHORT ||
                                   t->data.keyword.keyword == TK_INT ||
                                   t->data.keyword.keyword == TK_LONG);
}
bool typeUnsignedIntegral(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_KEYWORD && (t->data.keyword.keyword == TK_UBYTE ||
                                   t->data.keyword.keyword == TK_USHORT ||
                                   t->data.keyword.keyword == TK_UINT ||
                                   t->data.keyword.keyword == TK_ULONG);
}
bool typeIntegral(Type const *t) {
  return typeSignedIntegral(t) || typeUnsignedIntegral(t);
}
bool typeFloating(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_KEYWORD && (t->data.keyword.keyword == TK_FLOAT ||
                                   t->data.keyword.keyword == TK_DOUBLE);
}
bool typeNumeric(Type const *t) { return typeIntegral(t) || typeFloating(t); }
bool typeCharacter(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_KEYWORD && (t->data.keyword.keyword == TK_CHAR ||
                                   t->data.keyword.keyword == TK_WCHAR);
}
bool typeBoolean(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_KEYWORD && t->data.keyword.keyword == TK_BOOL;
}
bool typePointer(Type const *t) { return stripCV(t)->kind == TK_POINTER; }
bool typeAnyPointer(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_POINTER || t->kind == TK_FUNPTR;
}
bool typeEnum(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_REFERENCE && t->data.reference.entry->kind == SK_ENUM;
}
bool typeArray(Type const *t) {
  t = stripCV(t);
  return t->kind == TK_ARRAY;
}
bool typeSwitchable(Type const *t) {
  return typeIntegral(t) || typeCharacter(t) || typeEnum(t);
}
Type *arithmeticTypeMerge(Type const *a, Type const *b) {
  if (a == NULL || b == NULL || !typeNumeric(a) || !typeNumeric(b)) return NULL;
  a = stripCV(a);
  b = stripCV(b);

  if (a->data.keyword.keyword == TK_DOUBLE ||
      b->data.keyword.keyword == TK_DOUBLE) {
    return keywordTypeCreate(TK_DOUBLE);
  } else if (a->data.keyword.keyword == TK_FLOAT ||
             b->data.keyword.keyword == TK_FLOAT) {
    return keywordTypeCreate(TK_FLOAT);
  } else {
    // 2-one-of table for type keyword merging
    //    A\B | ub | b | us | s | ui | i | ul | l
    // ubyte  | A  | s | -----------B------------
    // byte   | s  | A | i  | B | l  | B | no | B
    // ushort | A  | i | A  | i | -------B-------
    // short  | --A--- | i  | A | l  | B | no | B
    // uint   | A  | l | A  | l | A  | l | B  | l
    // int    | -------A------- | l  | A | no | l
    // ulong  | A  | n | A  | n | A  | n | A  | n
    // long   | -----------A------------ | no | A
    switch (a->data.keyword.keyword) {
      case TK_UBYTE: {
        switch (b->data.keyword.keyword) {
          case TK_UBYTE: {
            return typeCopy(a);
          }
          case TK_BYTE: {
            return keywordTypeCreate(TK_SHORT);
          }
          default: {
            return typeCopy(b);
          }
        }
      }
      case TK_BYTE: {
        switch (b->data.keyword.keyword) {
          case TK_UBYTE: {
            return keywordTypeCreate(TK_SHORT);
          }
          case TK_BYTE: {
            return typeCopy(a);
          }
          case TK_USHORT: {
            return keywordTypeCreate(TK_INT);
          }
          case TK_UINT: {
            return keywordTypeCreate(TK_LONG);
          }
          case TK_ULONG: {
            return NULL;
          }
          default: {
            return typeCopy(b);
          }
        }
      }
      case TK_USHORT: {
        switch (b->data.keyword.keyword) {
          case TK_UBYTE:
          case TK_USHORT: {
            return typeCopy(a);
          }
          case TK_BYTE:
          case TK_SHORT: {
            return keywordTypeCreate(TK_INT);
          }
          default: {
            return typeCopy(b);
          }
        }
      }
      case TK_SHORT: {
        switch (b->data.keyword.keyword) {
          case TK_UBYTE:
          case TK_BYTE:
          case TK_SHORT: {
            return typeCopy(a);
          }
          case TK_USHORT: {
            return keywordTypeCreate(TK_INT);
          }
          case TK_UINT: {
            return keywordTypeCreate(TK_LONG);
          }
          case TK_ULONG: {
            return NULL;
          }
          default: {
            return typeCopy(b);
          }
        }
      }
      case TK_UINT: {
        switch (b->data.keyword.keyword) {
          case TK_UBYTE:
          case TK_USHORT:
          case TK_UINT: {
            return typeCopy(a);
          }
          case TK_ULONG: {
            return NULL;
          }
          default: {
            return keywordTypeCreate(TK_LONG);
          }
        }
      }
      case TK_INT: {
        switch (b->data.keyword.keyword) {
          case TK_UINT:
          case TK_LONG: {
            return keywordTypeCreate(TK_LONG);
          }
          case TK_ULONG: {
            return NULL;
          }
          default: {
            return typeCopy(a);
          }
        }
      }
      case TK_ULONG: {
        if (typeUnsignedIntegral(b)) {
          return typeCopy(a);
        } else {
          return NULL;
        }
      }
      case TK_LONG: {
        if (b->data.keyword.keyword != TK_ULONG) {
          return typeCopy(a);
        } else {
          return NULL;
        }
      }
      default: {
        error(__FILE__, __LINE__, "non-numeric type encountered");
      }
    }
  }
}
static Type *ternaryPointerBaseMerge(Type const *a, Type const *b) {
  if (a->kind == TK_QUALIFIED || b->kind == TK_QUALIFIED) {
    return qualifiedTypeCreate(
        ternaryPointerBaseMerge(stripCV(a), stripCV(b)),
        (a->kind == TK_QUALIFIED && a->data.qualified.constQual) ||
            (b->kind == TK_QUALIFIED && b->data.qualified.constQual),
        (a->kind == TK_QUALIFIED && a->data.qualified.volatileQual) ||
            (b->kind == TK_QUALIFIED && b->data.qualified.volatileQual));
  } else if (typeEqual(a, b)) {
    return typeCopy(a);
  } else {
    return keywordTypeCreate(TK_VOID);
  }
}
Type *ternaryTypeMerge(Type const *a, Type const *b) {
  if (a == NULL || b == NULL) return NULL;

  if (a->kind == TK_QUALIFIED || b->kind == TK_QUALIFIED) {
    return qualifiedTypeCreate(
        ternaryTypeMerge(stripCV(a), stripCV(b)),
        (a->kind == TK_QUALIFIED && a->data.qualified.constQual) ||
            (b->kind == TK_QUALIFIED && b->data.qualified.constQual),
        (a->kind == TK_QUALIFIED && a->data.qualified.volatileQual) ||
            (b->kind == TK_QUALIFIED && b->data.qualified.volatileQual));
  } else if (typeEqual(a, b)) {
    return typeCopy(a);
  } else if (typeNumeric(a) && typeNumeric(b)) {
    return arithmeticTypeMerge(a, b);
  } else if (typeCharacter(a) && typeCharacter(b)) {
    return keywordTypeCreate(TK_WCHAR);
  } else if (a->kind == TK_POINTER && b->kind == TK_POINTER) {
    return pointerTypeCreate(
        ternaryPointerBaseMerge(a->data.pointer.base, b->data.pointer.base));
  } else {
    return NULL;
  }
}
Type *comparisonTypeMerge(Type const *a, Type const *b) {
  a = stripCV(a);
  b = stripCV(b);

  if ((typeNumeric(a) || typeCharacter(a)) &&
      (typeNumeric(b) || typeCharacter(b))) {
    return ternaryTypeMerge(a, b);
  } else if (typeBoolean(a) && typeBoolean(b)) {
    return typeCopy(a);
  } else if (typeEnum(a) && typeEnum(b)) {
    return typeCopy(a);
  } else if (typePointer(a) && typePointer(b) &&
             (typeImplicitlyConvertable(a, b) ||
              typeImplicitlyConvertable(b, a))) {
    return pointerTypeCreate(keywordTypeCreate(TK_VOID));
  } else {
    return NULL;
  }
}

size_t typeSizeof(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_VOID:  // for void pointer arithmetic
        case TK_UBYTE:
        case TK_BYTE:
        case TK_CHAR:
        case TK_BOOL: {
          return 1;
        }
        case TK_USHORT:
        case TK_SHORT: {
          return 2;
        }
        case TK_UINT:
        case TK_INT:
        case TK_WCHAR:
        case TK_FLOAT: {
          return 4;
        }
        case TK_ULONG:
        case TK_LONG:
        case TK_DOUBLE: {
          return 8;
        }
        default: {
          error(__FILE__, __LINE__, "invalid keyword type");
        }
      }
    }
    case TK_QUALIFIED: {
      return typeSizeof(t->data.qualified.base);
    }
    case TK_POINTER:
    case TK_FUNPTR: {
      return POINTER_WIDTH;
    }
    case TK_ARRAY: {
      return typeSizeof(t->data.array.type) * t->data.array.length;
    }
    case TK_AGGREGATE: {
      size_t size = 0;
      for (size_t idx = 0; idx < t->data.aggregate.types.size; ++idx) {
        size += typeSizeof(t->data.aggregate.types.elements[idx]);
        if (idx < t->data.aggregate.types.size - 1) {
          size = incrementToMultiple(
              size, typeAlignof(t->data.aggregate.types.elements[idx + 1]));
        } else {
          size = incrementToMultiple(size, typeAlignof(t));
        }
      }
      return size;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *entry = t->data.reference.entry;
      switch (entry->kind) {
        case SK_STRUCT: {
          size_t size = 0;
          for (size_t idx = 0; idx < entry->data.structType.fieldTypes.size;
               ++idx) {
            size += typeSizeof(entry->data.structType.fieldTypes.elements[idx]);
            if (idx < entry->data.structType.fieldTypes.size - 1) {
              size = incrementToMultiple(
                  size,
                  typeAlignof(
                      entry->data.structType.fieldTypes.elements[idx + 1]));
            } else {
              size = incrementToMultiple(size, typeAlignof(t));
            }
          }
          return size;
        }
        case SK_UNION: {
          size_t size = 0;
          for (size_t idx = 0; idx < entry->data.unionType.optionTypes.size;
               ++idx) {
            size_t compare =
                typeSizeof(entry->data.unionType.optionTypes.elements[idx]);
            if (compare > size) size = compare;
          }
          return size;
        }
        case SK_ENUM: {
          return typeSizeof(entry->data.enumType.backingType);
        }
        case SK_TYPEDEF: {
          return typeSizeof(entry->data.typedefType.actual);
        }
        default: {
          error(__FILE__, __LINE__, "can't take the size of an unsized symbol");
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "invalid type kind");
    }
  }
}
size_t structOffsetof(struct SymbolTableEntry const *e, char const *field) {
  size_t offset = 0;
  for (size_t idx = 0;
       strcmp(e->data.structType.fieldNames.elements[idx], field) != 0; ++idx) {
    offset += typeSizeof(e->data.structType.fieldTypes.elements[idx]);
    offset = incrementToMultiple(
        offset, typeAlignof(e->data.structType.fieldTypes.elements[idx + 1]));
  }
}
size_t typeAlignof(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD:
    case TK_POINTER:
    case TK_FUNPTR: {
      return typeSizeof(t);
    }
    case TK_QUALIFIED: {
      return typeAlignof(t->data.qualified.base);
    }
    case TK_ARRAY: {
      return typeAlignof(t->data.array.type);
    }
    case TK_AGGREGATE: {
      size_t alignment = 0;
      for (size_t idx = 0; idx < t->data.aggregate.types.size; ++idx) {
        size_t compare = typeAlignof(t->data.aggregate.types.elements[idx]);
        if (compare > alignment) alignment = compare;
      }
      return alignment;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *entry = t->data.reference.entry;
      switch (entry->kind) {
        case SK_STRUCT: {
          size_t alignment = 0;
          for (size_t idx = 0; idx < entry->data.structType.fieldTypes.size;
               ++idx) {
            size_t compare =
                typeAlignof(entry->data.structType.fieldTypes.elements[idx]);
            if (compare > alignment) alignment = compare;
          }
          return alignment;
        }
        case SK_UNION: {
          size_t alignment = 0;
          for (size_t idx = 0; idx < entry->data.unionType.optionTypes.size;
               ++idx) {
            size_t compare =
                typeAlignof(entry->data.unionType.optionTypes.elements[idx]);
            if (compare > alignment) alignment = compare;
          }
          return alignment;
        }
        case SK_ENUM: {
          return typeAlignof(entry->data.enumType.backingType);
        }
        case SK_TYPEDEF: {
          return typeAlignof(entry->data.typedefType.actual);
        }
        default: {
          error(__FILE__, __LINE__,
                "can't take the alignment of an unsized symbol");
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "invalid type kind");
    }
  }
}
bool typeComplete(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      return t->data.keyword.keyword != TK_VOID;
    }
    case TK_QUALIFIED: {
      return typeComplete(t->data.qualified.base);
    }
    case TK_POINTER:
    case TK_FUNPTR: {
      return true;
    }
    case TK_ARRAY: {
      return t->data.array.length != 0 && typeComplete(t->data.array.type);
    }
    case TK_REFERENCE: {
      return t->data.reference.entry->kind != SK_OPAQUE;
    }
    default: {
      error(__FILE__, __LINE__, "can't have a symbol of that type anyways");
    }
  }
}
/**
 * does the given type directly reference the entry
 * i.e. sizeof(type) is related to sizeof(e)
 */
static bool typeDirectlyReferences(Type const *t, SymbolTableEntry const *e) {
  switch (t->kind) {
    case TK_KEYWORD:
    case TK_POINTER:
    case TK_FUNPTR: {
      return false;
    }
    case TK_QUALIFIED: {
      return typeDirectlyReferences(t->data.qualified.base, e);
    }
    case TK_ARRAY: {
      return typeDirectlyReferences(t->data.array.type, e);
    }
    case TK_REFERENCE: {
      return t->data.reference.entry == e;
    }
    default: {
      error(__FILE__, __LINE__, "can't have a symbol of that type anyways");
    }
  }
}
bool structRecursive(SymbolTableEntry const *e) {
  for (size_t idx = 0; idx < e->data.structType.fieldTypes.size; ++idx) {
    Type const *t = e->data.structType.fieldTypes.elements[idx];
    if (typeDirectlyReferences(t, e)) return true;
  }
  return false;
}
bool unionRecursive(SymbolTableEntry const *e) {
  for (size_t idx = 0; idx < e->data.unionType.optionTypes.size; ++idx) {
    Type const *t = e->data.unionType.optionTypes.elements[idx];
    if (typeDirectlyReferences(t, e)) return true;
  }
  return false;
}
bool typedefRecursive(SymbolTableEntry const *e) {
  return typeDirectlyReferences(e->data.typedefType.actual, e);
}
AllocHint typeAllocation(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      return t->data.keyword.keyword == TK_FLOAT ||
                     t->data.keyword.keyword == TK_DOUBLE
                 ? AH_FP
                 : AH_GP;
    }
    case TK_QUALIFIED: {
      return typeAllocation(t->data.qualified.base);
    }
    case TK_POINTER:
    case TK_FUNPTR: {
      return AH_GP;
    }
    case TK_ARRAY:
    case TK_AGGREGATE: {
      return AH_MEM;
    }
    case TK_REFERENCE: {
      SymbolTableEntry const *entry = t->data.reference.entry;
      switch (entry->kind) {
        case SK_STRUCT:
        case SK_UNION: {
          return AH_MEM;
        }
        case SK_ENUM: {
          return AH_GP;
        }
        case SK_TYPEDEF: {
          return typeAllocation(entry->data.typedefType.actual);
        }
        default: {
          error(__FILE__, __LINE__, "invalid symbol kind");
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "invalid type kind");
    }
  }
}
char *typeVectorToString(Vector const *v) {
  if (v->size == 0) {
    return strdup("");
  } else {
    char *base = typeToString(v->elements[0]);
    for (size_t idx = 1; idx < v->size; ++idx) {
      char *tmp = base;
      char *add = typeToString(v->elements[idx]);
      base = format("%s, %s", tmp, add);
      free(tmp);
      free(add);
    }
    return base;
  }
}
char *typeToString(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_VOID: {
          return strdup("void");
        }
        case TK_UBYTE: {
          return strdup("ubyte");
        }
        case TK_BYTE: {
          return strdup("byte");
        }
        case TK_CHAR: {
          return strdup("char");
        }
        case TK_USHORT: {
          return strdup("ushort");
        }
        case TK_SHORT: {
          return strdup("short");
        }
        case TK_UINT: {
          return strdup("uint");
        }
        case TK_INT: {
          return strdup("int");
        }
        case TK_WCHAR: {
          return strdup("wchar");
        }
        case TK_ULONG: {
          return strdup("ulong");
        }
        case TK_LONG: {
          return strdup("long");
        }
        case TK_FLOAT: {
          return strdup("float");
        }
        case TK_DOUBLE: {
          return strdup("double");
        }
        case TK_BOOL: {
          return strdup("bool");
        }
        default: {
          error(__FILE__, __LINE__, "invalid type keyword enum given");
        }
      }
    }
    case TK_QUALIFIED: {
      char *base = typeToString(t->data.qualified.base);
      char *retval;
      if (t->data.qualified.constQual && t->data.qualified.volatileQual)
        retval = format("%s volatile const", base);
      else if (t->data.qualified.constQual)
        retval = format("%s const", base);
      else  // at least one of const, volatile must be true
        retval = format("%s volatile", base);
      free(base);
      return retval;
    }
    case TK_POINTER: {
      char *base = typeToString(t->data.pointer.base);
      char *retval;
      if (base[strlen(base) - 1] == '*')
        retval = format("%s*", base);
      else
        retval = format("%s *", base);
      free(base);
      return retval;
    }
    case TK_ARRAY: {
      char *base = typeToString(t->data.array.type);
      char *retval = format("%s[%lu]", base, t->data.array.length);
      free(base);
      return retval;
    }
    case TK_FUNPTR: {
      char *returnType = typeToString(t->data.funPtr.returnType);
      char *args = typeVectorToString(&t->data.funPtr.argTypes);
      char *retval = format("%s(%s)", returnType, args);
      free(returnType);
      free(args);
      return retval;
    }
    case TK_AGGREGATE: {
      char *elms = typeVectorToString(&t->data.aggregate.types);
      char *retval = format("{%s}", elms);
      free(elms);
      return retval;
    }
    case TK_REFERENCE: {
      return strdup(t->data.reference.entry->id);
    }
    default: {
      error(__FILE__, __LINE__, "invalid typekind enum encountered");
    }
  }
}
void typeFree(Type *t) {
  if (t == NULL) return;

  switch (t->kind) {
    case TK_QUALIFIED: {
      typeFree(t->data.qualified.base);
      break;
    }
    case TK_POINTER: {
      typeFree(t->data.pointer.base);
      break;
    }
    case TK_ARRAY: {
      typeFree(t->data.array.type);
      break;
    }
    case TK_FUNPTR: {
      vectorUninit(&t->data.funPtr.argTypes, (void (*)(void *))typeFree);
      typeFree(t->data.funPtr.returnType);
      break;
    }
    case TK_AGGREGATE: {
      vectorUninit(&t->data.aggregate.types, (void (*)(void *))typeFree);
      break;
    }
    default: {
      break;  // nothing to do
    }
  }
  free(t);
}

void typeVectorFree(Vector *v) {
  vectorUninit(v, (void (*)(void *))typeFree);
  free(v);
}
