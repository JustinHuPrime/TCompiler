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
 * @param blocks vector to put new blocks in
 * @param src source temp
 * @param fromType type of the input temp
 * @param toType type of the output temp
 * @param label this block's label
 * @param nextLabel next block's label
 * @param file file the expression is in
 * @returns temp with produced value
 */
static IROperand *translateCast(Vector *blocks, IROperand *src,
                                Type const *fromType, Type const *toType,
                                size_t label, size_t nextLabel,
                                FileListEntry *file) {
  return NULL;  // TODO
}

/**
 * translate a predicate
 *
 * @param block vector to put new blocks in
 * @param e expression to translate
 * @param label this block's label
 * @param trueLabel label to go to if true
 * @param falseLabel label to go to if false
 * @param file file the statement is in
 */
static void translateExpressionPredicate(Vector *blocks, Node const *e,
                                         size_t label, size_t trueLabel,
                                         size_t falseLabel,
                                         FileListEntry *file) {
  // TODO
}

/**
 * translate an expression for effect
 *
 * @param block vector to put new blocks in
 * @param e statement to translate
 * @param label this block's label
 * @param nextLabel label the next block is at
 * @param file file the statement is in
 */
static void translateExpressionVoid(Vector *blocks, Node const *e, size_t label,
                                    size_t nextLabel, FileListEntry *file) {
  // TODO
}

/**
 * translate an expression for its value
 *
 * @param block vector to put new blocks in
 * @param e statement to translate
 * @param label this block's label
 * @param nextLabel label the next block is at
 * @param file file the statement is in
 * @returns temp with produced value
 */
static IROperand *translateExpressionValue(Vector *blocks, Node const *e,
                                           size_t label, size_t nextLabel,
                                           FileListEntry *file) {
  return NULL;  // TODO
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
      // TODO
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
        size_t returnCast = fresh(file);
        IROperand *value = translateExpressionValue(
            blocks, stmt->data.returnStmt.value, label, returnCast, file);
        size_t returnMove = fresh(file);
        IROperand *casted = translateCast(
            blocks, value, expressionTypeof(stmt->data.returnStmt.value),
            returnType, returnCast, returnMove, file);
        IRBlock *b = BLOCK(returnMove, blocks);
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
        size_t toBodyLink = fresh(file);
        size_t toExitLink = fresh(file);

        generateFunctionEntry(blocks, entry, returnValueAddressTemp, toBodyLink,
                              file);

        translateStmt(&frag->data.text.blocks, body->data.funDefn.body,
                      toBodyLink, toExitLink, toExitLink, 0, 0, returnValueTemp,
                      entry->data.function.returnType, file);

        generateFunctionExit(blocks, entry, returnValueAddressTemp,
                             returnValueTemp, toExitLink, file);
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