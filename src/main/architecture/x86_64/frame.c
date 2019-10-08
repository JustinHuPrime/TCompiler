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

// implementation of x86_64 function frames

#include "architecture/x86_64/frame.h"

#include "constants.h"
#include "ir/frame.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "typecheck/symbolTable.h"
#include "util/format.h"
#include "util/functional.h"
#include "util/internalError.h"

#include <stdlib.h>
#include <string.h>

typedef struct X86_64GlobalAccess {
  Access base;
  char *label;
} X86_64GlobalAccess;
static void x86_64GlobalAccessDtor(Access *baseAccess) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;
  free(access->label);
  free(access);
}
static IROperand *x86_64GlobalAccessLoad(Access *baseAccess,
                                         IREntryVector *code,
                                         TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  size_t result = NEW(tempAllocator);
  IR(code, MEM_LOAD(access->base.size,
                    TEMP(result, access->base.size, access->base.alignment,
                         access->base.kind),
                    NAME(strdup(access->label))));
  return TEMP(result, access->base.size, access->base.alignment,
              access->base.kind);
}
static void x86_64GlobalAccessStore(Access *baseAccess, IREntryVector *code,
                                    IROperand *input,
                                    TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  IR(code, MEM_STORE(access->base.size, NAME(strdup(access->label)), input));
}
static IROperand *x86_64GlobalAccessAddrof(Access *baseAccess,
                                           IREntryVector *code,
                                           TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  return NAME(strdup(access->label));
}
static char *x86_64GlobalAccessGetLabel(Access *baseAccess) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;
  return strdup(access->label);
}
AccessVTable *X86_64GlobalAccessVTable = NULL;
static AccessVTable *getX86_64GlobalAccessVTable(void) {
  if (X86_64GlobalAccessVTable == NULL) {
    X86_64GlobalAccessVTable = malloc(sizeof(AccessVTable));
    X86_64GlobalAccessVTable->dtor = x86_64GlobalAccessDtor;
    X86_64GlobalAccessVTable->load = x86_64GlobalAccessLoad;
    X86_64GlobalAccessVTable->store = x86_64GlobalAccessStore;
    X86_64GlobalAccessVTable->addrof = x86_64GlobalAccessAddrof;
    X86_64GlobalAccessVTable->getLabel = x86_64GlobalAccessGetLabel;
  }
  return X86_64GlobalAccessVTable;
}
Access *x86_64GlobalAccessCtor(size_t size, size_t alignment, AllocHint kind,
                               char *label) {
  X86_64GlobalAccess *access = malloc(sizeof(X86_64GlobalAccess));
  access->base.vtable = getX86_64GlobalAccessVTable();
  access->base.size = size;

  access->base.kind = kind;
  access->label = label;
  return (Access *)access;
}

typedef struct X86_64TempAccess {
  Access base;
  size_t tempNum;
} X86_64TempAccess;
static void x86_64TempAccessDtor(Access *baseAccess) {
  X86_64TempAccess *access = (X86_64TempAccess *)baseAccess;
  free(access);
}
static IROperand *x86_64TempAccessLoad(Access *baseAccess, IREntryVector *code,
                                       TempAllocator *tempAllocator) {
  X86_64TempAccess *access = (X86_64TempAccess *)baseAccess;

  return TEMP(access->tempNum, access->base.size, access->base.alignment,
              access->base.kind);
}
static void x86_64TempAccessStore(Access *baseAccess, IREntryVector *code,
                                  IROperand *input,
                                  TempAllocator *tempAllocator) {
  X86_64TempAccess *access = (X86_64TempAccess *)baseAccess;

  IR(code, MOVE(access->base.size,
                TEMP(access->tempNum, access->base.size, access->base.alignment,
                     access->base.kind),
                input));
}
AccessVTable *X86_64TempAccessVTable = NULL;
static AccessVTable *getX86_64TempAccessVTable(void) {
  if (X86_64TempAccessVTable == NULL) {
    X86_64TempAccessVTable = malloc(sizeof(AccessVTable));
    X86_64TempAccessVTable->dtor = x86_64TempAccessDtor;
    X86_64TempAccessVTable->load = x86_64TempAccessLoad;
    X86_64TempAccessVTable->store = x86_64TempAccessStore;
    X86_64TempAccessVTable->addrof =
        (IROperand * (*)(Access *, IREntryVector *, TempAllocator *))
            invalidFunction;
    X86_64TempAccessVTable->getLabel = (char *(*)(Access *))invalidFunction;
  }
  return X86_64TempAccessVTable;
}
static Access *x86_64TempAccessCtor(size_t size, size_t alignment,
                                    AllocHint kind, size_t tempNum) {
  X86_64TempAccess *access = malloc(sizeof(X86_64TempAccess));
  access->base.vtable = getX86_64TempAccessVTable();
  access->base.size = size;
  access->base.alignment = alignment;
  access->base.kind = kind;
  access->tempNum = tempNum;
  return (Access *)access;
}

typedef struct X86_64RegAccess {
  Access base;
  size_t regNum;
} X86_64RegAccess;
static void x86_64RegAccessDtor(Access *baseAccess) {
  X86_64RegAccess *access = (X86_64RegAccess *)baseAccess;
  free(access);
}
static IROperand *x86_64RegAccessLoad(Access *baseAccess, IREntryVector *code,
                                      TempAllocator *tempAllocator) {
  X86_64RegAccess *access = (X86_64RegAccess *)baseAccess;

  return REG(access->regNum);
}
static void x86_64RegAccessStore(Access *baseAccess, IREntryVector *code,
                                 IROperand *input,
                                 TempAllocator *tempAllocator) {
  X86_64RegAccess *access = (X86_64RegAccess *)baseAccess;

  IR(code, MOVE(access->base.size, REG(access->regNum), input));
}
AccessVTable *X86_64RegAccessVTable = NULL;
static AccessVTable *getX86_64RegAccessVTable(void) {
  if (X86_64RegAccessVTable == NULL) {
    X86_64RegAccessVTable = malloc(sizeof(AccessVTable));
    X86_64RegAccessVTable->dtor = x86_64RegAccessDtor;
    X86_64RegAccessVTable->load = x86_64RegAccessLoad;
    X86_64RegAccessVTable->store = x86_64RegAccessStore;
    X86_64RegAccessVTable->addrof =
        (IROperand * (*)(Access *, IREntryVector *, TempAllocator *))
            invalidFunction;
    X86_64RegAccessVTable->getLabel = (char *(*)(Access *))invalidFunction;
  }
  return X86_64RegAccessVTable;
}
static Access *x86_64RegAccessCtor(size_t size, size_t alignment,
                                   AllocHint kind, size_t regNum) {
  X86_64RegAccess *access = malloc(sizeof(X86_64RegAccess));
  access->base.vtable = getX86_64RegAccessVTable();
  access->base.size = size;
  access->base.alignment = alignment;
  access->base.kind = kind;
  access->regNum = regNum;
  return (Access *)access;
}

typedef struct X86_64StackAccess {
  Access base;
  int64_t bpOffset;
} X86_64StackAccess;
static void x86_64StackAccessDtor(Access *baseAccess) {
  X86_64StackAccess *access = (X86_64StackAccess *)baseAccess;
  free(access);
}
static IROperand *x86_64StackAccessLoad(Access *baseAccess, IREntryVector *code,
                                        TempAllocator *tempAllocator) {
  X86_64StackAccess *access = (X86_64StackAccess *)baseAccess;

  size_t result = NEW(tempAllocator);
  IR(code, STACK_LOAD(access->base.size,
                      TEMP(result, access->base.size, access->base.alignment,
                           access->base.kind),
                      access->bpOffset));
  return TEMP(result, access->base.size, access->base.alignment,
              access->base.kind);
}
static void x86_64StackAccessStore(Access *baseAccess, IREntryVector *code,
                                   IROperand *input,
                                   TempAllocator *tempAllocator) {
  X86_64StackAccess *access = (X86_64StackAccess *)baseAccess;

  IR(code, STACK_STORE(access->base.size, access->bpOffset, input));
}
static IROperand *x86_64StackAccessAddrof(Access *baseAccess,
                                          IREntryVector *code,
                                          TempAllocator *tempAllocator) {
  X86_64StackAccess *access = (X86_64StackAccess *)baseAccess;

  size_t address = NEW(tempAllocator);
  IR(code, BINOP(POINTER_WIDTH, IO_ADD,
                 TEMP(address, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                 REG(X86_64_RBP), STACKOFFSET(access->bpOffset)));
  return TEMP(address, access->base.size, access->base.alignment,
              access->base.kind);
}
AccessVTable *X86_64StackAccessVTable = NULL;
static AccessVTable *getX86_64StackAccessVTable(void) {
  if (X86_64StackAccessVTable == NULL) {
    X86_64StackAccessVTable = malloc(sizeof(AccessVTable));
    X86_64StackAccessVTable->dtor = x86_64StackAccessDtor;
    X86_64StackAccessVTable->load = x86_64StackAccessLoad;
    X86_64StackAccessVTable->store = x86_64StackAccessStore;
    X86_64StackAccessVTable->addrof = x86_64StackAccessAddrof;
    X86_64StackAccessVTable->getLabel = (char *(*)(Access *))invalidFunction;
  }
  return X86_64StackAccessVTable;
}
static Access *x86_64StackAccessCtor(size_t size, size_t alignment,
                                     AllocHint kind, int64_t bpOffset) {
  X86_64StackAccess *access = malloc(sizeof(X86_64StackAccess));
  access->base.vtable = getX86_64StackAccessVTable();
  access->base.size = size;
  access->base.alignment = alignment;
  access->base.kind = kind;
  access->bpOffset = bpOffset;
  return (Access *)access;
}

typedef struct X86_64FunctionAccess {
  Access base;
  char *name;
} X86_64FunctionAccess;
static void x86_64FunctionAccessDtor(Access *baseAccess) {
  X86_64FunctionAccess *access = (X86_64FunctionAccess *)baseAccess;
  free(access->name);
  free(access);
}
static IROperand *x86_64FunctionAccessLoad(Access *baseAccess,
                                           IREntryVector *code,
                                           TempAllocator *tempAllocator) {
  X86_64FunctionAccess *access = (X86_64FunctionAccess *)baseAccess;

  return NAME(strdup(access->name));
}
static char *x86_64FunctionAccessGetLabel(Access *baseAccess) {
  X86_64FunctionAccess *access = (X86_64FunctionAccess *)baseAccess;
  return strdup(access->name);
}
AccessVTable *X86_64FunctionAccessVTable = NULL;
static AccessVTable *getX86_64FunctionAccessVTable(void) {
  if (X86_64FunctionAccessVTable == NULL) {
    X86_64FunctionAccessVTable = malloc(sizeof(AccessVTable));
    X86_64FunctionAccessVTable->dtor = x86_64FunctionAccessDtor;
    X86_64FunctionAccessVTable->load = x86_64FunctionAccessLoad;
    X86_64FunctionAccessVTable->store =
        (void (*)(Access *, IREntryVector *, IROperand *,
                  TempAllocator *))invalidFunction;
    X86_64FunctionAccessVTable->addrof =
        (IROperand * (*)(Access *, IREntryVector *, TempAllocator *))
            invalidFunction;
    X86_64FunctionAccessVTable->getLabel = x86_64FunctionAccessGetLabel;
  }
  return X86_64FunctionAccessVTable;
}
Access *x86_64FunctionAccessCtor(char *name) {
  X86_64FunctionAccess *access = malloc(sizeof(X86_64FunctionAccess));
  access->base.vtable = getX86_64FunctionAccessVTable();
  access->base.size = POINTER_WIDTH;
  access->base.kind = AH_GP;
  access->name = name;
  return (Access *)access;
}

typedef struct {
  size_t scopeSize;
  IREntryVector *prologue;
  IREntryVector *epilogue;
} X86_64FrameScope;
static X86_64FrameScope *x86_64FrameScopeCreate(void) {
  X86_64FrameScope *scope = malloc(sizeof(X86_64FrameScope));
  scope->scopeSize = 0;
  scope->prologue = irEntryVectorCreate();
  scope->epilogue = irEntryVectorCreate();
  return scope;
}
static void x86_64FrameScopeDestroy(X86_64FrameScope *scope) {
  if (scope->prologue != NULL) {
    irEntryVectorDestroy(scope->prologue);
  }
  if (scope->epilogue != NULL) {
    irEntryVectorDestroy(scope->epilogue);
  }
  free(scope);
}

typedef Stack X86_64FrameScopeStack;
static void x86_64FrameScopeStackInit(X86_64FrameScopeStack *stack) {
  stackInit(stack);
}
static void x86_64FrameScopeStackPush(X86_64FrameScopeStack *stack,
                                      X86_64FrameScope *scope) {
  stackPush(stack, scope);
}
static X86_64FrameScope *x86_64FrameScopeStackPeek(
    X86_64FrameScopeStack *stack) {
  return stackPeek(stack);
}
static X86_64FrameScope *x86_64FrameScopeStackPop(
    X86_64FrameScopeStack *stack) {
  return stackPop(stack);
}
static void x86_64FrameScopeStackUninit(X86_64FrameScopeStack *stack) {
  stackUninit(stack, (void (*)(void *))x86_64FrameScopeDestroy);
}

typedef enum {
  X86_64_PC_NO_CLASS,
  X86_64_PC_POINTER,
  X86_64_PC_INTEGER,
  X86_64_PC_SSE,
  X86_64_PC_MEMORY,
} X86_64ParamClass;

size_t const MAX_GP_ARGS = 6;
X86_64Register const GP_ARG_REGISTERS[] = {
    X86_64_RDI, X86_64_RSI, X86_64_RDX, X86_64_RCX, X86_64_R8, X86_64_R9,
};
size_t const MAX_SSE_ARGS = 8;
X86_64Register const SSE_ARG_REGISTERS[] = {
    X86_64_XMM0, X86_64_XMM1, X86_64_XMM2, X86_64_XMM3,
    X86_64_XMM4, X86_64_XMM5, X86_64_XMM6, X86_64_XMM7,
};

typedef struct X86_64Frame {
  Frame base;

  size_t nextGPArg;
  size_t nextSSEArg;
  int64_t nextMemArg;

  int64_t bpOffset;
  size_t frameSize;

  IREntryVector *functionPrologue;
  IREntryVector *functionEpilogue;

  X86_64FrameScopeStack scopes;
} X86_64Frame;
FrameVTable *X86_64FrameVTable = NULL;
// assumes that the frame is an x86_64 frame
static void x86_64FrameDtor(Frame *baseFrame) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;

  free(frame->base.name);

  if (frame->functionPrologue != NULL) {
    irEntryVectorDestroy(frame->functionPrologue);
  }
  if (frame->functionEpilogue != NULL) {
    irEntryVectorDestroy(frame->functionEpilogue);
  }

  x86_64FrameScopeStackUninit(&frame->scopes);

  free(frame);
}
static int64_t x86_64AllocStack(X86_64Frame *frame, size_t size) {
  // FIXME: should refer to the top most frame
  // size must not be too large
  if (size > INT64_MAX) {
    error(__FILE__, __LINE__, "overly large stack allocation requested");
  }
  int64_t retVal = frame->bpOffset;
  frame->bpOffset -= (int64_t)size;
  frame->frameSize += size;
  return retVal;
}
static void x86_64FrameAlignTo(X86_64Frame *frame, size_t unsignedAlignment) {
  // FIXME: should refer to the top most frame
  // alignment is, at most, REGISTER_WIDTH
  int64_t alignment = (int64_t)unsignedAlignment;
  int64_t diff = frame->bpOffset % alignment;
  int64_t offset =
      (alignment + diff) % alignment;  // how much to take away from bpOffset,
                                       // how much to add to frameSize
                                       // is positive
  frame->bpOffset -= offset;
  frame->frameSize += (uint64_t)offset;
}
static Access *x86_64PassGP(X86_64Frame *frame, size_t size, bool escapes,
                            TempAllocator *tempAllocator) {
  // alignment == size for all T GP types
  if (frame->nextGPArg >= MAX_GP_ARGS) {
    // comes in via stack @ nextMemArg
    if (escapes) {
      // put it in aligned mem
      x86_64FrameAlignTo(frame, size);
      int64_t offset = x86_64AllocStack(frame, size);
      size_t temp = NEW(tempAllocator);
      IR(frame->functionPrologue,
         STACK_LOAD(size, TEMP(temp, size, size, AH_GP), frame->nextMemArg));
      frame->nextMemArg += 8;
      IR(frame->functionPrologue,
         STACK_STORE(size, offset, TEMP(temp, size, size, AH_GP)));
      return x86_64StackAccessCtor(size, size, AH_GP, offset);
    } else {
      // put it in a GP temp
      size_t destTemp = NEW(tempAllocator);
      STACK_LOAD(size, TEMP(destTemp, size, size, AH_GP), frame->nextMemArg);
      frame->nextMemArg += 8;
      return x86_64TempAccessCtor(size, size, AH_GP, destTemp);
    }
  } else {
    size_t regNum = GP_ARG_REGISTERS[frame->nextGPArg++];
    // comes in via reg n, needs to be put in a temp or in mem
    if (escapes) {
      // put it in aligned mem
      x86_64FrameAlignTo(frame, size);
      int64_t offset = x86_64AllocStack(frame, size);
      IR(frame->functionPrologue, STACK_STORE(size, offset, REG(regNum)));
      return x86_64StackAccessCtor(size, size, AH_GP, offset);
    } else {
      // put it in a GP temp
      size_t destTemp = NEW(tempAllocator);
      IR(frame->functionPrologue,
         MOVE(size, TEMP(destTemp, size, size, AH_GP), REG(regNum)));
      return x86_64TempAccessCtor(size, size, AH_GP, destTemp);
    }
  }
}
static Access *x86_64PassSSE(X86_64Frame *frame, size_t size, bool escapes,
                             TempAllocator *tempAllocator) {
  // alignof == sizeof for all T SSE types
  if (frame->nextSSEArg >= MAX_SSE_ARGS) {
    // comes in via stack @ nextMemArg
    if (escapes) {
      // put it in aligned mem
      x86_64FrameAlignTo(frame, size);
      int64_t offset = x86_64AllocStack(frame, size);
      size_t temp = NEW(tempAllocator);
      IR(frame->functionPrologue,
         STACK_LOAD(size, TEMP(temp, size, size, AH_SSE), frame->nextMemArg));
      frame->nextMemArg += 8;
      IR(frame->functionPrologue,
         STACK_STORE(size, offset, TEMP(temp, size, size, AH_SSE)));
      return x86_64StackAccessCtor(size, size, AH_SSE, offset);
    } else {
      // put it in a GP temp
      size_t destTemp = NEW(tempAllocator);
      STACK_LOAD(size, TEMP(destTemp, size, size, AH_SSE), frame->nextMemArg);
      frame->nextMemArg += 8;
      return x86_64TempAccessCtor(size, size, AH_SSE, destTemp);
    }
  } else {
    size_t regNum = SSE_ARG_REGISTERS[frame->nextSSEArg++];
    // comes in via reg n, needs to be put in a temp or in mem
    if (escapes) {
      // put it in aligned mem
      x86_64FrameAlignTo(frame, size);
      int64_t offset = x86_64AllocStack(frame, size);
      IR(frame->functionPrologue, STACK_STORE(size, offset, REG(regNum)));
      return x86_64StackAccessCtor(size, size, AH_SSE, offset);
    } else {
      // put it in a GP temps
      size_t destTemp = NEW(tempAllocator);
      IR(frame->functionPrologue,
         MOVE(size, TEMP(destTemp, size, size, AH_SSE), REG(regNum)));
      return x86_64TempAccessCtor(size, size, AH_SSE, destTemp);
    }
  }
}
static Access *x86_64AllocArg(Frame *baseFrame, Type const *type, bool escapes,
                              TempAllocator *tempAllocator) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL: {
      // INTEGER
      return x86_64PassGP(frame, BYTE_WIDTH, escapes, tempAllocator);
    }
    case K_CHAR: {
      // INTEGER
      return x86_64PassGP(frame, CHAR_WIDTH, escapes, tempAllocator);
    }
    case K_USHORT:
    case K_SHORT: {
      // INTEGER
      return x86_64PassGP(frame, SHORT_WIDTH, escapes, tempAllocator);
    }
    case K_UINT:
    case K_INT: {
      // INTEGER
      return x86_64PassGP(frame, INT_WIDTH, escapes, tempAllocator);
    }
    case K_WCHAR: {
      // INTEGER
      return x86_64PassGP(frame, WCHAR_WIDTH, escapes, tempAllocator);
    }
    case K_ULONG:
    case K_LONG: {
      // INTEGER
      return x86_64PassGP(frame, LONG_WIDTH, escapes, tempAllocator);
    }
    case K_FLOAT: {
      // SSE
      return x86_64PassSSE(frame, FLOAT_WIDTH, escapes, tempAllocator);
    }
    case K_DOUBLE: {
      // SSE
      return x86_64PassSSE(frame, DOUBLE_WIDTH, escapes, tempAllocator);
    }
    case K_STRUCT: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_UNION: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_ENUM: {
      return x86_64PassGP(frame, typeSizeof(type), escapes, tempAllocator);
    }
    case K_TYPEDEF: {
      // Pass the actual type
      return x86_64AllocArg(
          baseFrame,
          type->data.reference.referenced->data.type.data.typedefType.type,
          escapes, tempAllocator);
    }
    case K_CONST: {
      // Pass the actual type
      return x86_64AllocArg(baseFrame, type->data.modifier.type, escapes,
                            tempAllocator);
    }
    case K_ARRAY: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_PTR:
    case K_FUNCTION_PTR: {
      // POINTER
      return x86_64PassGP(frame, POINTER_WIDTH, escapes, tempAllocator);
    }
    default: { error(__FILE__, __LINE__, "invalid type given to allocArg"); }
  }
}
static Access *x86_64AllocLocalMem(X86_64Frame *frame, size_t size,
                                   size_t alignment, AllocHint kind,
                                   TempAllocator *tempAllocator) {
  x86_64FrameAlignTo(frame, alignment);
  int64_t offset = x86_64AllocStack(frame, size);
  return x86_64StackAccessCtor(size, alignment, kind, offset);
}
static Access *x86_64AllocLocalTemp(X86_64Frame *frame, size_t size,
                                    AllocHint kind,
                                    TempAllocator *tempAllocator) {
  // alignof == sizeof for all T GP and SSE types
  return x86_64TempAccessCtor(size, size, kind, NEW(tempAllocator));
}
static Access *x86_64AllocLocal(Frame *baseFrame, Type const *type,
                                bool escapes, TempAllocator *tempAllocator) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, BYTE_WIDTH, BYTE_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, BYTE_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_CHAR: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, CHAR_WIDTH, CHAR_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, CHAR_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_USHORT:
    case K_SHORT: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, SHORT_WIDTH, SHORT_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, SHORT_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_UINT:
    case K_INT: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, INT_WIDTH, INT_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, INT_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_WCHAR: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, INT_WIDTH, INT_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, INT_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_ULONG:
    case K_LONG: {
      // INTEGER
      if (escapes) {
        return x86_64AllocLocalMem(frame, LONG_WIDTH, LONG_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, LONG_WIDTH, AH_GP, tempAllocator);
      }
    }
    case K_FLOAT: {
      // SSE
      if (escapes) {
        return x86_64AllocLocalMem(frame, FLOAT_WIDTH, FLOAT_WIDTH, AH_SSE,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, FLOAT_WIDTH, AH_SSE, tempAllocator);
      }
    }
    case K_DOUBLE: {
      // SSE
      if (escapes) {
        return x86_64AllocLocalMem(frame, DOUBLE_WIDTH, DOUBLE_WIDTH, AH_SSE,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, DOUBLE_WIDTH, AH_SSE, tempAllocator);
      }
    }
    case K_STRUCT: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_UNION: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_ENUM: {
      // INTEGER
      size_t size = typeSizeof(type);
      if (escapes) {
        return x86_64AllocLocalMem(frame, size, size, AH_GP, tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, size, AH_GP, tempAllocator);
      }
    }
    case K_TYPEDEF: {
      // Pass the actual type
      return x86_64AllocLocal(
          baseFrame,
          type->data.reference.referenced->data.type.data.typedefType.type,
          escapes, tempAllocator);
    }
    case K_CONST: {
      // Pass the actual type
      return x86_64AllocLocal(baseFrame, type->data.modifier.type, escapes,
                              tempAllocator);
    }
    case K_ARRAY: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_PTR:
    case K_FUNCTION_PTR: {
      // POINTER
      if (escapes) {
        return x86_64AllocLocalMem(frame, POINTER_WIDTH, POINTER_WIDTH, AH_GP,
                                   tempAllocator);
      } else {
        return x86_64AllocLocalTemp(frame, POINTER_WIDTH, AH_GP, tempAllocator);
      }
    }
    default: { error(__FILE__, __LINE__, "invalid type given to allocLocal"); }
  }
}
static Access *x86_64ReturnGP(X86_64Frame *frame, size_t size) {
  // alignof == sizeof for all T GP types
  return x86_64RegAccessCtor(size, size, AH_GP, X86_64_RAX);
}
static Access *x86_64ReturnSSE(X86_64Frame *frame, size_t size) {
  // alignof == sizeof for all T SSE types
  return x86_64RegAccessCtor(size, size, AH_SSE, X86_64_XMM0);
}
static Access *x86_64AllocRetVal(Frame *baseFrame, Type const *type,
                                 TempAllocator *tempAllocator) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL: {
      // INTEGER
      return x86_64ReturnGP(frame, BYTE_WIDTH);
    }
    case K_CHAR: {
      // INTEGER
      return x86_64ReturnGP(frame, BYTE_WIDTH);
    }
    case K_USHORT:
    case K_SHORT: {
      // INTEGER
      return x86_64ReturnGP(frame, SHORT_WIDTH);
    }
    case K_UINT:
    case K_INT: {
      // INTEGER
      return x86_64ReturnGP(frame, INT_WIDTH);
    }
    case K_WCHAR: {
      // INTEGER
      return x86_64ReturnGP(frame, WCHAR_WIDTH);
    }
    case K_ULONG:
    case K_LONG: {
      // INTEGER
      return x86_64ReturnGP(frame, LONG_WIDTH);
    }
    case K_FLOAT: {
      // SSE
      return x86_64ReturnSSE(frame, FLOAT_WIDTH);
    }
    case K_DOUBLE: {
      // SSE
      return x86_64ReturnSSE(frame, DOUBLE_WIDTH);
    }
    case K_STRUCT: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_UNION: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_ENUM: {
      return x86_64ReturnGP(frame, typeSizeof(type));
    }
    case K_TYPEDEF: {
      // Pass the actual type
      return x86_64AllocRetVal(
          baseFrame,
          type->data.reference.referenced->data.type.data.typedefType.type,
          tempAllocator);
    }
    case K_CONST: {
      // Pass the actual type
      return x86_64AllocRetVal(baseFrame, type->data.modifier.type,
                               tempAllocator);
    }
    case K_ARRAY: {
      // Composite
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
    }
    case K_PTR:
    case K_FUNCTION_PTR: {
      // POINTER
      return x86_64ReturnGP(frame, POINTER_WIDTH);
    }
    default: { error(__FILE__, __LINE__, "invalid type given to allocRetVal"); }
  }
}
static void x86_64ScopeStart(Frame *baseFrame) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;
  x86_64FrameScopeStackPush(&frame->scopes, x86_64FrameScopeCreate());
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static IREntryVector *x86_64ScopeEnd(Frame *baseFrame, IREntryVector *body,
                                     TempAllocator *tempAllocator) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;

  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static IROperand *x86_64IndirectCall(Frame *this, IROperand *who,
                                     IROperandVector *actualArgs,
                                     Type const *functionType,
                                     IREntryVector *out,
                                     TempAllocator *tempAllocator) {
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static IROperand *x86_64DirectCall(Frame *this, char *who,
                                   IROperandVector *actualArgs,
                                   OverloadSetElement const *function,
                                   IREntryVector *out,
                                   TempAllocator *tempAllocator) {
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static FrameVTable *getX86_64FrameVTable(void) {
  if (X86_64FrameVTable == NULL) {
    X86_64FrameVTable = malloc(sizeof(FrameVTable));
    X86_64FrameVTable->dtor = x86_64FrameDtor;
    X86_64FrameVTable->allocArg = x86_64AllocArg;
    X86_64FrameVTable->allocLocal = x86_64AllocLocal;
    X86_64FrameVTable->allocRetVal = x86_64AllocRetVal;
    X86_64FrameVTable->scopeStart = x86_64ScopeStart;
    X86_64FrameVTable->scopeEnd = x86_64ScopeEnd;
    X86_64FrameVTable->indirectCall = x86_64IndirectCall;
    X86_64FrameVTable->directCall = x86_64DirectCall;
  }
  return X86_64FrameVTable;
}
Frame *x86_64FrameCtor(char *name) {
  X86_64Frame *frame = malloc(sizeof(X86_64Frame));
  frame->base.vtable = getX86_64FrameVTable();
  frame->base.name = name;

  frame->nextGPArg = 0;
  frame->nextSSEArg = 0;
  frame->nextMemArg = 16;

  frame->bpOffset = -8;
  frame->frameSize = 0;

  frame->functionPrologue = irEntryVectorCreate();
  frame->functionEpilogue = irEntryVectorCreate();

  x86_64FrameScopeStackInit(&frame->scopes);
  return (Frame *)frame;
}

typedef struct X86_64LabelGenerator {
  LabelGenerator base;
  size_t nextCode;
  size_t nextData;
} X86_64LabelGenerator;
static void x86_64LabelGeneratorDtor(LabelGenerator *baseGenerator) {
  X86_64LabelGenerator *generator = (X86_64LabelGenerator *)baseGenerator;
  free(generator);
}
static char *x86_64LabelGeneratorGenerateCodeLabel(
    LabelGenerator *baseGenerator) {
  X86_64LabelGenerator *generator = (X86_64LabelGenerator *)baseGenerator;
  return format(".L%zu", generator->nextCode++);
}
static char *x86_64LabelGeneratorGenerateDataLabel(
    LabelGenerator *baseGenerator) {
  X86_64LabelGenerator *generator = (X86_64LabelGenerator *)baseGenerator;
  return format(".LC%zu", generator->nextData++);
}
LabelGeneratorVTable *X86_64LabelGeneratorVTable = NULL;
static LabelGeneratorVTable *getX86_64LabelGeneratorVTable(void) {
  if (X86_64LabelGeneratorVTable == NULL) {
    X86_64LabelGeneratorVTable = malloc(sizeof(LabelGeneratorVTable));
    X86_64LabelGeneratorVTable->dtor = x86_64LabelGeneratorDtor;
    X86_64LabelGeneratorVTable->generateCodeLabel =
        x86_64LabelGeneratorGenerateCodeLabel;
    X86_64LabelGeneratorVTable->generateDataLabel =
        x86_64LabelGeneratorGenerateDataLabel;
  }
  return X86_64LabelGeneratorVTable;
}
LabelGenerator *x86_64LabelGeneratorCtor(void) {
  X86_64LabelGenerator *generator = malloc(sizeof(X86_64LabelGenerator));
  generator->base.vtable = getX86_64LabelGeneratorVTable();
  generator->nextCode = 0;
  generator->nextData = 0;
  return (LabelGenerator *)generator;
}