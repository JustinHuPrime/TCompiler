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

// x86_64 assembly shorthand implementation

#include "architecture/x86_64/shorthand.h"

#include "ir/ir.h"

void X86_64INSERT(X86_64InstructionVector *v, X86_64Instruction *i) {
  x86_64InstructionVectorInsert(v, i);
}
X86_64Instruction *X86_64INSTR(char *skeleton) {
  return x86_64InstructionCreate(skeleton);
}
X86_64Instruction *X86_64MOVE(char *skeleton) {
  return x86_64MoveInstructionCreate(skeleton);
}
void X86_64USE(X86_64Instruction *i, IROperand const *u, size_t size) {
  x86_64OperandVectorInsert(&i->uses, x86_64OperandCreate(u, size));
}
void X86_64DEF(X86_64Instruction *i, IROperand const *d, size_t size) {
  x86_64OperandVectorInsert(&i->defines, x86_64OperandCreate(d, size));
}
void X86_64OTHER(X86_64Instruction *i, IROperand const *o, size_t size) {
  x86_64OperandVectorInsert(&i->other, x86_64OperandCreate(o, size));
}