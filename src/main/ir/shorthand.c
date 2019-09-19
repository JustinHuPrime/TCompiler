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

// Implementation of shorthand for the IR

#include "ir/shorthand.h"

#include "ir/ir.h"

IROperand *TEMP(size_t n) { return tempIROperandCreate(n); }
IROperand *REG(size_t n) { return regIROperandCreate(n); }
IROperand *UBYTE(uint8_t value) { return ubyteIROperandCreate(value); }
IROperand *BYTE(int8_t value) { return byteIROperandCreate(value); }
IROperand *USHORT(uint16_t value) { return ushortIROperandCreate(value); }
IROperand *SHORT(int16_t value) { return shortIROperandCreate(value); }
IROperand *UINT(uint32_t value) { return uintIROperandCreate(value); }
IROperand *INT(int32_t value) { return intIROperandCreate(value); }
IROperand *ULONG(uint64_t value) { return ulongIROperandCreate(value); }
IROperand *LONG(int64_t value) { return longIROperandCreate(value); }
IROperand *FLOAT(uint32_t bits) { return floatIROperandCreate(bits); }
IROperand *DOUBLE(uint64_t bits) { return doubleIROperandCreate(bits); }
IROperand *LABEL(char *name) { return labelIROperandCreate(name); }

IREntry *CONST(size_t size, IROperand *constant) {
  return constantIREntryCreate(size, constant);
}
IREntry *ASM(char *assembly) {
  return asmIREntryCreate(asmIROperandCreate(assembly));
}
IREntry *LABEL_DEF(IROperand *label) { return labelIREntryCreate(label); }
IREntry *MOVE(size_t size, IROperand *dest, IROperand *source) {
  return moveIREntryCreate(size, dest, source);
}
IREntry *STORE(size_t size, IROperand *destAddr, IROperand *source) {
  return storeIREntryCreate(size, destAddr, source);
}
IREntry *LOAD(size_t size, IROperand *dest, IROperand *sourceAddr) {
  return loadIREntryCreate(size, dest, sourceAddr);
}
IREntry *BINOP(size_t size, IROperator op, IROperand *dest, IROperand *arg1,
               IROperand *arg2) {
  return binopIREntryCreate(size, op, dest, arg1, arg2);
}
IREntry *UNOP(size_t size, IROperator op, IROperand *dest, IROperand *arg) {
  return unopIREntryCreate(size, op, dest, arg);
}
IREntry *JUMP(IROperand *dest) { return jumpIREntryCreate(dest); }
IREntry *CJUMP(size_t size, IROperator op, IROperand *dest, IROperand *lhs,
               IROperand *rhs) {
  return cjumpIREntryCreate(size, op, dest, lhs, rhs);
}
IREntry *CALL(IROperand *who) { return callIREntryCreate(who); }
IREntry *RETURN(void) { return returnIREntryCreate(); }