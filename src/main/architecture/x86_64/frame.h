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

// x86_64 specific function frame

#ifndef TLC_ARCHITECTURE_X86_64_64_FRAME_H_
#define TLC_ARCHITECTURE_X86_64_64_FRAME_H_

#include "ir/ir.h"

// Frame layout (using the System V ABI):
// Note that everything below 8(%rbp) is allocated and deallocated by the caller
//
//        8n + 16(%rbp) | memory argument eightbyte n
//             16(%rbp) | memory argument eightbyte 0
//              8(%rbp) | return address - current frame starts here
//              0(%rbp) | spilled registers, saved registers, and mem temps,
//                        aligned to 8 bytes each, totaling m eightbytes
//            -8m(%rbp) | escaping local variables, aligned to data type,
//                        totaling p eightbytes
// 0(%rsp) OR -8p(%rbp) | top of stack frame

// when a function call is made, the stack frame is extended thus:
//                  -8p(%rbp) | in-memory actual argument eightbyte q
// o(%rsp) OR -8(p + q)(%rbp) | in-memory actual argument eightbyte 0

struct Frame;
struct Access;
struct LabelGenerator;
typedef Vector TypeVector;
struct Type;

// symbolic constants for x86_64 register numbers
typedef enum {
  // GP registers
  X86_64_RAX,
  X86_64_RBX,
  X86_64_RCX,
  X86_64_RDX,

  // GP index registers
  X86_64_RSI,
  X86_64_RDI,

  // GP stack pointer registers
  X86_64_RSP,
  X86_64_RBP,

  // GP registers
  X86_64_R8,
  X86_64_R9,
  X86_64_R10,
  X86_64_R11,
  X86_64_R12,
  X86_64_R13,
  X86_64_R14,
  X86_64_R15,

  // SSE registers
  X86_64_XMM0,
  X86_64_XMM1,
  X86_64_XMM2,
  X86_64_XMM3,
  X86_64_XMM4,
  X86_64_XMM5,
  X86_64_XMM6,
  X86_64_XMM7,
  X86_64_XMM8,
  X86_64_XMM9,
  X86_64_XMM10,
  X86_64_XMM11,
  X86_64_XMM12,
  X86_64_XMM13,
  X86_64_XMM14,
  X86_64_XMM15,
} X86_64Register;

// constructors for x86_64
struct Frame *x86_64FrameCtor(char *name);
struct Access *x86_64GlobalAccessCtor(size_t size, size_t alignment,
                                      AllocHint kind, char *label);
struct Access *x86_64FunctionAccessCtor(char *name);
struct LabelGenerator *x86_64LabelGeneratorCtor(void);

#endif  // TLC_ARCHITECTURE_X86_64_64_FRAME_H_