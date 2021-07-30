// Copyright 2020-2021 Justin Hu
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
 * Assembly level stuff for x86_64 linux
 */

#ifndef TLC_ARCH_X86_64_LINUX_ASM_H_
#define TLC_ARCH_X86_64_LINUX_ASM_H_

#include <stddef.h>

extern char const *const X86_64_LINUX_LOCAL_LABEL_FORMAT;
extern size_t const X86_64_LINUX_REGISTER_WIDTH;
extern size_t const X86_64_LINUX_STACK_ALIGNMENT;

typedef enum {
  X86_64_RAX,
  X86_64_RBX,
  X86_64_RCX,
  X86_64_RDX,
  X86_64_RSI,
  X86_64_RDI,
  X86_64_RSP,
  X86_64_RBP,
  X86_64_R8,
  X86_64_R9,
  X86_64_R10,
  X86_64_R11,
  X86_64_R12,
  X86_64_R13,
  X86_64_R14,
  X86_64_R15,
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
} X86_64_Register;

char const *x86_64LinuxPrettyPrintRegister(size_t reg);

#endif  // TLC_ARCH_X86_64_LINUX_ASM_H_