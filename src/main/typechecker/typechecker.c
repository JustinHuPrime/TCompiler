// Copyright 2019, 2021 Justin Hu
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

#include "typechecker/typechecker.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileList.h"
#include "internalError.h"

/**
 * typechecks a code file
 *
 * @param entry entry to typecheck
 */
static void typecheckFile(FileListEntry *entry) {
  // TODO
}

int typecheck(void) {
  bool errored = false;

  // for each code file, type check it
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (fileList.entries[idx].isCode) typecheckFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;
  }

  if (errored) return -1;

  return 0;
}