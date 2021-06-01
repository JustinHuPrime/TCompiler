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

#include "arch/x86_64-linux/abi.h"

#include "translation/translation.h"

IRBlock *x86_64LinuxGenerateFunctionEntry(SymbolTableEntry const *entry,
                                          FileListEntry *file) {
  IRBlock *b = irBlockCreate(fresh(file));
  // TODO
  return b;
}
IRBlock *x86_64LinuxGenerateFunctionExit(SymbolTableEntry const *entry,
                                         size_t returnValueTemp,
                                         FileListEntry *file) {
  IRBlock *b = irBlockCreate(fresh(file));
  Type const *returnType = entry->data.function.returnType;

  // is it a void function?
  if (returnType->kind == TK_KEYWORD &&
      returnType->data.keyword.keyword == TK_VOID) {
    // just return
    // TODO
  } else {
    // look at type
    // TODO
  }
  return b;
}