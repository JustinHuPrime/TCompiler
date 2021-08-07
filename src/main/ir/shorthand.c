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

#include "ir/shorthand.h"

#include "arch/interface.h"
#include "util/conversions.h"
#include "util/numericSizing.h"

IROperand *TEMP(size_t name, size_t alignment, size_t size, AllocHint kind) {
  return tempOperandCreate(name, alignment, size, kind);
}
IROperand *TEMPOF(size_t name, Type const *t) {
  return tempOperandCreate(name, typeAlignof(t), typeSizeof(t),
                           typeAllocation(t));
}
IROperand *TEMPVAR(SymbolTableEntry const *e) {
  return tempOperandCreate(
      e->data.variable.temp, typeAlignof(e->data.variable.type),
      typeSizeof(e->data.variable.type),
      e->data.variable.escapes ? AH_MEM
                               : typeAllocation(e->data.variable.type));
}
IROperand *TEMPPTR(size_t name) {
  return tempOperandCreate(name, POINTER_WIDTH, POINTER_WIDTH, AH_GP);
}
IROperand *TEMPBOOL(size_t name) {
  return tempOperandCreate(name, BOOL_WIDTH, BOOL_WIDTH, AH_GP);
}
IROperand *REG(size_t name, size_t size) {
  return regOperandCreate(name, size);
}
IROperand *CONSTANT(size_t alignment, IRDatum *datum) {
  IROperand *o = constantOperandCreate(alignment);
  vectorInsert(&o->data.constant.data, datum);
  return o;
}
IROperand *LOCAL(size_t name) { return localOperandCreate(name); }
IROperand *GLOBAL(char const *name) {
  return globalOperandCreate(strdup(name));
}
IROperand *OFFSET(int64_t offset) {
  return CONSTANT(POINTER_WIDTH, longDatumCreate(s64ToU64(offset)));
}

static IRInstruction *oneArgInstructionCreate(IROperator op, IROperand *arg1) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = arg1;
  return retval;
}
static IRInstruction *twoArgInstructionCreate(IROperator op, IROperand *arg1,
                                              IROperand *arg2) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = arg1;
  retval->args[1] = arg2;
  return retval;
}
static IRInstruction *threeArgInstructionCreate(IROperator op, IROperand *arg1,
                                                IROperand *arg2,
                                                IROperand *arg3) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = arg1;
  retval->args[1] = arg2;
  retval->args[2] = arg3;
  return retval;
}
static IRInstruction *fourArgInstructionCreate(IROperator op, IROperand *arg1,
                                               IROperand *arg2, IROperand *arg3,
                                               IROperand *arg4) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = arg1;
  retval->args[1] = arg2;
  retval->args[2] = arg3;
  retval->args[3] = arg4;
  return retval;
}
IRInstruction *MARK_TEMP(IROperator marker, IROperand *temp) {
  return oneArgInstructionCreate(marker, temp);
}
IRInstruction *MOVE(IROperand *dest, IROperand *src) {
  return twoArgInstructionCreate(IO_MOVE, dest, src);
}
IRInstruction *MEM_STORE(IROperand *addr, IROperand *src, IROperand *offset) {
  return threeArgInstructionCreate(IO_MEM_STORE, addr, src, offset);
}
IRInstruction *MEM_LOAD(IROperand *dest, IROperand *addr, IROperand *offset) {
  return threeArgInstructionCreate(IO_MEM_LOAD, dest, addr, offset);
}
IRInstruction *STK_STORE(IROperand *offset, IROperand *src) {
  return twoArgInstructionCreate(IO_STK_STORE, offset, src);
}
IRInstruction *STK_LOAD(IROperand *dest, IROperand *offset) {
  return twoArgInstructionCreate(IO_STK_LOAD, dest, offset);
}
IRInstruction *OFFSET_STORE(IROperand *dest, IROperand *src,
                            IROperand *offset) {
  return threeArgInstructionCreate(IO_OFFSET_STORE, dest, src, offset);
}
IRInstruction *OFFSET_LOAD(IROperand *dest, IROperand *src, IROperand *offset) {
  return threeArgInstructionCreate(IO_OFFSET_LOAD, dest, src, offset);
}
IRInstruction *BINOP(IROperator op, IROperand *dest, IROperand *lhs,
                     IROperand *rhs) {
  return threeArgInstructionCreate(op, dest, lhs, rhs);
}
IRInstruction *UNOP(IROperator op, IROperand *dest, IROperand *src) {
  return twoArgInstructionCreate(op, dest, src);
}
IRInstruction *JUMP(IROperand *dest) {
  return oneArgInstructionCreate(IO_JUMP, dest);
}
IRInstruction *CJUMP(IROperator op, size_t trueDest, size_t falseDest,
                     IROperand *lhs, IROperand *rhs) {
  return fourArgInstructionCreate(op, LOCAL(trueDest), LOCAL(falseDest), lhs,
                                  rhs);
}
IRInstruction *BJUMP(IROperator op, size_t trueDest, size_t falseDest,
                     IROperand *condition) {
  return threeArgInstructionCreate(op, LOCAL(trueDest), LOCAL(falseDest),
                                   condition);
}
IRInstruction *CALL(IROperand *who) {
  return oneArgInstructionCreate(IO_CALL, who);
}
IRInstruction *RETURN(void) { return irInstructionCreate(IO_RETURN); }

void IR(IRBlock *b, IRInstruction *i) { insertNodeEnd(&b->instructions, i); }

IRBlock *BLOCK(size_t label, Vector *v) {
  IRBlock *b = irBlockCreate(label);
  vectorInsert(v, b);
  return b;
}