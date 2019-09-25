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

// Shorthand for writing the IR

#ifndef TLC_IR_SHORTHAND_H_
#define TLC_IR_SHORTHAND_H_

#include "ir/ir.h"

#include <stddef.h>
#include <stdint.h>

// equivalent to tempAllocatorAllocate
size_t NEW(TempAllocator *);

// shorthands for IROperand construction
IROperand *TEMP(size_t n, size_t size, size_t alignment, AllocHint kind);
IROperand *REG(size_t n);
IROperand *UBYTE(uint8_t value);
IROperand *BYTE(int8_t value);
IROperand *USHORT(uint16_t value);
IROperand *SHORT(int16_t value);
IROperand *UINT(uint32_t value);
IROperand *INT(int32_t value);
IROperand *ULONG(uint64_t value);
IROperand *LONG(int64_t value);
IROperand *FLOAT(uint32_t bits);
IROperand *DOUBLE(uint64_t bits);
IROperand *LABEL(char *name);

// shorthands for IREntry construction
IREntry *CONST(size_t size, IROperand *);
IREntry *ASM(char *assembly);  // constructs the IROperand as well
IREntry *LABEL_DEF(IROperand *label);
IREntry *MOVE(size_t size, IROperand *dest, IROperand *source);
IREntry *MEM_STORE(size_t size, IROperand *destAddr, IROperand *source);
IREntry *MEM_LOAD(size_t size, IROperand *dest, IROperand *sourceAddr);
IREntry *STACK_STORE(size_t size, int64_t destOffset,
                     IROperand *source);  // wraps the offset
IREntry *STACK_LOAD(size_t size, IROperand *dest,
                    int64_t sourceOffset);  // wraps the offset
IREntry *OFFSET_STORE(size_t size, IROperand *destMemTemp, IROperand *source,
                      IROperand *offset);
IREntry *OFFSET_LOAD(size_t size, IROperand *dest, IROperand *sourceMemTemp,
                     IROperand *offset);
IREntry *BINOP(size_t size, IROperator op, IROperand *dest, IROperand *arg1,
               IROperand *arg2);
IREntry *UNOP(size_t size, IROperator op, IROperand *dest, IROperand *arg);
IREntry *JUMP(IROperand *dest);
IREntry *CJUMP(size_t size, IROperator op, IROperand *dest, IROperand *lhs,
               IROperand *rhs);
IREntry *CALL(IROperand *who);
IREntry *RETURN(void);

// equivalent to irVectorInsert
void IR(IRVector *, IREntry *);

#endif  // TLC_IR_SHORTHAND_H_