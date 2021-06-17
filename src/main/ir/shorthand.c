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

IROperand *TEMP(size_t name, size_t alignment, size_t size, AllocHint kind) {
  return tempOperandCreate(name, alignment, size, kind);
}
IROperand *REG(size_t name) { return regOperandCreate(name); }
IROperand *CONSTANT(size_t alignment, IRDatum *datum) {
  IROperand *o = constantOperandCreate(alignment);
  vectorInsert(&o->data.constant.data, datum);
  return o;
}
IROperand *LOCAL(size_t name) {
  return labelOperandCreate(format(localLabelFormat(), name));
}
IROperand *GLOBAL(char *name) { return labelOperandCreate(strdup(name)); }
IROperand *OFFSET(int64_t offset) { return offsetOperandCreate(offset); }

IRInstruction *ASM(char const *assembly) {
  return irInstructionCreate(IO_ASM, 0, NULL,
                             assemblyOperandCreate(strdup(assembly)), NULL);
}
IRInstruction *MOVE(size_t size, IROperand *dest, IROperand *src) {
  return irInstructionCreate(IO_MOVE, size, dest, src, NULL);
}
IRInstruction *MEM_STORE(size_t size, IROperand *addr, IROperand *src,
                         IROperand *offset) {
  return irInstructionCreate(IO_MEM_STORE, size, addr, src, offset);
}
IRInstruction *MEM_LOAD(size_t size, IROperand *dest, IROperand *addr,
                        IROperand *offset) {
  return irInstructionCreate(IO_MEM_LOAD, size, dest, addr, offset);
}
IRInstruction *STK_STORE(size_t size, IROperand *offset, IROperand *src) {
  return irInstructionCreate(IO_STK_STORE, size, offset, src, NULL);
}
IRInstruction *STK_LOAD(size_t size, IROperand *dest, IROperand *offset) {
  return irInstructionCreate(IO_STK_LOAD, size, dest, offset, NULL);
}
IRInstruction *OFFSET_STORE(size_t size, IROperand *dest, IROperand *src,
                            IROperand *offset) {
  return irInstructionCreate(IO_OFFSET_STORE, size, dest, src, offset);
}
IRInstruction *OFFSET_LOAD(size_t size, IROperand *dest, IROperand *src,
                           IROperand *offset) {
  return irInstructionCreate(IO_OFFSET_LOAD, size, dest, src, offset);
}
IRInstruction *BINOP(size_t size, IROperator op, IROperand *dest,
                     IROperand *lhs, IROperand *rhs) {
  return irInstructionCreate(op, size, dest, lhs, rhs);
}
IRInstruction *UNOP(size_t size, IROperator op, IROperand *dest,
                    IROperand *src) {
  return irInstructionCreate(op, size, dest, src, NULL);
}
IRInstruction *JUMP(size_t dest) {
  return irInstructionCreate(IO_JUMP, 0, LOCAL(dest), NULL, NULL);
}
IRInstruction *CJUMP(size_t size, IROperator op, size_t dest, IROperand *lhs,
                     IROperand *rhs) {
  return irInstructionCreate(op, size, LOCAL(dest), lhs, rhs);
}
IRInstruction *CALL(IROperand *who) {
  return irInstructionCreate(IO_CALL, 0, NULL, who, NULL);
}
IRInstruction *RETURN(void) {
  return irInstructionCreate(IO_RETURN, 0, NULL, NULL, NULL);
}

void IR(IRBlock *b, IRInstruction *i) { insertNodeEnd(&b->instructions, i); }

IRBlock *BLOCK(size_t label, Vector *v) {
  IRBlock *b = irBlockCreate(label);
  vectorInsert(v, b);
  return b;
}