// Copyright 2020-2022 Justin Hu
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
  X86_64_LINUX_RSP,  // special register, not allocated
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
  X86_64_LINUX_RFLAGS,  // special register, not allocated
} X86_64LinuxRegister;

char const *x86_64LinuxPrettyPrintRegister(size_t reg);

typedef enum {
  X86_64_LINUX_OM_READ = 1,  /**< operation depends on this operand's value */
  x86_64_LINUX_OM_WRITE = 2, /**< operation writes to this operand */
  X86_64_LINUX_OM_IMPLICIT =
      4, /**< this operand isn't directly used in the operation */
} X86_64LinuxOperandMode;
typedef enum {
  X86_64_LINUX_OK_REG,
  X86_64_LINUX_OK_TEMP,
  X86_64_LINUX_OK_OFFSET_TEMP,
  X86_64_LINUX_OK_LOCAL_LABEL,
  X86_64_LINUX_OK_GLOBAL_LABEL,
  X86_64_LINUX_OK_NUMBER,
} X86_64LinuxOperandKind;
typedef struct X86_64LinuxOperand {
  X86_64LinuxOperandMode mode;
  X86_64LinuxOperandKind kind;
  union {
    /**
     * Literally a register, for example:
     * rax
     */
    struct {
      X86_64LinuxRegister reg;
      size_t size;
    } reg;
    /**
     * A temporary variable
     * If kind = AH_GP, then it's a GP register, for example:
     * rax
     * If kind = AH_FP, then it's an XMM register, for example:
     * xmm0
     * If kind = AH_MEM, then it's a stack location, for example:
     * [rsp + 8]
     * If escapes is true (it has its address taken), then it's always allocated
     * its own memory location
     */
    struct {
      size_t name;
      size_t alignment;
      size_t size;
      AllocHint kind;
      bool escapes;
    } temp;
    /**
     * An offset temporary variable
     * base as kind OK_TEMP and it's a MEM temp
     * offset is something equivalent to a GP register or a constant
     * For example (for a GP-register-equivalent offset):
     * [rsp + rax + 8]
     * For example (for a constant offset):
     * [rsp + 16]
     */
    struct {
      struct X86_64LinuxOperand *base;
      struct X86_64LinuxOperand *offset;
    } offsetTemp;
    /**
     * a global label used as a literal; considered a 32 bit constant
     */
    struct {
      char *label;
    } globalLabel;
    /**
     * a local label used as a literal; considered a 32 bit constant
     */
    struct {
      size_t label;
    } localLabel;
    /**
     * a numeric constant, up to 64 bits
     */
    struct {
      uint64_t value;
      size_t size;
    } number;
  } data;
} X86_64LinuxOperand;

typedef enum {
  X86_64_LINUX_IK_REGULAR, /**< regular instruction, goes to next instruction */
  X86_64_LINUX_IK_MOVE,  /**< move instruction, can elide instruction if the two
                            operands are in the same register */
  X86_64_LINUX_IK_JUMP,  /**< jump instruction, goes to jump target */
  X86_64_LINUX_IK_CJUMP, /**< conditional jump instruction, goes to jump target
                            or next instruction */
  X86_64_LINUX_IK_JUMPTABLE, /**< jump table, goes to any listed instruction in
                                jump table */
  X86_64_LINUX_IK_LEAVE,     /**< instruction leaves the function entirely */
  X86_64_LINUX_IK_LABEL,     /**< label */
} X86_64LinuxInstructionKind;
typedef struct {
  X86_64LinuxInstructionKind kind;
  char *skeleton;
  Vector operands;
  union {
    SizeVector jumpTargets;
    size_t label;
  } data;
} X86_64LinuxInstruction;

typedef enum {
  X86_64_LINUX_FK_TEXT, /**< code for a function body */
  X86_64_LINUX_FK_DATA, /**< either .data, .rodata, or .bss */
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

typedef struct {
  char *header;
  char *footer;
  Vector frags;
} X86_64LinuxFile;

void x86_64LinuxFileFree(X86_64LinuxFile *file);

/**
 * generate assembly from IR
 */
void x86_64LinuxGenerateAsm(void);

#endif  // TLC_ARCH_X86_64_LINUX_ASM_H_