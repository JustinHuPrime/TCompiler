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

#include "arch/interface.h"

#include "options.h"
#include "util/internalError.h"
#include "x86_64-linux/abi.h"
#include "x86_64-linux/asm.h"
#include "x86_64-linux/backend.h"
#include "x86_64-linux/irValidation.h"

char const *prettyPrintRegister(size_t reg) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      return x86_64LinuxPrettyPrintRegister(reg);
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}

void generateFunctionEntry(LinkedList *blocks, SymbolTableEntry *entry,
                           size_t returnValueAddressTemp, size_t nextLabel,
                           FileListEntry *file) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      x86_64LinuxGenerateFunctionEntry(blocks, entry, returnValueAddressTemp,
                                       nextLabel, file);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}
void generateFunctionExit(LinkedList *blocks, SymbolTableEntry const *entry,
                          size_t returnValueAddressTemp, size_t returnValueTemp,
                          size_t prevLink, FileListEntry *file) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      x86_64LinuxGenerateFunctionExit(blocks, entry, returnValueAddressTemp,
                                      returnValueTemp, prevLink, file);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}
IROperand *generateFunctionCall(IRBlock *b, IROperand *fun, IROperand **args,
                                Type const *funType, FileListEntry *file) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      return x86_64LinuxGenerateFunctionCall(b, fun, args, funType, file);
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}

int validateIRArchSpecific(char const *phase, bool blocked) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      return x86_64LinuxValidateIRArchSpecific(phase, blocked);
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}

void backend(void) {
  switch (options.arch) {
    case OPTION_A_X86_64_LINUX: {
      x86_64LinuxBackend();
      break;
    }
    default: {
      error(__FILE__, __LINE__, "unrecognized architecture");
    }
  }
}