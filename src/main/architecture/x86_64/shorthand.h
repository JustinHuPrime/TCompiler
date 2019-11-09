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

// x86_64 assembly shorthand

#include "architecture/x86_64/assembly.h"

struct IROperand;

// x86_64InstructionVectorInsert
void X86_64INSERT(X86_64InstructionVector *, X86_64Instruction *);
// x86_64InstructionCreate
X86_64Instruction *X86_64INSTR(char *);
// x86_64MoveInstructionCreate
X86_64Instruction *X86_64MOVE(char *);
// x86_64JumpInstructionCreate
X86_64Instruction *X86_64JUMP(char *, char *);
// x86_64CJumpInstructionCreate
X86_64Instruction *X86_64CJUMP(char *, char *);
// x86_64OperandVectorInsert(&i->uses, x86_64OperandCreate(use, size))
void X86_64USE(X86_64Instruction *, struct IROperand const *, size_t);
// x86_64OperandVectorInsert(&i->defines, x86_64OperandCreate(def, size))
void X86_64DEF(X86_64Instruction *, struct IROperand const *, size_t);
// x86_64OperandVectorInsert(&i->other, x86_64OperandCreate(other, size))
void X86_64OTHER(X86_64Instruction *, struct IROperand const *, size_t);