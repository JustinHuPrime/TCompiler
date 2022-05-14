// Copyright 2019, 2021-2022 Justin Hu
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

#include "ast/symbolTable.h"
#include "ir/ir.h"

/**
 * temporary value
 */
IROperand *TEMP(size_t name, size_t alignment, size_t size, AllocHint kind);
/**
 * temporary value from type
 */
IROperand *TEMPOF(size_t name, Type const *t);
/**
 * temporary value storing a non-global variable
 */
IROperand *TEMPVAR(SymbolTableEntry const *e);
/**
 * temporary pointer
 */
IROperand *TEMPPTR(size_t name);
/**
 * temporary boolean
 */
IROperand *TEMPBOOL(size_t name);
/**
 * register
 */
IROperand *REG(size_t name, size_t size);
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
 * offset constant
 */
IROperand *OFFSET(int64_t offset);

/**
 * local label (for use during trace scheduling)
 */
IRInstruction *LABEL(size_t name);
/**
 * mark a temp in some non-code-generating way
 * @param marker marker to use
 * @param temp temp to use
 */
IRInstruction *MARK_TEMP(IROperator marker, IROperand *temp);
/**
 * simple move
 * @param dest destination temp or reg
 * @param src source temp, reg, constant, or label
 */
IRInstruction *MOVE(IROperand *dest, IROperand *src);
/**
 * move to memory
 * @param addr destination address in temp, reg, or label
 * @param src source temp, reg, constant, or label
 * @param offset offset into destination address
 */
IRInstruction *MEM_STORE(IROperand *addr, IROperand *src, IROperand *offset);
/**
 * move from memory
 * @param dest destination temp or reg
 * @param addr source address in temp, reg, or label
 * @param offset offset into destination address
 */
IRInstruction *MEM_LOAD(IROperand *dest, IROperand *addr, IROperand *offset);
/**
 * store to stack
 * @param offset destination stack offset as offset (0 == top of stack) (must be
 * non-negative)
 * @param src source temp, reg, constant, or label
 */
IRInstruction *STK_STORE(IROperand *offset, IROperand *src);
/**
 * load from stack
 * @param dest destination temp or reg
 * @param offset source stack offset as offset
 */
IRInstruction *STK_LOAD(IROperand *dest, IROperand *offset);
/**
 * store into part of temp
 * @param dest destination temp
 * @param src source temp, reg, constant, or label
 * @param offset offset as an offset or temp
 */
IRInstruction *OFFSET_STORE(IROperand *dest, IROperand *src, IROperand *offset);
/**
 * load from part of temp
 * @param dest destination temp or reg
 * @param src source temp
 * @param offset offset as an offset or temp
 */
IRInstruction *OFFSET_LOAD(IROperand *dest, IROperand *src, IROperand *offset);
/**
 * generic arithmetic/bitwise/comparison binop
 * @param op operator
 * @param dest destination temp or reg
 * @param lhs left-hand temp, reg, constant, or offset
 * @param rhs right-hand temp, reg, constant, or offset
 */
IRInstruction *BINOP(IROperator op, IROperand *dest, IROperand *lhs,
                     IROperand *rhs);
/**
 * generic arithmetic/bitwise/logic/conversion unop
 * @param op operator
 * @param dest destination temp or reg
 * @param src source temp or reg
 */
IRInstruction *UNOP(IROperator op, IROperand *dest, IROperand *src);
/**
 * unconditional jump to local label
 * @param dest destination numeric id
 */
IRInstruction *JUMP(size_t dest);
/**
 * jump table jump
 * @param dest destination temp
 * @param tableFrag table of destination possibilities
 */
IRInstruction *JUMPTABLE(IROperand *dest, size_t tableFrag);
/**
 * comparison conditional jump to local label
 * @param op comparison operation
 * @param trueDest destination numeric id if test is true
 * @param falseDest destination id if test is false
 * @param lhs comparison lhs temp or reg
 * @param rhs comparison rhs temp or reg
 */
IRInstruction *CJUMP(IROperator op, size_t trueDest, size_t falseDest,
                     IROperand *lhs, IROperand *rhs);
/**
 * unary conditional jump to local label
 * @param op IO_J2Z or IO_J2NZ
 * @param trueDest destination numeric id if test is true
 * @param falseDest dstination id if test is false
 * @param condition temp to condition on (jump if zero/not zero)
 */
IRInstruction *BJUMP(IROperator op, size_t trueDest, size_t falseDest,
                     IROperand *condition);
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

/**
 * create a new block and add it to the given vector
 *
 * @param label label of the block
 * @param blocks list of IRBlock to insert into
 */
IRBlock *BLOCK(size_t label, LinkedList *blocks);

#endif  // TLC_IR_SHORTHAND_H_