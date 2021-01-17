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
#include <stdlib.h>
#include <string.h>

#include "buildStab.h"
#include "fileList.h"
#include "internalError.h"
#include "parser/common.h"

// token stuff

/**
 * grabs the next token from unparsed
 *
 * like lex, but reads tokens from unparsed nodes
 *
 * assumes you don't go past the end of the unparsed
 *
 * @param unparsed node to read from
 * @param t token to write into
 */
static void next(Node *unparsed, Token *t) {
  // get pointer to saved token
  Token *saved =
      unparsed->data.unparsed.tokens->elements[unparsed->data.unparsed.curr];
  // give ownership of it to the caller
  memcpy(t, saved, sizeof(Token));
  // free malloc'ed block - this token has its guts ripped out
  free(saved);
  // clear it - we don't need to remember it any more
  unparsed->data.unparsed.tokens->elements[unparsed->data.unparsed.curr] = NULL;
  unparsed->data.unparsed.curr++;
}
static void prev(Node *unparsed, Token *t) {
  // malloc new block
  Token *saved = malloc(sizeof(Token));
  // copy from t (t now has its guts ripped out)
  memcpy(saved, t, sizeof(Token));
  // save it
  unparsed->data.unparsed.tokens->elements[--unparsed->data.unparsed.curr] =
      saved;
}

// miscellaneous functions

/**
 * skips tokens until an end of stmt is encountered
 *
 * consumes semicolons, leaves start of stmt tokens (including any ids) and
 * left/right braces (components of a compoundStmt individually fail and panic,
 * but a compoundStmt never ends with a panic)
 *
 * @param unparsed unparsed node to read from
 */
static void panicStmt(Node *unparsed) {
  while (true) {
    Token token;
    next(unparsed, &token);
    switch (token.type) {
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
      case TT_TYPEDEF:
      case TT_EOF: {
        prev(unparsed, &token);
        return;
      }
      default: {
        tokenUninit(&token);
        break;
      }
    }
  }
}

/**
 * skips tokens until a start of switch or unambiguous start of stmt is
 * encountered
 *
 * leaves start of stmt tokens (excluding ids) and start of case tokens
 *
 * @param unparsed unparsed node to read from
 */
static void panicSwitch(Node *unparsed) {
  while (true) {
    Token token;
    next(unparsed, &token);
    switch (token.type) {
      case TT_IF:
      case TT_WHILE:
      case TT_DO:
      case TT_FOR:
      case TT_SWITCH:
      case TT_BREAK:
      case TT_CONTINUE:
      case TT_RETURN:
      case TT_ASM:
      case TT_OPAQUE:
      case TT_STRUCT:
      case TT_UNION:
      case TT_ENUM:
      case TT_TYPEDEF:
      case TT_CASE:
      case TT_DEFAULT:
      case TT_EOF: {
        prev(unparsed, &token);
        return;
      }
      default: {
        tokenUninit(&token);
        break;
      }
    }
  }
}

/**
 * skips tokens until the end of a field/option boundary
 *
 * semicolons are consumed, right braces and the start of a type are left
 *
 * @param unparsed unparsed node to read from
 */
static void panicStructOrUnion(Node *unparsed) {
  while (true) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_SEMI: {
        return;
      }
      case TT_VOID:
      case TT_UBYTE:
      case TT_BYTE:
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
      case TT_EOF:
      case TT_RBRACE: {
        prev(unparsed, &peek);
        return;
      }
      default: {
        tokenUninit(&peek);
        break;
      }
    }
  }
}

// context ignorant parsers

/**
 * parses an extended int literal
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseExtendedIntLiteral(FileListEntry *entry, Node *unparsed,
                                     Environment *env) {
  return NULL;  // TODO: write this
}

/**
 * parses an ID or scoped ID
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 *
 * @returns node or null on error
 */
static Node *parseAnyId(FileListEntry *entry, Node *unparsed) {
  return NULL;  // TODO: write this
}

/**
 * parses a definitely scoped ID
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 *
 * @returns node or null on error
 */
static Node *parseScopedId(FileListEntry *entry, Node *unparsed) {
  return NULL;  // TODO: write this
}

/**
 * parses an ID (not scoped)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 *
 * @returns node or null on error
 */
static Node *parseId(FileListEntry *entry, Node *unparsed) {
  Token id;
  next(unparsed, &id);

  if (id.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &id);

    prev(unparsed, &id);
    return NULL;
  }

  return idNodeCreate(&id);
}

/**
 * parses a type
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseType(FileListEntry *entry, Node *unparsed, Environment *env) {
  return NULL;  // TODO: write this
}

/**
 * parses a field or option declaration
 *
 * @param
 */
static Node *parseFieldOrOptionDecl(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Token *start) {
  return NULL;  // TODO: write this
}

/**
 * parses an assignment expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseAssignmentExpression(FileListEntry *entry, Node *unparsed,
                                       Environment *env) {
  return NULL;  // TODO: write this
}

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

static Node *parseStmt(FileListEntry *, Node *, Environment *);
/**
 * parses a switch case
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseSwitchCase(FileListEntry *entry, Node *unparsed,
                             Environment *env, Token *start) {
  Vector *values = vectorCreate();
  Node *value = parseExtendedIntLiteral(entry, unparsed, env);
  if (value == NULL) {
    nodeVectorFree(values);
    return NULL;
  }
  vectorInsert(values, value);

  Token colon;
  next(unparsed, &colon);
  if (colon.type != TT_COLON) {
    errorExpectedToken(entry, TT_COLON, &colon);

    prev(unparsed, &colon);

    nodeVectorFree(values);
    return NULL;
  }

  while (true) {
    Token peek;
    next(unparsed, &peek);
    if (peek.type == TT_CASE) {
      value = parseExtendedIntLiteral(entry, unparsed, env);
      if (value == NULL) {
        nodeVectorFree(values);
        return NULL;
      }
      vectorInsert(values, value);

      next(unparsed, &colon);
      if (colon.type != TT_COLON) {
        errorExpectedToken(entry, TT_COLON, &colon);

        prev(unparsed, &colon);

        nodeVectorFree(values);
        return NULL;
      }
    } else {
      prev(unparsed, &peek);

      environmentPush(env, hashMapCreate());
      Node *body = parseStmt(entry, unparsed, env);
      HashMap *bodyStab = environmentPop(env);
      if (body == NULL) {
        panicSwitch(unparsed);

        nodeVectorFree(values);
        stabFree(bodyStab);
        return NULL;
      }

      return switchCaseNodeCreate(start, values, body, bodyStab);
    }
  }
}

/**
 * parses a switch default
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseSwitchDefault(FileListEntry *entry, Node *unparsed,
                                Environment *env, Token *start) {
  Token colon;
  next(unparsed, &colon);
  if (colon.type != TT_COLON) {
    errorExpectedToken(entry, TT_COLON, &colon);

    prev(unparsed, &colon);
    panicSwitch(unparsed);
    return NULL;
  }

  environmentPush(env, hashMapCreate());
  Node *body = parseStmt(entry, unparsed, env);
  HashMap *bodyStab = environmentPop(env);
  if (body == NULL) {
    panicSwitch(unparsed);

    stabFree(bodyStab);
    return NULL;
  }

  return switchDefaultNodeCreate(start, body, bodyStab);
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
  Token lbrace;
  next(unparsed, &lbrace);

  Vector *stmts = vectorCreate();
  environmentPush(env, hashMapCreate());

  while (true) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_RBRACE: {
        return compoundStmtNodeCreate(&lbrace, stmts, environmentPop(env));
      }
      case TT_EOF: {
        fprintf(stderr, "%s:%zu:%zu: error: unmatched left brace\n",
                entry->inputFilename, lbrace.line, lbrace.character);
        entry->errored = true;

        prev(unparsed, &peek);

        return compoundStmtNodeCreate(&lbrace, stmts, environmentPop(env));
      }
      default: {
        prev(unparsed, &peek);
        Node *stmt = parseStmt(entry, unparsed, env);
        if (stmt != NULL) vectorInsert(stmts, stmt);
        break;
      }
    }
  }
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
  Token lparen;
  next(unparsed, &lparen);
  if (lparen.type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, &lparen);

    prev(unparsed, &lparen);
    panicStmt(unparsed);
    return NULL;
  }

  Node *predicate = parseExpression(entry, unparsed, env);
  if (predicate == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token rparen;
  next(unparsed, &rparen);
  if (rparen.type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, &rparen);

    prev(unparsed, &rparen);
    panicStmt(unparsed);

    nodeFree(predicate);
    return NULL;
  }

  environmentPush(env, hashMapCreate());
  Node *consequent = parseStmt(entry, unparsed, env);
  HashMap *consequentStab = environmentPop(env);
  if (consequent == NULL) {
    stabFree(consequentStab);
    nodeFree(predicate);
    return NULL;
  }

  Token elseKwd;
  next(unparsed, &elseKwd);
  if (elseKwd.type != TT_ELSE)
    return ifStmtNodeCreate(start, predicate, consequent, consequentStab, NULL,
                            NULL);

  environmentPush(env, hashMapCreate());
  Node *alternative = parseStmt(entry, unparsed, env);
  HashMap *alternativeStab = environmentPop(env);
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

/**
 * parses a while statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseWhileStmt(FileListEntry *entry, Node *unparsed,
                            Environment *env, Token *start) {
  Token lparen;
  next(unparsed, &lparen);
  if (lparen.type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, &lparen);

    prev(unparsed, &lparen);
    panicStmt(unparsed);
    return NULL;
  }

  Node *condition = parseExpression(entry, unparsed, env);
  if (condition == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token rparen;
  next(unparsed, &rparen);
  if (rparen.type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, &rparen);

    prev(unparsed, &rparen);
    panicStmt(unparsed);

    nodeFree(condition);
    return NULL;
  }

  environmentPush(env, hashMapCreate());
  Node *body = parseStmt(entry, unparsed, env);
  HashMap *bodyStab = environmentPop(env);
  if (body == NULL) {
    stabFree(bodyStab);
    nodeFree(condition);
    return NULL;
  }

  return whileStmtNodeCreate(start, condition, body, bodyStab);
}

/**
 * parses a do-while statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseDoWhileStmt(FileListEntry *entry, Node *unparsed,
                              Environment *env, Token *start) {
  environmentPush(env, hashMapCreate());
  Node *body = parseStmt(entry, unparsed, env);
  HashMap *bodyStab = environmentPop(env);
  if (body == NULL) {
    panicStmt(unparsed);

    stabFree(bodyStab);
    return NULL;
  }

  Token whileKwd;
  next(unparsed, &whileKwd);
  if (whileKwd.type != TT_WHILE) {
    errorExpectedToken(entry, TT_WHILE, &whileKwd);

    prev(unparsed, &whileKwd);
    panicStmt(unparsed);

    nodeFree(body);
    stabFree(bodyStab);
    return NULL;
  }

  Token lparen;
  next(unparsed, &lparen);
  if (lparen.type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, &lparen);

    prev(unparsed, &lparen);
    panicStmt(unparsed);

    nodeFree(body);
    stabFree(bodyStab);
    return NULL;
  }

  Node *condition = parseExpression(entry, unparsed, env);
  if (condition == NULL) {
    panicStmt(unparsed);

    nodeFree(body);
    stabFree(bodyStab);
    return NULL;
  }

  Token rparen;
  next(unparsed, &rparen);
  if (rparen.type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, &rparen);

    prev(unparsed, &rparen);
    panicStmt(unparsed);

    nodeFree(condition);
    nodeFree(body);
    stabFree(bodyStab);
    return NULL;
  }

  return doWhileStmtNodeCreate(start, body, bodyStab, condition);
}

/**
 * parses a for statement initializer
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseForInitStmt(FileListEntry *entry, Node *unparsed,
                              Environment *env) {
  Token peek;
  next(unparsed, &peek);
  switch (peek.type) {
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
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
    case TT_LSQUARE:
    case TT_SEMI: {
      prev(unparsed, &peek);
      return parseStmt(entry, unparsed, env);
    }
    default: {
      errorExpectedString(
          entry, "a variable declaration, an expression, or a semicolon",
          &peek);

      prev(unparsed, &peek);
      panicStmt(unparsed);
      return NULL;
    }
  }
}

/**
 * parses a for statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseForStmt(FileListEntry *entry, Node *unparsed,
                          Environment *env, Token *start) {
  Token lparen;
  next(unparsed, &lparen);
  if (lparen.type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, &lparen);

    prev(unparsed, &lparen);
    panicStmt(unparsed);
    return NULL;
  }

  environmentPush(env, hashMapCreate());
  Node *initializer = parseForInitStmt(entry, unparsed, env);
  if (initializer == NULL) {
    panicStmt(unparsed);

    stabFree(environmentPop(env));
    return NULL;
  }

  Node *condition = parseExpression(entry, unparsed, env);
  if (condition == NULL) {
    panicStmt(unparsed);

    nodeFree(initializer);
    stabFree(environmentPop(env));
  }

  Token semi;
  next(unparsed, &semi);
  if (semi.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semi);

    prev(unparsed, &semi);
    panicStmt(unparsed);

    nodeFree(condition);
    nodeFree(initializer);
    stabFree(environmentPop(env));
  }

  Token peek;
  next(unparsed, &peek);
  Node *increment = NULL;
  if (peek.type != TT_RPAREN) {
    // increment isn't null
    prev(unparsed, &peek);
    increment = parseExpression(entry, unparsed, env);
    if (increment == NULL) {
      panicStmt(unparsed);

      nodeFree(condition);
      nodeFree(initializer);
      stabFree(environmentPop(env));
      return NULL;
    }
  }

  Token rparen;
  next(unparsed, &rparen);
  if (rparen.type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, &rparen);

    prev(unparsed, &rparen);
    panicStmt(unparsed);

    nodeFree(increment);
    nodeFree(condition);
    nodeFree(initializer);
    stabFree(environmentPop(env));
    return NULL;
  }

  environmentPush(env, hashMapCreate());
  Node *body = parseStmt(entry, unparsed, env);
  HashMap *bodyStab = environmentPop(env);
  if (body == NULL) {
    nodeFree(increment);
    nodeFree(condition);
    nodeFree(initializer);
    stabFree(environmentPop(env));
    return NULL;
  }

  HashMap *loopStab = environmentPop(env);
  return forStmtNodeCreate(start, loopStab, initializer, condition, increment,
                           body, bodyStab);
}

/**
 * parses a switch statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseSwitchStmt(FileListEntry *entry, Node *unparsed,
                             Environment *env, Token *start) {
  Token lparen;
  next(unparsed, &lparen);
  if (lparen.type != TT_LPAREN) {
    errorExpectedToken(entry, TT_LPAREN, &lparen);

    prev(unparsed, &lparen);
    panicStmt(unparsed);
    return NULL;
  }

  Node *condition = parseExpression(entry, unparsed, env);
  if (condition == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token rparen;
  next(unparsed, &rparen);
  if (rparen.type != TT_RPAREN) {
    errorExpectedToken(entry, TT_RPAREN, &rparen);

    prev(unparsed, &rparen);
    panicStmt(unparsed);

    nodeFree(condition);
    return NULL;
  }

  Token lbrace;
  next(unparsed, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    prev(unparsed, &lbrace);
    panicStmt(unparsed);

    nodeFree(condition);
  }

  Vector *cases = vectorCreate();
  bool doneCases = false;
  while (!doneCases) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_CASE: {
        // start of a case
        Node *caseNode = parseSwitchCase(entry, unparsed, env, &peek);
        if (caseNode == NULL) {
          panicSwitch(unparsed);
          continue;
        }
        vectorInsert(cases, caseNode);
        break;
      }
      case TT_DEFAULT: {
        Node *defaultNode = parseSwitchDefault(entry, unparsed, env, &peek);
        if (defaultNode == NULL) {
          panicSwitch(unparsed);
          continue;
        }
        vectorInsert(cases, defaultNode);
        break;
      }
      case TT_RBRACE: {
        doneCases = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace of a switch case", &peek);

        prev(unparsed, &peek);
        panicStmt(unparsed);

        nodeVectorFree(cases);
        nodeFree(condition);
        return NULL;
      }
    }
  }

  if (cases->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one case in a switch "
            "statement\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeVectorFree(cases);
    nodeFree(condition);
    return NULL;
  }

  return switchStmtNodeCreate(start, condition, cases);
}

/**
 * parses a break statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseBreakStmt(FileListEntry *entry, Node *unparsed,
                            Environment *env, Token *start) {
  Token semi;
  next(unparsed, &semi);
  if (semi.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semi);

    prev(unparsed, &semi);
    panicStmt(unparsed);
    return NULL;
  }

  return breakStmtNodeCreate(start);
}

/**
 * parses a continue statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseContinueStmt(FileListEntry *entry, Node *unparsed,
                               Environment *env, Token *start) {
  Token semi;
  next(unparsed, &semi);
  if (semi.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semi);

    prev(unparsed, &semi);
    panicStmt(unparsed);
    return NULL;
  }

  return continueStmtNodeCreate(start);
}

/**
 * parses a return statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseReturnStmt(FileListEntry *entry, Node *unparsed,
                             Environment *env, Token *start) {
  Token peek;
  next(unparsed, &peek);
  if (peek.type == TT_SEMI) {
    return returnStmtNodeCreate(start, NULL);
  } else {
    prev(unparsed, &peek);
    Node *value = parseExpression(entry, unparsed, env);

    Token semi;
    next(unparsed, &semi);
    if (semi.type == TT_SEMI) {
      errorExpectedToken(entry, TT_SEMI, &semi);

      prev(unparsed, &semi);
      panicStmt(unparsed);

      nodeFree(value);
      return NULL;
    }

    return returnStmtNodeCreate(start, value);
  }
}

/**
 * parses an asm statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseAsmStmt(FileListEntry *entry, Node *unparsed,
                          Environment *env, Token *start) {
  Token str;
  next(unparsed, &str);
  if (str.type != TT_LIT_STRING) {
    errorExpectedToken(entry, TT_LIT_STRING, &str);

    prev(unparsed, &str);
    panicStmt(unparsed);
    return NULL;
  }

  Token semi;
  next(unparsed, &semi);
  if (semi.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semi);

    prev(unparsed, &semi);
    panicStmt(unparsed);

    tokenUninit(&str);
    return NULL;
  }

  return asmStmtNodeCreate(start, stringLiteralNodeCreate(&str));
}

/**
 * parses a variable definition statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseVarDefnStmt(FileListEntry *entry, Node *unparsed,
                              Environment *env) {
  Node *typeNode = parseType(entry, unparsed, env);
  if (typeNode == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Vector *names = vectorCreate();
  Vector *initializers = vectorCreate();
  bool done = false;
  while (!done) {
    Node *id = parseId(entry, unparsed);
    if (id == NULL) {
      panicStmt(unparsed);

      nodeVectorFree(initializers);
      nodeVectorFree(names);
      nodeFree(typeNode);
      return NULL;
    }
    vectorInsert(names, id);

    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_EQ: {
        // has initializer
        Node *initializer = parseAssignmentExpression(entry, unparsed, env);
        if (initializer == NULL) {
          panicStmt(unparsed);

          nodeVectorFree(initializers);
          nodeVectorFree(names);
          nodeFree(typeNode);
          return NULL;
        }
        vectorInsert(initializers, initializer);

        next(unparsed, &peek);
        switch (peek.type) {
          case TT_COMMA: {
            // declaration continues
            break;
          }
          case TT_SEMI: {
            // end of declaration
            done = true;
            break;
          }
          default: {
            errorExpectedString(entry, "a comma or a semicolon", &peek);

            prev(unparsed, &peek);
            panicStmt(unparsed);

            nodeVectorFree(initializers);
            nodeVectorFree(names);
            nodeFree(typeNode);
            return NULL;
          }
        }
        break;
      }
      case TT_COMMA: {
        // continue definition
        vectorInsert(initializers, NULL);
        break;
      }
      case TT_SEMI: {
        // done
        if (names->size == 0) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: expected at least one name in a variable "
                  "declaration\n",
                  entry->inputFilename, typeNode->line, typeNode->character);
          entry->errored = true;

          nodeVectorFree(initializers);
          nodeVectorFree(names);
          nodeFree(typeNode);
          return NULL;
        }

        done = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a comma, a semicolon, or an equals sign",
                            &peek);

        prev(unparsed, &peek);
        panicStmt(unparsed);

        nodeVectorFree(initializers);
        nodeVectorFree(names);
        nodeFree(typeNode);
        return NULL;
      }
    }
  }

  Type *type = nodeToType(typeNode, env);
  if (type == NULL) {
    nodeVectorFree(initializers);
    nodeVectorFree(names);
    nodeFree(typeNode);
    return NULL;
  }

  for (size_t idx = 0; idx < names->size; idx++) {
    Node *name = names->elements[idx];
    name->data.id.entry =
        variableStabEntryCreate(entry, name->line, name->character);
    name->data.id.entry->data.variable.type = typeCopy(type);

    SymbolTableEntry *existing =
        hashMapGet(environmentTop(env), name->data.id.id);
    if (existing != NULL) {
      // whoops - this already exists! complain!
      errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                         existing->file, existing->line, existing->character);
    }

    hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
  }
  typeFree(type);

  return varDefnStmtNodeCreate(typeNode, names, initializers);
}

/**
 * parses an expression statement
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 *
 * @returns node or null on error
 */
static Node *parseExpressionStmt(FileListEntry *entry, Node *unparsed,
                                 Environment *env) {
  Node *expression = parseExpression(entry, unparsed, env);
  if (expression == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token semi;
  next(unparsed, &semi);
  if (semi.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semi);

    prev(unparsed, &semi);
    panicStmt(unparsed);

    nodeFree(expression);
    return NULL;
  }

  return expressionStmtNodeCreate(expression);
}

/**
 * parses an opaque decl (within a function)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseOpaqueDecl(FileListEntry *entry, Node *unparsed,
                             Environment *env, Token *start) {
  Node *name = parseId(entry, unparsed);
  if (name == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token semicolon;
  next(unparsed, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    prev(unparsed, &semicolon);
    panicStmt(unparsed);

    nodeFree(name);
    return NULL;
  }

  name->data.id.entry =
      opaqueStabEntryCreate(entry, start->line, start->character);

  SymbolTableEntry *existing =
      hashMapGet(environmentTop(env), name->data.id.id);
  if (existing != NULL) {
    // whoops - this already exists! complain!
    errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                       existing->file, existing->line, existing->character);
  }

  hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
  return opaqueDeclNodeCreate(start, name);
}

/**
 * parses a struct decl (within a function)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseStructDecl(FileListEntry *entry, Node *unparsed,
                             Environment *env, Token *start) {
  Node *name = parseId(entry, unparsed);
  if (name == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token lbrace;
  next(unparsed, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    prev(unparsed, &lbrace);
    panicStmt(unparsed);

    nodeFree(name);
    return NULL;
  }

  Vector *fields = vectorCreate();
  bool doneFields = false;
  while (!doneFields) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_VOID:
      case TT_UBYTE:
      case TT_BYTE:
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
      case TT_ID: {
        // start of a field
        Node *field = parseFieldOrOptionDecl(entry, unparsed, env, &peek);
        if (field == NULL) {
          panicStructOrUnion(unparsed);
          continue;
        }
        vectorInsert(fields, field);
        break;
      }
      case TT_RBRACE: {
        doneFields = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or a field", &peek);

        prev(unparsed, &peek);
        panicStmt(unparsed);

        nodeFree(name);
        nodeVectorFree(fields);
        return NULL;
      }
    }
  }

  if (fields->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one field in a struct "
            "declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeFree(name);
    nodeVectorFree(fields);
    return NULL;
  }

  Token semicolon;
  next(unparsed, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    prev(unparsed, &semicolon);
    panicStmt(unparsed);

    nodeFree(name);
    nodeVectorFree(fields);
  }

  Node *body = structDeclNodeCreate(start, name, fields);
  SymbolTableEntry *existing =
      hashMapGet(environmentTop(env), name->data.id.id);
  if (existing != NULL) {
    if (existing->kind == SK_OPAQUE) {
      // overwrite the opaque
      name->data.id.entry = existing;
      existing->kind = SK_STRUCT;
      vectorInit(&existing->data.structType.fieldNames);
      vectorInit(&existing->data.structType.fieldTypes);
      finishStructStab(entry, body, name->data.id.entry, env);
    } else {
      // whoops - this already exists! complain!
      errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                         existing->file, existing->line, existing->character);
    }
  } else {
    // create a new entry
    name->data.id.entry =
        structStabEntryCreate(entry, start->line, start->character);
    hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
    finishStructStab(entry, body, name->data.id.entry, env);
  }

  return body;
}

/**
 * parses a union decl (within a function)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseUnionDecl(FileListEntry *entry, Node *unparsed,
                            Environment *env, Token *start) {
  Node *name = parseId(entry, unparsed);
  if (name == NULL) {
    panicStmt(unparsed);
    return NULL;
  }

  Token lbrace;
  next(unparsed, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    prev(unparsed, &lbrace);
    panicStmt(unparsed);

    nodeFree(name);
    return NULL;
  }

  Vector *options = vectorCreate();
  bool doneOptions = false;
  while (!doneOptions) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_VOID:
      case TT_UBYTE:
      case TT_BYTE:
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
      case TT_ID: {
        // start of an option
        Node *option = parseFieldOrOptionDecl(entry, unparsed, env, &peek);
        if (option == NULL) {
          panicStructOrUnion(unparsed);
          continue;
        }
        vectorInsert(options, option);
        break;
      }
      case TT_RBRACE: {
        doneOptions = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or an option", &peek);

        prev(unparsed, &peek);
        panicStmt(unparsed);

        nodeFree(name);
        nodeVectorFree(options);
        return NULL;
      }
    }
  }

  if (options->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one options in a union "
            "declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeFree(name);
    nodeVectorFree(options);
    return NULL;
  }

  Token semicolon;
  next(unparsed, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    prev(unparsed, &semicolon);
    panicStmt(unparsed);

    nodeFree(name);
    nodeVectorFree(options);
  }

  Node *body = unionDeclNodeCreate(start, name, options);
  SymbolTableEntry *existing =
      hashMapGet(environmentTop(env), name->data.id.id);
  if (existing != NULL) {
    if (existing->kind == SK_OPAQUE) {
      // overwrite the opaque
      name->data.id.entry = existing;
      existing->kind = SK_UNION;
      vectorInit(&existing->data.unionType.optionNames);
      vectorInit(&existing->data.unionType.optionTypes);
      finishUnionStab(entry, body, name->data.id.entry, env);
    } else {
      // whoops - this already exists! complain!
      errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                         existing->file, existing->line, existing->character);
    }
  } else {
    // create a new entry
    name->data.id.entry =
        unionStabEntryCreate(entry, start->line, start->character);
    hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
    finishUnionStab(entry, body, name->data.id.entry, env);
  }

  return body;
}

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
  Token peek;
  next(unparsed, &peek);
  switch (peek.type) {
    case TT_LBRACE: {
      // another compoundStmt
      prev(unparsed, &peek);
      return parseCompoundStmt(entry, unparsed, env);
    }
    case TT_IF: {
      return parseIfStmt(entry, unparsed, env, &peek);
    }
    case TT_WHILE: {
      return parseWhileStmt(entry, unparsed, env, &peek);
    }
    case TT_DO: {
      return parseDoWhileStmt(entry, unparsed, env, &peek);
    }
    case TT_FOR: {
      return parseForStmt(entry, unparsed, env, &peek);
    }
    case TT_SWITCH: {
      return parseSwitchStmt(entry, unparsed, env, &peek);
    }
    case TT_BREAK: {
      return parseBreakStmt(entry, unparsed, env, &peek);
    }
    case TT_CONTINUE: {
      return parseContinueStmt(entry, unparsed, env, &peek);
    }
    case TT_RETURN: {
      return parseReturnStmt(entry, unparsed, env, &peek);
    }
    case TT_ASM: {
      return parseAsmStmt(entry, unparsed, env, &peek);
    }
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
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
      prev(unparsed, &peek);
      return parseVarDefnStmt(entry, unparsed, env);
    }
    case TT_ID: {
      // maybe varDefn, maybe expressionStmt - disambiguate
      Node idNode;
      idNode.type = NT_ID;
      idNode.line = peek.line;
      idNode.character = peek.character;
      idNode.data.id.id = peek.string;
      idNode.data.id.entry = NULL;

      SymbolTableEntry *symbolEntry = environmentLookup(env, &idNode, false);
      if (symbolEntry == NULL) {
        prev(unparsed, &peek);
        panicStmt(unparsed);
        return NULL;
      }

      switch (symbolEntry->kind) {
        case SK_VARIABLE:
        case SK_FUNCTION:
        case SK_ENUMCONST: {
          prev(unparsed, &peek);
          return parseExpressionStmt(entry, unparsed, env);
        }
        case SK_OPAQUE:
        case SK_STRUCT:
        case SK_UNION:
        case SK_ENUM:
        case SK_TYPEDEF: {
          prev(unparsed, &peek);
          return parseVarDefnStmt(entry, unparsed, env);
        }
        default: {
          error(__FILE__, __LINE__, "invalid SymbolKind enum encountered");
        }
      }
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
      // unambiguously an expressionStmt
      prev(unparsed, &peek);
      return parseExpressionStmt(entry, unparsed, env);
    }
    case TT_OPAQUE: {
      return parseOpaqueDecl(entry, unparsed, env, &peek);
    }
    case TT_STRUCT: {
      return parseStructDecl(entry, unparsed, env, &peek);
    }
    case TT_UNION: {
      return parseUnionDecl(entry, unparsed, env, &peek);
    }
    case TT_ENUM: {
      // TODO: enum
      return NULL;
    }
    case TT_TYPEDEF: {
      // TODO: typedef
      return NULL;
    }
    case TT_SEMI: {
      return nullStmtNodeCreate(&peek);
    }
    default: {
      // unexpected token
      errorExpectedString(entry, "a declaration or a statement", &peek);

      prev(unparsed, &peek);

      panicStmt(unparsed);
      return NULL;
    }
  }
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