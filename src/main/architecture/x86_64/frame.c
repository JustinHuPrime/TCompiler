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

#include "ir/frame.h"
#include "ir/ir.h"
#include "typecheck/symbolTable.h"
#include "util/container/optimization.h"

#include <stdlib.h>
#include <string.h>

typedef struct X86_64GlobalAccess {
  Access access;
  char *label;
} X86_64GlobalAccess;
AccessVTable *X86_64GlobalAccessVTable = NULL;
static void x86_64GlobalAccessDtor(Access *baseAccess) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;
  free(access->label);
  free(access);
}
static IROperand *x86_64GlobalAccessLoad(Access *baseAccess, IRVector *code,
                                         TempAllocator *tempAllocator) {
  X86_64GlobalAccess *access = (X86_64GlobalAccess *)baseAccess;
  return NULL;  // TODO: write this
}
static AccessVTable *getX86_64GlobalAccessVTable(void) {
  if (X86_64GlobalAccessVTable == NULL) {
    X86_64GlobalAccessVTable = malloc(sizeof(AccessVTable));
    X86_64GlobalAccessVTable->dtor = x86_64GlobalAccessDtor;
    X86_64GlobalAccessVTable->load = x86_64GlobalAccessLoad;
    X86_64GlobalAccessVTable->store;
    X86_64GlobalAccessVTable->addrof;
  }
  return X86_64GlobalAccessVTable;
}
Access *x86_64GlobalAccessCtor(char *label) {
  X86_64GlobalAccess *access = malloc(sizeof(X86_64GlobalAccess));
  access->access.vtable = getX86_64GlobalAccessVTable();
  access->label = label;
  return (Access *)access;
}

typedef struct X86_64TempAccess {
  Access access;
  size_t tempNum;
} X86_64TempAccess;
AccessVTable *X86_64TempAccessVTable = NULL;
static AccessVTable *getX86_64TempAccessVTable(void) {
  if (X86_64TempAccessVTable == NULL) {
    X86_64TempAccessVTable = malloc(sizeof(AccessVTable));
    X86_64TempAccessVTable->dtor;
    X86_64TempAccessVTable->load;
    X86_64TempAccessVTable->store;
    X86_64TempAccessVTable->addrof;
  }
  return X86_64TempAccessVTable;
}

typedef struct X86_64MemoryAccess {
  Access access;
} X86_64MemoryAccess;
AccessVTable *X86_64MemoryAccessVTable = NULL;
static AccessVTable *getX86_64MemoryAccessVTable(void) {
  if (X86_64MemoryAccessVTable == NULL) {
    X86_64MemoryAccessVTable = malloc(sizeof(AccessVTable));
    X86_64MemoryAccessVTable->dtor;
    X86_64MemoryAccessVTable->load;
    X86_64MemoryAccessVTable->store;
    X86_64MemoryAccessVTable->addrof;
  }
  return X86_64MemoryAccessVTable;
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
