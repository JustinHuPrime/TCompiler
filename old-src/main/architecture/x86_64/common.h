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

// common x86_64 data and functions

#ifndef TLC_ARCHITECTURE_X86_64_COMMON_H_
#define TLC_ARCHITECTURE_X86_64_COMMON_H_

#include <stdbool.h>
#include <stddef.h>

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

extern X86_64Register const X86_64_CALLEE_SAVE[];
extern size_t const X86_64_NUM_CALLEE_SAVE;
extern X86_64Register const X86_64_CALLER_SAVE[];
extern size_t const X86_64_NUM_CALLER_SAVE;

X86_64Register x86_64RegNumToRegister(size_t reg);
bool x86_64RegIsSSE(X86_64Register);
bool x86_64RegIsCalleeSave(X86_64Register);

#endif  // TLC_ARCHITECTURE_X86_64_COMMON_H_