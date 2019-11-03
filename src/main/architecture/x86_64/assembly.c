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
#include "constants.h"
#include "ir/allocHint.h"
#include "ir/ir.h"
#include "translate/translate.h"
#include "util/format.h"
#include "util/internalError.h"

#include <stdlib.h>
#include <string.h>

static X86_64Operand *x86_64OperandCreate(X86_64OperandKind kind) {
  X86_64Operand *op = malloc(sizeof(X86_64Operand));
  op->kind = kind;
  return op;
}
X86_64Operand *x86_64RegCreate(IROperand const *reg) {
  X86_64Operand *op = x86_64OperandCreate(X86_64_OK_REG);
  op->data.reg.reg = x86_64RegNumToRegister(reg->data.reg.n);
  return op;
}
X86_64Operand *x86_64TempCreate(IROperand const *temp) {
  X86_64Operand *op = x86_64OperandCreate(X86_64_OK_TEMP);
  op->data.temp.n = temp->data.temp.n;
  op->data.temp.size = temp->data.temp.size;
  op->data.temp.alignment = temp->data.temp.alignment;
  op->data.temp.kind = temp->data.temp.kind;
  return op;
}
X86_64Operand *x86_64StackOffsetCreate(IROperand const *offset) {
  X86_64Operand *op = x86_64OperandCreate(X86_64_OK_STACKOFFSET);
  op->data.stackOffset.offset = offset->data.stackOffset.stackOffset;
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

X86_64Instruction *x86_64InstructionCreate(char *skeleton) {
  X86_64Instruction *i = malloc(sizeof(X86_64Instruction));
  i->skeleton = skeleton;
  x86_64OperandVectorInit(&i->defines);
  x86_64OperandVectorInit(&i->uses);
  x86_64OperandVectorInit(&i->other);
  i->isMove = false;
  return i;
}
X86_64Instruction *x86_64MoveInstructionCreate(char *skeleton) {
  X86_64Instruction *i = malloc(sizeof(X86_64Instruction));
  i->skeleton = skeleton;
  x86_64OperandVectorInit(&i->defines);
  x86_64OperandVectorInit(&i->uses);
  x86_64OperandVectorInit(&i->other);
  i->isMove = true;
  return i;
}
void x86_64InstructionDestroy(X86_64Instruction *i) {
  free(i->skeleton);
  x86_64OperandVectorUninit(&i->defines);
  x86_64OperandVectorUninit(&i->uses);
  x86_64OperandVectorUninit(&i->other);
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
X86_64Fragment *x86_64TextFragmentCreate(char *header, char *footer) {
  X86_64Fragment *f = x86_64FragmentCreate(X86_64_FK_TEXT);
  f->data.text.header = header;
  f->data.text.footer = footer;
  x86_64InstructionVectorInit(&f->data.text.body);
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

static bool sizeIsAtomic(size_t size) {
  return size == BYTE_WIDTH || size == SHORT_WIDTH || size == INT_WIDTH ||
         size == LONG_WIDTH;
}
static char atomicSizeToGPSizePostfix(size_t size) {
  switch (size) {
    case 1: {
      return 'b';
    }
    case 2: {
      return 'w';
    }
    case 4: {
      return 'l';
    }
    case 8: {
      return 'q';
    }
    default: { error(__FILE__, __LINE__, "not an atomic gp size"); }
  }
}
static char atomicSizeToFPSizePostfix(size_t size) {
  switch (size) {
    case 4: {
      return 's';
    }
    case 8: {
      return 'd';
    }
    default: { error(__FILE__, __LINE__, "not a atomic fp size"); }
  }
}
// moves
static void regRegMoveInstructionSelect(IREntry *entry,
                                        X86_64InstructionVector *assembly) {
  // note - move size must be atomic here
  // two cases - sse/sse, gp/gp
  X86_64Instruction *i;
  if (x86_64RegIsSSE(x86_64RegNumToRegister(entry->dest->data.reg.n))) {
    i = x86_64MoveInstructionCreate(format(
        "\tmovs%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
  } else {
    // reg is gp
    i = x86_64MoveInstructionCreate(
        format("\tmov%c\t`u, `d\n", atomicSizeToGPSizePostfix(entry->opSize)));
  }
  x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->defines, x86_64RegCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void tempRegMoveInstructionSelect(IREntry *entry,
                                         X86_64InstructionVector *assembly) {
  // note - move size must be atomic here
  X86_64Instruction *i;
  if (x86_64RegIsSSE(x86_64RegNumToRegister(entry->dest->data.reg.n))) {
    i = x86_64MoveInstructionCreate(format(
        "\tmovs%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
  } else {
    // reg is gp
    i = x86_64MoveInstructionCreate(
        format("\tmov%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
  }
  x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void stackOffsetRegMoveInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // note - move size must be 8 here
  X86_64Instruction *i = x86_64InstructionCreate(strdup("\tmovq\t$`o, `d\n"));
  x86_64OperandVectorInsert(&i->other, x86_64StackOffsetCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->defines, x86_64RegCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void nameRegMoveInstructionSelect(IREntry *entry,
                                         X86_64InstructionVector *assembly,
                                         Options *options) {
  // note - move size must be 8 here
  X86_64Instruction *i;
  switch (optionsGet(options, optionPositionIndependence)) {
    case O_PI_NONE: {
      i = x86_64InstructionCreate(
          format("\tmovq\t$%s, `d\n", entry->arg1->data.name.name));
      break;
    }
    case O_PI_PIE: {
      i = x86_64InstructionCreate(
          format("\tleaq\t%s(%%rip), `d\n", entry->arg1->data.name.name));
      break;
    }
    case O_PI_PIC: {
      i = x86_64InstructionCreate(format("\tmovq\t%s@GOTPCREL(%%rip), `d\n",
                                         entry->arg1->data.name.name));
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid PositionIndependenceType enum");
    }
  }
  x86_64OperandVectorInsert(&i->defines, x86_64RegCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void constantRegMoveInstructionSelect(IREntry *entry,
                                             X86_64InstructionVector *assembly,
                                             X86_64FragmentVector *frags,
                                             LabelGenerator *labelGenerator) {
  // note - move size must be atomic here
  X86_64Instruction *i;
  if (regIsSSE(x86_64RegNumToRegister(entry->dest->data.reg.n))) {
    char *constant = labelGeneratorGenerateDataLabel(labelGenerator);
    if (entry->opSize == 4) {
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t4\n"
                            "%s:\n"
                            "\t.long\t%zu",
                            constant, entry->arg1->data.constant.bits)));
    } else {
      // entry->opSize == 8
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t8\n"
                            "%s:\n"
                            "\t.quad\t%lu",
                            constant, entry->arg1->data.constant.bits)));
    }
    i = x86_64InstructionCreate(format("\tmovs%c\t%s(%%rip), `d\n",
                                       atomicSizeToFPSizePostfix(entry->opSize),
                                       constant));
    free(constant);
  } else {
    // scalar load
    i = x86_64InstructionCreate(format("\tmov%c\t$%lu, `d\n",
                                       atomicSizeToGPSizePostfix(entry->opSize),
                                       entry->arg1->data.constant.bits));
  }
  x86_64OperandVectorInsert(&i->defines, x86_64RegCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void regTempMoveInstructionSelect(IREntry *entry,
                                         X86_64InstructionVector *assembly) {
  // note - move size must be atomic here
  X86_64Instruction *i;
  if (entry->dest->data.temp.kind == AH_SSE) {
    i = x86_64MoveInstructionCreate(format(
        "\tmovs%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
  } else {
    // temp is gp
    i = x86_64MoveInstructionCreate(
        format("\tmov%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
  }
  x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void tempTempMoveInstructionSelect(IREntry *entry,
                                          X86_64InstructionVector *assembly) {
  X86_64Instruction *i;
  switch (entry->dest->data.temp.kind) {
    case AH_GP: {
      X86_64Instruction *i = x86_64MoveInstructionCreate(format(
          "\tmov%c\t`u, `d\n", atomicSizeToGPSizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
      break;
    }
    case AH_MEM: {
      error(__FILE__, __LINE__, "not yet implemented");
      break;
    }
    case AH_SSE: {
      X86_64Instruction *i = x86_64MoveInstructionCreate(format(
          "\tmovs%c\t`u, `d\n", atomicSizeToSSESizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid AllocHint enum"); }
  }
  x86_64InstructionVectorInsert(assembly, i);
}
static void stackOffsetTempMoveInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // note - move size must be 8 here
  X86_64Instruction *i = x86_64InstructionCreate(strdup("\tmovq\t$`o, `d\n"));
  x86_64OperandVectorInsert(&i->other, x86_64StackOffsetCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->defines, x86_64RegCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void nameTempMoveInstructionSelect(IREntry *entry,
                                          X86_64InstructionVector *assembly,
                                          Options *options) {
  // note - move size must be 8 here
  // note - move size must be 8 here
  X86_64Instruction *i;
  switch (optionsGet(options, optionPositionIndependence)) {
    case O_PI_NONE: {
      i = x86_64InstructionCreate(
          format("\tmovq\t$%s, `d\n", entry->arg1->data.name.name));
      break;
    }
    case O_PI_PIE: {
      i = x86_64InstructionCreate(
          format("\tleaq\t%s(%%rip), `d\n", entry->arg1->data.name.name));
      break;
    }
    case O_PI_PIC: {
      i = x86_64InstructionCreate(format("\tmovq\t%s@GOTPCREL(%%rip), `d\n",
                                         entry->arg1->data.name.name));
      break;
    }
  }
  x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void constantTempMoveInstructionSelect(IREntry *entry,
                                              X86_64InstructionVector *assembly,
                                              X86_64FragmentVector *frags,
                                              LabelGenerator *labelGenerator,
                                              Options *options) {
  // note - move size must be atomic here
  X86_64Instruction *i;
  if (regIsSSE(x86_64RegNumToRegister(entry->dest->data.reg.n))) {
    char *constant = labelGeneratorGenerateDataLabel(labelGenerator);
    if (entry->opSize == 4) {
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t4\n"
                            "%s:\n"
                            "\t.long\t%zu",
                            constant, entry->arg1->data.constant.bits)));
    } else {
      // entry->opSize == 8
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t8\n"
                            "%s:\n"
                            "\t.quad\t%lu",
                            constant, entry->arg1->data.constant.bits)));
    }
    i = x86_64InstructionCreate(format("\tmovs%c\t%s(%%rip), `d\n",
                                       atomicSizeToFPSizePostfix(entry->opSize),
                                       constant));
    free(constant);
  } else {
    // scalar load
    i = x86_64InstructionCreate(format("\tmov%c\t$%lu, `d\n",
                                       atomicSizeToGPSizePostfix(entry->opSize),
                                       entry->arg1->data.constant.bits));
  }
  x86_64OperandVectorInsert(&i->defines, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
// mem store
static void regTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // is atomic sized, addr in temp
  X86_64Instruction *i;
  if (x86_64RegIsSSE(x86_64RegNumToRegister(entry->arg1->data.reg.n))) {
    i = x86_64InstructionCreate(format(
        "\tmovs%c\t`u, (`u)\n", atomicSizeToSSESizePostfix(entry->opSize)));
  } else {
    // is GP reg
    i = x86_64InstructionCreate(format(
        "\tmov%c\t`u, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize)));
  }
  x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void tempTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  X86_64Instruction *i;
  switch (entry->dest->data.temp.kind) {
    case AH_GP: {
      X86_64Instruction *i = x86_64InstructionCreate(format(
          "\tmov%c\t`u, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      break;
    }
    case AH_MEM: {
      error(__FILE__, __LINE__, "not yet implemented");
      break;
    }
    case AH_SSE: {
      X86_64Instruction *i = x86_64InstructionCreate(format(
          "\tmovs%c\t`u, (`u)\n", atomicSizeToSSESizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid AllocHint enum"); }
  }
  x86_64InstructionVectorInsert(assembly, i);
}
static void stackOffsetTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // size must be 8
  X86_64Instruction *i = x86_64InstructionCreate(strdup("\tmovq\t$`o, (`u)"));
  x86_64OperandVectorInsert(&i->other, x86_64StackOffsetCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void nameTempMemStoreInstructionSelect(IREntry *entry,
                                              X86_64InstructionVector *assembly,
                                              TempAllocator *tempAllocator,
                                              Options *options) {
  // size must be 8, align is 8, kind is always AH_GP
  // note - in rip relative modes, must move to temp first - move allows only
  // one mem access
  switch (optionsGet(options, optionPositionIndependence)) {
    case O_PI_NONE: {
      X86_64Instruction *i = x86_64InstructionCreate(
          format("\tmovq\t$%s, (`u)\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      x86_64InstructionVectorInsert(assembly, i);
      break;
    }
    case O_PI_PIE: {
      IROperand *temp = tempIROperandCreate(
          tempAllocatorAllocate(tempAllocator), 8, 8, AH_GP);
      X86_64Instruction *load = x86_64InstructionCreate(
          format("\tleaq\t%s(%%rip), `d\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&load->defines, temp);
      x86_64InstructionVectorInsert(assembly, load);
      X86_64Instruction *store =
          x86_64InstructionCreate(strdup("\tmovq\t`u, (`u)\n"));
      x86_64OperandVectorInsert(&store->uses, x86_64TempCreate(temp));
      x86_64OperandVectorInsert(&store->uses, entry->dest);
      x86_64InstructionVectorInsert(assembly, store);
      irOperandDestroy(temp);
      break;
    }
    case O_PI_PIC: {
      IROperand *temp = tempIROperandCreate(
          tempAllocatorAllocate(tempAllocator), 8, 8, AH_GP);
      X86_64Instruction *load = x86_64InstructionCreate(format(
          "\tmovq\t%s@GOTPCREL(%%rip), `d\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&load->defines, temp);
      x86_64InstructionVectorInsert(assembly, load);
      X86_64Instruction *store =
          x86_64InstructionCreate(strdup("\tmovq\t`u, (`u)\n"));
      x86_64OperandVectorInsert(&store->uses, x86_64TempCreate(temp));
      x86_64OperandVectorInsert(&store->uses, entry->dest);
      x86_64InstructionVectorInsert(assembly, store);
      irOperandDestroy(temp);
      break;
    }
  }
}
static void constRegMemStoreInstructionSelect(IREntry *entry,
                                              X86_64InstructionVector *assembly,
                                              X86_64FragmentVector *frags,
                                              LabelGenerator *labelGenerator,
                                              TempAllocator *tempAllocator) {
  // note - move size must be atomic here
  if (regIsSSE(x86_64RegNumToRegister(entry->dest->data.reg.n))) {
    char *constant = labelGeneratorGenerateDataLabel(labelGenerator);
    if (entry->opSize == 4) {
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t4\n"
                            "%s:\n"
                            "\t.long\t%zu",
                            constant, entry->arg1->data.constant.bits)));
    } else {
      // entry->opSize == 8
      x86_64FragmentVectorInsert(
          frags, x86_64DataFragmentCreate(
                     format("\t.section\t.rodata\n"
                            "\t.align\t8\n"
                            "%s:\n"
                            "\t.quad\t%lu",
                            constant, entry->arg1->data.constant.bits)));
    }
    IROperand *temp = tempIROperandCreate(tempAllocatorAllocate(tempAllocator),
                                          entry->opSize, entry->opSize, AH_GP);
    X86_64Instruction *load = x86_64InstructionCreate(
        format("\tmovs%c\t%s(%%rip), `d\n",
               atomicSizeToFPSizePostfix(entry->opSize), constant));
    x86_64OperandVectorInsert(&load->defines, temp);
    x86_64InstructionVectorInsert(assembly, load);
    X86_64Instruction *store = x86_64InstructionCreate(format(
        "\tmovs%c\t`u, (`u)\n", atomicSizeToFPSizePostfix(entry->opSize)));
    x86_64OperandVectorInsert(&store->uses, x86_64TempCreate(temp));
    x86_64OperandVectorInsert(&store->uses, entry->dest);
    x86_64InstructionVectorInsert(assembly, store);
    irOperandDestroy(temp);
    free(constant);
  } else {
    // scalar load

    X86_64Instruction *i = x86_64InstructionCreate(format(
        "\tmov%c\t$%lu, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize),
        entry->arg1->data.constant.bits));
    x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
    x86_64InstructionVectorInsert(assembly, i);
  }
}
static void regTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // is atomic sized, addr in temp
  X86_64Instruction *i;
  if (x86_64RegIsSSE(x86_64RegNumToRegister(entry->arg1->data.reg.n))) {
    i = x86_64InstructionCreate(format(
        "\tmovs%c\t`u, (`u)\n", atomicSizeToSSESizePostfix(entry->opSize)));
  } else {
    // is GP reg
    i = x86_64InstructionCreate(format(
        "\tmov%c\t`u, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize)));
  }
  x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void tempTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  X86_64Instruction *i;
  switch (entry->dest->data.temp.kind) {
    case AH_GP: {
      X86_64Instruction *i = x86_64InstructionCreate(format(
          "\tmov%c\t`u, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      break;
    }
    case AH_MEM: {
      error(__FILE__, __LINE__, "not yet implemented");
      break;
    }
    case AH_SSE: {
      X86_64Instruction *i = x86_64InstructionCreate(format(
          "\tmovs%c\t`u, (`u)\n", atomicSizeToSSESizePostfix(entry->opSize)));
      x86_64OperandVectorInsert(&i->uses, x86_64RegCreate(entry->arg1));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid AllocHint enum"); }
  }
  x86_64InstructionVectorInsert(assembly, i);
}
static void stackOffsetTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly) {
  // size must be 8
  X86_64Instruction *i = x86_64InstructionCreate(strdup("\tmovq\t$`o, (`u)"));
  x86_64OperandVectorInsert(&i->other, x86_64StackOffsetCreate(entry->arg1));
  x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void nameTempMemStoreInstructionSelect(IREntry *entry,
                                              X86_64InstructionVector *assembly,
                                              TempAllocator *tempAllocator,
                                              Options *options) {
  // size must be 8, align is 8, kind is always AH_GP
  // note - in rip relative modes, must move to temp first - move allows only
  // one mem access
  switch (optionsGet(options, optionPositionIndependence)) {
    case O_PI_NONE: {
      X86_64Instruction *i = x86_64InstructionCreate(
          format("\tmovq\t$%s, (`u)\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
      x86_64InstructionVectorInsert(assembly, i);
      break;
    }
    case O_PI_PIE: {
      IROperand *temp = tempIROperandCreate(
          tempAllocatorAllocate(tempAllocator), 8, 8, AH_GP);
      X86_64Instruction *load = x86_64InstructionCreate(
          format("\tleaq\t%s(%%rip), `d\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&load->defines, temp);
      x86_64InstructionVectorInsert(assembly, load);
      X86_64Instruction *store =
          x86_64InstructionCreate(strdup("\tmovq\t`u, (`u)\n"));
      x86_64OperandVectorInsert(&store->uses, x86_64TempCreate(temp));
      x86_64OperandVectorInsert(&store->uses, entry->dest);
      x86_64InstructionVectorInsert(assembly, store);
      irOperandDestroy(temp);
      break;
    }
    case O_PI_PIC: {
      IROperand *temp = tempIROperandCreate(
          tempAllocatorAllocate(tempAllocator), 8, 8, AH_GP);
      X86_64Instruction *load = x86_64InstructionCreate(format(
          "\tmovq\t%s@GOTPCREL(%%rip), `d\n", entry->arg1->data.name.name));
      x86_64OperandVectorInsert(&load->defines, temp);
      x86_64InstructionVectorInsert(assembly, load);
      X86_64Instruction *store =
          x86_64InstructionCreate(strdup("\tmovq\t`u, (`u)\n"));
      x86_64OperandVectorInsert(&store->uses, x86_64TempCreate(temp));
      x86_64OperandVectorInsert(&store->uses, entry->dest);
      x86_64InstructionVectorInsert(assembly, store);
      irOperandDestroy(temp);
      break;
    }
  }
}
static void constTempMemStoreInstructionSelect(
    IREntry *entry, X86_64InstructionVector *assembly,
    X86_64FragmentVector *frags, LabelGenerator *labelGenerator,
    TempAllocator *tempAllocator) {
  // note - move size must be atomic here
  X86_64Instruction *i = x86_64InstructionCreate(
      format("\tmov%c\t$%lu, (`u)\n", atomicSizeToGPSizePostfix(entry->opSize),
             entry->arg1->data.constant.bits));
  x86_64OperandVectorInsert(&i->uses, x86_64TempCreate(entry->dest));
  x86_64InstructionVectorInsert(assembly, i);
}
static void textInstructionSelect(X86_64Fragment *frag, Fragment *irFrag,
                                  X86_64FragmentVector *frags,
                                  LabelGenerator *labelGenerator,
                                  Options *options) {
  X86_64Frame *frame = (X86_64Frame *)irFrag->data.text.frame;
  TempAllocator *tempAllocator = irFrag->data.text.tempAllocator;
  IREntryVector *ir = irFrag->data.text.ir;
  X86_64InstructionVector *assembly = &frag->data.text.body;

  for (size_t idx = 0; idx < ir->size; idx++) {
    IREntry *entry = ir->elements[idx];
    switch (entry->op) {
      case IO_ASM: {
        x86_64InstructionVectorInsert(
            assembly, x86_64InstructionCreate(
                          strdup(entry->arg1->data.assembly.assembly)));
        break;
      }
      case IO_LABEL: {
        x86_64InstructionVectorInsert(
            assembly, x86_64InstructionCreate(
                          format("%s:\n", entry->arg1->data.name.name)));
        break;
      }
      case IO_MOVE: {
        switch (entry->dest->kind) {
          case OK_REG: {
            switch (entry->arg1->kind) {
              case OK_REG: {
                regRegMoveInstructionSelect(entry, assembly);
                break;
              }
              case OK_TEMP: {
                tempRegMoveInstructionSelect(entry, assembly);
                break;
              }
              case OK_STACKOFFSET: {
                stackOffsetRegMoveInstructionSelect(entry, assembly);
                break;
              }
              case OK_NAME: {
                nameRegMoveInstructionSelect(entry, assembly, options);
                break;
              }
              case OK_CONSTANT: {
                constantRegMoveInstructionSelect(entry, assembly, frags,
                                                 labelGenerator);
                break;
              }
              default: {
                error(__FILE__, __LINE__, "unexpected argument kind");
              }
            }
            break;
          }
          case OK_TEMP: {
            switch (entry->arg1->kind) {
              case OK_REG: {
                regTempMoveInstructionSelect(entry, assembly);
              }
              case OK_TEMP: {
                tempTempMoveInstructionSelect(entry, assembly);
              }
              case OK_STACKOFFSET: {
                stackOffsetTempMoveInstructionSelect(entry, assembly);
                break;
              }
              case OK_NAME: {
                nameTempMoveInstructionSelect(entry, assembly, options);
                break;
              }
              case OK_CONSTANT: {
                constantTempMoveInstructionSelect(entry, assembly, frags,
                                                  labelGenerator);
                break;
              }
              default: {
                error(__FILE__, __LINE__, "unexpected argument kind");
              }
            }
            break;
          }
          default: { error(__FILE__, __LINE__, "unexpected argument kind"); }
        }
        break;
      }
      case IO_MEM_STORE: {
        switch (entry->dest->kind) {
          case OK_REG: {
            switch (entry->arg1->kind) {
              case OK_REG: {
                regRegMemStoreInstructionSelect(entry, assembly);
                break;
              }
              case OK_TEMP: {
                tempRegMemStoreInstructionSelect(entry, assembly);
                break;
              }
              case OK_STACKOFFSET: {
                stackOffsetRegMemStoreInstructionSelect(entry, assembly);
                break;
              }
              case OK_NAME: {
                nameRegMemStoreInstructionSelect(entry, assembly);
                break;
              }
              case OK_CONSTANT: {
                constantRegMemStoreInstructionSelect(entry, assembly, frags,
                                                     labelGenerator);
                break;
              }
              default: {
                error(__FILE__, __LINE__, "unexpected argument kind");
              }
            }
            break;
          }
          case OK_TEMP: {
            switch (entry->arg1->kind) {
              case OK_REG: {
                regTempMemStoreInstructionSelect(entry, assembly);
              }
              case OK_TEMP: {
                tempTempMemStoreInstructionSelect(entry, assembly);
              }
              case OK_STACKOFFSET: {
                stackOffsetTempMemStoreInstructionSelect(entry, assembly);
                break;
              }
              case OK_NAME: {
                nameTempMemStoreInstructionSelect(entry, assembly,
                                                  tempAllocator, options);
                break;
              }
              case OK_CONSTANT: {
                constantTempMemStoreInstructionSelect(entry, assembly, frags,
                                                      labelGenerator);
                break;
              }
              default: {
                error(__FILE__, __LINE__, "unexpected argument kind");
              }
            }
            break;
          }
          default: { error(__FILE__, __LINE__, "unexpected argument kind"); }
        }
        break;
      }
      case IO_MEM_LOAD: {
        break;
      }
      case IO_STK_STORE: {
        break;
      }
      case IO_STK_LOAD: {
        break;
      }
      case IO_OFFSET_STORE: {
        break;
      }
      case IO_OFFSET_LOAD: {
        break;
      }
      case IO_ADD: {
        break;
      }
      case IO_FP_ADD: {
        break;
      }
      case IO_SUB: {
        break;
      }
      case IO_FP_SUB: {
        break;
      }
      case IO_SMUL: {
        break;
      }
      case IO_UMUL: {
        break;
      }
      case IO_FP_MUL: {
        break;
      }
      case IO_SDIV: {
        break;
      }
      case IO_UDIV: {
        break;
      }
      case IO_FP_DIV: {
        break;
      }
      case IO_SMOD: {
        break;
      }
      case IO_UMOD: {
        break;
      }
      case IO_SLL: {
        break;
      }
      case IO_SLR: {
        break;
      }
      case IO_SAR: {
        break;
      }
      case IO_AND: {
        break;
      }
      case IO_XOR: {
        break;
      }
      case IO_OR: {
        break;
      }
      case IO_L: {
        break;
      }
      case IO_LE: {
        break;
      }
      case IO_E: {
        break;
      }
      case IO_NE: {
        break;
      }
      case IO_GE: {
        break;
      }
      case IO_G: {
        break;
      }
      case IO_A: {
        break;
      }
      case IO_AE: {
        break;
      }
      case IO_B: {
        break;
      }
      case IO_BE: {
        break;
      }
      case IO_FP_L: {
        break;
      }
      case IO_FP_LE: {
        break;
      }
      case IO_FP_E: {
        break;
      }
      case IO_FP_NE: {
        break;
      }
      case IO_FP_GE: {
        break;
      }
      case IO_FP_G: {
        break;
      }
      case IO_NEG: {
        break;
      }
      case IO_FP_NEG: {
        break;
      }
      case IO_LNOT: {
        break;
      }
      case IO_NOT: {
        break;
      }
      case IO_SX_SHORT: {
        break;
      }
      case IO_SX_INT: {
        break;
      }
      case IO_SX_LONG: {
        break;
      }
      case IO_ZX_SHORT: {
        break;
      }
      case IO_ZX_INT: {
        break;
      }
      case IO_ZX_LONG: {
        break;
      }
      case IO_U_TO_FLOAT: {
        break;
      }
      case IO_U_TO_DOUBLE: {
        break;
      }
      case IO_S_TO_FLOAT: {
        break;
      }
      case IO_S_TO_DOUBLE: {
        break;
      }
      case IO_F_TO_FLOAT: {
        break;
      }
      case IO_F_TO_DOUBLE: {
        break;
      }
      case IO_TRUNC_BYTE: {
        break;
      }
      case IO_TRUNC_SHORT: {
        break;
      }
      case IO_TRUNC_INT: {
        break;
      }
      case IO_F_TO_BYTE: {
        break;
      }
      case IO_F_TO_SHORT: {
        break;
      }
      case IO_F_TO_INT: {
        break;
      }
      case IO_F_TO_LONG: {
        break;
      }
      case IO_JUMP: {
        break;
      }
      case IO_JL: {
        break;
      }
      case IO_JLE: {
        break;
      }
      case IO_JE: {
        break;
      }
      case IO_JNE: {
        break;
      }
      case IO_JGE: {
        break;
      }
      case IO_JG: {
        break;
      }
      case IO_JA: {
        break;
      }
      case IO_JAE: {
        break;
      }
      case IO_JB: {
        break;
      }
      case IO_JBE: {
        break;
      }
      case IO_FP_JL: {
        break;
      }
      case IO_FP_JLE: {
        break;
      }
      case IO_FP_JE: {
        break;
      }
      case IO_FP_JNE: {
        break;
      }
      case IO_FP_JGE: {
        break;
      }
      case IO_FP_JG: {
        break;
      }
      case IO_CALL: {
        break;
      }
      case IO_RETURN: {
        x86_64InstructionVectorInsert(
            assembly, x86_64InstructionCreate(strdup("\tret\n")));
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "invalid or unexpected ir operator encountered");
      }
    }
  }
}

// data fragment stuff
static bool isLocalLabel(char const *str) { return strncmp(str, ".L", 2) == 0; }
static char *tstrToX86_64Str(uint8_t *str) {
  char *buffer = strdup("");
  for (; *str != 0; str++) {
    buffer = format(
        "%s"
        "\t.byte\t%hhu",
        buffer, *str);
  }
  buffer = format(
      "%s"
      "\t.byte\t0\n",
      buffer);
  return buffer;
}
static char *twstrToX86_64WStr(uint32_t *wstr) {
  char *buffer = strdup("");
  for (; *wstr != 0; wstr++) {
    buffer = format(
        "%s"
        "\t.long\t%u",
        buffer, *wstr);
  }
  buffer = format(
      "%s"
      "\t.long\t0\n",
      buffer);
  return buffer;
}
static char *dataToString(IREntryVector *data) {
  char *acc = strdup("");
  for (size_t idx = 0; idx < data->size; idx++) {
    IREntry *datum = data->elements[idx];
    // datum->op == IO_CONST
    IROperand *value = datum->arg1;
    switch (value->kind) {
      case OK_CONSTANT: {
        switch (datum->opSize) {
          case 1: {  // BYTE_WIDTH, CHAR_WIDTH
            acc = format(
                "%s"
                "\t.byte\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 2: {  // SHORT_WIDTH
            acc = format(
                "%s"
                "\t.int\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 4: {  // INT_WIDTH, WCHAR_WIDTH
            acc = format(
                "%s"
                "\t.long\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 8: {  // LONG_WIDTH, POINTER_WIDTH
            acc = format(
                "%s"
                "\t.quad\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
        }
        break;
      }
      case OK_NAME: {
        acc = format(
            "%s"
            "\t.quad\t%s\n",
            acc, value->data.name.name);
        break;
      }
      case OK_STRING: {
        char *str = tstrToX86_64Str(value->data.string.data);
        acc = format(
            "%s"
            "%s",
            acc, str);
        free(str);
        break;
      }
      case OK_WSTRING: {
        char *str = twstrToX86_64WStr(value->data.wstring.data);
        acc = format(
            "%s"
            "%s",
            acc, str);
        free(str);
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid constant operand kind encountered");
      }
    }
  }
  return acc;
}

static X86_64File *fileInstructionSelect(IRFile *ir, Options *options) {
  X86_64File *file =
      x86_64FileCreate(format("\t.file\t\"%s\"\n", ir->sourceFilename),
                       format("\t.ident\t\"%s\"\n"
                              "\t.section\t.note.GNU-stack,\"\",@progbits\n",
                              VERSION_STRING));

  for (size_t idx = 0; idx < ir->fragments.size; idx++) {
    Fragment *irFrag = ir->fragments.elements[idx];
    switch (irFrag->kind) {
      case FK_BSS: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.bss.size);
        }
        x86_64FragmentVectorInsert(&file->fragments,
                                   x86_64DataFragmentCreate(format(
                                       "%s"
                                       "\t.bss\n"
                                       "\t.align\t%zu\t"
                                       "%s:\n"
                                       "\t.zero\t%zu\n",
                                       prefix, irFrag->data.bss.alignment,
                                       irFrag->label, irFrag->data.bss.size)));
        free(prefix);
        break;
      }
      case FK_RODATA: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu\n",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.rodata.size);
        }
        char *data = dataToString(irFrag->data.rodata.ir);
        x86_64FragmentVectorInsert(
            &file->fragments,
            x86_64DataFragmentCreate(format(
                "%s"
                "\t.section\t.rodata\n"
                "\t.align\t%zu\n"
                "%s:\n"
                "%s",
                prefix, irFrag->data.rodata.alignment, irFrag->label, data)));
        free(prefix);
        break;
      }
      case FK_DATA: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu\n",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.rodata.size);
        }
        char *data = dataToString(irFrag->data.rodata.ir);
        x86_64FragmentVectorInsert(
            &file->fragments,
            x86_64DataFragmentCreate(format(
                "%s"
                "\t.data\n"
                "\t.align\t%zu\n"
                "%s:\n"
                "%s",
                prefix, irFrag->data.rodata.alignment, irFrag->label, data)));
        free(prefix);
        break;
      }
      case FK_TEXT: {
        X86_64Fragment *frag = x86_64TextFragmentCreate(
            format("\t.text\n"
                   "\t.globl\t%s\n"
                   "\t.type \t%s, @function\n",
                   irFrag->label, irFrag->label),
            format("\t.size\t%s, .-%s\n", irFrag->label, irFrag->label));
        textInstructionSelect(frag, irFrag, &file->fragments,
                              ir->labelGenerator, options);
        x86_64FragmentVectorInsert(&file->fragments, frag);
        break;
      }
    }
  }

  return file;
}

void x86_64InstructionSelect(FileX86_64FileMap *asmFileMap,
                             FileIRFileMap *irFileMap, Options *options) {
  fileX86_64FileMapInit(asmFileMap);

  for (size_t idx = 0; idx < irFileMap->capacity; idx++) {
    char const *key = irFileMap->keys[idx];
    if (key != NULL) {
      IRFile *file = irFileMap->values[idx];
      fileX86_64FileMapPut(asmFileMap, irFileMap->keys[idx],
                           fileInstructionSelect(file, options));
    }
  }
}