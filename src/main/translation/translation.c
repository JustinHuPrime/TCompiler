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

#include "fileList.h"
#include "ir/ir.h"
#include "util/conversions.h"
#include "util/internalError.h"
#include "util/numericSizing.h"
#include "util/string.h"

/**
 * next id returned by fresh
 */
static size_t nextId = 1;
/**
 * generate a fresh numeric identifier
 */
static size_t fresh(void) { return nextId++; }

/**
 * generate the name prefix for the file
 *
 * @param id module id
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
      suffix = format("%s%zu%s", old, strlen(component->data.id.id),
                      component->data.id.id);
      free(old);
    }
  }

  char *retval = format("_T%s", suffix);
  free(suffix);
  return retval;
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
                                 Type const *type, Node const *initializer) {
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
                           initializer);
      break;
    }
    case TK_POINTER: {
      if (initializer->data.literal.literalType == LT_NULL) {
        vectorInsert(data, longDatumCreate(0));
      } else if (initializer->data.literal.literalType == LT_STRING) {
        size_t label = fresh();
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df =
            dataFragCreate(FT_RODATA, format(".LC%zu", label), CHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     stringDatumCreate(
                         tstrdup(initializer->data.literal.data.stringVal)));
      } else {
        size_t label = fresh();
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df =
            dataFragCreate(FT_RODATA, format(".LC%zu", label), WCHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     wstringDatumCreate(
                         twstrdup(initializer->data.literal.data.wstringVal)));
      }
      break;
    }
    case TK_ARRAY: {
      Vector *initializers = initializer->data.literal.data.aggregateInitVal;
      for (size_t idx = 0; idx < initializers->size; ++idx)
        translateInitializer(data, irFrags, type->data.array.type,
                             initializers->elements[idx]);
      break;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *entry = type->data.reference.entry;
      switch (entry->kind) {
        case SK_ENUM: {
          // TODO
          break;
        }
        case SK_STRUCT: {
          // TODO
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
                             char const *namePrefix, Vector *irFrags) {
  Type const *type = name->data.id.entry->data.variable.type;
  IRFrag *df;
  if (initializer == NULL || initializerAllZero(initializer)) {
    df = dataFragCreate(FT_BSS,
                        format("%s%zu%s", namePrefix, strlen(name->data.id.id),
                               name->data.id.id),
                        typeAlignof(type));
    vectorInsert(&df->data.data.data, paddingDatumCreate(typeSizeof(type)));
    vectorInsert(irFrags, df);
  } else {
    df = dataFragCreate(
        type->kind == TK_QUALIFIED && type->data.qualified.constQual ? FT_RODATA
                                                                     : FT_DATA,
        format("%s%zu%s", namePrefix, strlen(name->data.id.id),
               name->data.id.id),
        typeAlignof(type));
    translateInitializer(&df->data.data.data, irFrags, type, initializer);
  }
  vectorInsert(irFrags, df);
}

/**
 * translate the given file
 */
static void translateFile(FileListEntry *entry) {
  char *namePrefix =
      generatePrefix(entry->ast->data.file.module->data.module.id);
  Vector *bodies = entry->ast->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; ++idx) {
    Node *body = bodies->elements[idx];
    switch (body->type) {
      case NT_FUNDEFN: {
        // TODO
        break;
      }
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        Vector *initializers = body->data.varDefn.initializers;
        for (size_t idx = 0; idx < names->size; ++idx)
          translateLiteral(names->elements[idx], initializers->elements[idx],
                           namePrefix, &entry->irFrags);
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