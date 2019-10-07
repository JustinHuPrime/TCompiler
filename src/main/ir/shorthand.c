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

#include "ir/frame.h"
#include "ir/ir.h"

size_t NEW(TempAllocator *t) { return tempAllocatorAllocate(t); }

char *NEW_LABEL(LabelGenerator *l) {
  return labelGeneratorGenerateCodeLabel(l);
}
char *NEW_DATA_LABEL(LabelGenerator *l) {
  return labelGeneratorGenerateDataLabel(l);
}

IROperand *TEMP(size_t n, size_t size, size_t alignment, AllocHint kind) {
  return tempIROperandCreate(n, size, alignment, kind);
}
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
IROperand *NAME(char *name) { return nameIROperandCreate(name); }
IROperand *STRING(uint8_t *string) { return stringIROperandCreate(string); }
IROperand *WSTRING(uint32_t *wstring) {
  return wstringIROperandCreate(wstring);
}
IROperand *STACKOFFSET(int64_t baseOffset) {
  return stackOffsetIROperandCreate(baseOffset);
}

IREntry *CONST(size_t size, IROperand *constant) {
  return constantIREntryCreate(size, constant);
}
IREntry *ASM(char *assembly) {
  return asmIREntryCreate(asmIROperandCreate(assembly));
}
IREntry *LABEL(char *label) { return labelIREntryCreate(NAME(label)); }
IREntry *MOVE(size_t size, IROperand *dest, IROperand *source) {
  return moveIREntryCreate(size, dest, source);
}
IREntry *MEM_STORE(size_t size, IROperand *destAddr, IROperand *source) {
  return memStoreIREntryCreate(size, destAddr, source);
}
IREntry *MEM_LOAD(size_t size, IROperand *dest, IROperand *sourceAddr) {
  return memLoadIREntryCreate(size, dest, sourceAddr);
}
IREntry *STACK_STORE(size_t size, int64_t destOffset, IROperand *source) {
  return stackStoreIREntryCreate(size, LONG(destOffset), source);
}
IREntry *STACK_LOAD(size_t size, IROperand *dest, int64_t sourceOffset) {
  return stackLoadIREntryCreate(size, dest, LONG(sourceOffset));
}
IREntry *OFFSET_STORE(size_t size, IROperand *destMemTemp, IROperand *source,
                      IROperand *offset) {
  return offsetStoreIREntryCreate(size, destMemTemp, source, offset);
}
IREntry *OFFSET_LOAD(size_t size, IROperand *dest, IROperand *sourceMemTemp,
                     IROperand *offset) {
  return offsetLoadIREntryCreate(size, dest, sourceMemTemp, offset);
}
IREntry *BINOP(size_t size, IROperator op, IROperand *dest, IROperand *arg1,
               IROperand *arg2) {
  return binopIREntryCreate(size, op, dest, arg1, arg2);
}
IREntry *UNOP(size_t size, IROperator op, IROperand *dest, IROperand *arg) {
  return unopIREntryCreate(size, op, dest, arg);
}
IREntry *JUMP(char *dest) { return jumpIREntryCreate(NAME(dest)); }
IREntry *CJUMP(size_t size, IROperator op, char *dest, IROperand *lhs,
               IROperand *rhs) {
  return cjumpIREntryCreate(size, op, NAME(dest), lhs, rhs);
}
IREntry *CALL(IROperand *who) { return callIREntryCreate(who); }
IREntry *RETURN(void) { return returnIREntryCreate(); }

void IR(IREntryVector *v, IREntry *e) { irEntryVectorInsert(v, e); }