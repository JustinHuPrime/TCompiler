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
static Token const *unparsedNext(Node *unparsed) {
  if (unparsed->data.unparsed.curr != unparsed->data.unparsed.tokens->size)
    return unparsed->data.unparsed.tokens
        ->elements[unparsed->data.unparsed.curr++];
  else
    return NULL;
}
static void unparsedPrev(Node *unparsed) { --unparsed->data.unparsed.curr; }

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
    switch (token->type) {
      case TT_SEMI: {
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
        unparsedPrev(unparsed);
        return;
      }
      default: {
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
 * parses a while statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 */
static Node *parseWhileStmt(FileListEntry *entry, Node *unparsed,
                            Environment *env, Token *start) {
  Token *lparen = unparsedNext(unparsed);
  if (lparen->type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, lparen);

    unparsedPrev(unparsed);
    panicStmt(entry);
    return NULL;
  }

  Node *condition = parseExpression(entry, unparsed, env);
  if (condition == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token *rparen = unparsedNext(unparsed);
  if (rparen->type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, rparen);

    unparsedPrev(unparsed);
    panicStmt(unparsed);

    nodeFree(condition);
    return NULL;
  }

  HashMap *bodyStab = hashMapCreate();
  environmentPush(env, bodyStab);
  Node *body = parseStmt(entry, unparsed, env);
  environmentPop(env);
  if (body == NULL) {
    stabFree(bodyStab);
    nodeFree(condition);
    return NULL;
  }

  return whileStmtNodeCreate(start, condition, body, bodyStab);
}

/**
 * parses an if statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseIfStmt(FileListEntry *entry, Node *unparsed, Environment *env,
                         Token *start) {
  Token *lparen = unparsedNext(unparsed);
  if (lparen->type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, lparen);

    unparsedPrev(unparsed);
    panicStmt(entry);
    return NULL;
  }

  Node *predicate = parseExpression(entry, unparsed, env);
  if (predicate == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token *rparen = unparsedNext(unparsed);
  if (rparen->type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, rparen);

    unparsedPrev(unparsed);
    panicStmt(unparsed);

    nodeFree(predicate);
    return NULL;
  }

  HashMap *consequentStab = hashMapCreate();
  environmentPush(env, consequentStab);
  Node *consequent = parseStmt(entry, unparsed, env);
  environmentPop(env);
  if (consequent == NULL) {
    stabFree(consequentStab);
    nodeFree(predicate);
    return NULL;
  }

  Token *elseKwd = unparsedNext(unparsed);
  if (elseKwd->type != TT_ELSE)
    return ifStmtNodeCreate(start, predicate, consequent, consequentStab, NULL,
                            NULL);

  HashMap *alternativeStab = hashMapCreate();
  environmentPush(env, alternativeStab);
  Node *alternative = parseStmt(entry, unparsed, env);
  environmentPop(env);
  if (alternative == NULL) {
    stabFree(alternativeStab);
    nodeFree(consequent);
    stabFree(consequentStab);
    nodeFree(predicate);
    return NULL;
  }

  return ifStmtNodeCreate(start, predicate, consequent, consequentStab,
                          alternative, alternativeStab);
}

static Node *parseCompoundStmt(FileListEntry *, Node *, Environment *);
/**
 * parses a statement
 *
 * @param entry entry that contains this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on an error
 */
static Node *parseStmt(FileListEntry *entry, Node *unparsed, Environment *env) {
  Token const *peek = unparsedNext(unparsed);
  switch (peek->type) {
    case TT_LBRACE: {
      // another compoundStmt
      unparsedPrev(unparsed);
      return parseCompoundStmt(entry, unparsed, env);
    }
    case TT_IF: {
      return parseIfStmt(entry, unparsed, env, peek);
    }
    case TT_WHILE: {
      return parseWhileStmt(entry, unparsed, env, peek);
    }
    case TT_DO: {
      // TODO: do
      break;
    }
    case TT_FOR: {
      // TODO: for
      break;
    }
    case TT_SWITCH: {
      // TODO: switch
      break;
    }
    case TT_BREAK: {
      // TODO: break
      break;
    }
    case TT_CONTINUE: {
      // TODO: continue
      break;
    }
    case TT_RETURN: {
      // TODO: return
      break;
    }
    case TT_ASM: {
      // TODO: asm
      break;
    }
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
    case TT_BOOL: {
      // TODO: unambiguously a varDefn
      break;
    }
    case TT_ID: {
      // TODO: maybe varDefn, maybe expressionStmt
      break;
    }
    case TT_STAR:
    case TT_AMP:
    case TT_INC:
    case TT_DEC:
    case TT_MINUS:
    case TT_BANG:
    case TT_TILDE:
    case TT_CAST:
    case TT_SIZEOF:
    case TT_LPAREN:
    case TT_LSQUARE: {
      // TODO: unambiguously an expressionStmt
      break;
    }
    case TT_OPAQUE: {
      // TODO: opaque
      // TODO: include semantics for opaque in functions in standard
      break;
    }
    case TT_STRUCT: {
      // TODO: struct
      break;
    }
    case TT_UNION: {
      // TODO: union
      break;
    }
    case TT_ENUM: {
      // TODO: enum
      break;
    }
    case TT_TYPEDEF: {
      // TODO: typedef
      break;
    }
    case TT_SEMI: {
      // nullStmt
      return nullStmtNodeCreate(peek);
    }
    default: {
      // unexpected token
      errorExpectedString(entry, "a declaration or a statement", peek);
      panicStmt(unparsed);
      return NULL;
    }
  }
}

/**
 * parses a compound stmt
 *
 * assumes correct parens (enforced in parseFuncBody)
 *
 * @param entry entry that contains this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node - never null
 */
static Node *parseCompoundStmt(FileListEntry *entry, Node *unparsed,
                               Environment *env) {
  Token const *lbrace = unparsedNext(unparsed);
  Vector *stmts = vectorCreate();
  HashMap *stab = hashMapCreate();
  environmentPush(env, stab);

  Token const *peek = unparsedNext(unparsed);
  while (true) {
    if (peek == NULL) {
      // deal with unexpected end
      fprintf(stderr, "%s:%zu:%zu: error: unmatched left brace\n",
              entry->inputFilename, lbrace->line, lbrace->character);
      entry->errored = true;
      return compoundStmtNodeCreate(lbrace, stmts, environmentPop(env));
    }
    switch (peek->type) {
      case TT_RBRACE: {
        return compoundStmtNodeCreate(lbrace, stmts, environmentPop(env));
      }
      default: {
        unparsedPrev(unparsed);
        Node *stmt = parseStmt(entry, unparsed, env);
        if (stmt != NULL) vectorInsert(stmts, stmt);
      }
    }
  }
}

void parseFunctionBody(FileListEntry *entry) {
  Environment env;
  environmentInit(&env, entry);

  for (size_t bodyIdx = 0; bodyIdx < entry->ast->data.file.bodies->size;
       ++bodyIdx) {
    Node *body = entry->ast->data.file.bodies->elements[bodyIdx];
    switch (body->type) {
      case NT_FUNDEFN: {
        HashMap *stab = hashMapCreate();
        environmentPush(&env, stab);

        // construct stab for arguments
        for (size_t argIdx = 0; argIdx < body->data.funDefn.argTypes->size;
             ++argIdx) {
          Node *argType = body->data.funDefn.argTypes->elements[argIdx];
          Node *argName = body->data.funDefn.argNames->elements[argIdx];
          SymbolTableEntry *stabEntry =
              variableStabEntryCreate(entry, argType->line, argType->character);
          stabEntry->data.variable.type = nodeToType(argType, &env);
          if (stabEntry->data.variable.type == NULL) entry->errored = true;
          hashMapPut(stabEntry, argName->data.id.id, stabEntry);
        }

        // parse and reference resolve body
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