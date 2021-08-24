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
#include <stdint.h>

#include "ast/ast.h"
#include "ir/ir.h"
#include "util/container/linkedList.h"

extern size_t const X86_64_LINUX_REGISTER_WIDTH;
extern size_t const X86_64_LINUX_STACK_ALIGNMENT;

typedef enum {
  X86_64_LINUX_RAX,
  X86_64_LINUX_RBX,
  X86_64_LINUX_RCX,
  X86_64_LINUX_RDX,
  X86_64_LINUX_RSI,
  X86_64_LINUX_RDI,
  X86_64_LINUX_RSP,
  X86_64_LINUX_RBP,
  X86_64_LINUX_R8,
  X86_64_LINUX_R9,
  X86_64_LINUX_R10,
  X86_64_LINUX_R11,
  X86_64_LINUX_R12,
  X86_64_LINUX_R13,
  X86_64_LINUX_R14,
  X86_64_LINUX_R15,
  X86_64_LINUX_XMM0,
  X86_64_LINUX_XMM1,
  X86_64_LINUX_XMM2,
  X86_64_LINUX_XMM3,
  X86_64_LINUX_XMM4,
  X86_64_LINUX_XMM5,
  X86_64_LINUX_XMM6,
  X86_64_LINUX_XMM7,
  X86_64_LINUX_XMM8,
  X86_64_LINUX_XMM9,
  X86_64_LINUX_XMM10,
  X86_64_LINUX_XMM11,
  X86_64_LINUX_XMM12,
  X86_64_LINUX_XMM13,
  X86_64_LINUX_XMM14,
  X86_64_LINUX_XMM15,
} X86_64LinuxRegister;

char const *x86_64LinuxPrettyPrintRegister(size_t reg);

typedef enum {
  X86_64_LINUX_OK_REG,
  X86_64_LINUX_OK_TEMP,
  X86_64_LINUX_OK_OFFSET,
} X86_64LinuxOperandKind;
typedef struct {
  X86_64LinuxOperandKind kind;
  union {
    struct {
      X86_64LinuxRegister reg;
    } reg;
    struct {
      size_t name;
      size_t alignment;
      size_t size;
      AllocHint kind;
    } temp;
    struct {
      int64_t offset;
    } offset;
  } data;
} X86_64LinuxOperand;
X86_64LinuxOperand *x86_64LinuxRegOperandCreate(X86_64LinuxRegister reg);
X86_64LinuxOperand *x86_64LinuxTempOperandCreate(IROperand *temp);
X86_64LinuxOperand *x86_64LinuxOffsetOperandCreate(int64_t offset);
void x86_64LinuxOperandFree(X86_64LinuxOperand *o);

typedef enum {
  X86_64_LINUX_IK_REGULAR,
  X86_64_LINUX_IK_MOVE,
  X86_64_LINUX_IK_JUMP,
  X86_64_LINUX_IK_JUMPTABLE,
  X86_64_LINUX_IK_CJUMP,
  X86_64_LINUX_IK_LEAVE,
  X86_64_LINUX_IK_LABEL,
} X86_64LinuxInstructionKind;
typedef struct {
  X86_64LinuxInstructionKind kind;
  /**
   * format string
   */
  char *skeleton;
  Vector defines; /**< vector of X86_64LinuxOperand */
  Vector uses;    /**< vector of X86_64LinuxOperand */
  union {
    SizeVector jumpTargets; /**< vector of uintptr_t */
    size_t labelName;
  } data;
} X86_64LinuxInstruction;
X86_64LinuxInstruction *x86_64LinuxInstructionCreate(
    X86_64LinuxInstructionKind kind, char *skeleton);
void x86_64LinuxInstructionFree(X86_64LinuxInstruction *i);

typedef enum {
  X86_64_LINUX_FK_TEXT,
  X86_64_LINUX_FK_DATA,
} X86_64LinuxFragKind;
typedef struct {
  X86_64LinuxFragKind kind;
  union {
    struct {
      char *data;
    } data;
    struct {
      char *header;
      char *footer;
      LinkedList instructions;
    } text;
  } data;
} X86_64LinuxFrag;
X86_64LinuxFrag *x86_64LinuxDataFragCreate(char *data);
X86_64LinuxFrag *x86_64LinuxTextFragCreate(char *header, char *footer);
void x86_64LinuxFragFree(X86_64LinuxFrag *frag);

typedef struct {
  char *header;
  char *footer;
  Vector frags;
} X86_64LinuxFile;
X86_64LinuxFile *x86_64LinuxFileCreate(char *header, char *footer);
void x86_64LinuxFileFree(X86_64LinuxFile *file);

#endif  // TLC_ARCH_X86_64_LINUX_ASM_H_