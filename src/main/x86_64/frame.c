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

// x86_64 frame implementation

#include "x86_64/frame.h"

#include "ir/ir.h"
#include "util/format.h"
#include "util/internalError.h"
#include "typecheck/symbolTable.h"

#include <stdlib.h>

typedef struct {
  Frame base;
  size_t numIntArgs;
  size_t numFloatArgs;
  size_t numMemArgs;
  size_t outArgStackSize;
} X86_64Frame;

typedef struct {
  Access base;
  char *labelName;
  bool labelOwned;
} X86_64GlobalAccess;
typedef struct {
  Access base;
  int64_t basePointerOffset;
} X86_64MemoryAccess;
typedef struct {
  Access base;
  size_t registerNumber;
} X86_64RegisterAccess;

typedef struct {
  LabelGenerator base;
  size_t nextLabel;
} X86_64LabelGenerator;

typedef enum {
  // general purpose registers
  X86_64_RAX,
  X86_64_RBX,
  X86_64_RCX,
  X86_64_RDX,
  X86_64_RDI,
  X86_64_RSI,
  X86_64_RBP,
  X86_64_RSP,
  X86_64_R8,
  X86_64_R9,
  X86_64_R10,
  X86_64_R11,
  X86_64_R12,
  X86_64_R13,
  X86_64_R14,
  X86_64_R15,

  // SSE registers - assuming basic x86-64 architecture, without MMX, AVX, or
  // AVX-512
  // also not using x87 FPU registers
  X86_64_XMM0,
  X86_64_XMM1,
  X86_64_XMM2,
  X86_64_XMM3,
  X86_64_XMM4,
  X86_64_XMM5,
  X86_64_XMM6,
  X86_64_XMM7,
  X86_64_XMM8,
  X86_64_XMM9,
  X86_64_XMM10,
  X86_64_XMM11,
  X86_64_XMM12,
  X86_64_XMM13,
  X86_64_XMM14,
  X86_64_XMM15,
} X86_64Register;
size_t const X86_64_INT_REGISTER_WIDTH = 8;

static void x86_64FrameDtor(X86_64Frame *frame) { free(frame); }
static IRStmVector *x86_64GenerateEntryExit(X86_64Frame *frame,
                                            IRStmVector *body,
                                            char *exitLabel) {
  notYetImplemented(__FILE__, __LINE__);
}
static IRExp *x86_64FPExp(void) {
  return regIRExpCreate(X86_64_RBP, X86_64_INT_REGISTER_WIDTH);
}
static Access *x86_64AllocLocal(X86_64Frame *frame, size_t size, bool escapes) {
  notYetImplemented(__FILE__, __LINE__);
}
static Access *x86_64AllocOutArg(X86_64Frame *frame, size_t size) {
  notYetImplemented(__FILE__, __LINE__);
}
static AccessVector *x86_64AllocInArgs(X86_64Frame *frame, TypeVector *types,
                                       BoolVector *escapes) {
  // see x86_64 SystemV ABI for calling convention information

  // we never have SSEUP, X87, X87FPU, COMPLEX_X87 arguments
  typedef enum {
    AC_POINTER,
    AC_INTEGER,
    AC_SSE,
    AC_NO_CLASS,
    AC_MEMORY,
  } ArgumentClass;

  ArgumentClass *classes = malloc(sizeof(ArgumentClass) * types->size);
  for (size_t idx = 0; idx < types->size; ++idx) {
    Type *type = types->elements[idx];
    
  }
}
FrameVTable const X86_64FrameVTable = {
    (void (*)(Frame *))x86_64FrameDtor,
    (IRStmVector * (*)(Frame *, IRStmVector *, char *)) x86_64GenerateEntryExit,
    (IRExp * (*)(void)) x86_64FPExp,
    (Access * (*)(Frame *, size_t, bool)) x86_64AllocLocal,
    (Access * (*)(Frame *, size_t)) x86_64AllocOutArg,
    (AccessVector * (*)(Frame *, TypeVector *, BoolVector *)) x86_64AllocInArgs,
};  // the frame vtable
Frame *x86_64FrameCtor(void) {
  X86_64Frame *frame = malloc(sizeof(X86_64Frame));
  frame->base.vtable = &X86_64FrameVTable;
  frame->numIntArgs = 0;
  frame->numFloatArgs = 0;
  frame->numMemArgs = 0;
  frame->outArgStackSize = 0;
  return (Frame *)frame;
}

static void x86_64GlobalAccessDtor(X86_64GlobalAccess *access) {
  if (access->labelOwned) {
    free(access->labelName);
  }
  free(access);
}
static IRExp *x86_64GlobalAccessValueExp(X86_64GlobalAccess *access,
                                         IRExp *fp) {
  irExpDestroy(fp);
  return memIRExpCreate(nameIRExpCreate(access->labelName));
}
static char *x86_64GlobalAccessGetLabel(X86_64GlobalAccess *access) {
  access->labelOwned = false;
  return access->labelName;
}
AccessVTable X86_64GlobalAccessVTable = {
    (void (*)(Access *))x86_64GlobalAccessDtor,
    (IRExp * (*)(Access *, IRExp *)) x86_64GlobalAccessValueExp,
    (char *(*)(struct Access *))x86_64GlobalAccessGetLabel,
};
Access *x86_64GlobalAccessCtor(char *label) {
  X86_64GlobalAccess *access = malloc(sizeof(X86_64GlobalAccess));
  access->base.vtable = &X86_64GlobalAccessVTable;
  access->labelName = label;
  access->labelOwned = true;
  return (Access *)access;
}

static void x86_64LabelGeneratorDtor(X86_64LabelGenerator *generator) {
  free(generator);
}
static char *x86_64LabelGeneratorGenerateDataLabel(
    X86_64LabelGenerator *generator) {
  return format(".LC%zu", generator->nextLabel++);
}
static char *x86_64LabelGeneratorGenerateCodeLabel(
    X86_64LabelGenerator *generator) {
  return format(".L%zu", generator->nextLabel++);
}
LabelGeneratorVTable X86_64LabelGeneratorVTable = {
    (void (*)(LabelGenerator *))x86_64LabelGeneratorDtor,
    (char *(*)(struct LabelGenerator *))x86_64LabelGeneratorGenerateDataLabel,
    (char *(*)(struct LabelGenerator *))x86_64LabelGeneratorGenerateCodeLabel,
};
LabelGenerator *x86_64LabelGeneratorCtor(void) {
  X86_64LabelGenerator *generator = malloc(sizeof(X86_64LabelGenerator));
  generator->base.vtable = &X86_64LabelGeneratorVTable;
  generator->nextLabel = 1;
  return (LabelGenerator *)generator;
}