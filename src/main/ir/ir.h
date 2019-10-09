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
  AH_GP,
  AH_MEM,
  AH_SSE,
} AllocHint;
typedef enum {
  OK_TEMP,
  OK_REG,
  OK_CONSTANT,
  OK_NAME,
  OK_ASM,
  OK_STRING,
  OK_WSTRING,
  OK_STACKOFFSET,
} OperandKind;
// an operand in an IR entry
typedef struct IROperand {
  OperandKind kind;
  union {
    struct {
      size_t n;
      size_t alignment;
      size_t size;
      AllocHint kind;
    } temp;
    struct {
      size_t n;  // specific to the target architecture
    } reg;
    struct {
      uint64_t bits;  // truncated based on size
    } constant;
    struct {
      char *name;
    } name;
    struct {
      char *assembly;
    } assembly;
    struct {
      uint8_t *data;
    } string;
    struct {
      uint32_t *data;
    } wstring;
    struct {
      int64_t stackOffset;
    } stackOffset;
  } data;
} IROperand;
// ctors
IROperand *tempIROperandCreate(size_t n, size_t size, size_t alignment,
                               AllocHint kind);
IROperand *regIROperandCreate(size_t n);
IROperand *ubyteIROperandCreate(uint8_t value);
IROperand *byteIROperandCreate(int8_t value);
IROperand *ushortIROperandCreate(uint16_t value);
IROperand *shortIROperandCreate(int16_t value);
IROperand *uintIROperandCreate(uint32_t value);
IROperand *intIROperandCreate(int32_t value);
IROperand *ulongIROperandCreate(uint64_t value);
IROperand *longIROperandCreate(int64_t value);
IROperand *floatIROperandCreate(uint32_t bits);
IROperand *doubleIROperandCreate(uint64_t bits);
IROperand *nameIROperandCreate(char *name);
IROperand *asmIROperandCreate(char *assembly);
IROperand *stringIROperandCreate(uint8_t *data);
IROperand *wstringIROperandCreate(uint32_t *data);
IROperand *stackOffsetIROperandCreate(int64_t baseOffset);
IROperand *irOperandCopy(IROperand const *);
// dtor
void irOperandDestroy(IROperand *);

typedef enum IROperator {
  IO_CONST,  // constant value in memory: opSize = sizeof(constant) or zero if a
             // string, dest = NULL, arg1 = constant bits or label, arg2 = NULL

  IO_ASM,  // inline assembly: opSize = 0, dest = NULL, arg1 = assembly, arg2 =
           // NULL

  IO_LABEL,  // label this entry: opSize = 0, dest = NULL, arg1 = label name,
             // arg2 = NULL

  IO_MOVE,          // move to temp or reg: opSize = sizeof(target), dest =
                    // target reg or temp, arg1 = source, arg2 = NULL
  IO_MEM_STORE,     // move from temp or reg to mem, opSize = sizeof(temp or
                    // reg), dest = destination address, arg1 = source, arg2 =
                    // NULL
  IO_MEM_LOAD,      // move from mem to temp or reg, opSize = sizeof(temp or
                    // reg), dest = destination temp or reg, arg1 = source
                    // address, arg2 = NULL
  IO_STK_STORE,     // move from temp or reg to stack at constant offset
                    // dest = destination offset (POINTER_WIDTH), arg1 = source
                    // data
  IO_STK_LOAD,      // move from stack at constant offset to register or temp
                    // dest = destination temp or reg, arg1 = destination offset
                    // (POINTER_WIDTH)
  IO_OFFSET_STORE,  // move from temp or reg to part of a mem temp
                    // dest = destination memtemp, arg1 = source temp or reg,
                    // arg2 = offset (POINTER_WIDTH)
  IO_OFFSET_LOAD,   // move from mem temp to temp or reg
                    // dest = destination temp or reg, arg1 = source memtemp,
                    // arg2 = offset (POINTER_WIDTH)

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
  IO_SLL,  // shift logical left - note that for all shifts, rhs size is 1
  IO_SLR,  // shift logical right
  IO_SAR,  // shift arithmetic right
  IO_AND,
  IO_XOR,
  IO_OR,
  IO_L,  // produce 1 if comparison is true, 0 if false
  IO_LE,
  IO_E,
  IO_NE,
  IO_GE,
  IO_G,
  IO_A,
  IO_AE,
  IO_B,
  IO_BE,
  IO_FP_L,
  IO_FP_LE,
  IO_FP_E,
  IO_FP_NE,
  IO_FP_GE,
  IO_FP_G,

  IO_NEG,  // plain unary operations: opSize = sizeof(operand), dest = result
           // storage, arg1 = target, arg2 = NULL
  IO_FP_NEG,
  IO_LNOT,
  IO_NOT,
  IO_SX_SHORT,  // sign extend integers, dest has size short
  IO_SX_INT,
  IO_SX_LONG,
  IO_ZX_SHORT,
  IO_ZX_INT,
  IO_ZX_LONG,
  IO_U_TO_FLOAT,  // integer to float
  IO_U_TO_DOUBLE,
  IO_S_TO_FLOAT,
  IO_S_TO_DOUBLE,
  IO_F_TO_FLOAT,  // float size changes
  IO_F_TO_DOUBLE,

  IO_JUMP,  // unconditional jump: opSize = 0, dest = jump target
            // address, arg1 = NULL, arg2 = NULL

  IO_JL,  // conditional jumps: opSize = sizeof(compared values), dest = jump
          // target address, arg1 = lhs, arg2 = rhs
  IO_JLE,
  IO_JE,
  IO_JNE,
  IO_JGE,
  IO_JG,
  IO_JA,
  IO_JAE,
  IO_JB,
  IO_JBE,
  IO_FP_JL,
  IO_FP_JLE,
  IO_FP_JE,
  IO_FP_JNE,
  IO_FP_JGE,
  IO_FP_JG,

  IO_CALL,    // function call: opSize = 0, dest = NULL, arg1 = function call
              // target, arg2 = NULL Note that the argument list is set up
              // beforehand by moves, handled by the stack frame
  IO_RETURN,  // return from function: opSize = 0, dest = NULL, arg1 = NULL,
              // arg2 = NULL note that the return value is set up beforehand by
              // moves, handled by the stack frame
} IROperator;
// a three-address code
typedef struct IREntry {
  IROperator op;
  size_t opSize;
  IROperand *dest;  // nullable, if const
  IROperand *arg1;
  IROperand *arg2;  // nullable
} IREntry;
// ctors
IREntry *constantIREntryCreate(size_t size, IROperand *constant);
IREntry *asmIREntryCreate(IROperand *assembly);
IREntry *labelIREntryCreate(IROperand *label);
IREntry *moveIREntryCreate(size_t size, IROperand *dest, IROperand *source);
IREntry *memStoreIREntryCreate(size_t size, IROperand *destAddr,
                               IROperand *source);
IREntry *memLoadIREntryCreate(size_t size, IROperand *dest,
                              IROperand *sourceAddr);
IREntry *stackStoreIREntryCreate(size_t size, IROperand *destOffset,
                                 IROperand *source);
IREntry *stackLoadIREntryCreate(size_t size, IROperand *dest,
                                IROperand *sourceOffset);
IREntry *offsetStoreIREntryCreate(size_t size, IROperand *destMemTemp,
                                  IROperand *source, IROperand *offset);
IREntry *offsetLoadIREntryCreate(size_t size, IROperand *dest,
                                 IROperand *sourceMemTemp, IROperand *offset);
IREntry *binopIREntryCreate(size_t size, IROperator op, IROperand *dest,
                            IROperand *arg1, IROperand *arg2);
IREntry *unopIREntryCreate(size_t size, IROperator op, IROperand *dest,
                           IROperand *arg);
IREntry *jumpIREntryCreate(IROperand *dest);
IREntry *cjumpIREntryCreate(size_t size, IROperator op, IROperand *dest,
                            IROperand *lhs, IROperand *rhs);
IREntry *callIREntryCreate(IROperand *who);
IREntry *returnIREntryCreate(void);
// dtor
void irEntryDestroy(IREntry *);

// vector of IROperands - used for function calling
typedef Vector IROperandVector;
IROperandVector *irOperandVectorCreate(void);
void irOperandVectorInit(IROperandVector *);
void irOperandVectorInsert(IROperandVector *, IROperand *);
void irOperandVectorUninit(IROperandVector *);
void irOperandVectorDestroy(IROperandVector *);

// vector of IREntries - used for data, rodata, and code fragments
typedef Vector IREntryVector;
IREntryVector *irEntryVectorCreate(void);
void irEntryVectorInit(IREntryVector *);
void irEntryVectorInsert(IREntryVector *, IREntry *);
IREntryVector *irEntryVectorMerge(IREntryVector *, IREntryVector *);
void irEntryVectorUninit(IREntryVector *);
void irEntryVectorDestroy(IREntryVector *);

// a linear allocator for temporary numbers
typedef struct TempAllocator {
  size_t next;
} TempAllocator;
// in-place ctor
TempAllocator *tempAllocatorCreate(void);
// produce a number
size_t tempAllocatorAllocate(TempAllocator *);
// in-place dtor
void tempAllocatorDestroy(TempAllocator *);

#endif  // TLC_IR_IR_H_