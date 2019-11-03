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

// x86_64 assembly representation

#ifndef TLC_ARCHITECTURE_X86_64_ASSEMBLY_H_
#define TLC_ARCHITECTURE_X86_64_ASSEMBLY_H_

#include "architecture/x86_64/common.h"
#include "ir/allocHint.h"
#include "util/container/hashMap.h"
#include "util/container/vector.h"
#include "util/options.h"

typedef HashMap FileIRFileMap;
struct IROperand;

typedef enum {
  X86_64_OK_REG,
  X86_64_OK_TEMP,
  X86_64_OK_STACKOFFSET,
} X86_64OperandKind;
typedef struct {
  X86_64OperandKind kind;
  union {
    struct {
      X86_64Register reg;
    } reg;
    struct {
      size_t n;
      size_t size;
      size_t alignment;
      AllocHint kind;
    } temp;
    struct {
      int64_t offset;
    } stackOffset;
  } data;
} X86_64Operand;
X86_64Operand *x86_64OperandCreate(struct IROperand const *);
void x86_64OperandDestroy(X86_64Operand *);

typedef Vector X86_64OperandVector;
void x86_64OperandVectorInit(X86_64OperandVector *);
void x86_64OperandVectorInsert(X86_64OperandVector *, X86_64Operand *);
void x86_64OperandVectorUninit(X86_64OperandVector *);

typedef struct {
  char *skeleton;  // format string. `d is a defined, `u is a use, `o is an
                   // other, `` is a literal backtick.
  X86_64OperandVector defines;
  X86_64OperandVector uses;
  X86_64OperandVector other;
  bool isMove;
} X86_64Instruction;
X86_64Instruction *x86_64InstructionCreate(char *skeleton);
X86_64Instruction *x86_64MoveInstructionCreate(char *skeleton);
void x86_64InstructionDestroy(X86_64Instruction *);

typedef Vector X86_64InstructionVector;
void x86_64InstructionVectorInit(X86_64InstructionVector *);
void x86_64InstructionVectorInsert(X86_64InstructionVector *,
                                   X86_64Instruction *);
void x86_64InstructionVectorUninit(X86_64InstructionVector *);

typedef enum {
  X86_64_FK_DATA,
  X86_64_FK_TEXT,
} X86_64FragmentKind;
typedef struct {
  X86_64FragmentKind kind;
  union {
    struct {
      char *data;
    } data;
    struct {
      char *header;
      char *footer;
      X86_64InstructionVector body;  // does not include stack exit or enter -
                                     // added at reg alloc time
    } text;
  } data;
} X86_64Fragment;
X86_64Fragment *x86_64DataFragmentCreate(char *data);
X86_64Fragment *x86_64TextFragmentCreate(char *header, char *footer);
void x86_64FragmentDestroy(X86_64Fragment *);

typedef Vector X86_64FragmentVector;
void x86_64FragmentVectorInit(X86_64FragmentVector *);
void x86_64FragmentVectorInsert(X86_64FragmentVector *, X86_64Fragment *);
void x86_64FragmentVectorUninit(X86_64FragmentVector *);

typedef struct {
  char *header;
  char *footer;
  X86_64FragmentVector fragments;
} X86_64File;
X86_64File *x86_64FileCreate(char *header, char *footer);
void x86_64FileDestroy(X86_64File *);

// associates assembly fragments in a file with the file name
typedef HashMap FileX86_64FileMap;
// in-place ctor
void fileX86_64FileMapInit(FileX86_64FileMap *);
// get
X86_64File *fileX86_64FileMapGet(FileX86_64FileMap *, char const *key);
// put
int fileX86_64FileMapPut(FileX86_64FileMap *, char const *key,
                         X86_64File *file);
// in-place dtor
void fileX86_64FileMapUninit(FileX86_64FileMap *);

void x86_64InstructionSelect(FileX86_64FileMap *asmFileMap,
                             FileIRFileMap *irFileMap, Options *options);

#endif  // TLC_ARCHITECTURE_X86_64_ASSEMBLY_H_