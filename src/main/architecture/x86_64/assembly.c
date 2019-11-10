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

// implementation of x86_64 assembly representation

#include "architecture/x86_64/assembly.h"

#include "architecture/x86_64/frame.h"
#include "ir/ir.h"
#include "util/internalError.h"

#include <stdlib.h>

X86_64Operand *x86_64OperandCreate(IROperand const *irOperand, size_t size) {
  X86_64Operand *op = malloc(sizeof(X86_64Operand));
  op->operandSize = size;
  switch (irOperand->kind) {
    case OK_REG: {
      op->kind = X86_64_OK_REG;
      op->data.reg.reg = x86_64RegNumToRegister(irOperand->data.reg.n);
      break;
    }
    case OK_TEMP: {
      op->kind = X86_64_OK_TEMP;
      op->data.temp.n = irOperand->data.temp.n;
      op->data.temp.size = irOperand->data.temp.size;
      op->data.temp.alignment = irOperand->data.temp.alignment;
      op->data.temp.kind = irOperand->data.temp.kind;
      break;
    }
    case OK_STACKOFFSET: {
      op->kind = X86_64_OK_STACKOFFSET;
      op->data.stackOffset.offset = irOperand->data.stackOffset.stackOffset;
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid operand type - constants should not be handled here");
    }
  }
  return op;
}
void x86_64OperandDestroy(X86_64Operand *op) { free(op); }

void x86_64OperandVectorInit(X86_64OperandVector *v) { vectorInit(v); }
void x86_64OperandVectorInsert(X86_64OperandVector *v, X86_64Operand *op) {
  vectorInsert(v, op);
}
void x86_64OperandVectorUninit(X86_64OperandVector *v) {
  vectorUninit(v, (void (*)(void *))x86_64OperandDestroy);
}

static X86_64Instruction *x86_64InstructionCreate(char *skeleton,
                                                  X86_64InstructionKind kind) {
  X86_64Instruction *i = malloc(sizeof(X86_64Instruction));
  i->kind = kind;
  i->skeleton = skeleton;
  x86_64OperandVectorInit(&i->defines);
  x86_64OperandVectorInit(&i->uses);
  x86_64OperandVectorInit(&i->other);
  return i;
}
X86_64Instruction *x86_64RegularInstructionCreate(char *skeleton) {
  return x86_64InstructionCreate(skeleton, X86_64_IK_REGULAR);
}
X86_64Instruction *x86_64MoveInstructionCreate(char *skeleton) {
  return x86_64InstructionCreate(skeleton, X86_64_IK_MOVE);
}
X86_64Instruction *x86_64JumpInstructionCreate(char *skeleton,
                                               char *jumpTarget) {
  X86_64Instruction *i = x86_64InstructionCreate(skeleton, X86_64_IK_JUMP);
  i->data.jumpTarget = jumpTarget;
  return i;
}
X86_64Instruction *x86_64CJumpInstructionCreate(char *skeleton,
                                                char *jumpTarget) {
  X86_64Instruction *i = x86_64InstructionCreate(skeleton, X86_64_IK_CJUMP);
  i->data.jumpTarget = jumpTarget;
  return i;
}
X86_64Instruction *x86_64LeaveInstructionCreate(char *skeleton) {
  return x86_64InstructionCreate(skeleton, X86_64_IK_LEAVE);
}
X86_64Instruction *x86_64SwitchInstuctionCreate(char *skeleton) {
  X86_64Instruction *i = x86_64InstructionCreate(skeleton, X86_64_IK_SWITCH);
  stringVectorInit(&i->data.switchTargets);
  return i;
}
X86_64Instruction *x86_64LabelInstructionCreate(char *skeleton,
                                                char *labelName) {
  X86_64Instruction *i = x86_64InstructionCreate(skeleton, X86_64_IK_LABEL);
  i->data.labelName = labelName;
  return i;
}
void x86_64InstructionDestroy(X86_64Instruction *i) {
  free(i->skeleton);
  x86_64OperandVectorUninit(&i->defines);
  x86_64OperandVectorUninit(&i->uses);
  x86_64OperandVectorUninit(&i->other);
  switch (i->kind) {
    case X86_64_IK_REGULAR:
    case X86_64_IK_MOVE:
    case X86_64_IK_LEAVE: {
      break;
    }
    case X86_64_IK_JUMP:
    case X86_64_IK_CJUMP: {
      free(i->data.jumpTarget);
      break;
    }
    case X86_64_IK_SWITCH: {
      stringVectorUninit(&i->data.switchTargets, true);
      break;
    }
    case X86_64_IK_LABEL: {
      free(i->data.labelName);
      break;
    }
  }
  free(i);
}

typedef Vector X86_64InstructionVector;
void x86_64InstructionVectorInit(X86_64InstructionVector *v) { vectorInit(v); }
void x86_64InstructionVectorInsert(X86_64InstructionVector *v,
                                   X86_64Instruction *i) {
  vectorInsert(v, i);
}
void x86_64InstructionVectorUninit(X86_64InstructionVector *v) {
  vectorUninit(v, (void (*)(void *))x86_64InstructionDestroy);
}

static X86_64Fragment *x86_64FragmentCreate(X86_64FragmentKind kind) {
  X86_64Fragment *f = malloc(sizeof(X86_64Fragment));
  f->kind = kind;
  return f;
}
X86_64Fragment *x86_64DataFragmentCreate(char *data) {
  X86_64Fragment *f = x86_64FragmentCreate(X86_64_FK_DATA);
  f->data.data.data = data;
  return f;
}
X86_64Fragment *x86_64TextFragmentCreate(char *header, char *footer,
                                         X86_64Frame *frame) {
  X86_64Fragment *f = x86_64FragmentCreate(X86_64_FK_TEXT);
  f->data.text.header = header;
  f->data.text.footer = footer;
  x86_64InstructionVectorInit(&f->data.text.body);
  f->data.text.frame = frame;
  return f;
}
void x86_64FragmentDestroy(X86_64Fragment *f) {
  switch (f->kind) {
    case X86_64_FK_DATA: {
      free(f->data.data.data);
      break;
    }
    case X86_64_FK_TEXT: {
      free(f->data.text.header);
      free(f->data.text.footer);
      x86_64InstructionVectorUninit(&f->data.text.body);
      f->data.text.frame->base.vtable->dtor((Frame *)f->data.text.frame);
      break;
    }
  }

  free(f);
}

void x86_64FragmentVectorInit(X86_64FragmentVector *v) { vectorInit(v); }
void x86_64FragmentVectorInsert(X86_64FragmentVector *v, X86_64Fragment *f) {
  vectorInsert(v, f);
}
void x86_64FragmentVectorUninit(X86_64FragmentVector *v) {
  vectorUninit(v, (void (*)(void *))x86_64FragmentDestroy);
}

X86_64File *x86_64FileCreate(char *header, char *footer) {
  X86_64File *f = malloc(sizeof(X86_64File));
  f->header = header;
  f->footer = footer;
  x86_64FragmentVectorInit(&f->fragments);
  return f;
}
void x86_64FileDestroy(X86_64File *f) {
  free(f->header);
  free(f->footer);
  x86_64FragmentVectorUninit(&f->fragments);
  free(f);
}

void fileX86_64FileMapInit(FileX86_64FileMap *m) { hashMapInit(m); }
X86_64File *fileX86_64FileMapGet(FileX86_64FileMap *m, char const *key) {
  return hashMapGet(m, key);
}
int fileX86_64FileMapPut(FileX86_64FileMap *m, char const *key,
                         X86_64File *file) {
  return hashMapPut(m, key, file, (void (*)(void *))x86_64FileDestroy);
}
void fileX86_64FileMapUninit(FileX86_64FileMap *m) {
  hashMapUninit(m, (void (*)(void *))x86_64FileDestroy);
}