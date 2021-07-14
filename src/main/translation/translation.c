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

#include "translation/translation.h"

#include "arch/interface.h"
#include "fileList.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "util/conversions.h"
#include "util/internalError.h"
#include "util/numericSizing.h"
#include "util/string.h"

size_t fresh(FileListEntry *file) { return file->nextId++; }

/**
 * form a name from a prefix and an id
 */
static char *suffixName(char const *prefix, char const *id) {
  return format("%s%zu%s", prefix, strlen(id), id);
}
/**
 * generate the name prefix of a module id
 */
static char *generatePrefix(Node *id) {
  char *suffix;

  if (id->type == NT_ID) {
    suffix = format("%zu%s", strlen(id->data.id.id), id->data.id.id);
  } else {
    suffix = strdup("");
    Vector *components = id->data.scopedId.components;
    for (size_t idx = 0; idx < components->size; ++idx) {
      Node *component = components->elements[idx];
      char *old = suffix;
      suffix = suffixName(old, component->data.id.id);
      free(old);
    }
  }

  char *retval = format("_T%s", suffix);
  free(suffix);
  return retval;
}
char *getMangledName(SymbolTableEntry *entry) {
  char *prefix =
      generatePrefix(entry->file->ast->data.file.module->data.module.id);
  char *retval = suffixName(prefix, entry->id);
  free(prefix);
  return retval;
}

typedef struct {
  union {
    uint64_t unsignedVal;
    int64_t signedVal;
  } value;
  size_t label;
} JumpTableEntry;
static int compareUnsignedJumpTableEntry(JumpTableEntry const *a,
                                         JumpTableEntry const *b) {
  if (a->value.unsignedVal < b->value.unsignedVal)
    return -1;
  else if (a->value.unsignedVal > b->value.unsignedVal)
    return 1;
  else
    return 0;
}
static int compareSignedJumpTableEntry(JumpTableEntry const *a,
                                       JumpTableEntry const *b) {
  if (a->value.signedVal < b->value.signedVal)
    return -1;
  else if (a->value.signedVal > b->value.signedVal)
    return 1;
  else
    return 0;
}
static IROperand *signedJumpTableEntryToConstant(JumpTableEntry const *e,
                                                 size_t size) {
  switch (size) {
    case 1: {
      return CONSTANT(size,
                      byteDatumCreate(s8ToU8((int8_t)e->value.signedVal)));
    }
    case 2: {
      return CONSTANT(size,
                      shortDatumCreate(s16ToU16((int16_t)e->value.signedVal)));
    }
    case 4: {
      return CONSTANT(size,
                      intDatumCreate(s32ToU32((int32_t)e->value.signedVal)));
    }
    case 8: {
      return CONSTANT(size, longDatumCreate(s64ToU64(e->value.signedVal)));
    }
    default: {
      error(__FILE__, __LINE__, "can't switch on a type of that size");
    }
  }
}
static IROperand *unsignedJumpTableEntryToConstant(JumpTableEntry const *e,
                                                   size_t size) {
  switch (size) {
    case 1: {
      return CONSTANT(size, byteDatumCreate((uint8_t)e->value.unsignedVal));
    }
    case 2: {
      return CONSTANT(size, shortDatumCreate((uint16_t)e->value.unsignedVal));
    }
    case 4: {
      return CONSTANT(size, intDatumCreate((uint32_t)e->value.unsignedVal));
    }
    case 8: {
      return CONSTANT(size, longDatumCreate(e->value.unsignedVal));
    }
    default: {
      error(__FILE__, __LINE__, "can't switch on a type of that size");
    }
  }
}

/**
 * get the type of an expression
 */
static Type const *expressionTypeof(Node const *e) {
  switch (e->type) {
    case NT_BINOPEXP: {
      return e->data.binOpExp.type;
    }
    case NT_TERNARYEXP: {
      return e->data.ternaryExp.type;
    }
    case NT_UNOPEXP: {
      return e->data.unOpExp.type;
    }
    case NT_FUNCALLEXP: {
      return e->data.funCallExp.type;
    }
    case NT_LITERAL: {
      return e->data.literal.type;
    }
    case NT_SCOPEDID: {
      return e->data.literal.type;
    }
    case NT_ID: {
      return e->data.id.type;
    }
    default: {
      error(__FILE__, __LINE__, "invalid expression type");
    }
  }
}

/**
 * produce true if given initializer results in all-zeroes
 */
static bool initializerAllZero(Node const *initializer) {
  switch (initializer->type) {
    case NT_SCOPEDID: {
      // enum literal
      SymbolTableEntry *entry = initializer->data.scopedId.entry;
      return entry->data.enumConst.signedness
                 ? entry->data.enumConst.data.signedValue == 0
                 : entry->data.enumConst.data.unsignedValue == 0;
    }
    case NT_LITERAL: {
      // other literal
      switch (initializer->data.literal.literalType) {
        case LT_UBYTE: {
          return initializer->data.literal.data.ubyteVal == 0;
        }
        case LT_BYTE: {
          return initializer->data.literal.data.byteVal == 0;
        }
        case LT_USHORT: {
          return initializer->data.literal.data.ushortVal == 0;
        }
        case LT_SHORT: {
          return initializer->data.literal.data.shortVal == 0;
        }
        case LT_UINT: {
          return initializer->data.literal.data.uintVal == 0;
        }
        case LT_INT: {
          return initializer->data.literal.data.intVal == 0;
        }
        case LT_ULONG: {
          return initializer->data.literal.data.ulongVal == 0;
        }
        case LT_LONG: {
          return initializer->data.literal.data.longVal == 0;
        }
        case LT_FLOAT: {
          return initializer->data.literal.data.floatBits == 0;
        }
        case LT_DOUBLE: {
          return initializer->data.literal.data.doubleBits == 0;
        }
        case LT_CHAR: {
          return initializer->data.literal.data.charVal == 0;
        }
        case LT_WCHAR: {
          return initializer->data.literal.data.wcharVal == 0;
        }
        case LT_STRING:
        case LT_WSTRING: {
          return false;
        }
        case LT_BOOL: {
          return initializer->data.literal.data.boolVal == false;
        }
        case LT_NULL: {
          return true;
        }
        case LT_AGGREGATEINIT: {
          Vector *aggregateInitVal =
              initializer->data.literal.data.aggregateInitVal;
          for (size_t idx = 0; idx < aggregateInitVal->size; ++idx) {
            if (!initializerAllZero(aggregateInitVal->elements[idx]))
              return false;
          }
          return true;
        }
        default: {
          error(__FILE__, __LINE__, "invalid literalType literal given");
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "non-literal node given");
    }
  }
}

/**
 * translate an initializer into IRDatums, given a type
 *
 * @param data vector to insert IRDatums into
 * @param irFrags vector to insert new IRFrags into (for translation of strings)
 * @param type type of initialized thing
 * @param initializer initializer to translate
 */
static void translateInitializer(Vector *data, Vector *irFrags,
                                 Type const *type, Node const *initializer,
                                 FileListEntry *file) {
  switch (type->kind) {
    case TK_KEYWORD: {
      switch (type->data.keyword.keyword) {
        case TK_UBYTE: {
          vectorInsert(
              data, byteDatumCreate(initializer->data.literal.data.ubyteVal));
          break;
        }
        case TK_BYTE: {
          vectorInsert(
              data,
              byteDatumCreate(s8ToU8(initializer->data.literal.data.byteVal)));
          break;
        }
        case TK_CHAR: {
          vectorInsert(data,
                       byteDatumCreate(initializer->data.literal.data.charVal));
          break;
        }
        case TK_USHORT: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, shortDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data, shortDatumCreate(
                                     initializer->data.literal.data.ushortVal));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_SHORT: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, shortDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_BYTE: {
              vectorInsert(data, shortDatumCreate(s16ToU16(
                                     initializer->data.literal.data.byteVal)));
              break;
            }
            case LT_SHORT: {
              vectorInsert(data, shortDatumCreate(s16ToU16(
                                     initializer->data.literal.data.shortVal)));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_UINT: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.ushortVal));
              break;
            }
            case LT_UINT: {
              vectorInsert(
                  data, intDatumCreate(initializer->data.literal.data.uintVal));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_INT: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_BYTE: {
              vectorInsert(data, intDatumCreate(s32ToU32(
                                     initializer->data.literal.data.byteVal)));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.ushortVal));
              break;
            }
            case LT_SHORT: {
              vectorInsert(data, intDatumCreate(s32ToU32(
                                     initializer->data.literal.data.shortVal)));
              break;
            }
            case LT_INT: {
              vectorInsert(data, intDatumCreate(s32ToU32(
                                     initializer->data.literal.data.intVal)));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_WCHAR: {
          switch (initializer->data.literal.literalType) {
            case LT_CHAR: {
              vectorInsert(
                  data, intDatumCreate(initializer->data.literal.data.charVal));
              break;
            }
            case LT_WCHAR: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.wcharVal));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_ULONG: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.ushortVal));
              break;
            }
            case LT_UINT: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.uintVal));
              break;
            }
            case LT_ULONG: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.ulongVal));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_LONG: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.ubyteVal));
              break;
            }
            case LT_BYTE: {
              vectorInsert(data, longDatumCreate(s64ToU64(
                                     initializer->data.literal.data.byteVal)));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.ushortVal));
              break;
            }
            case LT_SHORT: {
              vectorInsert(data, longDatumCreate(s64ToU64(
                                     initializer->data.literal.data.shortVal)));
              break;
            }
            case LT_UINT: {
              vectorInsert(data, longDatumCreate(
                                     initializer->data.literal.data.uintVal));
              break;
            }
            case LT_INT: {
              vectorInsert(data, longDatumCreate(s64ToU64(
                                     initializer->data.literal.data.intVal)));
              break;
            }
            case LT_LONG: {
              vectorInsert(data, longDatumCreate(s64ToU64(
                                     initializer->data.literal.data.longVal)));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_FLOAT: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, intDatumCreate(uintToFloatBits(
                                     initializer->data.literal.data.ubyteVal)));
              break;
            }
            case LT_BYTE: {
              vectorInsert(data, intDatumCreate(intToFloatBits(
                                     initializer->data.literal.data.byteVal)));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data,
                           intDatumCreate(uintToFloatBits(
                               initializer->data.literal.data.ushortVal)));
              break;
            }
            case LT_SHORT: {
              vectorInsert(data, intDatumCreate(intToFloatBits(
                                     initializer->data.literal.data.shortVal)));
              break;
            }
            case LT_UINT: {
              vectorInsert(data, intDatumCreate(uintToFloatBits(
                                     initializer->data.literal.data.uintVal)));
              break;
            }
            case LT_INT: {
              vectorInsert(data, intDatumCreate(intToFloatBits(
                                     initializer->data.literal.data.intVal)));
              break;
            }
            case LT_ULONG: {
              vectorInsert(data, intDatumCreate(uintToFloatBits(
                                     initializer->data.literal.data.ulongVal)));
              break;
            }
            case LT_LONG: {
              vectorInsert(data, intDatumCreate(intToFloatBits(
                                     initializer->data.literal.data.longVal)));
              break;
            }
            case LT_FLOAT: {
              vectorInsert(data, intDatumCreate(
                                     initializer->data.literal.data.floatBits));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_DOUBLE: {
          switch (initializer->data.literal.literalType) {
            case LT_UBYTE: {
              vectorInsert(data, longDatumCreate(uintToDoubleBits(
                                     initializer->data.literal.data.ubyteVal)));
              break;
            }
            case LT_BYTE: {
              vectorInsert(data, longDatumCreate(intToDoubleBits(
                                     initializer->data.literal.data.byteVal)));
              break;
            }
            case LT_USHORT: {
              vectorInsert(data,
                           longDatumCreate(uintToDoubleBits(
                               initializer->data.literal.data.ushortVal)));
              break;
            }
            case LT_SHORT: {
              vectorInsert(data, longDatumCreate(intToDoubleBits(
                                     initializer->data.literal.data.shortVal)));
              break;
            }
            case LT_UINT: {
              vectorInsert(data, longDatumCreate(uintToDoubleBits(
                                     initializer->data.literal.data.uintVal)));
              break;
            }
            case LT_INT: {
              vectorInsert(data, longDatumCreate(intToDoubleBits(
                                     initializer->data.literal.data.intVal)));
              break;
            }
            case LT_ULONG: {
              vectorInsert(data, longDatumCreate(uintToDoubleBits(
                                     initializer->data.literal.data.ulongVal)));
              break;
            }
            case LT_LONG: {
              vectorInsert(data, longDatumCreate(intToDoubleBits(
                                     initializer->data.literal.data.longVal)));
              break;
            }
            case LT_FLOAT: {
              vectorInsert(data,
                           longDatumCreate(floatBitsToDoubleBits(
                               initializer->data.literal.data.floatBits)));
              break;
            }
            case LT_DOUBLE: {
              vectorInsert(
                  data,
                  longDatumCreate(initializer->data.literal.data.doubleBits));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid initializer type");
            }
          }
          break;
        }
        case TK_BOOL: {
          if (initializer->data.literal.data.boolVal)
            vectorInsert(data, byteDatumCreate(1));
          else
            vectorInsert(data, byteDatumCreate(0));
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid keyword type being initialized");
        }
      }
      break;
    }
    case TK_QUALIFIED: {
      translateInitializer(data, irFrags, type->data.qualified.base,
                           initializer, file);
      break;
    }
    case TK_POINTER: {
      // note - null pointers handled as bss blocks
      if (initializer->data.literal.literalType == LT_STRING) {
        size_t label = fresh(file);
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df = dataFragCreate(
            FT_RODATA, format(localLabelFormat(), label), CHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     stringDatumCreate(
                         tstrdup(initializer->data.literal.data.stringVal)));
        vectorInsert(irFrags, df);
      } else {
        size_t label = fresh(file);
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df = dataFragCreate(
            FT_RODATA, format(localLabelFormat(), label), WCHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     wstringDatumCreate(
                         twstrdup(initializer->data.literal.data.wstringVal)));
        vectorInsert(irFrags, df);
      }
      break;
    }
    case TK_ARRAY: {
      Vector *initializers = initializer->data.literal.data.aggregateInitVal;
      for (size_t idx = 0; idx < initializers->size; ++idx)
        translateInitializer(data, irFrags, type->data.array.type,
                             initializers->elements[idx], file);
      break;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *entry = type->data.reference.entry;
      switch (entry->kind) {
        case SK_ENUM: {
          SymbolTableEntry *constEntry = initializer->data.scopedId.entry;
          switch (constEntry->data.enumConst.parent->data.enumType.backingType
                      ->data.keyword.keyword) {
            case TK_UBYTE: {
              vectorInsert(
                  data,
                  byteDatumCreate(
                      (uint8_t)constEntry->data.enumConst.data.unsignedValue));
              break;
            }
            case TK_BYTE: {
              vectorInsert(
                  data,
                  byteDatumCreate(s8ToU8(
                      (int8_t)constEntry->data.enumConst.data.signedValue)));
              break;
            }
            case TK_USHORT: {
              vectorInsert(
                  data,
                  shortDatumCreate(
                      (uint16_t)constEntry->data.enumConst.data.unsignedValue));
              break;
            }
            case TK_SHORT: {
              vectorInsert(
                  data,
                  shortDatumCreate(s16ToU16(
                      (int16_t)constEntry->data.enumConst.data.signedValue)));
              break;
            }
            case TK_UINT: {
              vectorInsert(
                  data,
                  intDatumCreate(
                      (uint32_t)constEntry->data.enumConst.data.unsignedValue));
              break;
            }
            case TK_INT: {
              vectorInsert(
                  data,
                  intDatumCreate(s32ToU32(
                      (int32_t)constEntry->data.enumConst.data.signedValue)));
              break;
            }
            case TK_ULONG: {
              vectorInsert(
                  data,
                  longDatumCreate(
                      (uint64_t)constEntry->data.enumConst.data.unsignedValue));
              break;
            }
            case TK_LONG: {
              vectorInsert(
                  data,
                  longDatumCreate(s64ToU64(
                      (int64_t)constEntry->data.enumConst.data.signedValue)));
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid enum backing type");
            }
          }
          break;
        }
        case SK_STRUCT: {
          size_t pos = 0;
          for (size_t idx = 0; idx < entry->data.structType.fieldTypes.size;
               ++idx) {
            translateInitializer(
                data, irFrags, entry->data.structType.fieldTypes.elements[idx],
                initializer->data.literal.data.aggregateInitVal->elements[idx],
                file);
            pos += typeSizeof(entry->data.structType.fieldTypes.elements[idx]);
            size_t padded;
            if (idx < entry->data.structType.fieldTypes.size - 1) {
              padded = incrementToMultiple(
                  pos,
                  typeAlignof(
                      entry->data.structType.fieldTypes.elements[idx + 1]));
            } else {
              padded = incrementToMultiple(pos, typeAlignof(type));
            }
            if (padded != pos)
              vectorInsert(data, paddingDatumCreate(padded - pos));
            pos = padded;
          }
          break;
        }
        default: {
          error(__FILE__, __LINE__,
                "attempted to initialize uninitializeable reference type");
        }
      }
      break;
    }
    default: {
      error(__FILE__, __LINE__, "type with no literals being initialized");
    }
  }
}

/**
 * translate a constant into a fragment
 */
static void translateLiteral(Node const *name, Node const *initializer,
                             char const *namePrefix, Vector *irFrags,
                             FileListEntry *file) {
  Type const *type = name->data.id.entry->data.variable.type;
  IRFrag *df;
  if (initializer == NULL || initializerAllZero(initializer)) {
    df = dataFragCreate(FT_BSS, suffixName(namePrefix, name->data.id.id),
                        typeAlignof(type));
    vectorInsert(&df->data.data.data, paddingDatumCreate(typeSizeof(type)));
  } else {
    df = dataFragCreate(
        type->kind == TK_QUALIFIED && type->data.qualified.constQual ? FT_RODATA
                                                                     : FT_DATA,
        suffixName(namePrefix, name->data.id.id), typeAlignof(type));
    translateInitializer(&df->data.data.data, irFrags, type, initializer, file);
  }
  vectorInsert(irFrags, df);
}

/**
 * translate a cast
 *
 * @param b block to add cast to
 * @param src source temp (owning)
 * @param fromType type of the input temp
 * @param toType type of the output temp
 * @param label this block's label
 * @param nextLabel next block's label
 * @param file file the expression is in
 * @returns temp with produced value
 */
static IROperand *translateCast(IRBlock *b, IROperand *src,
                                Type const *fromType, Type const *toType,
                                FileListEntry *file) {
  return NULL;  // TODO
}

/**
 * translate a multiplication by a constant size for pointer arithmetic
 *
 * @param b block to add to
 * @param target target value
 * @param targetType type of target value
 * @param pointedSize size to multiply by
 * @returns owning temp containing result
 */
static IROperand *translatePointerArithmeticScale(IRBlock *b, IROperand *target,
                                                  Type const *targetType,
                                                  size_t pointedSize) {
  return NULL;  // TODO
}

/**
 * translate an increment
 *
 * @param b block to add to
 * @param target target temp
 * @param targetType type of target
 * @returns owning temp containing result
 */
static IROperand *translateIncrement(IRBlock *b, IROperand *target,
                                     Type const *targetType) {
  return NULL;  // TODO
}
/**
 * translate a decrement
 *
 * @param b block to add to
 * @param target target temp
 * @param targetType type of target
 * @returns owning temp containing result
 */
static IROperand *translateDecrement(IRBlock *b, IROperand *target,
                                     Type const *targetType) {
  return NULL;  // TODO
}
/**
 * translate a negation
 *
 * @param b block to add to
 * @param target target temp
 * @param targetType type of target
 * @returns owning temp containing result
 */
static IROperand *translateNegation(IRBlock *b, IROperand *target,
                                    Type const *targetType) {
  return NULL;  // TODO
}
/**
 * translate a logical not
 *
 * @param b block to add to
 * @param target target temp
 * @param targetType type of target
 * @returns owning temp containing result
 */
static IROperand *translateLnot(IRBlock *b, IROperand *target,
                                Type const *targetType) {
  return NULL;  // TODO
}
/**
 * translate a bitwise not
 *
 * @param b block to add to
 * @param target target temp
 * @param targetType type of target
 * @returns owning temp containing result
 */
static IROperand *translateBitNot(IRBlock *b, IROperand *target,
                                  Type const *targetType) {
  return NULL;  // TODO
}
/**
 * table of translation functions for UnOpType
 */
IROperand *(*const UNOP_TRANSLATORS[])(IRBlock *, IROperand *, Type const *) = {
    NULL,
    NULL,
    translateIncrement,
    translateDecrement,
    translateNegation,
    translateLnot,
    translateBitNot,
    translateIncrement,
    translateDecrement,
    translateNegation,
    translateLnot,
    translateBitNot,
    NULL,
    NULL,
    NULL,
};

/**
 * translate a multiplication
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateMultiplication(IRBlock *b, IROperand *lhs,
                                          Type const *lhsType, IROperand *rhs,
                                          Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a division
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateDivision(IRBlock *b, IROperand *lhs,
                                    Type const *lhsType, IROperand *rhs,
                                    Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a modulo
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateModulo(IRBlock *b, IROperand *lhs,
                                  Type const *lhsType, IROperand *rhs,
                                  Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate an addition
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateAddition(IRBlock *b, IROperand *lhs,
                                    Type const *lhsType, IROperand *rhs,
                                    Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a subtraction
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateSubtraction(IRBlock *b, IROperand *lhs,
                                       Type const *lhsType, IROperand *rhs,
                                       Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a left shift
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateLShift(IRBlock *b, IROperand *lhs,
                                  Type const *lhsType, IROperand *rhs,
                                  Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate an arithmetic right shift
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateARShift(IRBlock *b, IROperand *lhs,
                                   Type const *lhsType, IROperand *rhs,
                                   Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a logical right shift
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateLRShift(IRBlock *b, IROperand *lhs,
                                   Type const *lhsType, IROperand *rhs,
                                   Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a bitwise and
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateBitAnd(IRBlock *b, IROperand *lhs,
                                  Type const *lhsType, IROperand *rhs,
                                  Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a bitwise xor
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateBitXor(IRBlock *b, IROperand *lhs,
                                  Type const *lhsType, IROperand *rhs,
                                  Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a bitwise or
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateBitOr(IRBlock *b, IROperand *lhs,
                                 Type const *lhsType, IROperand *rhs,
                                 Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * determine the IROperator for a boolean comparison
 */
static IROperator binopToComparison(BinOpType binop, bool floating,
                                    bool signedInt) {
  switch (binop) {
    case BO_EQ: {
      return floating ? IO_FE : IO_E;
    }
    case BO_NEQ: {
      return floating ? IO_FNE : IO_NE;
    }
    case BO_LT: {
      return floating ? IO_FL : signedInt ? IO_L : IO_B;
    }
    case BO_LTEQ: {
      return floating ? IO_FLE : signedInt ? IO_LE : IO_BE;
    }
    case BO_GT: {
      return floating ? IO_FG : signedInt ? IO_G : IO_A;
    }
    case BO_GTEQ: {
      return floating ? IO_FGE : signedInt ? IO_GE : IO_AE;
    }
    default: {
      error(__FILE__, __LINE__, "invalid comparison binop");
    }
  }
}
/**
 * translate an equality comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateEq(IRBlock *b, IROperand *lhs, Type const *lhsType,
                              IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate an inequality comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateNeq(IRBlock *b, IROperand *lhs, Type const *lhsType,
                               IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a less than comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateLt(IRBlock *b, IROperand *lhs, Type const *lhsType,
                              IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a greater than comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateGt(IRBlock *b, IROperand *lhs, Type const *lhsType,
                              IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a less than or equal to comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateLtEq(IRBlock *b, IROperand *lhs, Type const *lhsType,
                                IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a greater than or equal to comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateGtEq(IRBlock *b, IROperand *lhs, Type const *lhsType,
                                IROperand *rhs, Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * translate a three-way comparison
 *
 * @param b block to add to
 * @param lhs lhs temp
 * @param lhsType type of lhs
 * @param rhs rhs temp
 * @param rhsType type of rhs
 * @returns owning temp containing result
 */
static IROperand *translateSpaceship(IRBlock *b, IROperand *lhs,
                                     Type const *lhsType, IROperand *rhs,
                                     Type const *rhsType) {
  return NULL;  // TODO
}
/**
 * table of translation functions for BinOpType
 */
IROperand *(*const BINOP_TRANSLATORS[])(IRBlock *, IROperand *, Type const *,
                                        IROperand *, Type const *) = {
    NULL,
    NULL,
    translateMultiplication,
    translateDivision,
    translateModulo,
    translateAddition,
    translateSubtraction,
    translateLShift,
    translateARShift,
    translateLRShift,
    translateBitAnd,
    translateBitXor,
    translateBitOr,
    NULL,
    NULL,
    NULL,
    NULL,
    translateBitAnd,
    translateBitXor,
    translateBitOr,
    translateEq,
    translateNeq,
    translateLt,
    translateGt,
    translateLtEq,
    translateGtEq,
    translateSpaceship,
    translateLShift,
    translateARShift,
    translateLRShift,
    translateAddition,
    translateSubtraction,
    translateMultiplication,
    translateDivision,
    translateModulo,
    NULL,
    NULL,
    NULL,
    NULL,
};

typedef enum {
  LK_TEMP,
  LK_MEM,
} LValueKind;
/**
 * an lvalue
 */
typedef struct {
  LValueKind kind;
  IROperand *operand; /**< either a temp to directly store something in, a
                         global address, or an address in a temp */
  int64_t
      staticOffset; /**< static offset to add to the dynamic offset, if any */
  IROperand *dynamicOffset; /**< nullable */
} LValue;
/**
 * ctor
 */
static LValue *lvalueCreate(LValueKind kind, IROperand *operand,
                            int64_t staticOffset, IROperand *dynamicOffset) {
  LValue *lvalue = malloc(sizeof(LValue));
  lvalue->kind = kind;
  lvalue->operand = operand;
  lvalue->staticOffset = staticOffset;
  lvalue->dynamicOffset = dynamicOffset;
  return lvalue;
}
/**
 * translate getting the offset of an lvalue
 *
 * @param b block to add translation to
 * @param lvalue lvalue
 * @param file file this block is in
 * @returns offset as an operand
 */
static IROperand *getLValueOffset(IRBlock *b, LValue const *lvalue,
                                  FileListEntry *file) {
  if (lvalue->dynamicOffset == NULL) {
    return OFFSET(lvalue->staticOffset);
  } else if (lvalue->staticOffset == 0) {
    return irOperandCopy(lvalue->dynamicOffset);
  } else {
    IROperand *offset = TEMPPTR(fresh(file));
    IR(b, BINOP(POINTER_WIDTH, IO_ADD, irOperandCopy(offset),
                irOperandCopy(lvalue->dynamicOffset),
                OFFSET(lvalue->staticOffset)));
    return offset;
  }
}
/**
 * translate a load from an lvalue
 *
 * @param b block to add load to
 * @param src source lvalue
 * @param dest destination temp (borrowed)
 * @param file file this block is in
 * @returns destination temp
 */
static IROperand *translateLValueLoad(IRBlock *b, LValue const *src,
                                      IROperand *dest, FileListEntry *file) {
  switch (src->kind) {
    case LK_TEMP: {
      if (src->staticOffset == 0 && src->dynamicOffset == NULL) {
        IR(b, MOVE(dest->data.temp.size, irOperandCopy(dest),
                   irOperandCopy(src->operand)));
      } else {
        IR(b, OFFSET_LOAD(dest->data.temp.size, irOperandCopy(dest),
                          irOperandCopy(src->operand),
                          getLValueOffset(b, src, file)));
      }
      return dest;
    }
    case LK_MEM: {
      IR(b,
         MEM_LOAD(dest->data.temp.size, irOperandCopy(dest),
                  irOperandCopy(src->operand), getLValueOffset(b, src, file)));
      return dest;
    }
    default: {
      error(__FILE__, __LINE__, "invalid lvalue kind");
    }
  }
}
/**
 * translate a store to an lvalue
 *
 * @param b block to add store to
 * @param dest destination lvalue
 * @param src source temp (owning)
 * @param file file this block is in
 */
static void translateLValueStore(IRBlock *b, LValue const *dest, IROperand *src,
                                 FileListEntry *file) {
  switch (dest->kind) {
    case LK_TEMP: {
      if (dest->staticOffset == 0 && dest->dynamicOffset == NULL) {
        IR(b, MOVE(src->data.temp.size, irOperandCopy(dest->operand),
                   irOperandCopy(src)));
      } else {
        IR(b, OFFSET_STORE(src->data.temp.size, irOperandCopy(dest->operand),
                           irOperandCopy(src), getLValueOffset(b, dest, file)));
      }
      break;
    }
    case LK_MEM: {
      IR(b, MEM_STORE(src->data.temp.size, irOperandCopy(dest->operand),
                      irOperandCopy(src), getLValueOffset(b, dest, file)));
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid lvalue kind");
    }
  }
}
static void lvalueFree(LValue *lv) {
  irOperandFree(lv->operand);
  free(lv);
}

static void translateExpressionVoid(Vector *, Node const *, size_t, size_t,
                                    FileListEntry *);
static IROperand *translateExpressionValue(Vector *, Node const *, size_t,
                                           size_t, FileListEntry *);
/**
 * translate an l-value expression
 *
 * @param block vector to put new blocks in
 * @param e expression to translate
 * @param label this block's label
 * @param nextLabel label the next block is at
 * @param file file the expression is in
 * @returns an l-value
 */
static LValue *translateExpressionLValue(Vector *blocks, Node const *e,
                                         size_t label, size_t nextLabel,
                                         FileListEntry *file) {
  switch (e->type) {
    case NT_BINOPEXP: {
      Node const *lhs = e->data.binOpExp.lhs;
      Node const *rhs = e->data.binOpExp.rhs;
      switch (e->data.binOpExp.op) {
        case BO_SEQ: {
          size_t rhsLabel = fresh(file);
          translateExpressionVoid(blocks, lhs, label, rhsLabel, file);
          return translateExpressionLValue(blocks, rhs, rhsLabel, nextLabel,
                                           file);
        }
        case BO_ASSIGN: {
          size_t rhsLabel = fresh(file);
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
          size_t assignmentLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          IRBlock *b = BLOCK(assignmentLabel, blocks);
          IROperand *castRhs = translateCast(b, rawRhs, expressionTypeof(rhs),
                                             expressionTypeof(lhs), file);
          translateLValueStore(b, lvalue, castRhs, file);
          IR(b, JUMP(nextLabel));
          return lvalue;
        }
        case BO_MULASSIGN:
        case BO_DIVASSIGN:
        case BO_MODASSIGN:
        case BO_ADDASSIGN:
        case BO_SUBASSIGN:
        case BO_LSHIFTASSIGN:
        case BO_ARSHIFTASSIGN:
        case BO_LRSHIFTASSIGN:
        case BO_BITANDASSIGN:
        case BO_BITXORASSIGN:
        case BO_BITORASSIGN: {
          size_t rhsLabel = fresh(file);
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
          size_t assignmentLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          IRBlock *b = BLOCK(assignmentLabel, blocks);
          IROperand *rawLhs = translateLValueLoad(
              b, lvalue, TEMPOF(fresh(file), expressionTypeof(lhs)), file);
          IROperand *rawResult = BINOP_TRANSLATORS[e->data.binOpExp.op](
              b, rawLhs, expressionTypeof(lhs), rawRhs, expressionTypeof(rhs));
          Type *merged =
              arithmeticTypeMerge(expressionTypeof(lhs), expressionTypeof(rhs));
          IROperand *castResult =
              translateCast(b, rawResult, merged, expressionTypeof(lhs), file);
          typeFree(merged);
          translateLValueStore(b, lvalue, castResult, file);
          IR(b, JUMP(nextLabel));
          return lvalue;
        }
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          size_t shortCircuitLabel = fresh(file);
          LValue *lvalue = translateExpressionLValue(blocks, lhs, label,
                                                     shortCircuitLabel, file);
          size_t rhsLabel = fresh(file);
          IRBlock *b = BLOCK(shortCircuitLabel, blocks);
          IROperand *lhsVal =
              translateLValueLoad(b, lvalue, TEMPBOOL(fresh(file)), file);
          IR(b, BJUMP(BOOL_WIDTH,
                      e->data.binOpExp.op == BO_LANDASSIGN ? IO_JZ : IO_JNZ,
                      nextLabel, lhsVal));
          IR(b, JUMP(rhsLabel));
          size_t assignmentLabel = fresh(file);
          IROperand *rhsVal = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          b = BLOCK(assignmentLabel, blocks);
          translateLValueStore(b, lvalue, rhsVal, file);
          IR(b, JUMP(nextLabel));
          return lvalue;
        }
        case BO_FIELD: {
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, nextLabel, file);
          SymbolTableEntry const *lhsEntry =
              expressionTypeof(lhs)->data.reference.entry;
          if (lhsEntry->kind == SK_STRUCT) {
            lvalue->staticOffset +=
                (int64_t)structOffsetof(lhsEntry, rhs->data.id.id);
          }
          return lvalue;
        }
        case BO_PTRFIELD: {
          SymbolTableEntry const *lhsEntry =
              expressionTypeof(lhs)->data.reference.entry;
          return lvalueCreate(
              LK_MEM,
              translateExpressionValue(blocks, lhs, label, nextLabel, file),
              lhsEntry->kind == SK_STRUCT
                  ? (int64_t)structOffsetof(lhsEntry, rhs->data.id.id)
                  : 0,
              NULL);
        }
        case BO_ARRAY: {
          if (typePointer(expressionTypeof(lhs))) {
            size_t rhsLabel = fresh(file);
            IROperand *lhsVal =
                translateExpressionValue(blocks, lhs, label, rhsLabel, file);
            size_t offsetLabel = fresh(file);
            IROperand *unscaledIndex = translateExpressionValue(
                blocks, rhs, rhsLabel, offsetLabel, file);
            IRBlock *b = BLOCK(offsetLabel, blocks);
            return lvalueCreate(
                LK_MEM, lhsVal, 0,
                translatePointerArithmeticScale(
                    b, unscaledIndex, expressionTypeof(rhs),
                    typeSizeof(expressionTypeof(lhs)->data.pointer.base)));
          } else {
            size_t rhsLabel = fresh(file);
            LValue *lvalue =
                translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
            size_t offsetLabel = fresh(file);
            IROperand *unscaledIndex = translateExpressionValue(
                blocks, rhs, rhsLabel, offsetLabel, file);
            IRBlock *b = BLOCK(offsetLabel, blocks);
            if (lvalue->dynamicOffset == NULL) {
              lvalue->dynamicOffset = translatePointerArithmeticScale(
                  b, unscaledIndex, expressionTypeof(rhs),
                  typeSizeof(expressionTypeof(lhs)->data.array.type));
            } else {
              IROperand *newOffset = TEMPPTR(fresh(file));
              IR(b,
                 BINOP(
                     POINTER_WIDTH, IO_ADD, irOperandCopy(newOffset),
                     lvalue->dynamicOffset,
                     translatePointerArithmeticScale(
                         b, unscaledIndex, expressionTypeof(rhs),
                         typeSizeof(expressionTypeof(lhs)->data.array.type))));
              lvalue->dynamicOffset = newOffset;
            }
            IR(b, JUMP(nextLabel));
            return lvalue;
          }
        }
        default: {
          error(__FILE__, __LINE__, "invalid lvalue binop");
        }
      }
    }
    case NT_UNOPEXP: {
      Node const *target = e->data.unOpExp.target;
      switch (e->data.unOpExp.op) {
        case UO_DEREF: {
          return lvalueCreate(
              LK_MEM,
              translateExpressionValue(blocks, target, label, nextLabel, file),
              0, NULL);
        }
        case UO_PREINC:
        case UO_PREDEC: {
          size_t modifyLabel = fresh(file);
          LValue *lvalue = translateExpressionLValue(blocks, target, label,
                                                     modifyLabel, file);
          IRBlock *b = BLOCK(modifyLabel, blocks);
          IROperand *value = translateLValueLoad(
              b, lvalue, TEMPOF(fresh(file), expressionTypeof(target)), file);
          IROperand *modified =
              (e->data.unOpExp.op == UO_PREINC
                   ? translateIncrement
                   : translateDecrement)(b, value, expressionTypeof(target));
          translateLValueStore(b, lvalue, modified, file);
          return lvalue;
        }
        case UO_PARENS: {
          return translateExpressionLValue(blocks, target, label, nextLabel,
                                           file);
        }
        default: {
          error(__FILE__, __LINE__, "invalid lvalue unop");
        }
      }
    }
    case NT_SCOPEDID: {
      if (e->data.scopedId.entry->data.variable.temp == 0) {
        return lvalueCreate(
            LK_MEM, GLOBAL(getMangledName(e->data.scopedId.entry)), 0, NULL);
      } else {
        return lvalueCreate(LK_TEMP,
                            TEMPOF(e->data.scopedId.entry->data.variable.temp,
                                   e->data.scopedId.type),
                            0, NULL);
      }
    }
    case NT_ID: {
      if (e->data.id.entry->data.variable.temp == 0) {
        return lvalueCreate(LK_MEM, GLOBAL(getMangledName(e->data.id.entry)), 0,
                            NULL);
      } else {
        return lvalueCreate(
            LK_TEMP,
            TEMPOF(e->data.id.entry->data.variable.temp, e->data.id.type), 0,
            NULL);
      }
    }
    default: {
      error(__FILE__, __LINE__, "invalid lvalue expression");
    }
  }
}

/**
 * determine the IROperator for a conditional jump
 */
static IROperator binopToCjump(BinOpType binop, bool floating, bool signedInt) {
  switch (binop) {
    case BO_EQ: {
      return floating ? IO_JFE : IO_JE;
    }
    case BO_NEQ: {
      return floating ? IO_JFNE : IO_JNE;
    }
    case BO_LT: {
      return floating ? IO_JFL : signedInt ? IO_JL : IO_JB;
    }
    case BO_LTEQ: {
      return floating ? IO_JFLE : signedInt ? IO_JLE : IO_JBE;
    }
    case BO_GT: {
      return floating ? IO_JFG : signedInt ? IO_JG : IO_JA;
    }
    case BO_GTEQ: {
      return floating ? IO_JFGE : signedInt ? IO_JGE : IO_JAE;
    }
    default: {
      error(__FILE__, __LINE__, "invalid comparison binop");
    }
  }
}
/**
 * translate a boolean variable predicate
 */
static void translateVariablePredicate(Vector *blocks, Node const *e,
                                       size_t label, size_t trueLabel,
                                       size_t falseLabel, FileListEntry *file) {
  size_t comparisonLabel = fresh(file);
  IRBlock *b = BLOCK(comparisonLabel, blocks);
  IR(b,
     BJUMP(typeSizeof(expressionTypeof(e)), IO_JNZ, trueLabel,
           translateExpressionValue(blocks, e, label, comparisonLabel, file)));
  IR(b, JUMP(falseLabel));
}
/**
 * translate a conditional jump predicate
 *
 * @param block vector to put new blocks in
 * @param e expression to translate
 * @param label this block's label
 * @param trueLabel label to go to if true
 * @param falseLabel label to go to if false
 * @param file file the expression is in
 */
static void translateExpressionPredicate(Vector *blocks, Node const *e,
                                         size_t label, size_t trueLabel,
                                         size_t falseLabel,
                                         FileListEntry *file) {
  switch (e->type) {
    case NT_BINOPEXP: {
      Node const *lhs = e->data.binOpExp.lhs;
      Node const *rhs = e->data.binOpExp.rhs;
      switch (e->data.binOpExp.op) {
        case BO_SEQ: {
          size_t rhsLabel = fresh(file);
          translateExpressionVoid(blocks, lhs, label, rhsLabel, file);
          translateExpressionPredicate(blocks, rhs, rhsLabel, trueLabel,
                                       falseLabel, file);
          break;
        }
        case BO_ASSIGN: {
          size_t rhsLabel = fresh(file);
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
          size_t assignmentLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          IRBlock *b = BLOCK(assignmentLabel, blocks);
          IROperand *castRhs = translateCast(b, rawRhs, expressionTypeof(rhs),
                                             expressionTypeof(lhs), file);
          translateLValueStore(b, lvalue, castRhs, file);
          IR(b, BJUMP(BOOL_WIDTH, IO_JNZ, trueLabel, castRhs));
          IR(b, JUMP(falseLabel));
          lvalueFree(lvalue);
          break;
        }
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          size_t shortCircuitLabel = fresh(file);
          LValue *lvalue = translateExpressionLValue(blocks, lhs, label,
                                                     shortCircuitLabel, file);
          size_t rhsLabel = fresh(file);
          IRBlock *b = BLOCK(shortCircuitLabel, blocks);
          IROperand *lhsVal =
              translateLValueLoad(b, lvalue, TEMPBOOL(fresh(file)), file);
          if (e->data.binOpExp.op == BO_LANDASSIGN)
            IR(b, BJUMP(BOOL_WIDTH, IO_JZ, falseLabel, lhsVal));
          else
            IR(b, BJUMP(BOOL_WIDTH, IO_JNZ, trueLabel, lhsVal));
          IR(b, JUMP(rhsLabel));
          size_t assignmentLabel = fresh(file);
          IROperand *rhsVal = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          b = BLOCK(assignmentLabel, blocks);
          translateLValueStore(b, lvalue, rhsVal, file);
          IR(b, BJUMP(BOOL_WIDTH, IO_JNZ, trueLabel, rhsVal));
          IR(b, JUMP(falseLabel));
          lvalueFree(lvalue);
          break;
        }
        case BO_LAND:
        case BO_LOR: {
          size_t rhsLabel = fresh(file);
          if (e->data.binOpExp.op == BO_LAND)
            translateExpressionPredicate(blocks, lhs, label, rhsLabel,
                                         falseLabel, file);
          else
            translateExpressionPredicate(blocks, lhs, label, trueLabel,
                                         rhsLabel, file);
          translateExpressionPredicate(blocks, rhs, rhsLabel, trueLabel,
                                       falseLabel, file);
          break;
        }
        case BO_EQ:
        case BO_NEQ:
        case BO_LT:
        case BO_GT:
        case BO_LTEQ:
        case BO_GTEQ: {
          size_t rhsLabel = fresh(file);
          IROperand *rawLhs =
              translateExpressionValue(blocks, lhs, label, rhsLabel, file);
          size_t compareLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       compareLabel, file);
          IRBlock *b = BLOCK(compareLabel, blocks);
          Type *merged =
              comparisonTypeMerge(expressionTypeof(lhs), expressionTypeof(rhs));
          IROperand *castedLhs =
              translateCast(b, rawLhs, expressionTypeof(lhs), merged, file);
          IROperand *castedRhs =
              translateCast(b, rawRhs, expressionTypeof(rhs), merged, file);
          IR(b, CJUMP(typeSizeof(merged),
                      binopToCjump(e->data.binOpExp.op, typeFloating(merged),
                                   typeSignedIntegral(merged)),
                      trueLabel, castedLhs, castedRhs));
          IR(b, JUMP(falseLabel));
          typeFree(merged);
          break;
        }
        case BO_FIELD:
        case BO_PTRFIELD:
        case BO_ARRAY:
        case BO_CAST: {
          translateVariablePredicate(blocks, e, label, trueLabel, falseLabel,
                                     file);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid binop");
        }
      }
      break;
    }
    case NT_UNOPEXP: {
      Node const *target = e->data.unOpExp.target;
      switch (e->data.unOpExp.op) {
        case UO_DEREF:
        case UO_LNOTASSIGN: {
          translateVariablePredicate(blocks, e, label, trueLabel, falseLabel,
                                     file);
          break;
        }
        case UO_LNOT: {
          translateExpressionPredicate(blocks, target, label, falseLabel,
                                       trueLabel, file);
          break;
        }
        case UO_PARENS: {
          translateExpressionPredicate(blocks, target, label, trueLabel,
                                       falseLabel, file);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid unop");
        }
      }
      break;
    }
    case NT_TERNARYEXP: {
      size_t consequentLabel = fresh(file);
      size_t alternativeLabel = fresh(file);
      translateExpressionPredicate(blocks, e->data.ternaryExp.predicate, label,
                                   consequentLabel, alternativeLabel, file);
      translateExpressionPredicate(blocks, e->data.ternaryExp.consequent,
                                   consequentLabel, trueLabel, falseLabel,
                                   file);
      translateExpressionPredicate(blocks, e->data.ternaryExp.alternative,
                                   alternativeLabel, trueLabel, falseLabel,
                                   file);
      break;
    }
    case NT_FUNCALLEXP:
    case NT_SCOPEDID:
    case NT_ID: {
      translateVariablePredicate(blocks, e, label, trueLabel, falseLabel, file);
      break;
    }
    case NT_LITERAL: {
      IRBlock *b = BLOCK(label, blocks);
      if (e->data.literal.data.boolVal)
        IR(b, JUMP(trueLabel));
      else
        IR(b, JUMP(falseLabel));
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid expression");
    }
  }
}

/**
 * translate an expression for its value
 *
 * @param block vector to put new blocks in
 * @param e expression to translate
 * @param label this block's label
 * @param nextLabel label the next block is at
 * @param file file the expression is in
 * @returns temp with produced value
 */
static IROperand *translateExpressionValue(Vector *blocks, Node const *e,
                                           size_t label, size_t nextLabel,
                                           FileListEntry *file) {
  switch (e->type) {
    case NT_BINOPEXP: {
      switch (e->data.binOpExp.op) {
        case BO_SEQ: {
          size_t rhsLabel = fresh(file);
          translateExpressionVoid(blocks, e->data.binOpExp.lhs, label, rhsLabel,
                                  file);
          return translateExpressionValue(blocks, e->data.binOpExp.rhs,
                                          rhsLabel, nextLabel, file);
        }
        case BO_ASSIGN: {
          return NULL;  // TODO
        }
        case BO_MULASSIGN: {
          return NULL;  // TODO
        }
        case BO_DIVASSIGN: {
          return NULL;  // TODO
        }
        case BO_MODASSIGN: {
          return NULL;  // TODO
        }
        case BO_ADDASSIGN: {
          return NULL;  // TODO
        }
        case BO_SUBASSIGN: {
          return NULL;  // TODO
        }
        case BO_LSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_ARSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_LRSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITANDASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITXORASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITORASSIGN: {
          return NULL;  // TODO
        }
        case BO_LANDASSIGN: {
          return NULL;  // TODO
        }
        case BO_LORASSIGN: {
          return NULL;  // TODO
        }
        case BO_LAND: {
          return NULL;  // TODO
        }
        case BO_LOR: {
          return NULL;  // TODO
        }
        case BO_BITAND: {
          return NULL;  // TODO
        }
        case BO_BITOR: {
          return NULL;  // TODO
        }
        case BO_BITXOR: {
          return NULL;  // TODO
        }
        case BO_EQ: {
          return NULL;  // TODO
        }
        case BO_NEQ: {
          return NULL;  // TODO
        }
        case BO_LT: {
          return NULL;  // TODO
        }
        case BO_GT: {
          return NULL;  // TODO
        }
        case BO_LTEQ: {
          return NULL;  // TODO
        }
        case BO_GTEQ: {
          return NULL;  // TODO
        }
        case BO_SPACESHIP: {
          return NULL;  // TODO
        }
        case BO_LSHIFT: {
          return NULL;  // TODO
        }
        case BO_ARSHIFT: {
          return NULL;  // TODO
        }
        case BO_LRSHIFT: {
          return NULL;  // TODO
        }
        case BO_ADD: {
          return NULL;  // TODO
        }
        case BO_SUB: {
          return NULL;  // TODO
        }
        case BO_MUL: {
          return NULL;  // TODO
        }
        case BO_DIV: {
          return NULL;  // TODO
        }
        case BO_MOD: {
          return NULL;  // TODO
        }
        case BO_FIELD: {
          return NULL;  // TODO
        }
        case BO_PTRFIELD: {
          return NULL;  // TODO
        }
        case BO_ARRAY: {
          return NULL;  // TODO
        }
        case BO_CAST: {
          return NULL;  // TODO
        }
        default: {
          error(__FILE__, __LINE__, "invalid binop");
        }
      }
    }
    case NT_UNOPEXP: {
      switch (e->data.unOpExp.op) {
        case UO_DEREF: {
          return NULL;  // TODO
        }
        case UO_ADDROF: {
          return NULL;  // TODO
        }
        case UO_PREINC: {
          return NULL;  // TODO
        }
        case UO_PREDEC: {
          return NULL;  // TODO
        }
        case UO_NEG: {
          return NULL;  // TODO
        }
        case UO_LNOT: {
          return NULL;  // TODO
        }
        case UO_BITNOT: {
          return NULL;  // TODO
        }
        case UO_POSTINC: {
          return NULL;  // TODO
        }
        case UO_POSTDEC: {
          return NULL;  // TODO
        }
        case UO_NEGASSIGN: {
          return NULL;  // TODO
        }
        case UO_LNOTASSIGN: {
          return NULL;  // TODO
        }
        case UO_BITNOTASSIGN: {
          return NULL;  // TODO
        }
        case UO_SIZEOFEXP: {
          return NULL;  // TODO
        }
        case UO_SIZEOFTYPE: {
          return NULL;  // TODO
        }
        case UO_PARENS: {
          return NULL;  // TODO
        }
        default: {
          error(__FILE__, __LINE__, "invalid unop");
        }
      }
    }
    case NT_TERNARYEXP: {
      return NULL;  // TODO
    }
    case NT_FUNCALLEXP: {
      return NULL;  // TODO
    }
    case NT_LITERAL: {
      return NULL;  // TODO
    }
    case NT_SCOPEDID: {
      return NULL;  // TODO
    }
    case NT_ID: {
      return NULL;  // TODO
    }
    default: {
      error(__FILE__, __LINE__, "invalid expression");
    }
  }
}

/**
 * translate an expression for effect
 *
 * @param block vector to put new blocks in
 * @param e expression to translate
 * @param label this block's label
 * @param nextLabel label the next block is at
 * @param file file the expression is in
 */
static void translateExpressionVoid(Vector *blocks, Node const *e, size_t label,
                                    size_t nextLabel, FileListEntry *file) {
  switch (e->type) {
    case NT_BINOPEXP: {
      Node const *lhs = e->data.binOpExp.lhs;
      Node const *rhs = e->data.binOpExp.rhs;
      switch (e->data.binOpExp.op) {
        case BO_SEQ:
        case BO_BITAND:
        case BO_BITOR:
        case BO_BITXOR:
        case BO_EQ:
        case BO_NEQ:
        case BO_LT:
        case BO_GT:
        case BO_LTEQ:
        case BO_GTEQ:
        case BO_SPACESHIP:
        case BO_LSHIFT:
        case BO_ARSHIFT:
        case BO_LRSHIFT:
        case BO_ADD:
        case BO_SUB:
        case BO_MUL:
        case BO_DIV:
        case BO_MOD:
        case BO_ARRAY: {
          size_t rhsLabel = fresh(file);
          translateExpressionVoid(blocks, lhs, label, rhsLabel, file);
          translateExpressionVoid(blocks, rhs, rhsLabel, nextLabel, file);
          break;
        }
        case BO_ASSIGN: {
          size_t rhsLabel = fresh(file);
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
          size_t assignmentLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          IRBlock *b = BLOCK(assignmentLabel, blocks);
          IROperand *castRhs = translateCast(b, rawRhs, expressionTypeof(rhs),
                                             expressionTypeof(lhs), file);
          translateLValueStore(b, lvalue, castRhs, file);
          IR(b, JUMP(nextLabel));
          lvalueFree(lvalue);
          break;
        }
        case BO_MULASSIGN:
        case BO_DIVASSIGN:
        case BO_MODASSIGN:
        case BO_ADDASSIGN:
        case BO_SUBASSIGN:
        case BO_LSHIFTASSIGN:
        case BO_ARSHIFTASSIGN:
        case BO_LRSHIFTASSIGN:
        case BO_BITANDASSIGN:
        case BO_BITXORASSIGN:
        case BO_BITORASSIGN: {
          size_t rhsLabel = fresh(file);
          LValue *lvalue =
              translateExpressionLValue(blocks, lhs, label, rhsLabel, file);
          size_t assignmentLabel = fresh(file);
          IROperand *rawRhs = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          IRBlock *b = BLOCK(assignmentLabel, blocks);
          IROperand *rawLhs = translateLValueLoad(
              b, lvalue, TEMPOF(fresh(file), expressionTypeof(lhs)), file);
          IROperand *rawResult = BINOP_TRANSLATORS[e->data.binOpExp.op](
              b, rawLhs, expressionTypeof(lhs), rawRhs, expressionTypeof(rhs));
          Type *merged =
              arithmeticTypeMerge(expressionTypeof(lhs), expressionTypeof(rhs));
          IROperand *castResult =
              translateCast(b, rawResult, merged, expressionTypeof(lhs), file);
          typeFree(merged);
          translateLValueStore(b, lvalue, castResult, file);
          IR(b, JUMP(nextLabel));
          lvalueFree(lvalue);
          break;
        }
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          size_t shortCircuitLabel = fresh(file);
          LValue *lvalue = translateExpressionLValue(blocks, lhs, label,
                                                     shortCircuitLabel, file);
          size_t rhsLabel = fresh(file);
          IRBlock *b = BLOCK(shortCircuitLabel, blocks);
          IROperand *lhsVal =
              translateLValueLoad(b, lvalue, TEMPBOOL(fresh(file)), file);
          IR(b, BJUMP(BOOL_WIDTH,
                      e->data.binOpExp.op == BO_LANDASSIGN ? IO_JZ : IO_JNZ,
                      nextLabel, lhsVal));
          IR(b, JUMP(rhsLabel));
          size_t assignmentLabel = fresh(file);
          IROperand *rhsVal = translateExpressionValue(blocks, rhs, rhsLabel,
                                                       assignmentLabel, file);
          b = BLOCK(assignmentLabel, blocks);
          translateLValueStore(b, lvalue, rhsVal, file);
          IR(b, JUMP(nextLabel));
          lvalueFree(lvalue);
          break;
        }
        case BO_LAND:
        case BO_LOR: {
          size_t rhsLabel = fresh(file);
          if (e->data.binOpExp.op == BO_LAND)
            translateExpressionPredicate(blocks, lhs, label, rhsLabel,
                                         nextLabel, file);
          else
            translateExpressionPredicate(blocks, lhs, label, nextLabel,
                                         rhsLabel, file);
          translateExpressionVoid(blocks, rhs, rhsLabel, nextLabel, file);
          break;
        }
        case BO_FIELD:
        case BO_PTRFIELD:
        case BO_CAST: {
          translateExpressionVoid(blocks, lhs, label, nextLabel, file);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid binop");
        }
      }
      break;
    }
    case NT_UNOPEXP: {
      Node const *target = e->data.unOpExp.target;
      switch (e->data.unOpExp.op) {
        case UO_DEREF:
        case UO_ADDROF:
        case UO_NEG:
        case UO_LNOT:
        case UO_BITNOT:
        case UO_PARENS: {
          translateExpressionVoid(blocks, target, label, nextLabel, file);
          break;
        }
        case UO_PREINC:
        case UO_POSTINC:
        case UO_PREDEC:
        case UO_POSTDEC:
        case UO_NEGASSIGN:
        case UO_LNOTASSIGN:
        case UO_BITNOTASSIGN: {
          size_t modifyLabel = fresh(file);
          LValue *lvalue = translateExpressionLValue(blocks, target, label,
                                                     modifyLabel, file);
          IRBlock *b = BLOCK(modifyLabel, blocks);
          IROperand *value = translateLValueLoad(
              b, lvalue, TEMPOF(fresh(file), expressionTypeof(target)), file);
          IROperand *modified =
              (UNOP_TRANSLATORS[e->data.unOpExp.op])(b, value,
                                                     expressionTypeof(target));
          translateLValueStore(b, lvalue, modified, file);
          lvalueFree(lvalue);
          break;
        }
        case UO_SIZEOFEXP:
        case UO_SIZEOFTYPE: {
          // nothing to translate - target of sizeof is not evaluated
          IRBlock *b = BLOCK(label, blocks);
          IR(b, JUMP(nextLabel));
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid unop");
        }
      }
      break;
    }
    case NT_TERNARYEXP: {
      size_t consequentLabel = fresh(file);
      size_t alternativeLabel = fresh(file);
      translateExpressionPredicate(blocks, e->data.ternaryExp.predicate, label,
                                   consequentLabel, alternativeLabel, file);
      translateExpressionVoid(blocks, e->data.ternaryExp.consequent,
                              consequentLabel, nextLabel, file);
      translateExpressionVoid(blocks, e->data.ternaryExp.alternative,
                              alternativeLabel, nextLabel, file);
      break;
    }
    case NT_FUNCALLEXP: {
      size_t callLabel = fresh(file);
      IRBlock *b = BLOCK(callLabel, blocks);
      irOperandFree(generateFunctionCall(
          b,
          translateExpressionValue(blocks, e->data.funCallExp.function, label,
                                   callLabel, file),
          expressionTypeof(e->data.funCallExp.function), file));
      IR(b, JUMP(nextLabel));
      break;
    }
    case NT_LITERAL: {
      IRBlock *b = BLOCK(label, blocks);
      IR(b, JUMP(nextLabel));
      break;
    }
    case NT_SCOPEDID:
    case NT_ID: {
      if (expressionTypeof(e)->kind == TK_QUALIFIED &&
          expressionTypeof(e)->data.qualified.volatileQual) {
        irOperandFree(
            translateExpressionValue(blocks, e, label, nextLabel, file));
      } else {
        IRBlock *b = BLOCK(label, blocks);
        IR(b, JUMP(nextLabel));
      }
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid expression");
    }
  }
}

/**
 * translate a statement
 *
 * @param blocks vector to put new blocks in
 * @param stmt statement to translate
 * @param label this block's label
 * @param nextLabel next block's label
 * @param returnLabel label the exit block is
 * @param breakLabel label to get to the outside of the loop
 * @param continueLabel label to get to the start of the loop
 * @param file file the statement is in
 */
static void translateStmt(Vector *blocks, Node *stmt, size_t label,
                          size_t nextLabel, size_t returnLabel,
                          size_t breakLabel, size_t continueLabel,
                          size_t returnValueTemp, Type const *returnType,
                          FileListEntry *file) {
  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      Vector *stmts = stmt->data.compoundStmt.stmts;
      if (stmts->size == 0) {
        // case: empty compound - just jump to the next thing
        IRBlock *b = BLOCK(label, blocks);
        IR(b, JUMP(nextLabel));
      } else if (stmts->size == 1) {
        // case: single element compound - treat it as if there were no compound
        translateStmt(blocks, stmts->elements[0], label, nextLabel, returnLabel,
                      breakLabel, continueLabel, returnValueTemp, returnType,
                      file);
      } else {
        // case: multi element compound
        size_t curr = label;
        for (size_t idx = 0; idx < stmts->size; ++idx) {
          if (idx == stmts->size - 1) {
            // last element
            translateStmt(blocks, stmts->elements[idx], curr, nextLabel,
                          returnLabel, breakLabel, continueLabel,
                          returnValueTemp, returnType, file);
          } else {
            size_t next = fresh(file);
            translateStmt(blocks, stmts->elements[idx], curr, next, returnLabel,
                          breakLabel, continueLabel, returnValueTemp,
                          returnType, file);
            curr = next;
          }
        }
      }
      break;
    }
    case NT_IFSTMT: {
      if (stmt->data.ifStmt.alternative == NULL) {
        // if -> consequent -> next
        //    \----------------^

        size_t trueLabel = fresh(file);
        translateExpressionPredicate(blocks, stmt->data.ifStmt.predicate, label,
                                     trueLabel, nextLabel, file);
        translateStmt(blocks, stmt->data.ifStmt.consequent, trueLabel,
                      nextLabel, returnLabel, breakLabel, continueLabel,
                      returnValueTemp, returnType, file);
      } else {
        // if -> consequent -> next
        //    \> alternative --^

        size_t trueLabel = fresh(file);
        size_t falseLabel = fresh(file);
        translateExpressionPredicate(blocks, stmt->data.ifStmt.predicate, label,
                                     trueLabel, falseLabel, file);
        translateStmt(blocks, stmt->data.ifStmt.consequent, trueLabel,
                      nextLabel, returnLabel, breakLabel, continueLabel,
                      returnValueTemp, returnType, file);
        translateStmt(blocks, stmt->data.ifStmt.alternative, falseLabel,
                      nextLabel, returnLabel, breakLabel, continueLabel,
                      returnValueTemp, returnType, file);
      }
      break;
    }
    case NT_WHILESTMT: {
      //               /------------v
      // condition check -> body -| next
      //               ^----------/

      size_t bodyLabel = fresh(file);
      translateExpressionPredicate(blocks, stmt->data.whileStmt.condition,
                                   label, bodyLabel, nextLabel, file);
      translateStmt(blocks, stmt->data.whileStmt.body, bodyLabel, label,
                    returnLabel, nextLabel, label, returnValueTemp, returnType,
                    file);
      break;
    }
    case NT_DOWHILESTMT: {
      // body -> condition check -> next
      //    ^------------------|

      size_t conditionLabel = fresh(file);
      translateStmt(blocks, stmt->data.doWhileStmt.body, label, conditionLabel,
                    returnLabel, nextLabel, conditionLabel, returnValueTemp,
                    returnType, file);
      translateExpressionPredicate(blocks, stmt->data.doWhileStmt.condition,
                                   conditionLabel, label, nextLabel, file);
      break;
    }
    case NT_FORSTMT: {
      size_t conditionLabel = fresh(file);
      translateStmt(blocks, stmt->data.forStmt.initializer, label,
                    conditionLabel, 0, 0, 0, returnValueTemp, returnType, file);
      size_t bodyLabel = fresh(file);
      translateExpressionPredicate(blocks, stmt->data.forStmt.condition,
                                   conditionLabel, bodyLabel, nextLabel, file);
      if (stmt->data.forStmt.increment != NULL) {
        //                       /----------------------v
        // init -> condition check -> body -> increment next
        //                       ^------------|

        size_t incrementLabel = fresh(file);
        translateStmt(blocks, stmt->data.forStmt.body, bodyLabel,
                      incrementLabel, returnLabel, nextLabel, incrementLabel,
                      returnValueTemp, returnType, file);
        translateExpressionVoid(blocks, stmt->data.forStmt.increment,
                                incrementLabel, conditionLabel, file);
      } else {
        //                       /---------v
        // init -> condition check -> body next
        //                       ^-------|

        translateStmt(blocks, stmt->data.forStmt.body, bodyLabel,
                      conditionLabel, returnLabel, nextLabel, conditionLabel,
                      returnValueTemp, returnType, file);
      }
      break;
    }
    case NT_SWITCHSTMT: {
      // condition -+-> case
      //            |      |
      //            |      v
      //            +-> case
      //               .
      //               .
      //               .
      //            |      |
      //            |      v
      //            +-> next

      size_t jumpSectionLabel = fresh(file);
      IROperand *o =
          translateExpressionValue(blocks, stmt->data.switchStmt.condition,
                                   label, jumpSectionLabel, file);

      Type const *switchedType =
          expressionTypeof(stmt->data.switchStmt.condition);
      size_t size = typeSizeof(switchedType);
      bool isSigned =
          typeSignedIntegral(switchedType) ||
          (typeEnum(switchedType) &&
           typeSignedIntegral(
               stripCV(switchedType)
                   ->data.reference.entry->data.enumType.backingType));

      Vector *cases = stmt->data.switchStmt.cases;
      size_t *caseLabels = malloc(sizeof(size_t) * cases->size);

      for (size_t idx = 0; idx < cases->size; ++idx)
        caseLabels[idx] = fresh(file);

      size_t defaultLabel = 0;
      size_t jumpTableLen = 0;
      for (size_t idx = 0; idx < cases->size; ++idx) {
        Node const *caseNode = cases->elements[idx];
        if (caseNode->type == NT_SWITCHDEFAULT) {
          defaultLabel = idx;
        } else {
          jumpTableLen += caseNode->data.switchCase.values->size;
        }
        translateStmt(blocks,
                      caseNode->type == NT_SWITCHCASE
                          ? caseNode->data.switchCase.body
                          : caseNode->data.switchDefault.body,
                      caseLabels[idx],
                      idx == cases->size - 1 ? nextLabel : caseLabels[idx + 1],
                      returnLabel, nextLabel, continueLabel, returnValueTemp,
                      returnType, file);
      }

      JumpTableEntry *jumpTable = malloc(sizeof(JumpTableEntry) * jumpTableLen);
      size_t currEntry = 0;
      for (size_t caseIdx = 0; caseIdx < cases->size; ++caseIdx) {
        Node const *caseNode = cases->elements[caseIdx];
        if (caseNode->type == NT_SWITCHCASE) {
          size_t label = caseLabels[caseIdx];
          for (size_t valueIdx = 0;
               valueIdx < caseNode->data.switchCase.values->size; ++valueIdx) {
            Node const *valueNode =
                caseNode->data.switchCase.values->elements[valueIdx];
            switch (valueNode->type) {
              case NT_LITERAL: {
                switch (valueNode->data.literal.literalType) {
                  case LT_UBYTE: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.ubyteVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.ubyteVal;
                    break;
                  }
                  case LT_BYTE: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.byteVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.byteVal;
                    break;
                  }
                  case LT_USHORT: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.ushortVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.ushortVal;
                    break;
                  }
                  case LT_SHORT: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.shortVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.shortVal;
                    break;
                  }
                  case LT_UINT: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.uintVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.uintVal;
                    break;
                  }
                  case LT_INT: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.intVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.intVal;
                    break;
                  }
                  case LT_ULONG: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          (int64_t)valueNode->data.literal.data.ulongVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.ulongVal;
                    break;
                  }
                  case LT_LONG: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.longVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.longVal;
                    break;
                  }
                  case LT_CHAR: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.charVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.charVal;
                    break;
                  }
                  case LT_WCHAR: {
                    if (isSigned)
                      jumpTable[currEntry].value.signedVal =
                          valueNode->data.literal.data.wcharVal;
                    else
                      jumpTable[currEntry].value.unsignedVal =
                          valueNode->data.literal.data.wcharVal;
                    break;
                  }
                  default: {
                    error(__FILE__, __LINE__,
                          "can't have a switch case value of that type");
                  }
                }
                break;
              }
              case NT_SCOPEDID: {
                if (isSigned)
                  jumpTable[currEntry].value.signedVal =
                      valueNode->data.scopedId.entry->data.enumConst.data
                          .signedValue;
                else
                  jumpTable[currEntry].value.unsignedVal =
                      valueNode->data.scopedId.entry->data.enumConst.data
                          .unsignedValue;
                break;
              }
              default: {
                error(__FILE__, __LINE__,
                      "can't have a switch case value with that node");
              }
            }
            jumpTable[currEntry++].label = label;
          }
        }
      }
      free(caseLabels);

      qsort(jumpTable, jumpTableLen, sizeof(JumpTableEntry),
            (int (*)(void const *,
                     void const *))(isSigned ? compareSignedJumpTableEntry
                                             : compareUnsignedJumpTableEntry));
      if (defaultLabel == 0) defaultLabel = nextLabel;
      size_t curr = jumpSectionLabel;
      if (isSigned) {
        for (size_t entryIdx = 0; entryIdx < jumpTableLen; ++entryIdx) {
          if (entryIdx == jumpTableLen - 1 ||
              jumpTable[entryIdx].value.signedVal !=
                  jumpTable[entryIdx + 1].value.signedVal - 1) {
            // singleton
            IRBlock *b = BLOCK(curr, blocks);
            IR(b, CJUMP(size, IO_E, jumpTable[entryIdx].label, irOperandCopy(o),
                        signedJumpTableEntryToConstant(&jumpTable[entryIdx],
                                                       size)));
            IR(b, JUMP(entryIdx == jumpTableLen - 1 ? defaultLabel
                                                    : (curr = fresh(file))));
          } else {
            // not a singleton
            size_t end = entryIdx + 1;
            while (end < jumpTableLen - 1 &&
                   jumpTable[end].value.signedVal ==
                       jumpTable[end + 1].value.signedVal - 1)
              ++end;

            size_t next = end == jumpTableLen - 1 ? defaultLabel : fresh(file);
            size_t tableLabel = fresh(file);
            IRFrag *table = dataFragCreate(
                FT_RODATA, format(localLabelFormat(), tableLabel),
                POINTER_WIDTH);
            vectorInsert(&file->irFrags, table);
            for (size_t blockIdx = entryIdx; blockIdx <= end; ++blockIdx)
              vectorInsert(&table->data.data.data,
                           labelDatumCreate(jumpTable[blockIdx].label));

            size_t gtFallthroughLabel = fresh(file);
            IRBlock *b = BLOCK(curr, blocks);
            IR(b, CJUMP(size, IO_L, defaultLabel, irOperandCopy(o),
                        signedJumpTableEntryToConstant(&jumpTable[entryIdx],
                                                       size)));
            IR(b, JUMP(gtFallthroughLabel));

            size_t tableDerefLabel = fresh(file);
            b = BLOCK(gtFallthroughLabel, blocks);
            IR(b, CJUMP(size, IO_G, next, irOperandCopy(o),
                        signedJumpTableEntryToConstant(&jumpTable[end], size)));
            IR(b, JUMP(tableDerefLabel));

            size_t offset = fresh(file);
            size_t castOffset;
            size_t multipliedOffset = fresh(file);
            size_t target = fresh(file);
            b = BLOCK(tableDerefLabel, blocks);
            IR(b,
               BINOP(
                   size, IO_SUB, TEMPOF(target, switchedType), irOperandCopy(o),
                   signedJumpTableEntryToConstant(&jumpTable[entryIdx], size)));
            if (size != 8) {
              castOffset = fresh(file);
              IR(b, UNOP(size, IO_SX_LONG, TEMPPTR(castOffset),
                         TEMPOF(offset, switchedType)));
            } else {
              castOffset = offset;
            }

            IR(b,
               BINOP(POINTER_WIDTH, IO_SMUL, TEMPPTR(multipliedOffset),
                     TEMPPTR(castOffset),
                     CONSTANT(POINTER_WIDTH, longDatumCreate(POINTER_WIDTH))));
            IR(b, BINOP(POINTER_WIDTH, IO_ADD, TEMPPTR(target),
                        TEMPPTR(multipliedOffset), LOCAL(tableLabel)));
            IR(b, JUMP(target));

            curr = next;
            entryIdx = end;
          }
        }
      } else {
        for (size_t entryIdx = 0; entryIdx < jumpTableLen; ++entryIdx) {
          if (entryIdx == jumpTableLen - 1 ||
              jumpTable[entryIdx].value.unsignedVal !=
                  jumpTable[entryIdx + 1].value.unsignedVal - 1) {
            // singleton
            IRBlock *b = BLOCK(curr, blocks);
            IR(b, CJUMP(size, IO_E, jumpTable[entryIdx].label, irOperandCopy(o),
                        unsignedJumpTableEntryToConstant(&jumpTable[entryIdx],
                                                         size)));
            IR(b, JUMP(entryIdx == jumpTableLen - 1 ? defaultLabel
                                                    : (curr = fresh(file))));
          } else {
            // not a singleton
            size_t end = entryIdx + 1;
            while (end < jumpTableLen - 1 &&
                   jumpTable[end].value.unsignedVal ==
                       jumpTable[end + 1].value.unsignedVal - 1)
              ++end;

            size_t next = end == jumpTableLen - 1 ? defaultLabel : fresh(file);
            size_t tableLabel = fresh(file);
            IRFrag *table = dataFragCreate(
                FT_RODATA, format(localLabelFormat(), tableLabel),
                POINTER_WIDTH);
            vectorInsert(&file->irFrags, table);
            for (size_t blockIdx = entryIdx; blockIdx <= end; ++blockIdx)
              vectorInsert(&table->data.data.data,
                           labelDatumCreate(jumpTable[blockIdx].label));

            size_t gtFallthroughLabel = fresh(file);
            IRBlock *b = BLOCK(curr, blocks);
            IR(b, CJUMP(size, IO_B, defaultLabel, irOperandCopy(o),
                        unsignedJumpTableEntryToConstant(&jumpTable[entryIdx],
                                                         size)));
            IR(b, JUMP(gtFallthroughLabel));

            size_t tableDerefLabel = fresh(file);
            b = BLOCK(gtFallthroughLabel, blocks);
            IR(b,
               CJUMP(size, IO_A, next, irOperandCopy(o),
                     unsignedJumpTableEntryToConstant(&jumpTable[end], size)));
            IR(b, JUMP(tableDerefLabel));

            size_t offset = fresh(file);
            size_t castOffset;
            size_t multipliedOffset = fresh(file);
            size_t target = fresh(file);
            b = BLOCK(tableDerefLabel, blocks);
            IR(b, BINOP(size, IO_SUB, TEMPOF(target, switchedType),
                        irOperandCopy(o),
                        unsignedJumpTableEntryToConstant(&jumpTable[entryIdx],
                                                         size)));
            if (size != 8) {
              castOffset = fresh(file);
              IR(b, UNOP(size, IO_ZX_LONG, TEMPPTR(castOffset),
                         TEMPOF(offset, switchedType)));
            } else {
              castOffset = offset;
            }

            IR(b,
               BINOP(POINTER_WIDTH, IO_UMUL, TEMPPTR(multipliedOffset),
                     TEMPPTR(castOffset),
                     CONSTANT(POINTER_WIDTH, longDatumCreate(POINTER_WIDTH))));
            IR(b, BINOP(POINTER_WIDTH, IO_ADD, TEMPPTR(target),
                        TEMPPTR(multipliedOffset), LOCAL(tableLabel)));
            IR(b, JUMP(target));

            curr = next;
            entryIdx = end;
          }
        }
      }
      irOperandFree(o);
      free(jumpTable);
      break;
    }
    case NT_BREAKSTMT: {
      IRBlock *b = BLOCK(label, blocks);
      IR(b, JUMP(breakLabel));
      break;
    }
    case NT_CONTINUESTMT: {
      IRBlock *b = BLOCK(label, blocks);
      IR(b, JUMP(continueLabel));
      break;
    }
    case NT_RETURNSTMT: {
      if (stmt->data.returnStmt.value != NULL) {
        size_t returnMoveLabel = fresh(file);
        IROperand *value = translateExpressionValue(
            blocks, stmt->data.returnStmt.value, label, returnMoveLabel, file);
        IRBlock *b = BLOCK(returnMoveLabel, blocks);
        IROperand *casted = translateCast(
            b, value, expressionTypeof(stmt->data.returnStmt.value), returnType,
            file);
        IR(b, MOVE(casted->data.temp.size,
                   TEMP(returnValueTemp, casted->data.temp.alignment,
                        casted->data.temp.size, casted->data.temp.kind),
                   casted));
        IR(b, JUMP(returnLabel));
      } else {
        IRBlock *b = BLOCK(label, blocks);
        IR(b, JUMP(returnLabel));
      }
      break;
    }
    case NT_ASMSTMT: {
      IRBlock *b = BLOCK(label, blocks);
      IR(b, ASM(stmt->data.asmStmt.assembly));
      IR(b, JUMP(nextLabel));
      break;
    }
    case NT_VARDEFNSTMT: {
      Vector *names = stmt->data.varDefnStmt.names;
      Vector *initializers = stmt->data.varDefnStmt.initializers;

      size_t curr = label;
      for (size_t idx = 0; idx < names->size; ++idx) {
        Node *name = names->elements[idx];
        Node const *initializer = initializers->elements[idx];
        if (idx == names->size - 1) {
          if (initializer == NULL) {
            IRBlock *b = BLOCK(curr, blocks);
            IR(b, JUMP(nextLabel));
            name->data.id.entry->data.variable.temp = fresh(file);
          } else {
            IROperand *o = translateExpressionValue(blocks, initializer, curr,
                                                    nextLabel, file);
            name->data.id.entry->data.variable.temp = o->data.temp.name;
            irOperandFree(o);
          }
        } else {
          if (initializer == NULL) {
            name->data.id.entry->data.variable.temp = fresh(file);
          } else {
            size_t next = fresh(file);
            IROperand *o =
                translateExpressionValue(blocks, initializer, curr, next, file);
            name->data.id.entry->data.variable.temp = o->data.temp.name;
            irOperandFree(o);
            curr = next;
          }
        }
      }
      break;
    }
    case NT_EXPRESSIONSTMT: {
      translateExpressionVoid(blocks, stmt->data.expressionStmt.expression,
                              label, nextLabel, file);
      break;
    }
    default: {
      IRBlock *b = BLOCK(label, blocks);
      IR(b, JUMP(nextLabel));
      break;
    }
  }
}

/**
 * translate the given file
 */
static void translateFile(FileListEntry *file) {
  char *namePrefix =
      generatePrefix(file->ast->data.file.module->data.module.id);
  Vector *bodies = file->ast->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; ++idx) {
    Node *body = bodies->elements[idx];
    switch (body->type) {
      case NT_FUNDEFN: {
        SymbolTableEntry *entry = body->data.funDefn.name->data.id.entry;
        IRFrag *frag = textFragCreate(
            suffixName(namePrefix, body->data.funDefn.name->data.id.id));
        vectorInsert(&file->irFrags, frag);
        Vector *blocks = &frag->data.text.blocks;

        size_t returnValueAddressTemp = fresh(file);
        size_t returnValueTemp = fresh(file);
        size_t bodyLabel = fresh(file);
        size_t exitLabel = fresh(file);

        generateFunctionEntry(blocks, entry, returnValueAddressTemp, bodyLabel,
                              file);

        translateStmt(&frag->data.text.blocks, body->data.funDefn.body,
                      bodyLabel, exitLabel, exitLabel, 0, 0, returnValueTemp,
                      entry->data.function.returnType, file);

        generateFunctionExit(blocks, entry, returnValueAddressTemp,
                             returnValueTemp, exitLabel, file);
        break;
      }
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        Vector *initializers = body->data.varDefn.initializers;
        for (size_t idx = 0; idx < names->size; ++idx)
          translateLiteral(names->elements[idx], initializers->elements[idx],
                           namePrefix, &file->irFrags, file);
        break;
      }
      default: {
        // no translation stuff otherwise
        break;
      }
    }
  }
  free(namePrefix);
}

void translate(void) {
  // for each code file, translate it
  for (size_t idx = 0; idx < fileList.size; ++idx)
    if (fileList.entries[idx].isCode) translateFile(&fileList.entries[idx]);
}