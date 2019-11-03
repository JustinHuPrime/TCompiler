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
#include "architecture/x86_64/shorthand.h"
#include "constants.h"
#include "ir/allocHint.h"
#include "ir/ir.h"
#include "translate/translate.h"
#include "util/format.h"
#include "util/internalError.h"

#include <stdlib.h>
#include <string.h>

X86_64Operand *x86_64OperandCreate(IROperand const *irOperand) {
  X86_64Operand *op = malloc(sizeof(X86_64Operand));
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

// static bool sizeIsAtomic(size_t size) {
//   return size == BYTE_WIDTH || size == SHORT_WIDTH || size == INT_WIDTH ||
//          size == LONG_WIDTH;
// }
// static char atomicSizeToGPSizePostfix(size_t size) {
//   switch (size) {
//     case 1: {
//       return 'b';
//     }
//     case 2: {
//       return 'w';
//     }
//     case 4: {
//       return 'l';
//     }
//     case 8: {
//       return 'q';
//     }
//     default: { error(__FILE__, __LINE__, "not an atomic gp size"); }
//   }
// }
// static char atomicSizeToFPSizePostfix(size_t size) {
//   switch (size) {
//     case 4: {
//       return 's';
//     }
//     case 8: {
//       return 'd';
//     }
//     default: { error(__FILE__, __LINE__, "not a atomic fp size"); }
//   }
// }
static bool operandIsAtomic(IROperand const *op) {
  return op->kind != OK_TEMP || op->data.temp.kind != AH_MEM;
}
static bool operandIsSSE(IROperand const *op) {
  switch (op->kind) {
    case OK_CONSTANT:
    case OK_NAME:
    case OK_STACKOFFSET: {
      return false;
    }
    case OK_REG: {
      return x86_64RegIsSSE(x86_64RegNumToRegister(op->data.reg.n));
    }
    case OK_TEMP: {
      return op->data.temp.kind == AH_SSE;
    }
    default: { error(__FILE__, __LINE__, "invalid OperandKind enum"); }
  }
}
static char const *generateTypeSuffix(size_t opSize, bool isSSE) {
  if (isSSE) {
    switch (opSize) {
      case 4: {
        return "ss";
      }
      case 8: {
        return "sd";
      }
      default: { error(__FILE__, __LINE__, "invalid operand size"); }
    }
  } else {
    // gp
    switch (opSize) {
      case 1: {
        return "b";
      }
      case 2: {
        return "w";
      }
      case 4: {
        return "l";
      }
      case 8: {
        return "q";
      }
      default: { error(__FILE__, __LINE__, "invalid operand size"); }
    }
  }
}
static void addFPConstant(X86_64FragmentVector *frags, size_t size,
                          char const *label, uint64_t bits) {
  x86_64FragmentVectorInsert(
      frags, x86_64DataFragmentCreate(
                 format("\t.section\t.rodata\n"
                        "\t.align\t%zu\n"
                        "%s:\n"
                        "\t%s\t%lu\n",
                        size, label, size == 4 ? ".long" : ".quad", bits)));
}
static IROperand *loadOperand(IROperand *op, bool isSSE, size_t size,
                              char const *typeSuffix,
                              X86_64InstructionVector *assembly,
                              X86_64FragmentVector *frags,
                              LabelGenerator *labelGenerator,
                              TempAllocator *tempAllocator, Options *options) {
  switch (op->kind) {
    case OK_CONSTANT: {
      if (isSSE) {
        // add rodata fragment
        char *label = labelGeneratorGenerateDataLabel(labelGenerator);
        addFPConstant(frags, size, label, op->data.constant.bits);
        IROperand *temp = tempIROperandCreate(
            tempAllocatorAllocate(tempAllocator), size, size, AH_SSE);

        X86_64Instruction *load =
            X86_64INSTR(format("\tmov%s\t%s(%%rip), `d\n", typeSuffix, label));
        X86_64DEF(load, temp);
        X86_64INSERT(assembly, load);

        free(label);
        return temp;
      } else {
        if (size == 8) {
          typeSuffix = "absq";  // special case for constant loads
        }
        IROperand *temp = tempIROperandCreate(
            tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

        X86_64Instruction *load = X86_64INSTR(
            format("\tmov%s\t$%lu, `d\n", typeSuffix, op->data.constant.bits));
        X86_64DEF(load, temp);
        X86_64INSERT(assembly, load);

        return temp;
      }
    }
    case OK_NAME: {
      switch (optionsGet(options, optionPositionIndependence)) {
        case O_PI_NONE: {
          IROperand *temp = tempIROperandCreate(
              tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

          X86_64Instruction *load =
              X86_64INSTR(format("\tmovq\t$%s, `d\n", op->data.name.name));
          X86_64DEF(load, temp);
          X86_64INSERT(assembly, load);

          return temp;
        }
        case O_PI_PIE: {
          IROperand *temp = tempIROperandCreate(
              tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

          X86_64Instruction *load = X86_64INSTR(
              format("\tleaq\t%s(%%rip), `d\n", op->data.name.name));
          X86_64DEF(load, temp);
          X86_64INSERT(assembly, load);

          return temp;
        }
        case O_PI_PIC: {
          IROperand *temp = tempIROperandCreate(
              tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

          X86_64Instruction *load = X86_64INSTR(
              format("\tmovq\t%s@GOTPCREL(%%rip), `d\n", op->data.name.name));
          X86_64DEF(load, temp);
          X86_64INSERT(assembly, load);

          return temp;
        }
        default: {
          error(__FILE__, __LINE__, "invalid PositionIndependenceType enum");
        }
      }
      break;
    }
    case OK_STACKOFFSET: {
      IROperand *temp = tempIROperandCreate(
          tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

      X86_64Instruction *load = X86_64INSTR(
          format("\tmovabsq\t$`o, `d\n"));  // note - should be optimized
                                            // after register allocation
      X86_64OTHER(load, op);
      X86_64USE(load, temp);
      X86_64INSERT(assembly, load);

      return temp;
    }
    case OK_REG:
    case OK_TEMP: {
      return op;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid or unexpected IROperandKind enum - should not be string, "
            "wstring, or asm");
    }
  }
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
        X86_64INSERT(assembly,
                     X86_64INSTR(strdup(entry->arg1->data.assembly.assembly)));
        break;
      }
      case IO_LABEL: {
        X86_64INSERT(assembly,
                     X86_64INSTR(format("%s:\n", entry->arg1->data.name.name)));
        break;
      }
      case IO_MOVE: {
        if (operandIsAtomic(entry->arg1)) {
          bool isSSE = operandIsSSE(entry->arg1);
          char const *typeSuffix = generateTypeSuffix(entry->opSize, isSSE);

          // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
          IROperand *from = loadOperand(entry->arg1, isSSE, entry->opSize,
                                        typeSuffix, assembly, frags,
                                        labelGenerator, tempAllocator, options);
          IROperand *to = loadOperand(entry->dest, isSSE, entry->opSize,
                                      typeSuffix, assembly, frags,
                                      labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, from);
          X86_64DEF(move, to);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        } else {
          error(__FILE__, __LINE__, "not yet implemented");
        }
        break;
      }
      case IO_MEM_STORE: {
        if (operandIsAtomic(entry->arg1)) {
          bool isSSE = operandIsSSE(entry->arg1);
          char const *typeSuffix = generateTypeSuffix(entry->opSize, isSSE);

          // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
          IROperand *from = loadOperand(entry->arg1, isSSE, entry->opSize,
                                        typeSuffix, assembly, frags,
                                        labelGenerator, tempAllocator, options);
          IROperand *to = loadOperand(entry->dest, isSSE, entry->opSize,
                                      typeSuffix, assembly, frags,
                                      labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, (`u)\n", typeSuffix));
          X86_64USE(move, from);
          X86_64USE(move, to);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        } else {
          error(__FILE__, __LINE__, "not yet implemented");
        }
        break;
      }
      case IO_MEM_LOAD: {
        if (operandIsAtomic(entry->arg1)) {
          bool isSSE = operandIsSSE(entry->arg1);
          char const *typeSuffix = generateTypeSuffix(entry->opSize, isSSE);

          // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
          IROperand *from = loadOperand(entry->arg1, isSSE, entry->opSize,
                                        typeSuffix, assembly, frags,
                                        labelGenerator, tempAllocator, options);
          IROperand *to = loadOperand(entry->dest, isSSE, entry->opSize,
                                      typeSuffix, assembly, frags,
                                      labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t(`u), `u\n", typeSuffix));
          X86_64USE(move, from);
          X86_64USE(move, to);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        } else {
          error(__FILE__, __LINE__, "not yet implemented");
        }
        break;
      }
      case IO_STK_STORE: {
        if (operandIsAtomic(entry->arg1)) {
          bool isSSE = operandIsSSE(entry->arg1);
          char const *typeSuffix = generateTypeSuffix(entry->opSize, isSSE);

          // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
          IROperand *from = loadOperand(entry->arg1, isSSE, entry->opSize,
                                        typeSuffix, assembly, frags,
                                        labelGenerator, tempAllocator, options);
          IROperand *to = loadOperand(entry->dest, isSSE, entry->opSize,
                                      typeSuffix, assembly, frags,
                                      labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, (%%rbp, `u)\n", typeSuffix));
          X86_64USE(move, from);
          X86_64USE(move, to);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        } else {
          error(__FILE__, __LINE__, "not yet implemented");
        }
        break;
      }
      case IO_STK_LOAD: {
        if (operandIsAtomic(entry->arg1)) {
          bool isSSE = operandIsSSE(entry->arg1);
          char const *typeSuffix = generateTypeSuffix(entry->opSize, isSSE);

          // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
          IROperand *from = loadOperand(entry->arg1, isSSE, entry->opSize,
                                        typeSuffix, assembly, frags,
                                        labelGenerator, tempAllocator, options);
          IROperand *to = loadOperand(entry->dest, isSSE, entry->opSize,
                                      typeSuffix, assembly, frags,
                                      labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t(%%rbp, `u), `u\n", typeSuffix));
          X86_64USE(move, from);
          X86_64USE(move, to);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        } else {
          error(__FILE__, __LINE__, "not yet implemented");
        }
        break;
      }
      case IO_OFFSET_STORE: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_OFFSET_LOAD: {
        error(__FILE__, __LINE__, "not yet implemented");
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