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

#include "parser/miscCheck.h"

#include <stdio.h>

#include "fileList.h"
#include "internalError.h"

/**
 * performs misc checks on a stmt
 *
 * @param entry entry this statement is in
 * @param stmt statement to check
 * @param inLoop are we in a loop
 * @param inSwitch are we in a switch
 */
static void checkStmt(FileListEntry *entry, Node *stmt, bool inSwitch) {
  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      for (size_t idx = 0; idx < stmt->data.compoundStmt.stmts->size; ++idx) {
        Node *component = stmt->data.compoundStmt.stmts->elements[idx];
        checkStmt(entry, component, inSwitch);
      }
      break;
    }
    case NT_IFSTMT: {
      checkStmt(entry, stmt->data.ifStmt.consequent, inSwitch);
      if (stmt->data.ifStmt.alternative != NULL)
        checkStmt(entry, stmt->data.ifStmt.alternative, inSwitch);
      break;
    }
    case NT_SWITCHSTMT: {
      Vector *cases = stmt->data.switchStmt.cases;
      for (size_t idx = 0; idx < cases->size; ++idx) {
        Node *caseNode = cases->elements[idx];
        switch (caseNode->type) {
          case NT_SWITCHCASE: {
            checkStmt(entry, caseNode->data.switchCase.body, true);
            break;
          }
          case NT_SWITCHDEFAULT: {
            checkStmt(entry, caseNode->data.switchDefault.body, true);
            break;
          }
          default: {
            error(__FILE__, __LINE__,
                  "invalid case or default node in a switch");
          }
        }
      }
      break;
    }
    case NT_BREAKSTMT: {
      if (!inSwitch) {
        fprintf(stderr,
                "%s:%zu:%zu: error: break statements may not be outside of a "
                "loop or a switch\n",
                entry->inputFilename, stmt->line, stmt->character);
        entry->errored = true;
      }
      break;
    }
    case NT_CONTINUESTMT: {
      fprintf(stderr,
              "%s:%zu:%zu: error: continue statements may not be outside of "
              "a loop\n",
              entry->inputFilename, stmt->line, stmt->character);
      entry->errored = true;
    }
    default: {
      break;  // no checks to do
    }
  }
}

void checkMisc(FileListEntry *entry) {
  Vector *bodies = entry->ast->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; ++idx) {
    Node *body = bodies->elements[idx];
    if (body->type == NT_FUNDEFN) {
      checkStmt(entry, body->data.funDefn.body, false);
    }
  }
}