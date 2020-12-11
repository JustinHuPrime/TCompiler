// Copyright 2020 Justin Hu
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

#include "parser/functionBody.h"

#include "fileList.h"
#include "parser/common.h"

void parseFunctionBody(FileListEntry *entry) {
  Environment env;
  environmentInit(&env, entry);

  for (size_t bodyIdx = 0; bodyIdx < entry->ast->data.file.bodies->size;
       ++bodyIdx) {
    Node *body = entry->ast->data.file.bodies->elements[bodyIdx];
    switch (body->type) {
      case NT_FUNDEFN: {
        // construct stab for arguments
        for (size_t argIdx = 0; argIdx < body->data.funDefn.argTypes->size;
             ++argIdx) {
          Node *argType = body->data.funDefn.argTypes->elements[argIdx];
          SymbolTableEntry *stabEntry =
              variableStabEntryCreate(entry, argType->line, argType->character);
          stabEntry->data.variable.type = nodeToType(argType, &env);
        }
        break;
      }
      default: {
        // don't need to do anything
        break;
      }
    }
  }
}