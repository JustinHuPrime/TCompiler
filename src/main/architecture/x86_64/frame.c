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
#include "util/container/optimization.h"
#include "util/functional.h"

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
static IROperand *x86_64GlobalAccessLoad(Access *baseAccess, IRVector *code,
                                         TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  size_t result = NEW(tempAllocator);
  IR(code, LOAD(access->base.size, TEMP(result, access->base.kind),
                LABEL(strdup(access->label))));
  return TEMP(result, access->base.kind);
}
static void x86_64GlobalAccessStore(Access *baseAccess, IRVector *code,
                                    IROperand *input,
                                    TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  IR(code, STORE(access->base.size, LABEL(strdup(access->label)), input));
}
static IROperand *x86_64GlobalAccessAddrof(Access *baseAccess, IRVector *code,
                                           TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;

  return LABEL(strdup(access->label));
}
AccessVTable *X86_64GlobalAccessVTable = NULL;
static AccessVTable *getX86_64GlobalAccessVTable(void) {
  if (X86_64GlobalAccessVTable == NULL) {
    X86_64GlobalAccessVTable = malloc(sizeof(AccessVTable));
    X86_64GlobalAccessVTable->dtor = x86_64GlobalAccessDtor;
    X86_64GlobalAccessVTable->load = x86_64GlobalAccessLoad;
    X86_64GlobalAccessVTable->store = x86_64GlobalAccessStore;
    X86_64GlobalAccessVTable->addrof = x86_64GlobalAccessAddrof;
  }
  return X86_64GlobalAccessVTable;
}
Access *x86_64GlobalAccessCtor(size_t size, AllocHint kind, char *label) {
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
static IROperand *x86_64TempAccessLoad(Access *baseAccess, IRVector *code,
                                       TempAllocator *tempAllocator) {
  X86_64TempAccess *access = (X86_64TempAccess *)baseAccess;

  return TEMP(access->tempNum, access->base.kind);
}
static void x86_64TempAccessStore(Access *baseAccess, IRVector *code,
                                  IROperand *input,
                                  TempAllocator *tempAllocator) {
  X86_64TempAccess *access = (X86_64TempAccess *)baseAccess;

  IR(code,
     MOVE(access->base.size, TEMP(access->tempNum, access->base.kind), input));
}
AccessVTable *X86_64TempAccessVTable = NULL;
static AccessVTable *getX86_64TempAccessVTable(void) {
  if (X86_64TempAccessVTable == NULL) {
    X86_64TempAccessVTable = malloc(sizeof(AccessVTable));
    X86_64TempAccessVTable->dtor = x86_64TempAccessDtor;
    X86_64TempAccessVTable->load = x86_64TempAccessLoad;
    X86_64TempAccessVTable->store = x86_64TempAccessStore;
    X86_64TempAccessVTable->addrof =
        (IROperand * (*)(Access *, IRVector *, TempAllocator *))
            invalidFunction;
  }
  return X86_64TempAccessVTable;
}
static Access *x86_64TempAccessCtor(size_t size, AllocHint kind,
                                    size_t tempNum) {
  X86_64TempAccess *access = malloc(sizeof(X86_64TempAccess));
  access->base.vtable = getX86_64TempAccessVTable();
  access->base.size = size;
  access->base.kind = kind;
  access->tempNum = tempNum;
  return (Access *)access;
}

typedef struct X86_64MemoryAccess {
  Access base;
  size_t bpOffset;
} X86_64MemoryAccess;
static void x86_64MemoryAccessDtor(Access *baseAccess) {
  X86_64MemoryAccess *access = (X86_64MemoryAccess *)baseAccess;
  free(access);
}
static IROperand *x86_64MemoryAccessLoad(Access *baseAccess, IRVector *code,
                                         TempAllocator *tempAllocator) {
  X86_64MemoryAccess *access = (X86_64MemoryAccess *)baseAccess;

  size_t address = NEW(tempAllocator);
  size_t result = NEW(tempAllocator);
  IR(code, BINOP(POINTER_WIDTH, IO_ADD, TEMP(address, AH_GP), REG(X86_64_RBP),
                 ULONG(access->bpOffset)));
  IR(code, LOAD(access->base.size, TEMP(result, access->base.kind),
                TEMP(address, AH_GP)));
  return TEMP(result, access->base.kind);
}
static void x86_64MemoryAccessStore(Access *baseAccess, IRVector *code,
                                    IROperand *input,
                                    TempAllocator *tempAllocator) {
  X86_64MemoryAccess *access = (X86_64MemoryAccess *)baseAccess;

  size_t address = NEW(tempAllocator);
  IR(code, BINOP(POINTER_WIDTH, IO_ADD, TEMP(address, AH_GP), REG(X86_64_RBP),
                 ULONG(access->bpOffset)));
  IR(code, STORE(access->base.size, TEMP(address, AH_GP), input));
}
static IROperand *x86_64MemoryAccessAddrof(Access *baseAccess, IRVector *code,
                                           TempAllocator *tempAllocator) {
  X86_64MemoryAccess *access = (X86_64MemoryAccess *)baseAccess;

  size_t address = NEW(tempAllocator);
  IR(code, BINOP(POINTER_WIDTH, IO_ADD, TEMP(address, AH_GP), REG(X86_64_RBP),
                 ULONG(access->bpOffset)));
  return TEMP(address, access->base.kind);
}
AccessVTable *X86_64MemoryAccessVTable = NULL;
static AccessVTable *getX86_64MemoryAccessVTable(void) {
  if (X86_64MemoryAccessVTable == NULL) {
    X86_64MemoryAccessVTable = malloc(sizeof(AccessVTable));
    X86_64MemoryAccessVTable->dtor = x86_64MemoryAccessDtor;
    X86_64MemoryAccessVTable->load = x86_64MemoryAccessLoad;
    X86_64MemoryAccessVTable->store = x86_64MemoryAccessStore;
    X86_64MemoryAccessVTable->addrof = x86_64MemoryAccessAddrof;
  }
  return X86_64MemoryAccessVTable;
}
static Access *x86_64MemoryAccessCtor(size_t size, AllocHint kind,
                                      size_t bpOffset) {
  X86_64MemoryAccess *access = malloc(sizeof(X86_64MemoryAccess));
  access->base.vtable = getX86_64MemoryAccessVTable();
  access->base.size = size;
  access->base.kind = kind;
  access->bpOffset = bpOffset;
  return (Access *)access;
}

typedef struct X86_64Frame {
  Frame frame;
  IRVector prologue;
  IRVector epilogue;
} X86_64Frame;
FrameVTable *X86_64VTable = NULL;

// assumes that the frame is an x86_64 frame
static void x86_64FrameDtor(Frame *baseFrame) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;
  irVectorUninit(&frame->prologue);
  irVectorUninit(&frame->epilogue);
  free(frame);
}
static Access *x86_64AllocArg(Frame *baseFrame, Type const *type, bool escapes,
                              TempAllocator *tempAllocator) {
  return NULL;  // TODO: write this
}
static Access *x86_64AllocLocal(Frame *baseFrame, Type const *type,
                                bool escapes, TempAllocator *tempAllocator) {
  return NULL;  // TODO: write this
}
static Access *x86_64AllocRetVal(Frame *baseFrame, Type const *type,
                                 TempAllocator *tempAllocator) {
  return NULL;  // TODO: write this
}
static void x86_64WrapBody(Frame *baseFrame, IRVector *out) {
  X86_64Frame *frame = (X86_64Frame *)baseFrame;

  IREntry **oldBody = (IREntry **)out->elements;
  size_t oldSize = out->size;

  out->size += frame->prologue.size + frame->epilogue.size;
  while (out->capacity < out->size) {
    out->capacity *= 2;
  }

  out->elements = malloc(sizeof(out->capacity * sizeof(IREntry *)));
  IREntry **writingPtr = (IREntry **)out->elements;

  memcpy(writingPtr, frame->prologue.elements,
         frame->prologue.size * sizeof(IREntry *));
  writingPtr += frame->prologue.size;
  irVectorInit(&frame->prologue);

  memcpy(writingPtr, oldBody, oldSize * sizeof(IREntry *));
  writingPtr += oldSize;
  free(oldBody);

  memcpy(writingPtr, frame->epilogue.elements,
         frame->epilogue.size * sizeof(IREntry *));
  irVectorInit(&frame->epilogue);
}
static FrameVTable *getX86_64VTable(void) {
  if (X86_64VTable == NULL) {
    X86_64VTable = malloc(sizeof(FrameVTable));
    X86_64VTable->dtor = x86_64FrameDtor;
    X86_64VTable->allocArg = x86_64AllocArg;
    X86_64VTable->allocLocal = x86_64AllocLocal;
    X86_64VTable->allocRetVal = x86_64AllocRetVal;
    X86_64VTable->wrapBody = x86_64WrapBody;
  }
  return X86_64VTable;
}
Frame *x86_64FrameCtor(void) {
  X86_64Frame *frame = malloc(sizeof(X86_64Frame));
  frame->frame.vtable = getX86_64VTable();
  irVectorInit(&frame->prologue);
  irVectorInit(&frame->epilogue);
  return (Frame *)frame;
}
