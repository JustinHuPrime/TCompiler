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

// implementation of x86_64 instruction selection
#include "architecture/x86_64/instructionSelection.h"

#include "architecture/x86_64/frame.h"
#include "architecture/x86_64/shorthand.h"
#include "constants.h"
#include "translate/translate.h"
#include "util/format.h"
#include "util/internalError.h"

#include <stdlib.h>
#include <string.h>

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
      case 16: {
        return "o";  // for division
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
        X86_64DEF(load, temp, size);
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
        X86_64DEF(load, temp, size);
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
          X86_64DEF(load, temp, size);
          X86_64INSERT(assembly, load);

          return temp;
        }
        case O_PI_PIE: {
          IROperand *temp = tempIROperandCreate(
              tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

          X86_64Instruction *load = X86_64INSTR(
              format("\tleaq\t%s(%%rip), `d\n", op->data.name.name));
          X86_64DEF(load, temp, size);
          X86_64INSERT(assembly, load);

          return temp;
        }
        case O_PI_PIC: {
          IROperand *temp = tempIROperandCreate(
              tempAllocatorAllocate(tempAllocator), size, size, AH_GP);

          X86_64Instruction *load = X86_64INSTR(
              format("\tmovq\t%s@GOTPCREL(%%rip), `d\n", op->data.name.name));
          X86_64DEF(load, temp, size);
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
      X86_64OTHER(load, op, size);
      X86_64USE(load, temp, size);
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
static void vectorInstructionSelect(IREntryVector *ir,
                                    X86_64InstructionVector *assembly,
                                    X86_64FragmentVector *frags,
                                    LabelGenerator *labelGenerator,
                                    TempAllocator *tempAllocator,
                                    Options *options) {
  for (size_t idx = 0; idx < ir->size; idx++) {
    IREntry *entry = ir->elements[idx];
    switch (entry->op) {
      case IO_ASM: {
        // special case - inline asm
        X86_64INSERT(assembly,
                     X86_64INSTR(strdup(entry->arg1->data.assembly.assembly)));
        break;
      }
      case IO_LABEL: {
        // special case - basically inline asm
        X86_64INSERT(assembly,
                     X86_64LABEL(format("%s:\n", entry->arg1->data.name.name),
                                 strdup(entry->arg1->data.name.name)));
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
          // entry->dest is always a temp or reg

          // execution
          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, from, entry->opSize);
          X86_64DEF(move, entry->dest, entry->opSize);
          X86_64INSERT(assembly, move);

          // cleanup
          if (from != entry->arg1) {
            irOperandDestroy(from);
          }
          // to is always a temp or reg
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
              X86_64INSTR(format("\tmov%s\t`u, (`u)\n", typeSuffix));
          X86_64USE(move, from, entry->opSize);
          X86_64USE(move, to, entry->opSize);
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
              X86_64INSTR(format("\tmov%s\t(`u), `u\n", typeSuffix));
          X86_64USE(move, from, entry->opSize);
          X86_64USE(move, to, entry->opSize);
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
              X86_64INSTR(format("\tmov%s\t`u, (%%rbp, `u)\n", typeSuffix));
          X86_64USE(move, from, entry->opSize);
          X86_64USE(move, to, entry->opSize);
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
              X86_64INSTR(format("\tmov%s\t(%%rbp, `u), `u\n", typeSuffix));
          X86_64USE(move, from, entry->opSize);
          X86_64USE(move, to, entry->opSize);
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
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tadd%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_ADD: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tadd%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_SUB: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tsub%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_SUB: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tsub%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_SMUL: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\timul%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_UMUL: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tmul%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_MUL: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tmul%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_SDIV: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        if (entry->opSize == 1) {
          // special case - no c_t_ required
          IROperand *rax = regIROperandCreate(X86_64_RAX);

          X86_64Instruction *move =
              X86_64INSTR(format("\tmovs%s%s\t`u, `d\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize * 2);
          X86_64INSERT(assembly, move);

          X86_64Instruction *op =
              X86_64INSTR(format("\tidiv%s\t`u\n", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize * 2);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, retrieve);

          irOperandDestroy(rax);
        } else {
          IROperand *rax = regIROperandCreate(X86_64_RAX);
          IROperand *rdx = regIROperandCreate(X86_64_RDX);

          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize);
          X86_64INSERT(assembly, move);

          X86_64Instruction *extend =
              X86_64INSTR(format("\tc%st%s\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(extend, rax, entry->opSize);
          X86_64DEF(extend, rax, entry->opSize);
          X86_64DEF(extend, rdx, entry->opSize);
          X86_64INSERT(assembly, extend);

          X86_64Instruction *op =
              X86_64INSTR(format("\tidiv%s\t`u", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize);
          X86_64DEF(op, rdx, entry->opSize);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, op);

          irOperandDestroy(rax);
          irOperandDestroy(rdx);
        }

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_UDIV: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        if (entry->opSize == 1) {
          // special case - no zeroing required
          IROperand *rax = regIROperandCreate(X86_64_RAX);

          X86_64Instruction *move =
              X86_64INSTR(format("\tmovz%s%s\t`u, `d\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize * 2);
          X86_64INSERT(assembly, move);

          X86_64Instruction *op =
              X86_64INSTR(format("\tdiv%s\t`u\n", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize * 2);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, retrieve);

          irOperandDestroy(rax);
        } else {
          IROperand *rax = regIROperandCreate(X86_64_RAX);
          IROperand *rdx = regIROperandCreate(X86_64_RDX);

          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize);
          X86_64INSERT(assembly, move);

          X86_64Instruction *zero =
              X86_64INSTR(format("\txor%s\n", typeSuffix));
          X86_64DEF(zero, rdx, entry->opSize);
          X86_64INSERT(assembly, zero);

          X86_64Instruction *op =
              X86_64INSTR(format("\tdiv%s\t`u", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize);
          X86_64DEF(op, rdx, entry->opSize);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, op);

          irOperandDestroy(rax);
          irOperandDestroy(rdx);
        }

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_DIV: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg1, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tdiv%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg2, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, op);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_SMOD: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        if (entry->opSize == 1) {
          // special case - no c_t_ required
          IROperand *rax = regIROperandCreate(X86_64_RAX);

          X86_64Instruction *move =
              X86_64INSTR(format("\tmovs%s%s\t`u, `d\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize * 2);
          X86_64INSERT(assembly, move);

          X86_64Instruction *op =
              X86_64INSTR(format("\tidiv%s\t`u\n", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize * 2);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t%%ah, `d\n", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, retrieve);

          irOperandDestroy(rax);
        } else {
          IROperand *rax = regIROperandCreate(X86_64_RAX);
          IROperand *rdx = regIROperandCreate(X86_64_RDX);

          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize);
          X86_64INSERT(assembly, move);

          X86_64Instruction *extend =
              X86_64INSTR(format("\tc%st%s\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(extend, rax, entry->opSize);
          X86_64DEF(extend, rax, entry->opSize);
          X86_64DEF(extend, rdx, entry->opSize);
          X86_64INSERT(assembly, extend);

          X86_64Instruction *op =
              X86_64INSTR(format("\tidiv%s\t`u", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize);
          X86_64DEF(op, rdx, entry->opSize);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d", typeSuffix));
          X86_64USE(retrieve, rdx, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, op);

          irOperandDestroy(rax);
          irOperandDestroy(rdx);
        }

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_UMOD: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        if (entry->opSize == 1) {
          // special case - no zeroing required
          IROperand *rax = regIROperandCreate(X86_64_RAX);

          X86_64Instruction *move =
              X86_64INSTR(format("\tmovz%s%s\t`u, `d\n", typeSuffix,
                                 generateTypeSuffix(entry->opSize * 2, false)));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize * 2);
          X86_64INSERT(assembly, move);

          X86_64Instruction *op =
              X86_64INSTR(format("\tdiv%s\t`u\n", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize * 2);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t%%ah, `d\n", typeSuffix));
          X86_64USE(retrieve, rax, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, retrieve);

          irOperandDestroy(rax);
        } else {
          IROperand *rax = regIROperandCreate(X86_64_RAX);
          IROperand *rdx = regIROperandCreate(X86_64_RDX);

          X86_64Instruction *move =
              X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
          X86_64USE(move, arg1, entry->opSize);
          X86_64DEF(move, rax, entry->opSize);
          X86_64INSERT(assembly, move);

          X86_64Instruction *zero =
              X86_64INSTR(format("\txor%s\n", typeSuffix));
          X86_64DEF(zero, rdx, entry->opSize);
          X86_64INSERT(assembly, zero);

          X86_64Instruction *op =
              X86_64INSTR(format("\tdiv%s\t`u", typeSuffix));
          X86_64USE(op, arg2, entry->opSize);
          X86_64DEF(op, rax, entry->opSize);
          X86_64DEF(op, rdx, entry->opSize);
          X86_64INSERT(assembly, op);

          X86_64Instruction *retrieve =
              X86_64MOVE(format("\tmov%s\t`u, `d", typeSuffix));
          X86_64USE(retrieve, rdx, entry->opSize);
          X86_64DEF(retrieve, to, entry->opSize);
          X86_64INSERT(assembly, op);

          irOperandDestroy(rax);
          irOperandDestroy(rdx);
        }

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_SLL: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_SLR: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_SAR: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_AND: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tand%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_XOR: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\txor%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_OR: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg2, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tor%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, arg1, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_L: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetl\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_LE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetle\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_E: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsete\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_NE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetne\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_GE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetge\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_G: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetg\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_A: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tseta\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_AE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetae\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_B: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetb\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_BE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetbe\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_L: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetl\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_LE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetle\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_E: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsete\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_NE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetne\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_GE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetge\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_G: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64Instruction *set = X86_64INSTR(format("\tsetg\t`d\n"));
        X86_64DEF(set, to, 1);
        X86_64INSERT(assembly, set);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_NEG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg1, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tneg%s\t`d\n", typeSuffix));
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        break;
      }
      case IO_FP_NEG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg1, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        // TODO: do an xorpd with loaded "negation" constant - might want to
        // collapse multiple negation constants into one.
        error(__FILE__, __LINE__, "not yet implemented");

        X86_64Instruction *op =
            X86_64INSTR(format("\tneg%s\t`d\n", typeSuffix));
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        break;
      }
      case IO_LNOT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg1, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\txor%s\t$1, `d\n", typeSuffix));
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        break;
      }
      case IO_NOT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *move =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(move, arg1, entry->opSize);
        X86_64DEF(move, to, entry->opSize);
        X86_64INSERT(assembly, move);

        X86_64Instruction *op =
            X86_64INSTR(format("\tnot%s\t`d\n", typeSuffix));
        X86_64DEF(op, to, entry->opSize);
        X86_64USE(op, to, entry->opSize);  // also used
        X86_64INSERT(assembly, move);

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        break;
      }
      case IO_SX_SHORT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovs%sw\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 2);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_SX_INT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovs%sl\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 4);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_SX_LONG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovs%sq\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 8);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_ZX_SHORT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovz%sw\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 2);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_ZX_INT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovz%sl\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 4);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_ZX_LONG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64INSTR(format("\tmovz%sq\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 8);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_U_TO_FLOAT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        // TODO: coerce into signed form
        error(__FILE__, __LINE__, "not yet implemented");
        X86_64Instruction *op =
            X86_64INSTR(format("\tcvtsi2ss%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, 8);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_U_TO_DOUBLE: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_S_TO_FLOAT: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_S_TO_DOUBLE: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_F_TO_FLOAT: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_F_TO_DOUBLE: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_TRUNC_BYTE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_TRUNC_SHORT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_TRUNC_INT: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *from =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *to = entry->dest;  // to is always a temp or reg

        // execution
        X86_64Instruction *op =
            X86_64MOVE(format("\tmov%s\t`u, `d\n", typeSuffix));
        X86_64USE(op, from, entry->opSize);
        X86_64DEF(op, to, entry->opSize);
        X86_64INSERT(assembly, op);

        // cleanup
        if (from != entry->arg1) {
          irOperandDestroy(from);
        }
        break;
      }
      case IO_F_TO_BYTE: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_F_TO_SHORT: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_F_TO_INT: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_F_TO_LONG: {
        error(__FILE__, __LINE__, "not yet implemented");
        break;
      }
      case IO_JUMP: {
        if (entry->dest->kind == OK_NAME) {
          // special case for OK_NAME
          X86_64INSERT(
              assembly,
              X86_64JUMP(format("\tjmp\t%s\n", entry->dest->data.name.name),
                         strdup(entry->dest->data.name.name)));
        } else {
          char const *typeSuffix = generateTypeSuffix(8, false);

          // setup - gets OK_CONST, OK_STACKOFFSET into right places
          IROperand *to =
              loadOperand(entry->dest, false, 8, typeSuffix, assembly, frags,
                          labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *jump =
              X86_64SWITCH(format("\tjmp%s\t*`u\n", typeSuffix));
          // TODO: include possible next instructions
          X86_64USE(jump, to, 8);
          X86_64INSERT(assembly, jump);

          // cleanup
          if (to != entry->dest) {
            irOperandDestroy(to);
          }
        }
        break;
      }
      case IO_JL: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjl\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JLE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjle\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tje\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JNE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjne\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JGE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjge\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjg\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JA: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tja\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JAE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjae\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JB: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjb\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_JBE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, false);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, false, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcmp%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjbe\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JL: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjl\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JLE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjle\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tje\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JNE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjne\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JGE: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjge\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_FP_JG: {
        char const *typeSuffix = generateTypeSuffix(entry->opSize, true);

        // setup - gets OK_CONST, OK_NAME, OK_STACKOFFSET into right places
        IROperand *arg1 =
            loadOperand(entry->arg1, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);
        IROperand *arg2 =
            loadOperand(entry->arg2, true, entry->opSize, typeSuffix, assembly,
                        frags, labelGenerator, tempAllocator, options);

        // execution
        X86_64Instruction *cmp =
            X86_64MOVE(format("\tcomi%s\t`u, `u\n", typeSuffix));
        X86_64USE(cmp, arg2, entry->opSize);
        X86_64DEF(cmp, arg1, entry->opSize);
        X86_64INSERT(assembly, cmp);

        X86_64INSERT(
            assembly,
            X86_64CJUMP(format("\tjg\t%s", entry->dest->data.name.name),
                        strdup(entry->dest->data.name.name)));

        // cleanup
        if (arg1 != entry->arg1) {
          irOperandDestroy(arg1);
        }
        if (arg2 != entry->arg2) {
          irOperandDestroy(arg2);
        }
        break;
      }
      case IO_CALL: {
        if (entry->arg1->kind == OK_NAME) {
          // special case for OK_NAME
          X86_64INSERT(
              assembly,
              X86_64INSTR(format("\tcall\t%s\n", entry->arg1->data.name.name)));
        } else {
          char const *typeSuffix = generateTypeSuffix(8, false);

          // setup - gets OK_CONST, OK_STACKOFFSET into right places
          IROperand *to =
              loadOperand(entry->arg1, false, 8, typeSuffix, assembly, frags,
                          labelGenerator, tempAllocator, options);

          // execution
          X86_64Instruction *call =
              X86_64INSTR(format("\tcall%s\t*`u\n", typeSuffix));
          X86_64USE(call, to, 8);
          X86_64INSERT(assembly, call);

          // cleanup
          if (to != entry->arg1) {
            irOperandDestroy(to);
          }
        }
        break;
      }
      case IO_RETURN: {
        // special case - no arguments
        X86_64INSERT(assembly, X86_64LEAVE(format("\tret\n")));
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "invalid or unexpected ir operator encountered");
      }
    }
  }
}
static void textInstructionSelect(X86_64Fragment *frag, Fragment *irFrag,
                                  X86_64FragmentVector *frags,
                                  LabelGenerator *labelGenerator,
                                  Options *options) {
  TempAllocator *tempAllocator = irFrag->data.text.tempAllocator;
  X86_64Frame *frame = (X86_64Frame *)irFrag->data.text.frame;
  X86_64InstructionVector *assembly = &frag->data.text.body;

  vectorInstructionSelect(frame->functionPrologue, assembly, frags,
                          labelGenerator, tempAllocator, options);
  vectorInstructionSelect(irFrag->data.text.ir, assembly, frags, labelGenerator,
                          tempAllocator, options);
  vectorInstructionSelect(frame->functionEpilogue, assembly, frags,
                          labelGenerator, tempAllocator, options);
}

// data fragment stuff
static bool isLocalLabel(char const *str) { return strncmp(str, ".L", 2) == 0; }
static char *tstrToX86_64Str(uint8_t *str) {
  char *buffer = strdup("");
  for (; *str != 0; str++) {
    char *newBuffer = format(
        "%s"
        "\t.byte\t%hhu\n",
        buffer, *str);
    free(buffer);
    buffer = newBuffer;
  }
  char *newBuffer = format(
      "%s"
      "\t.byte\t0\n",
      buffer);
  free(buffer);
  buffer = newBuffer;
  return buffer;
}
static char *twstrToX86_64WStr(uint32_t *wstr) {
  char *buffer = strdup("");
  for (; *wstr != 0; wstr++) {
    char *newBuffer = format(
        "%s"
        "\t.long\t%u\n",
        buffer, *wstr);
    free(buffer);
    buffer = newBuffer;
  }
  char *newBuffer = format(
      "%s"
      "\t.long\t0\n",
      buffer);
  free(buffer);
  buffer = newBuffer;
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
            char *newAcc = format(
                "%s"
                "\t.byte\t%lu\n",
                acc, value->data.constant.bits);
            free(acc);
            acc = newAcc;
            break;
          }
          case 2: {  // SHORT_WIDTH
            char *newAcc = format(
                "%s"
                "\t.int\t%lu\n",
                acc, value->data.constant.bits);
            free(acc);
            acc = newAcc;
            break;
          }
          case 4: {  // INT_WIDTH, WCHAR_WIDTH
            char *newAcc = format(
                "%s"
                "\t.long\t%lu\n",
                acc, value->data.constant.bits);
            free(acc);
            acc = newAcc;
            break;
          }
          case 8: {  // LONG_WIDTH, POINTER_WIDTH
            char *newAcc = format(
                "%s"
                "\t.quad\t%lu\n",
                acc, value->data.constant.bits);
            free(acc);
            acc = newAcc;
            break;
          }
        }
        break;
      }
      case OK_NAME: {
        char *newAcc = format(
            "%s"
            "\t.quad\t%s\n",
            acc, value->data.name.name);
        free(acc);
        acc = newAcc;
        break;
      }
      case OK_STRING: {
        char *str = tstrToX86_64Str(value->data.string.data);
        char *newAcc = format(
            "%s"
            "%s",
            acc, str);
        free(acc);
        acc = newAcc;
        free(str);
        break;
      }
      case OK_WSTRING: {
        char *str = twstrToX86_64WStr(value->data.wstring.data);
        char *newAcc = format(
            "%s"
            "%s",
            acc, str);
        free(acc);
        acc = newAcc;
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
        free(data);
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
            format("\t.size\t%s, .-%s\n", irFrag->label, irFrag->label),
            (X86_64Frame *)irFrag->data.text.frame);
        textInstructionSelect(frag, irFrag, &file->fragments,
                              ir->labelGenerator, options);
        irFrag->data.text.frame = NULL;
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