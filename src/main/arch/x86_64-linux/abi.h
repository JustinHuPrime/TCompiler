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
 * System V ABI for x86_64 linux
 */

#ifndef TLC_ARCH_X86_64_LINUX_ABI_H_
#define TLC_ARCH_X86_64_LINUX_ABI_H_

#include "ast/symbolTable.h"
#include "ir/ir.h"

void x86_64LinuxGenerateFunctionEntry(Vector *blocks, SymbolTableEntry *entry,
                                      size_t returnValueAddressTemp,
                                      size_t nextLabel, FileListEntry *file);
void x86_64LinuxGenerateFunctionExit(Vector *blocks,
                                     SymbolTableEntry const *entry,
                                     size_t returnValueAddressTemp,
                                     size_t returnValueTemp, size_t prevLink,
                                     FileListEntry *file);
IROperand *x86_64LinuxGenerateFunctionCall(IRBlock *b, IROperand *fun, Type const *funType,
                                FileListEntry *file);

#endif  // TLC_ARCH_X86_64_LINUX_ABI_H_