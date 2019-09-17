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

// Three-address code based intermediate representation

#ifndef TLC_IR_IR_H_
#define TLC_IR_IR_H_

#include "util/container/vector.h"

#include <stdint.h>

typedef enum {
  OK_TEMP,
  OK_REG,
  OK_CONSTANT,
  OK_LABEL,
  OK_ASM,
} OperandKind;
typedef struct IROperand {
  OperandKind kind;
  union {
    struct {
      size_t n;
    } temp;
    struct {
      size_t n;  // specific to the target architecture
    } reg;
    struct {
      uint64_t bits;  // truncated based on size
    } constant;
    struct {
      char *name;
    } label;
    struct {
      char *assembly;
    } assembly;
  } operand;
} IROperand;
// IROperand *irOperandTemp(size_t n);
// IROperand *irOperandReg(size_t n);
// IROperand *irOperandUByte(uint8_t value);
// IROperand *irOperandByte(int8_t value);
// IROperand *irOperandUShort(uint16_t value);
// IROperand *irOperandShort(int16_t value);
// IROperand *irOperandUInt(uint32_t value);
// IROperand *irOperandInt(int32_t value);
// IROperand *irOperandULong(uint64_t value);
// IROperand *irOperandLong(uint64_t value);
// IROperand *irOperandFloat(uint32_t bits);
// IROperand *irOperandDouble(uint64_t bits);
// IROperand *irOperandLabel(char *);
void irOperandDestroy(IROperand *);

typedef enum {
  IO_CONST,  // constant value in memory: opSize = sizeof(constant), dest =
             // NULL, arg1 = constant bits, arg2 = NULL

  IO_ASM,  // inline assembly: opSize = 0, dest = NULL, arg1 = assembly, arg2 =
           // NULL

  IO_LABEL,  // label this entry: opSize = 0, dest = NULL, arg1 = label name,
             // arg2 = NULL

  IO_MOVE,           // move to temp or reg: opSize = sizeof(target), dest =
                     // target reg or temp, arg1 = source
  IO_MOVE_TO_MEM,    // move from temp or reg to mem, opSize = sizeof(temp or
                     // reg), dest = destination address, arg1 = source, arg2 =
                     // NULL
  IO_MOVE_FROM_MEM,  // move from mem to temp or reg, opSize = sizeof(temp or
                     // reg), dest = destination temp or reg, arg1 = source
                     // address, arg2 = NULL
  IO_MOVE_BETWEEN_MEM,  // move from mem to mem, opSize = sizeof(mem to move),
                        // dest = destination address, arg1 = source address,
                        // arg2 = NULL // TODO: do we use/need this?

  IO_ADD,  // plain binary operations: opSize = sizeof(operands), dest = result
           // storage, arg1 = argument 1, arg2 = argument 2
  IO_FP_ADD,
  IO_SUB,
  IO_FP_SUB,
  IO_SMUL,
  IO_UMUL,
  IO_FP_MUL,
  IO_SDIV,
  IO_UDIV,
  IO_FP_DIV,
  IO_SMOD,
  IO_UMOD,
  // TODO: other arithmetic operations

  IO_NEG,  // plain unary operations: opSize = sizeof(operand), dest = result
           // storage, arg1 = target, arg2 = NULL

  IO_JUMP,  // unconditional jump: opSize = 0, dest = jump target
            // address, arg1 = NULL, arg2 = NULL

  IO_JL,  // conditional jumps: opSize = sizeof(compared values), dest = jump
          // target address, arg1 = lhs, arg2 = rhs
  IO_JLE,
  IO_JE,
  IO_JNE,
  IO_JGE,
  IO_JG,

  IO_CALL,    // function call: opSize = 0, dest = function call target, arg1 =
              // NULL, arg2 = NULL Note that the argument list is set up
              // beforehand by moves, handled by the stack frame
  IO_RETURN,  // return from function: opSize = 0, dest = NULL, arg1 = NULL,
              // arg2 = NULL note that the return value is set up beforehand by
              // moves, handled by the stack frame
} IROperator;
typedef struct IREntry {
  IROperator op;
  size_t opSize;
  IROperand *dest;  // nullable, if const
  IROperand *arg1;
  IROperand *arg2;  // nullable
} IREntry;
// TODO: constant constructors
// IREntry *irEntryMoveCreate(size_t opSize, IROperand *dest, IROperand
// *source); IREntry *irEntryMoveToMemCreate(size_t opSize, IROperand *destAddr,
//                                 IROperand *source);
// IREntry *irEntryMoveFromMemCreate(size_t opSize, IROperand *dest,
//                                   IROperand *sourceAddr);
// IREntry *irEntryPlainBinopCreate(size_t opSize, IROperator op, IROperand
// *dest,
//                                  IROperand *arg1, IROperand *arg2);
// IREntry *irEntryPlainUnopCreate(size_t opSize, IROperator op, IROperand
// *dest,
//                                 IROperand *arg);
// IREntry *irEntryJump(char *dest);
void irEntryDestroy(IREntry *);

typedef Vector IRVector;
IRVector *irVectorCreate(void);
void irVectorInit(IRVector *);
void irVectorInsert(IRVector *, IREntry *);
void irVectorUninit(IRVector *);
void irVectorDestroy(IRVector *);

#endif  // TLC_IR_IR_H_