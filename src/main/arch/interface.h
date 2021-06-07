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
 * common arch-neutral interface
 */

#ifndef TLC_ARCH_INTERFACE_H_
#define TLC_ARCH_INTERFACE_H_

#include "ast/symbolTable.h"
#include "ir/ir.h"

/**
 * get the format string to format local (numeric) labels
 * turns a size_t into an assembly label string
 */
char const *localLabelFormat(void);

/**
 * generate a function entry sequence
 *
 * @param entry who to generate the sequence for
 * @param returnValueAddressTemp temp to store return value's address in (unused
 * if function doesn't return a value via memory)
 * @param file file this is going to be in
 * @returns basic block containing entry sequence (but no jump to next block)
 */
IRBlock *generateFunctionEntry(SymbolTableEntry *entry,
                               size_t returnValueAddressTemp,
                               FileListEntry *file);
/**
 * generate a function exit sequence
 *
 * @param entry who to generate the sequence for
 * @param returnValueAddressTemp temp to get return value's address from (unused
 * if function doesn't return a value via memory)
 * @param returnValueTemp temp to get return value from (unused if function
 * returns void)
 * @param file file this is going to be in
 * @returns basic block containing exit sequence
 */
IRBlock *generateFunctionExit(SymbolTableEntry const *entry,
                              size_t returnValueAddressTemp,
                              size_t returnValueTemp, FileListEntry *file);

#endif  // TLC_ARCH_INTERFACE_H_