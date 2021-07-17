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
 * pretty print a register number
 */
char const *prettyPrintRegister(size_t reg);

/**
 * generate a function entry sequence
 *
 * @param blocks vector to insert generated block into
 * @param entry who to generate the sequence for
 * @param returnValueAddressTemp temp to store return value's address in (unused
 * if function doesn't return a value via memory)
 * @param nextLabel label to jump to
 * @param file file this is going to be in
 */
void generateFunctionEntry(Vector *blocks, SymbolTableEntry *entry,
                           size_t returnValueAddressTemp, size_t nextLabel,
                           FileListEntry *file);
/**
 * generate a function exit sequence
 *
 * @param blocks vector to insert generated block into
 * @param entry who to generate the sequence for
 * @param returnValueAddressTemp temp to get return value's address from (unused
 * if function doesn't return a value via memory)
 * @param returnValueTemp temp to get return value from (unused if function
 * returns void)
 * @param label id that the previous block jumps to
 * @param file file this is going to be in
 */
void generateFunctionExit(Vector *blocks, SymbolTableEntry const *entry,
                          size_t returnValueAddressTemp, size_t returnValueTemp,
                          size_t label, FileListEntry *file);

/**
 * generate a function call sequence
 *
 * @param b block to insert code into
 * @param fun function to call
 * @param args array of arguments to use in this call (length determined by
 * funType, owning)
 * @param funType type of function to call
 * @param file file this is going to be in
 * @returns IROperand with return value, or NULL if void function called
 */
IROperand *generateFunctionCall(IRBlock *b, IROperand *fun, IROperand **args,
                                Type const *funType, FileListEntry *file);

#endif  // TLC_ARCH_INTERFACE_H_