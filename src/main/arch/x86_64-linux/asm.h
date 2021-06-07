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

extern char const *const X86_64_LINUX_LOCAL_LABEL_FORMAT;

typedef enum {
  RAX,
  RBX,
  RCX,
  RDX,
  RSI,
  RDI,
  RSP,
  RBP,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  XMM0,
  XMM1,
  XMM2,
  XMM3,
  XMM4,
  XMM5,
  XMM6,
  XMM7,
  XMM8,
  XMM9,
  XMM10,
  XMM11,
  XMM12,
  XMM13,
  XMM14,
  XMM15,
} X86_64_Register;

#endif  // TLC_ARCH_X86_64_LINUX_ASM_H_