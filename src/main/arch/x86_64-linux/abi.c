// Copyright 2020-2021 Justin Hu
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

#include "arch/x86_64-linux/abi.h"

#include "arch/x86_64-linux/asm.h"
#include "ir/shorthand.h"
#include "translation/translation.h"
#include "util/internalError.h"
#include "util/numericSizing.h"

typedef enum {
  X86_64_LINUX_TC_GP,
  X86_64_LINUX_TC_SSE,
  X86_64_LINUX_TC_MEMORY,
  X86_64_LINUX_TC_NO_CLASS,
} TypeClass;

/**
 * produces the layout of a type
 *
 * @param t type to lay out (typeSizeof(t) <= 16, but we don't rely on that)
 * @returns pointer to TypeClass whose length is typeSizeof(t) - note that this
 * never involves TC_MEMORY
 */
static TypeClass *layout(Type const *t) {
  size_t const size = typeSizeof(t);
  TypeClass *retval = malloc(sizeof(TypeClass) * size);
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_VOID: {
          for (size_t idx = 0; idx < size; ++idx)
            retval[idx] = X86_64_LINUX_TC_NO_CLASS;
          return retval;
        }
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
          for (size_t idx = 0; idx < size; ++idx)
            retval[idx] = X86_64_LINUX_TC_GP;
          return retval;
        }
        case TK_FLOAT:
        case TK_DOUBLE: {
          for (size_t idx = 0; idx < size; ++idx)
            retval[idx] = X86_64_LINUX_TC_SSE;
          return retval;
        }
        default: {
          error(__FILE__, __LINE__, "invalid typeKeyword");
        }
      }
      break;
    }
    case TK_QUALIFIED: {
      return layout(t->data.qualified.base);
    }
    case TK_POINTER:
    case TK_FUNPTR: {
      for (size_t idx = 0; idx < size; ++idx) retval[idx] = X86_64_LINUX_TC_GP;
      return retval;
    }
    case TK_ARRAY: {
      size_t elementSize = typeSizeof(t->data.array.type);
      TypeClass *elementLayout = layout(t->data.array.type);
      for (size_t idx = 0; idx < t->data.array.length; ++idx)
        memcpy(retval + idx * elementSize, elementLayout,
               sizeof(TypeClass) * elementSize);
      free(elementLayout);
      return retval;
    }
    case TK_REFERENCE: {
      SymbolTableEntry *entry = t->data.reference.entry;
      switch (entry->kind) {
        case SK_ENUM: {
          return layout(entry->data.enumType.backingType);
        }
        case SK_STRUCT: {
          for (size_t idx = 0; idx < size; ++idx)
            retval[idx] = X86_64_LINUX_TC_NO_CLASS;

          size_t offset = 0;
          for (size_t idx = 0; idx < entry->data.structType.fieldTypes.size;
               ++idx) {
            Type const *fieldType =
                entry->data.structType.fieldTypes.elements[idx];
            size_t fieldSize = typeSizeof(fieldType);
            TypeClass *fieldLayout = layout(fieldType);
            memcpy(retval + offset, fieldLayout, sizeof(TypeClass) * fieldSize);
            free(fieldLayout);

            offset += fieldSize;
            if (idx < entry->data.structType.fieldTypes.size - 1) {
              offset = incrementToMultiple(
                  offset,
                  typeAlignof(
                      entry->data.structType.fieldTypes.elements[idx + 1]));
            }
          }
          return retval;
        }
        case SK_TYPEDEF: {
          return layout(entry->data.typedefType.actual);
        }
        case SK_UNION: {
          for (size_t idx = 0; idx < size; ++idx)
            retval[idx] = X86_64_LINUX_TC_NO_CLASS;

          for (size_t optionIdx = 0;
               optionIdx < entry->data.unionType.optionTypes.size;
               ++optionIdx) {
            Type const *optionType =
                entry->data.unionType.optionTypes.elements[optionIdx];
            size_t optionSize = typeSizeof(optionType);
            TypeClass *optionLayout = layout(optionType);

            for (size_t byte = 0; byte < optionSize; ++byte) {
              if (retval[byte] == X86_64_LINUX_TC_NO_CLASS)
                retval[byte] = optionLayout[byte];
              else if (retval[byte] == X86_64_LINUX_TC_SSE &&
                       optionLayout[byte] == X86_64_LINUX_TC_GP)
                retval[byte] = X86_64_LINUX_TC_GP;
            }
          }
          return retval;
        }
        default: {
          error(__FILE__, __LINE__, "can't construct that type anyways");
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "can't construct that type anyways");
    }
  }
}

/**
 * assign a type to one or two type classes
 *
 * @param t type to classify
 * @param out pointer to array of two TypeClasses - if the first is memory or
 * the second is none, then only one register (or memory) is used; if the second
 * is set to something, two registers are needed
 */
static void classify(Type const *t, TypeClass *out) {
  size_t size = typeSizeof(t);
  if (size > 16) {
    out[0] = X86_64_LINUX_TC_MEMORY;
    out[1] = X86_64_LINUX_TC_NO_CLASS;
    return;
  }

  TypeClass *typeLayout = layout(t);
  out[0] = X86_64_LINUX_TC_SSE;
  for (size_t idx = 0; idx < size && idx < 8; ++idx) {
    if (typeLayout[idx] == X86_64_LINUX_TC_GP) {
      out[0] = X86_64_LINUX_TC_GP;
      break;
    }
    // note - will always see an SSE or a GP - we never need to align to more
    // than 8 bytes
  }

  if (size > 8) {
    out[1] = X86_64_LINUX_TC_SSE;
    for (size_t idx = 8; idx < size; ++idx) {
      if (typeLayout[idx] == X86_64_LINUX_TC_GP) {
        out[1] = X86_64_LINUX_TC_GP;
        break;
      }
    }
  } else {
    out[1] = X86_64_LINUX_TC_NO_CLASS;
  }

  free(typeLayout);
}

/**
 * registers used to pass general purpose arguments
 */
static X86_64_Register GP_ARG_REGS[] = {
    X86_64_RDI, X86_64_RSI, X86_64_RDX, X86_64_RCX, X86_64_R8, X86_64_R9,
};
static size_t GP_ARG_REG_MAX = 6;
/**
 * registers used to pass SSE arguments
 */
static X86_64_Register SSE_ARG_REGS[] = {
    X86_64_XMM0, X86_64_XMM1, X86_64_XMM2, X86_64_XMM3,
    X86_64_XMM4, X86_64_XMM5, X86_64_XMM6, X86_64_XMM7,
};
static size_t SSE_ARG_REG_MAX = 8;
/**
 * registers used to return general purpose arguments
 */
static X86_64_Register GP_RETURN_REGS[] = {
    X86_64_RAX,
    X86_64_RDX,
};
/**
 * registers used to return SSE arguments
 */
static X86_64_Register SSE_RETURN_REGS[] = {
    X86_64_XMM0,
    X86_64_XMM1,
};

IRBlock *x86_64LinuxGenerateFunctionEntry(size_t *linkOut,
                                          SymbolTableEntry *entry,
                                          size_t returnValueAddressTemp,
                                          FileListEntry *file) {
  IRBlock *b = irBlockCreate(fresh(file));

  size_t gpArgIdx = 0;
  size_t sseArgIdx = 0;
  size_t stackOffset = 8;

  Type const *returnType = entry->data.function.returnType;
  TypeClass returnTypeClass[2];
  classify(returnType, returnTypeClass);

  if (returnTypeClass[0] == X86_64_LINUX_TC_MEMORY) {
    IR(b,
       MOVE(POINTER_WIDTH,
            TEMP(returnValueAddressTemp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
            REG(GP_ARG_REGS[gpArgIdx++])));
  }

  // for each argument, left to right
  Vector const *argumentTypes = &entry->data.function.argumentTypes;
  Vector *argumentEntries = &entry->data.function.argumentEntries;
  for (size_t idx = 0; idx < argumentTypes->size; ++idx) {
    Type const *argType = argumentTypes->elements[idx];
    size_t size = typeSizeof(argType);
    size_t alignment = typeAlignof(argType);
    AllocHint allocation = typeAllocation(argType);
    SymbolTableEntry *argumentEntry = argumentEntries->elements[idx];

    TypeClass argTypeClass[2];
    classify(argType, argTypeClass);
    size_t numGP = (argTypeClass[0] == X86_64_LINUX_TC_GP ? 1U : 0U) +
                   (argTypeClass[1] == X86_64_LINUX_TC_GP ? 1U : 0U);
    size_t numSSE = (argTypeClass[0] == X86_64_LINUX_TC_SSE ? 1U : 0U) +
                    (argTypeClass[1] == X86_64_LINUX_TC_SSE ? 1U : 0U);

    if (argTypeClass[0] == X86_64_LINUX_TC_MEMORY ||
        gpArgIdx + numGP > GP_ARG_REG_MAX ||
        sseArgIdx + numSSE > SSE_ARG_REG_MAX) {
      // either must be passed in memory, or ran out of room to pass in
      // registers
      IR(b, STK_LOAD(size,
                     TEMP(argumentEntry->data.variable.temp = fresh(file),
                          alignment, size, allocation),
                     OFFSET((int64_t)stackOffset)));
      stackOffset = incrementToMultiple(stackOffset + size, 8);
    } else {
      // passed in registers
      size_t temp = (argumentEntry->data.variable.temp = fresh(file));
      if (argTypeClass[0] == X86_64_LINUX_TC_GP &&
          argTypeClass[1] == X86_64_LINUX_TC_NO_CLASS) {
        IR(b, MOVE(size, TEMP(temp, alignment, size, allocation),
                   REG(GP_ARG_REGS[gpArgIdx++])));
      } else if (argTypeClass[0] == X86_64_LINUX_TC_GP) {
        IR(b, OFFSET_LOAD(8, TEMP(temp, alignment, size, allocation),
                          REG(GP_ARG_REGS[gpArgIdx++]), OFFSET(0)));
      } else if (argTypeClass[0] == X86_64_LINUX_TC_SSE &&
                 argTypeClass[1] == X86_64_LINUX_TC_NO_CLASS) {
        IR(b, MOVE(size, TEMP(temp, alignment, size, allocation),
                   REG(SSE_ARG_REGS[sseArgIdx++])));
      } else if (argTypeClass[0] == X86_64_LINUX_TC_SSE) {
        IR(b, OFFSET_LOAD(8, TEMP(temp, alignment, size, allocation),
                          REG(SSE_ARG_REGS[sseArgIdx++]), OFFSET(0)));
      }

      if (argTypeClass[1] == X86_64_LINUX_TC_GP) {
        IR(b, OFFSET_LOAD(size - 8, TEMP(temp, alignment, size, allocation),
                          REG(GP_ARG_REGS[gpArgIdx++]), OFFSET(8)));
      } else if (argTypeClass[1] == X86_64_LINUX_TC_SSE) {
        IR(b, OFFSET_LOAD(size - 8, TEMP(temp, alignment, size, allocation),
                          REG(SSE_ARG_REGS[sseArgIdx++]), OFFSET(8)));
      }
    }
  }

  IR(b, JUMP(*linkOut = fresh(file)));
  return b;
}
IRBlock *x86_64LinuxGenerateFunctionExit(SymbolTableEntry const *entry,
                                         size_t returnValueAddressTemp,
                                         size_t returnValueTemp,
                                         size_t prevLink, FileListEntry *file) {
  IRBlock *b = irBlockCreate(prevLink);
  Type const *returnType = entry->data.function.returnType;

  // is it a void function?
  if (!(returnType->kind == TK_KEYWORD &&
        returnType->data.keyword.keyword == TK_VOID)) {
    // look at the return type
    TypeClass returnTypeClass[2];
    classify(returnType, returnTypeClass);

    size_t size = typeSizeof(returnType);
    size_t alignment = typeAlignof(returnType);
    AllocHint allocation = typeAllocation(returnType);

    if (returnTypeClass[0] == X86_64_LINUX_TC_MEMORY) {
      // returned in memory (pointer was given to us above)
      IR(b, MEM_STORE(size,
                      TEMP(returnValueAddressTemp, POINTER_WIDTH, POINTER_WIDTH,
                           AH_GP),
                      TEMP(returnValueTemp, alignment, size, allocation)));
    } else {
      // returned in registers
      size_t gpReturnIdx = 0;
      size_t sseReturnIdx = 0;
      if (returnTypeClass[0] == X86_64_LINUX_TC_GP &&
          returnTypeClass[1] == X86_64_LINUX_TC_NO_CLASS) {
        IR(b, MOVE(POINTER_WIDTH, REG(GP_RETURN_REGS[gpReturnIdx++]),
                   TEMP(returnValueTemp, alignment, size, allocation)));
      } else if (returnTypeClass[0] == X86_64_LINUX_TC_GP) {
        IR(b, OFFSET_LOAD(8, REG(GP_RETURN_REGS[gpReturnIdx++]),
                          TEMP(returnValueTemp, alignment, size, allocation),
                          OFFSET(0)));
      } else if (returnTypeClass[0] == X86_64_LINUX_TC_SSE &&
                 returnTypeClass[1] == X86_64_LINUX_TC_NO_CLASS) {
        IR(b, MOVE(POINTER_WIDTH, REG(SSE_RETURN_REGS[sseReturnIdx++]),
                   TEMP(returnValueTemp, alignment, size, allocation)));
      } else if (returnTypeClass[0] == X86_64_LINUX_TC_SSE) {
        IR(b, OFFSET_LOAD(8, REG(SSE_RETURN_REGS[sseReturnIdx++]),
                          TEMP(returnValueTemp, alignment, size, allocation),
                          OFFSET(0)));
      }

      if (returnTypeClass[1] == X86_64_LINUX_TC_GP) {
        IR(b, OFFSET_LOAD(size - 8, REG(GP_RETURN_REGS[gpReturnIdx++]),
                          TEMP(returnValueTemp, alignment, size, allocation),
                          OFFSET(8)));
      } else if (returnTypeClass[1] == X86_64_LINUX_TC_SSE) {
        IR(b, OFFSET_LOAD(size - 8, REG(SSE_RETURN_REGS[sseReturnIdx++]),
                          TEMP(returnValueTemp, alignment, size, allocation),
                          OFFSET(8)));
      }
    }
  }
  IR(b, RETURN());
  return b;
}