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
Type *referenceTypeCreate(SymbolTableEntry *entry, char *id) {
  Type *t = typeCreate(TK_REFERENCE);
  t->data.reference.entry = entry;
  t->data.reference.id = id;
  return t;
}
Type *typeCopy(Type const *t) {
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
      return referenceTypeCreate(t->data.reference.entry,
                                 strdup(t->data.reference.id));
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
static Type const *stripCV(Type const *t) {
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
               from->data.keyword.keyword == TK_UINT;
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
    //          same ||
    //          at least as CV-qualfied && pointer is void
    Type const *toBase = stripCV(to->data.pointer.base);
    return typeEqual(from->data.array.type, to->data.pointer.base) ||
           (atLeastAsCVQualified(to->data.pointer.base,
                                 from->data.array.type) &&
            toBase->kind == TK_KEYWORD &&
            toBase->data.keyword.keyword == TK_VOID);
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
      return strdup(t->data.reference.id);
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
    case TK_REFERENCE: {
      free(t->data.reference.id);
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
