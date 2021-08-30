// Copyright 2021 Justin Hu
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

#include "arch/x86_64-linux/backend.h"

#include "fileList.h"
#include "ir/ir.h"

void x86_64LinuxBackend(void) {
  // assembly generation
  // TODO

  // done with IR
  for (size_t idx = 0; idx < fileList.size; ++idx)
    vectorUninit(&fileList.entries[idx].irFrags, (void (*)(void *))irFragFree);

  // assembly optimization 1
  // TODO

  // register allocation
  // TODO

  // assembly optimization 2
  // TODO

  // write out
  // TODO
}