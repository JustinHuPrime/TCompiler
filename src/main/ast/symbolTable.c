// Copyright 2019-2021 Justin Hu
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
#include <string.h>

#include "fileList.h"
#include "internalError.h"
#include "util/format.h"
#include "util/functional.h"

void stabFree(HashMap *stab) {
  if (stab != NULL) {
    hashMapUninit(stab, (void (*)(void *))stabEntryFree);
    free(stab);
  }
}

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
Type *modifiedTypeCreate(TypeModifier modifier, Type *modified) {
  Type *t = typeCreate(TK_MODIFIED);
  t->data.modified.modifier = modifier;
  t->data.modified.modified = modified;
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
    case TK_MODIFIED: {
      return modifiedTypeCreate(t->data.modified.modifier,
                                typeCopy(t->data.modified.modified));
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
    case TK_MODIFIED: {
      return a->data.modified.modifier == b->data.modified.modifier &&
             typeEqual(a->data.modified.modified, b->data.modified.modified);
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
    case TK_MODIFIED: {
      char *base = typeToString(t->data.modified.modified);
      char *retval;
      switch (t->data.modified.modifier) {
        case TM_CONST: {
          retval = format("%s const", base);
          break;
        }
        case TM_VOLATILE: {
          retval = format("%s volatile", base);
          break;
        }
        case TM_POINTER: {
          if (base[strlen(base) - 1] == '*')
            retval = format("%s*", base);
          else
            retval = format("%s *", base);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid type modifier enum given");
        }
      }
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
    case TK_MODIFIED: {
      typeFree(t->data.modified.modified);
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

static char const *const SYMBOL_KIND_NAMES[] = {
    "a variable",     "a function",
    "an opaque type", "a structure type",
    "a union type",   "an enumeration type",
    "a type alias",   "an enumeration constant",
};
char const *symbolKindToString(SymbolKind kind) {
  return SYMBOL_KIND_NAMES[kind];
}

/**
 * initializes a symbol table entry
 */
static SymbolTableEntry *stabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character, SymbolKind kind) {
  SymbolTableEntry *e = malloc(sizeof(SymbolTableEntry));
  e->kind = kind;
  e->file = file;
  e->line = line;
  e->character = character;
  return e;
}

SymbolTableEntry *opaqueStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_OPAQUE);
  e->data.opaqueType.definition = NULL;
  return e;
}
SymbolTableEntry *structStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_STRUCT);
  vectorInit(&e->data.structType.fieldNames);
  vectorInit(&e->data.structType.fieldTypes);
  return e;
}
SymbolTableEntry *unionStabEntryCreate(FileListEntry *file, size_t line,
                                       size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_UNION);
  vectorInit(&e->data.unionType.optionNames);
  vectorInit(&e->data.unionType.optionTypes);
  return e;
}
SymbolTableEntry *enumStabEntryCreate(FileListEntry *file, size_t line,
                                      size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_ENUM);
  vectorInit(&e->data.enumType.constantNames);
  vectorInit(&e->data.enumType.constantValues);
  return e;
}
SymbolTableEntry *enumConstStabEntryCreate(FileListEntry *file, size_t line,
                                           size_t character,
                                           SymbolTableEntry *parent) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_ENUMCONST);
  e->data.enumConst.parent = parent;
  return e;
}
SymbolTableEntry *typedefStabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_TYPEDEF);
  e->data.typedefType.actual = NULL;
  return e;
}
SymbolTableEntry *variableStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_VARIABLE);
  e->data.variable.type = NULL;
  return e;
}
SymbolTableEntry *functionStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, SK_FUNCTION);
  e->data.function.returnType = NULL;
  vectorInit(&e->data.function.argumentTypes);
  return e;
}

Type *structLookupField(SymbolTableEntry *structEntry, char const *field) {
  for (size_t idx = 0; idx < structEntry->data.structType.fieldNames.size;
       ++idx) {
    if (strcmp(structEntry->data.structType.fieldNames.elements[idx], field) ==
        0)
      return structEntry->data.structType.fieldTypes.elements[idx];
  }
  return NULL;
}
Type *unionLookupOption(SymbolTableEntry *unionEntry, char const *option) {
  for (size_t idx = 0; idx < unionEntry->data.unionType.optionNames.size;
       ++idx) {
    if (strcmp(unionEntry->data.unionType.optionNames.elements[idx], option) ==
        0)
      return unionEntry->data.unionType.optionTypes.elements[idx];
  }
  return NULL;
}
SymbolTableEntry *enumLookupEnumConst(SymbolTableEntry *enumEntry,
                                      char const *name) {
  for (size_t idx = 0; idx < enumEntry->data.enumType.constantNames.size;
       ++idx) {
    if (strcmp(enumEntry->data.enumType.constantNames.elements[idx], name) == 0)
      return enumEntry->data.enumType.constantValues.elements[idx];
  }
  return NULL;
}

void stabEntryFree(SymbolTableEntry *e) {
  switch (e->kind) {
    case SK_STRUCT: {
      vectorUninit(&e->data.structType.fieldNames, nullDtor);
      vectorUninit(&e->data.structType.fieldTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_UNION: {
      vectorUninit(&e->data.unionType.optionNames, nullDtor);
      vectorUninit(&e->data.unionType.optionTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_ENUM: {
      vectorUninit(&e->data.enumType.constantNames, nullDtor);
      vectorUninit(&e->data.enumType.constantValues,
                   (void (*)(void *))stabEntryFree);
      break;
    }
    case SK_TYPEDEF: {
      if (e->data.typedefType.actual != NULL)
        typeFree(e->data.typedefType.actual);
      break;
    }
    case SK_VARIABLE: {
      if (e->data.variable.type != NULL) typeFree(e->data.variable.type);
      break;
    }
    case SK_FUNCTION: {
      if (e->data.function.returnType != NULL)
        typeFree(e->data.function.returnType);
      vectorUninit(&e->data.function.argumentTypes, (void (*)(void *))typeFree);
      break;
    }
    default: {
      break;
    }
  }
  free(e);
}