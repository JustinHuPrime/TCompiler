// Copyright 2019, 2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * shorthand for easier writing of IR
 */

#ifndef TLC_IR_SHORTHAND_H_
#define TLC_IR_SHORTHAND_H_

#include "ir/ir.h"

/**
 * temporary value
 */
IROperand *TEMP(size_t name, size_t alignment, size_t size, AllocHint kind);
/**
 * register
 */
IROperand *REG(size_t name);
/**
 * constant - single datum
 */
IROperand *CONSTANT(size_t alignment, IRDatum *datum);
/**
 * local label
 */
IROperand *LOCAL(size_t name);
/**
 * global label
 */
IROperand *GLOBAL(char *name);
/**
 * constant offset
 */
IROperand *OFFSET(int64_t offset);

/**
 * inline asm
 */
IRInstruction *ASM(char const *assembly);
/**
 * label
 * @param name unique label number
 */
IRInstruction *LABEL(size_t name);
/**
 * simple move
 * @param size sizeof op
 * @param dest destination temp or reg
 * @param src source temp, reg, constant, or label
 */
IRInstruction *MOVE(size_t size, IROperand *dest, IROperand *src);
/**
 * move to memory
 * @param size sizeof op
 * @param addr destination address in temp, reg, or label
 * @param src source temp, reg, constant, or label
 */
IRInstruction *MEM_STORE(size_t size, IROperand *addr, IROperand *src);
/**
 * move from memory
 * @param size sizeof op
 * @param dest destination temp or reg
 * @param addr source address in temp, reg, or label
 */
IRInstruction *MEM_LOAD(size_t size, IROperand *dest, IROperand *addr);
/**
 * store to stack
 * @param size sizeof op
 * @param offset destination stack offset as offset (0 == return address; stack
 * pointer changes due to temps will tweak the offset anyways) (interpretation
 * of signedness is architecture-specific)
 * @param src source temp, reg, constant, or label
 */
IRInstruction *STK_STORE(size_t size, IROperand *offset, IROperand *src);
/**
 * load from stack
 * @param size sizeof op
 * @param dest destination temp or reg
 * @param offset source stack offset as offset
 */
IRInstruction *STK_LOAD(size_t size, IROperand *dest, IROperand *offset);
/**
 * store into part of temp
 * @param size sizeof op
 * @param dest destination temp
 * @param src source temp, reg, constant, or label
 * @param offset offset as an offset or temp
 */
IRInstruction *OFFSET_STORE(size_t size, IROperand *dest, IROperand *src,
                            IROperand *offset);
/**
 * load from part of temp
 * @param size sizeof op
 * @param dest destination temp or reg
 * @param src source temp
 * @param offset offset as an offset or temp
 */
IRInstruction *OFFSET_LOAD(size_t size, IROperand *dest, IROperand *src,
                           IROperand *offset);
/**
 * generic arithmetic/bitwise/comparison binop
 * @param size sizeof op
 * @param op operator
 * @param dest destination temp or reg
 * @param lhs left-hand temp or reg
 * @param rhs right-hand temp or reg
 */
IRInstruction *BINOP(size_t size, IROperator op, IROperand *dest,
                     IROperand *lhs, IROperand *rhs);
/**
 * generic arithmetic/bitwise/logic/conversion unop
 * @param size sizeof op
 * @param op operator
 * @param dest destination temp or reg
 * @param src source temp or reg
 */
IRInstruction *UNOP(size_t size, IROperator op, IROperand *dest,
                    IROperand *src);
/**
 * unconditional jump to local label
 * @param dest destination numeric id
 */
IRInstruction *JUMP(size_t dest);
/**
 * conditional jump to local label
 * @param size sizeof comparison
 * @param op comparison operation
 * @param dest destination numeric id
 * @param lhs comparison lhs temp or reg
 * @param rhs comparison rhs temp or reg
 */
IRInstruction *CJUMP(size_t size, IROperator op, size_t dest, IROperand *lhs,
                     IROperand *rhs);
/**
 * call a function with the given (mangled) name
 * @param who label or temp or reg to call
 */
IRInstruction *CALL(IROperand *who);
/**
 * return from a function
 */
IRInstruction *RETURN(void);

/**
 * insert the instruction at the end of the block
 */
void IR(IRBlock *b, IRInstruction *i);

#endif  // TLC_IR_SHORTHAND_H_