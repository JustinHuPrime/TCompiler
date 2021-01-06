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

#include <stdio.h>

#include "fileList.h"
#include "parser/common.h"

// TODO: also need to link ids to their referenced things
// TODO: remember to replace token strings with null if the string is used

// token stuff

/**
 * gets a references
 */
static Token *unparsedNext(Node *unparsed) {
  if (unparsed->data.unparsed.curr != unparsed->data.unparsed.tokens->size) {
    Token *t = unparsed->data.unparsed.tokens
                   ->elements[unparsed->data.unparsed.curr++];
    unparsed->data.unparsed.tokens->elements[unparsed->data.unparsed.curr++] =
        NULL;
    return t;
  } else {
    return NULL;
  }
}
static void unparsedPrev(Node *unparsed, Token *t) {
  unparsed->data.unparsed.tokens->elements[--unparsed->data.unparsed.curr] = t;
}

// miscellaneous functions

/**
 * skips nodes until an end of stmt is encountered
 *
 * consumes semicolons, leaves start of stmt tokens (including any ids) and
 * left/right braces (components of a compoundStmt individually fail and panic,
 * but a compoundStmt never ends with a panic)
 *
 * @param unparsed unparsed node to read from
 */
static void panicStmt(Node *unparsed) {
  while (true) {
    Token *token = unparsedNext(unparsed);
    if (token == NULL) {
      // hit EOF while panicking - stop here.
      return;
    }
    switch (token->type) {
      case TT_SEMI: {
        tokenUninit(token);
        return;
      }
      case TT_LBRACE:
      case TT_RBRACE:
      case TT_IF:
      case TT_WHILE:
      case TT_DO:
      case TT_FOR:
      case TT_SWITCH:
      case TT_BREAK:
      case TT_CONTINUE:
      case TT_RETURN:
      case TT_ASM:
      case TT_VOID:
      case TT_UBYTE:
      case TT_CHAR:
      case TT_USHORT:
      case TT_UINT:
      case TT_INT:
      case TT_WCHAR:
      case TT_ULONG:
      case TT_LONG:
      case TT_FLOAT:
      case TT_DOUBLE:
      case TT_BOOL:
      case TT_ID:
      case TT_OPAQUE:
      case TT_STRUCT:
      case TT_UNION:
      case TT_ENUM:
      case TT_TYPEDEF: {
        unparsedPrev(unparsed, token);
        return;
      }
      default: {
        tokenUninit(token);
        break;
      }
    }
  }
}

// context ignorant parsers

/**
 * parses an expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseExpression(FileListEntry *entry, Node *unparsed,
                             Environment *env) {
  return NULL;  // TODO: write this
}

// context sensitive parsers
/**
 * parses a compound stmt
 *
 * @param entry entry that contains this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node - never null
 */
static Node *parseCompoundStmt(FileListEntry *entry, Node *unparsed,
                               Environment *env) {
  return NULL;  // TODO: write this
}

void parseFunctionBody(FileListEntry *entry) {
  Environment env;
  environmentInit(&env, entry);

  for (size_t bodyIdx = 0; bodyIdx < entry->ast->data.file.bodies->size;
       ++bodyIdx) {
    // for each top level thing
    Node *body = entry->ast->data.file.bodies->elements[bodyIdx];
    switch (body->type) {
      // if it's a funDefn
      case NT_FUNDEFN: {
        // setup stab for arguments
        HashMap *stab = hashMapCreate();
        environmentPush(&env, stab);

        for (size_t argIdx = 0; argIdx < body->data.funDefn.argTypes->size;
             ++argIdx) {
          Node *argType = body->data.funDefn.argTypes->elements[argIdx];
          Node *argName = body->data.funDefn.argNames->elements[argIdx];
          SymbolTableEntry *stabEntry =
              variableStabEntryCreate(entry, argType->line, argType->character);
          stabEntry->data.variable.type = nodeToType(argType, &env);
          if (stabEntry->data.variable.type == NULL) entry->errored = true;
          hashMapPut(stab, argName->data.id.id, stabEntry);
        }

        // parse and reference resolve body, replacing it in the original ast
        Node *unparsed = body->data.funDefn.body;
        body->data.funDefn.body = parseCompoundStmt(entry, unparsed, &env);
        nodeFree(unparsed);

        body->data.funDefn.argStab = environmentPop(&env);
        break;
      }
      default: {
        // don't need to do anything
        break;
      }
    }
  }

  environmentUninit(&env);
}