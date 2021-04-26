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

#include "parser/functionBody.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buildStab.h"
#include "fileList.h"
#include "parser/common.h"
#include "util/conversions.h"
#include "util/internalError.h"

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
  unparsed->data.unparsed.tokens->elements[unparsed->data.unparsed.curr++] =
      NULL;
}

/**
 * returns a token to unparsed
 *
 * like lex, but returns tokens to unparsed nodes
 *
 * assumes you don't go past the start of the unparsed
 *
 * @param unparsed node to write to
 * @param t token to read from
 */
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
 * consumes semicolons, leaves start of stmt tokens (not including ids) and
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

/**
 * skips tokens until the end of an enum constant
 *
 * commas are consumed, EOFs, and right braces are left
 *
 * @param unparsed unparsed node to read from
 */
static void panicEnum(Node *unparsed) {
  while (true) {
    Token token;
    next(unparsed, &token);
    switch (token.type) {
      case TT_COMMA: {
        return;
      }
      case TT_EOF:
      case TT_RBRACE: {
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

// context ignorant parsers

/**
 * parses an ID or scoped ID
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 *
 * @returns node or null on error
 */
static Node *parseAnyId(FileListEntry *entry, Node *unparsed) {
  Token idToken;
  next(unparsed, &idToken);
  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
    prev(unparsed, &idToken);
    return NULL;
  }

  // maybe it's a scoped id?
  Token scope;
  next(unparsed, &scope);
  if (scope.type != TT_SCOPE) {
    // not a scoped id
    prev(unparsed, &scope);
    return idNodeCreate(&idToken);
  } else {
    // scoped id - saw scope
    Vector *components = vectorCreate();
    vectorInsert(components, idNodeCreate(&idToken));
    while (true) {
      // expect an id, add it to the node
      next(unparsed, &idToken);
      if (idToken.type != TT_ID) {
        errorExpectedToken(entry, TT_ID, &idToken);

        prev(unparsed, &idToken);

        nodeVectorFree(components);
        return NULL;
      } else {
        vectorInsert(components, idNodeCreate(&idToken));
      }

      // if there's a scope, keep going, else return
      next(unparsed, &scope);
      if (scope.type != TT_SCOPE) {
        prev(unparsed, &scope);
        return scopedIdNodeCreate(components);
      }
    }
  }
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
  Vector *components = vectorCreate();
  while (true) {
    Token peek;
    // expect an id, add it to the node
    next(unparsed, &peek);
    if (peek.type != TT_ID) {
      errorExpectedToken(entry, TT_ID, &peek);

      prev(unparsed, &peek);

      nodeVectorFree(components);
      return NULL;
    } else {
      vectorInsert(components, idNodeCreate(&peek));
    }

    // if there's a scope, keep going, else return
    next(unparsed, &peek);
    if (peek.type != TT_SCOPE) {
      prev(unparsed, &peek);

      if (components->size >= 2) {
        return scopedIdNodeCreate(components);
      } else {
        errorExpectedToken(entry, TT_SCOPE, &peek);

        nodeVectorFree(components);
        return NULL;
      }
    }
  }
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
  Token peek;
  next(unparsed, &peek);
  switch (peek.type) {
    case TT_LIT_CHAR: {
      return charLiteralNodeCreate(&peek);
    }
    case TT_LIT_WCHAR: {
      return wcharLiteralNodeCreate(&peek);
    }
    case TT_LIT_INT_B: {
      int8_t sign;
      uint64_t magnitude;
      int retval = binaryToInteger(peek.string, &sign, &magnitude);
      if (retval != 0) {
        errorIntOverflow(entry, &peek);

        tokenUninit(&peek);

        return NULL;
      }
      Node *n = sizedIntegerLiteralNodeCreate(&peek, sign, magnitude);
      if (n == NULL) errorIntOverflow(entry, &peek);
      return n;
    }
    case TT_LIT_INT_O: {
      int8_t sign;
      uint64_t magnitude;
      int retval = octalToInteger(peek.string, &sign, &magnitude);
      if (retval != 0) {
        errorIntOverflow(entry, &peek);

        tokenUninit(&peek);

        return NULL;
      }
      Node *n = sizedIntegerLiteralNodeCreate(&peek, sign, magnitude);
      if (n == NULL) errorIntOverflow(entry, &peek);
      return n;
    }
    case TT_LIT_INT_0:
    case TT_LIT_INT_D: {
      int8_t sign;
      uint64_t magnitude;
      int retval = decimalToInteger(peek.string, &sign, &magnitude);
      if (retval != 0) {
        errorIntOverflow(entry, &peek);

        tokenUninit(&peek);

        return NULL;
      }
      Node *n = sizedIntegerLiteralNodeCreate(&peek, sign, magnitude);
      if (n == NULL) errorIntOverflow(entry, &peek);
      return n;
    }
    case TT_LIT_INT_H: {
      int8_t sign;
      uint64_t magnitude;
      int retval = hexadecimalToInteger(peek.string, &sign, &magnitude);
      if (retval != 0) {
        errorIntOverflow(entry, &peek);

        tokenUninit(&peek);

        return NULL;
      }
      Node *n = sizedIntegerLiteralNodeCreate(&peek, sign, magnitude);
      if (n == NULL) errorIntOverflow(entry, &peek);
      return n;
    }
    case TT_ID: {
      prev(unparsed, &peek);
      Node *n = parseScopedId(entry, unparsed);
      if (n == NULL) {
        return NULL;
      }

      SymbolTableEntry *stabEntry = environmentLookup(env, n, false);
      if (stabEntry == NULL) {
        nodeFree(n);
        return NULL;
      } else if (stabEntry->kind != SK_ENUMCONST) {
        fprintf(stderr,
                "%s:%zu:%zu: error: expected an extended integer "
                "literal, found %s\n",
                entry->inputFilename, n->line, n->character,
                symbolKindToString(stabEntry->kind));
        entry->errored = true;

        nodeFree(n);
        return NULL;
      }

      return n;
    }
    case TT_BAD_CHAR:
    case TT_BAD_BIN:
    case TT_BAD_HEX: {
      return NULL;
    }
    default: {
      errorExpectedString(entry, "an exended integer literal", &peek);

      prev(unparsed, &peek);

      return NULL;
    }
  }
}

static Node *parseLiteral(FileListEntry *entry, Node *unparsed,
                          Environment *env);
/**
 * parses an aggregate initializer
 *
 * @param entry entry to lex from
 * @param start first token in aggregate init
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAggregateInitializer(FileListEntry *entry, Node *unparsed,
                                       Environment *env, Token *start) {
  Vector *literals = vectorCreate();
  while (true) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_LIT_STRING:
      case TT_LIT_WSTRING:
      case TT_LIT_CHAR:
      case TT_LIT_WCHAR:
      case TT_LIT_INT_0:
      case TT_LIT_INT_B:
      case TT_LIT_INT_O:
      case TT_LIT_INT_D:
      case TT_LIT_INT_H:
      case TT_LIT_DOUBLE:
      case TT_LIT_FLOAT:
      case TT_TRUE:
      case TT_FALSE:
      case TT_NULL:
      case TT_ID:
      case TT_LSQUARE:
      case TT_BAD_STRING:
      case TT_BAD_CHAR:
      case TT_BAD_BIN:
      case TT_BAD_HEX: {
        // this is the start of a field
        prev(unparsed, &peek);
        Node *literal = parseLiteral(entry, unparsed, env);
        if (literal == NULL) {
          nodeVectorFree(literals);
          return NULL;
        }
        vectorInsert(literals, literal);

        next(unparsed, &peek);
        switch (peek.type) {
          case TT_RSQUARE: {
            // end of the init
            Node *n = literalNodeCreate(LT_AGGREGATEINIT, start);
            n->data.literal.data.aggregateInitVal = literals;
            return n;
          }
          case TT_COMMA: {
            break;  // continue on
          }
          default: {
            errorExpectedString(entry, "a comma or a right square bracket",
                                &peek);

            prev(unparsed, &peek);

            nodeVectorFree(literals);
            return NULL;
          }
        }
        break;
      }
      case TT_RSQUARE: {
        // end of the init
        Node *n = literalNodeCreate(LT_AGGREGATEINIT, start);
        n->data.literal.data.aggregateInitVal = literals;
        return n;
      }
      default: {
        errorExpectedString(entry, "a literal", &peek);

        prev(unparsed, &peek);

        nodeVectorFree(literals);
        return NULL;
      }
    }
  }
}

/**
 * parses a literal
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseLiteral(FileListEntry *entry, Node *unparsed,
                          Environment *env) {
  Token peek;
  next(unparsed, &peek);
  switch (peek.type) {
    case TT_LIT_CHAR:
    case TT_LIT_WCHAR:
    case TT_LIT_INT_B:
    case TT_LIT_INT_O:
    case TT_LIT_INT_0:
    case TT_LIT_INT_D:
    case TT_LIT_INT_H:
    case TT_ID: {
      prev(unparsed, &peek);
      return parseExtendedIntLiteral(entry, unparsed, env);
    }
    case TT_LIT_STRING: {
      return stringLiteralNodeCreate(&peek);
    }
    case TT_LIT_WSTRING: {
      return wstringLiteralNodeCreate(&peek);
    }
    case TT_LIT_DOUBLE: {
      uint64_t bits = doubleStringToBits(peek.string);
      Node *n = literalNodeCreate(LT_DOUBLE, &peek);
      n->data.literal.data.doubleBits = bits;
      tokenUninit(&peek);
      return n;
    }
    case TT_LIT_FLOAT: {
      uint32_t bits = floatStringToBits(peek.string);
      Node *n = literalNodeCreate(LT_FLOAT, &peek);
      n->data.literal.data.floatBits = bits;
      tokenUninit(&peek);
      return n;
    }

    case TT_TRUE:
    case TT_FALSE: {
      Node *n = literalNodeCreate(LT_BOOL, &peek);
      n->data.literal.data.boolVal = peek.type == TT_TRUE;
      return n;
    }
    case TT_NULL: {
      return literalNodeCreate(LT_NULL, &peek);
    }
    case TT_LSQUARE: {
      // aggregate initializer
      return parseAggregateInitializer(entry, unparsed, env, &peek);
    }
    case TT_BAD_CHAR:
    case TT_BAD_BIN:
    case TT_BAD_HEX:
    case TT_BAD_STRING: {
      return NULL;
    }
    default: {
      errorExpectedString(entry, "a literal", &peek);

      prev(unparsed, &peek);

      return NULL;
    }
  }
}

/**
 * parses a type
 *
 * @param entry entry to lex from
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param startId first id node in type or null if no first id node
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseType(FileListEntry *entry, Node *unparsed, Environment *env,
                       Node *startId) {
  Node *type;

  if (startId == NULL) {
    Token start;
    next(unparsed, &start);
    switch (start.type) {
      case TT_VOID: {
        type = keywordTypeNodeCreate(TK_VOID, &start);
        break;
      }
      case TT_UBYTE: {
        type = keywordTypeNodeCreate(TK_UBYTE, &start);
        break;
      }
      case TT_BYTE: {
        type = keywordTypeNodeCreate(TK_BYTE, &start);
        break;
      }
      case TT_CHAR: {
        type = keywordTypeNodeCreate(TK_CHAR, &start);
        break;
      }
      case TT_USHORT: {
        type = keywordTypeNodeCreate(TK_USHORT, &start);
        break;
      }
      case TT_SHORT: {
        type = keywordTypeNodeCreate(TK_SHORT, &start);
        break;
      }
      case TT_UINT: {
        type = keywordTypeNodeCreate(TK_UINT, &start);
        break;
      }
      case TT_INT: {
        type = keywordTypeNodeCreate(TK_INT, &start);
        break;
      }
      case TT_WCHAR: {
        type = keywordTypeNodeCreate(TK_WCHAR, &start);
        break;
      }
      case TT_ULONG: {
        type = keywordTypeNodeCreate(TK_ULONG, &start);
        break;
      }
      case TT_LONG: {
        type = keywordTypeNodeCreate(TK_LONG, &start);
        break;
      }
      case TT_FLOAT: {
        type = keywordTypeNodeCreate(TK_FLOAT, &start);
        break;
      }
      case TT_DOUBLE: {
        type = keywordTypeNodeCreate(TK_DOUBLE, &start);
        break;
      }
      case TT_BOOL: {
        type = keywordTypeNodeCreate(TK_BOOL, &start);
        break;
      }
      case TT_ID: {
        prev(unparsed, &start);
        type = parseAnyId(entry, unparsed);
        if (type == NULL) return NULL;
        break;
      }
      default: {
        errorExpectedString(entry, "a type", &start);

        prev(unparsed, &start);

        return NULL;
      }
    }
  } else {
    type = startId;
  }

  while (true) {
    Token next1;
    next(unparsed, &next1);
    switch (next1.type) {
      case TT_CONST: {
        type = modifiedTypeNodeCreate(TMK_CONST, type);
        break;
      }
      case TT_VOLATILE: {
        type = modifiedTypeNodeCreate(TMK_VOLATILE, type);
        break;
      }
      case TT_LSQUARE: {
        Node *size = parseExtendedIntLiteral(entry, unparsed, env);
        if (size == NULL) {
          nodeFree(type);

          return NULL;
        }

        Token rsquare;
        next(unparsed, &rsquare);
        if (rsquare.type != TT_RSQUARE) {
          errorExpectedToken(entry, TT_RSQUARE, &rsquare);

          prev(unparsed, &rsquare);

          nodeFree(type);
          nodeFree(size);
          return NULL;
        }

        type = arrayTypeNodeCreate(type, size);
        break;
      }
      case TT_STAR: {
        type = modifiedTypeNodeCreate(TMK_POINTER, type);
        break;
      }
      case TT_LPAREN: {
        Vector *argTypes = vectorCreate();
        Vector *argNames = vectorCreate();
        bool doneArgs = false;

        Token peek;
        next(unparsed, &peek);
        if (peek.type == TT_RPAREN)
          doneArgs = true;
        else
          prev(unparsed, &peek);
        while (!doneArgs) {
          Token next2;
          next(unparsed, &next2);
          switch (next2.type) {
            case TT_VOID:
            case TT_UBYTE:
            case TT_BYTE:
            case TT_CHAR:
            case TT_USHORT:
            case TT_SHORT:
            case TT_UINT:
            case TT_INT:
            case TT_WCHAR:
            case TT_ULONG:
            case TT_LONG:
            case TT_FLOAT:
            case TT_DOUBLE:
            case TT_BOOL:
            case TT_ID: {
              prev(unparsed, &next2);
              Node *argType = parseType(entry, unparsed, env, NULL);
              if (argType == NULL) {
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                nodeFree(type);
                return NULL;
              }
              vectorInsert(argTypes, argType);

              Token id;
              next(unparsed, &id);
              if (id.type == TT_ID)
                // has an identifier - ignore this
                tokenUninit(&id);
              else
                prev(unparsed, &id);

              Token next3;
              next(unparsed, &next3);
              switch (next3.type) {
                case TT_COMMA: {
                  // more to follow
                  break;
                }
                case TT_RPAREN: {
                  // done this one
                  doneArgs = true;
                  break;
                }
                default: {
                  errorExpectedString(entry, "a comma or a right parenthesis",
                                      &next3);

                  prev(unparsed, &next3);

                  nodeVectorFree(argTypes);
                  nodeFree(type);
                  return NULL;
                }
              }
              break;
            }
            default: {
              errorExpectedString(entry, "a type", &next2);

              prev(unparsed, &next2);

              nodeVectorFree(argTypes);
              nodeFree(type);
              return NULL;
            }
          }
        }

        type = funPtrTypeNodeCreate(type, argTypes, argNames);
        break;
      }
      default: {
        prev(unparsed, &next1);
        return type;
      }
    }
  }
}

/**
 * parses a field or option declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseFieldOrOptionDecl(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Token *start) {
  prev(unparsed, start);
  Node *type = parseType(entry, unparsed, env, NULL);
  if (type == NULL) {
    return NULL;
  }

  Vector *names = vectorCreate();
  bool done = false;
  while (!done) {
    Token id;
    next(unparsed, &id);
    if (id.type != TT_ID) {
      errorExpectedToken(entry, TT_ID, &id);

      prev(unparsed, &id);

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
    }

    vectorInsert(names, idNodeCreate(&id));

    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_SEMI: {
        // end of the names
        done = true;
        break;
      }
      case TT_COMMA: {
        // comma between names - do nothing
        break;
      }
      default: {
        errorExpectedString(entry, "a semicolon or a comma", &peek);

        prev(unparsed, &peek);

        nodeFree(type);
        nodeVectorFree(names);
        return NULL;
      }
    }
  }

  if (names->size == 0) {
    nodeFree(type);
    nodeVectorFree(names);
    return NULL;
  }

  return varDeclNodeCreate(type, names);
}

static Node *parseExpression(FileListEntry *entry, Node *unparsed,
                             Environment *env, Node *start);

static Node *parseAssignmentExpression(FileListEntry *entry, Node *unparsed,
                                       Environment *env, Node *start);

/**
 * parses a primary expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parsePrimaryExpression(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Node *start) {
  if (start == NULL) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_ID: {
        prev(unparsed, &peek);
        Node *n = parseAnyId(entry, unparsed);
        SymbolTableEntry *stabEntry = environmentLookup(env, n, false);
        if (stabEntry == NULL) {
          return NULL;
        } else if (stabEntry->kind != SK_ENUMCONST &&
                   stabEntry->kind != SK_FUNCTION &&
                   stabEntry->kind != SK_VARIABLE) {
          fprintf(stderr, "%s:%zu:%zu: error: cannot use a type as a variable",
                  entry->inputFilename, n->line, n->character);
          fprintf(stderr, "%s:%zu:%zu: note: declared here",
                  stabEntry->file->inputFilename, stabEntry->line,
                  stabEntry->character);
          entry->errored = true;
        } else {
          if (n->type == NT_ID) {
            n->data.id.entry = stabEntry;
          } else {
            n->data.scopedId.entry = stabEntry;
          }
        }
        return n;
      }
      case TT_LIT_STRING:
      case TT_LIT_WSTRING:
      case TT_LIT_CHAR:
      case TT_LIT_WCHAR:
      case TT_LIT_INT_0:
      case TT_LIT_INT_B:
      case TT_LIT_INT_O:
      case TT_LIT_INT_D:
      case TT_LIT_INT_H:
      case TT_LIT_DOUBLE:
      case TT_LIT_FLOAT:
      case TT_TRUE:
      case TT_FALSE:
      case TT_NULL:
      case TT_LSQUARE:
      case TT_BAD_CHAR:
      case TT_BAD_BIN:
      case TT_BAD_HEX:
      case TT_BAD_STRING: {
        prev(unparsed, &peek);
        return parseLiteral(entry, unparsed, env);
      }
      case TT_CAST: {
        Token langle;
        next(unparsed, &langle);
        if (langle.type != TT_LANGLE) {
          errorExpectedToken(entry, TT_LANGLE, &langle);

          prev(unparsed, &langle);
          return NULL;
        }

        Node *type = parseType(entry, unparsed, env, NULL);
        if (type == NULL) {
          return NULL;
        }

        Token rangle;
        next(unparsed, &rangle);
        if (rangle.type != TT_RANGLE) {
          errorExpectedToken(entry, TT_RANGLE, &rangle);

          prev(unparsed, &rangle);

          nodeFree(type);
          return NULL;
        }

        Token lparen;
        next(unparsed, &lparen);
        if (lparen.type != TT_LPAREN) {
          errorExpectedToken(entry, TT_LPAREN, &lparen);

          prev(unparsed, &lparen);

          nodeFree(type);
          return NULL;
        }

        Node *target = parseExpression(entry, unparsed, env, NULL);
        if (target == NULL) {
          nodeFree(type);
          return NULL;
        }

        Token rparen;
        next(unparsed, &rparen);
        if (rparen.type != TT_RPAREN) {
          errorExpectedToken(entry, TT_RPAREN, &rparen);

          prev(unparsed, &rparen);

          nodeFree(target);
          nodeFree(type);
          return NULL;
        }

        Type *parsedType = nodeToType(type, env);
        if (parsedType == NULL) {
          nodeFree(target);
          nodeFree(type);
          return NULL;
        }

        return castExpNodeCreate(&peek, type, parsedType, target);
      }
      case TT_SIZEOF: {
        Token lparen;
        next(unparsed, &lparen);
        if (lparen.type != TT_LPAREN) {
          errorExpectedToken(entry, TT_LPAREN, &lparen);
          return NULL;
        }

        Token sizeofPeek;
        next(unparsed, &sizeofPeek);
        switch (sizeofPeek.type) {
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
            // unambiguously a type
            prev(unparsed, &sizeofPeek);
            Node *target = parseType(entry, unparsed, env, NULL);
            if (target == NULL) {
              return NULL;
            }

            Token rparen;
            next(unparsed, &rparen);
            if (rparen.type != TT_RPAREN) {
              errorExpectedToken(entry, TT_RPAREN, &lparen);

              prev(unparsed, &rparen);

              nodeFree(target);
              return NULL;
            }

            return prefixUnOpExpNodeCreate(UO_SIZEOFTYPE, &peek, target);
          }
          case TT_ID: {
            // maybe a type, maybe an expression - disambiguate

            prev(unparsed, &peek);
            Node *idNode = parseAnyId(entry, unparsed);

            SymbolTableEntry *symbolEntry =
                environmentLookup(env, idNode, false);
            if (symbolEntry == NULL) {
              nodeFree(idNode);
              return NULL;
            }

            switch (symbolEntry->kind) {
              case SK_VARIABLE:
              case SK_FUNCTION:
              case SK_ENUMCONST: {
                Node *target = parseExpression(entry, unparsed, env, idNode);
                if (target == NULL) {
                  return NULL;
                }

                Token rparen;
                next(unparsed, &rparen);
                if (rparen.type != TT_RPAREN) {
                  errorExpectedToken(entry, TT_RPAREN, &lparen);

                  prev(unparsed, &rparen);

                  nodeFree(target);
                  return NULL;
                }

                return prefixUnOpExpNodeCreate(UO_SIZEOFEXP, &peek, target);
              }
              case SK_OPAQUE:
              case SK_STRUCT:
              case SK_UNION:
              case SK_ENUM:
              case SK_TYPEDEF: {
                prev(unparsed, &sizeofPeek);
                Node *target = parseType(entry, unparsed, env, idNode);
                if (target == NULL) {
                  return NULL;
                }

                Token rparen;
                next(unparsed, &rparen);
                if (rparen.type != TT_RPAREN) {
                  errorExpectedToken(entry, TT_RPAREN, &lparen);

                  prev(unparsed, &rparen);

                  nodeFree(target);
                  return NULL;
                }

                return prefixUnOpExpNodeCreate(UO_SIZEOFTYPE, &peek, target);
              }
              default: {
                error(__FILE__, __LINE__,
                      "invalid SymbolKind enum encountered");
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
          case TT_LIT_INT_0:
          case TT_LIT_INT_B:
          case TT_BAD_BIN:
          case TT_LIT_INT_O:
          case TT_LIT_INT_D:
          case TT_LIT_INT_H:
          case TT_BAD_HEX:
          case TT_LIT_CHAR:
          case TT_BAD_CHAR:
          case TT_LIT_WCHAR:
          case TT_LIT_FLOAT:
          case TT_LIT_DOUBLE:
          case TT_LIT_STRING:
          case TT_BAD_STRING:
          case TT_LIT_WSTRING:
          case TT_TRUE:
          case TT_FALSE:
          case TT_NULL:
          case TT_LSQUARE: {
            // unambiguously an expression
            prev(unparsed, &sizeofPeek);
            Node *target = parseExpression(entry, unparsed, env, NULL);
            if (target == NULL) {
              return NULL;
            }

            Token rparen;
            next(unparsed, &rparen);
            if (rparen.type != TT_RPAREN) {
              errorExpectedToken(entry, TT_RPAREN, &lparen);

              prev(unparsed, &rparen);

              nodeFree(target);
              return NULL;
            }

            return prefixUnOpExpNodeCreate(UO_SIZEOFEXP, &peek, target);
          }
          default: {
            // unexpected token
            errorExpectedString(entry, "a type or an expression", &sizeofPeek);

            prev(unparsed, &sizeofPeek);
            return NULL;
          }
        }
      }
      case TT_LPAREN: {
        Node *exp = parseExpression(entry, unparsed, env, NULL);
        if (exp == NULL) {
          return NULL;
        }

        Token rparen;
        next(unparsed, &rparen);
        if (rparen.type != TT_RPAREN) {
          errorExpectedToken(entry, TT_RPAREN, &rparen);

          nodeFree(exp);
          return NULL;
        }

        return prefixUnOpExpNodeCreate(UO_PARENS, &peek, exp);
      }
      default: {
        errorExpectedString(entry, "a primary expression", &peek);

        prev(unparsed, &peek);
        return NULL;
      }
    }
  } else {
    if (start->type == NT_ID) {
      start->data.id.entry = environmentLookup(env, start, false);
    } else {
      start->data.scopedId.entry = environmentLookup(env, start, false);
    }
    return start;
  }
}

/**
 * parses a postfix expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parsePostfixExpression(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Node *start) {
  Node *exp = parsePrimaryExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_DOT:
      case TT_ARROW: {
        Node *id = parseId(entry, unparsed);
        if (id == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(accessorTokenToBinop(op.type), exp, id);
        break;
      }
      case TT_LPAREN: {
        Vector *arguments = vectorCreate();

        Token peek;
        next(unparsed, &peek);
        if (peek.type == TT_RPAREN) {
          exp = funCallExpNodeCreate(exp, arguments);
        } else {
          prev(unparsed, &peek);
          Node *arg = parseAssignmentExpression(entry, unparsed, env, NULL);
          if (arg == NULL) {
            nodeVectorFree(arguments);
            nodeFree(exp);
            return NULL;
          }
          vectorInsert(arguments, arg);

          bool done = false;
          while (!done) {
            next(unparsed, &peek);
            switch (peek.type) {
              case TT_RPAREN: {
                exp = funCallExpNodeCreate(exp, arguments);
                done = true;
                break;
              }
              case TT_COMMA: {
                Node *arg =
                    parseAssignmentExpression(entry, unparsed, env, NULL);
                if (arg == NULL) {
                  nodeVectorFree(arguments);
                  nodeFree(exp);
                  return NULL;
                }

                vectorInsert(arguments, arg);
                break;
              }
              default: {
                errorExpectedString(entry, "a comma or a right-parenthesis",
                                    &peek);

                prev(unparsed, &peek);

                nodeVectorFree(arguments);
                nodeFree(exp);
                return NULL;
              }
            }
          }
        }
        break;
      }
      case TT_LSQUARE: {
        Node *index = parseExpression(entry, unparsed, env, NULL);
        if (index == NULL) {
          nodeFree(exp);
          return NULL;
        }

        Token rsquare;
        next(unparsed, &rsquare);
        if (rsquare.type != TT_RSQUARE) {
          errorExpectedToken(entry, TT_RSQUARE, &rsquare);

          nodeFree(index);
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(BO_ARRAY, exp, index);
        break;
      }
      case TT_INC:
      case TT_DEC:
      case TT_NEGASSIGN:
      case TT_LNOTASSIGN:
      case TT_BITNOTASSIGN: {
        exp = postfixUnOpExpNodeCreate(postfixTokenToUnop(op.type), exp);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a prefix expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parsePrefixExpression(FileListEntry *entry, Node *unparsed,
                                   Environment *env, Node *start) {
  if (start == NULL) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_STAR:
      case TT_AMP:
      case TT_INC:
      case TT_DEC:
      case TT_MINUS:
      case TT_BANG:
      case TT_TILDE: {
        Node *target = parsePrefixExpression(entry, unparsed, env, NULL);
        if (target == NULL) {
          return NULL;
        }

        return prefixUnOpExpNodeCreate(prefixTokenToUnop(peek.type), &peek,
                                       target);
      }
      default: {
        prev(unparsed, &peek);
        return parsePostfixExpression(entry, unparsed, env, NULL);
      }
    }
  } else {
    return parsePostfixExpression(entry, unparsed, env, start);
  }
}

/**
 * parses a multiplication expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseMultiplicationExpression(FileListEntry *entry, Node *unparsed,
                                           Environment *env, Node *start) {
  Node *exp = parsePrefixExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_STAR:
      case TT_SLASH:
      case TT_PERCENT: {
        Node *rhs = parsePrefixExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(multiplicationTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses an addition expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseAdditionExpression(FileListEntry *entry, Node *unparsed,
                                     Environment *env, Node *start) {
  Node *exp = parseMultiplicationExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_PLUS:
      case TT_MINUS: {
        Node *rhs = parseMultiplicationExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(additionTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a shift expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseShiftExpression(FileListEntry *entry, Node *unparsed,
                                  Environment *env, Node *start) {
  Node *exp = parseAdditionExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_LSHIFT:
      case TT_ARSHIFT:
      case TT_LRSHIFT: {
        Node *rhs = parseAdditionExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(shiftTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a spaceship expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseSpaceshipExpression(FileListEntry *entry, Node *unparsed,
                                      Environment *env, Node *start) {
  Node *exp = parseShiftExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    if (op.type != TT_SPACESHIP) {
      prev(unparsed, &op);
      return exp;
    }

    Node *rhs = parseShiftExpression(entry, unparsed, env, NULL);
    if (rhs == NULL) {
      nodeFree(exp);
      return NULL;
    }

    exp = binOpExpNodeCreate(BO_SPACESHIP, exp, rhs);
  }
}

/**
 * parses a comparison expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseComparisonExpression(FileListEntry *entry, Node *unparsed,
                                       Environment *env, Node *start) {
  Node *exp = parseSpaceshipExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_LANGLE:
      case TT_RANGLE:
      case TT_LTEQ:
      case TT_GTEQ: {
        Node *rhs = parseSpaceshipExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(comparisonTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses an equality expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseEqualityExpression(FileListEntry *entry, Node *unparsed,
                                     Environment *env, Node *start) {
  Node *exp = parseComparisonExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_EQ:
      case TT_NEQ: {
        Node *rhs = parseComparisonExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(equalityTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a bitwise expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseBitwiseExpression(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Node *start) {
  Node *exp = parseEqualityExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_AMP:
      case TT_BAR:
      case TT_CARET: {
        Node *rhs = parseEqualityExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(bitwiseTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a logical expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseLogicalExpression(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Node *start) {
  Node *exp = parseBitwiseExpression(entry, unparsed, env, start);
  if (exp == NULL) {
    return NULL;
  }

  while (true) {
    Token op;
    next(unparsed, &op);
    switch (op.type) {
      case TT_LAND:
      case TT_LOR: {
        Node *rhs = parseLogicalExpression(entry, unparsed, env, NULL);
        if (rhs == NULL) {
          nodeFree(exp);
          return NULL;
        }

        exp = binOpExpNodeCreate(logicalTokenToBinop(op.type), exp, rhs);
        break;
      }
      default: {
        prev(unparsed, &op);
        return exp;
      }
    }
  }
}

/**
 * parses a ternary expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseTernaryExpression(FileListEntry *entry, Node *unparsed,
                                    Environment *env, Node *start) {
  Node *predicate = parseLogicalExpression(entry, unparsed, env, start);
  if (predicate == NULL) {
    return NULL;
  }

  Token question;
  next(unparsed, &question);
  if (question.type != TT_QUESTION) {
    prev(unparsed, &question);
    return predicate;
  }

  Node *consequent = parseExpression(entry, unparsed, env, NULL);
  if (consequent == NULL) {
    nodeFree(predicate);
    return NULL;
  }

  Token colon;
  next(unparsed, &colon);
  if (colon.type != TT_COLON) {
    errorExpectedToken(entry, TT_COLON, &colon);

    prev(unparsed, &colon);

    nodeFree(consequent);
    nodeFree(predicate);
    return NULL;
  }

  Node *alternative = parseTernaryExpression(entry, unparsed, env, NULL);
  if (alternative == NULL) {
    nodeFree(consequent);
    nodeFree(predicate);
    return NULL;
  }

  return ternaryExpNodeCreate(predicate, consequent, alternative);
}

/**
 * parses an assignment expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseAssignmentExpression(FileListEntry *entry, Node *unparsed,
                                       Environment *env, Node *start) {
  Node *lhs = parseTernaryExpression(entry, unparsed, env, start);
  if (lhs == NULL) {
    return NULL;
  }

  Token op;
  next(unparsed, &op);
  switch (op.type) {
    case TT_ASSIGN:
    case TT_MULASSIGN:
    case TT_DIVASSIGN:
    case TT_MODASSIGN:
    case TT_ADDASSIGN:
    case TT_SUBASSIGN:
    case TT_LSHIFTASSIGN:
    case TT_ARSHIFTASSIGN:
    case TT_LRSHIFTASSIGN:
    case TT_BITANDASSIGN:
    case TT_BITXORASSIGN:
    case TT_BITORASSIGN:
    case TT_LANDASSIGN:
    case TT_LORASSIGN: {
      Node *rhs = parseAssignmentExpression(entry, unparsed, env, NULL);
      if (rhs == NULL) {
        nodeFree(lhs);
        return NULL;
      }

      return binOpExpNodeCreate(assignmentTokenToBinop(op.type), lhs, rhs);
    }
    default: {
      prev(unparsed, &op);
      return lhs;
    }
  }
}

/**
 * parses an expression
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first id in expression, or null if none provided
 *
 * @returns node or null on error
 */
static Node *parseExpression(FileListEntry *entry, Node *unparsed,
                             Environment *env, Node *start) {
  Node *lhs = parseAssignmentExpression(entry, unparsed, env, start);
  if (lhs == NULL) {
    return NULL;
  }

  Token comma;
  next(unparsed, &comma);
  if (comma.type != TT_COMMA) {
    prev(unparsed, &comma);
    return lhs;
  }

  Node *rhs = parseExpression(entry, unparsed, env, NULL);
  if (rhs == NULL) {
    nodeFree(lhs);
    return NULL;
  }

  return binOpExpNodeCreate(BO_SEQ, lhs, rhs);
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

  Node *predicate = parseExpression(entry, unparsed, env, NULL);
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
  if (elseKwd.type != TT_ELSE) {
    prev(unparsed, &elseKwd);
    return ifStmtNodeCreate(start, predicate, consequent, consequentStab, NULL,
                            NULL);
  }

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

  Node *condition = parseExpression(entry, unparsed, env, NULL);
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

  Node *condition = parseExpression(entry, unparsed, env, NULL);
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
    case TT_LIT_INT_0:
    case TT_LIT_INT_B:
    case TT_BAD_BIN:
    case TT_LIT_INT_O:
    case TT_LIT_INT_D:
    case TT_LIT_INT_H:
    case TT_BAD_HEX:
    case TT_LIT_CHAR:
    case TT_BAD_CHAR:
    case TT_LIT_WCHAR:
    case TT_LIT_FLOAT:
    case TT_LIT_DOUBLE:
    case TT_LIT_STRING:
    case TT_BAD_STRING:
    case TT_LIT_WSTRING:
    case TT_TRUE:
    case TT_FALSE:
    case TT_NULL:
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

  Node *condition = parseExpression(entry, unparsed, env, NULL);
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
  // could technically use a peek function here, but it's only this one case
  next(unparsed, &peek);
  Node *increment = NULL;
  if (peek.type != TT_RPAREN) {
    // increment isn't null
    prev(unparsed, &peek);
    increment = parseExpression(entry, unparsed, env, NULL);
    if (increment == NULL) {
      panicStmt(unparsed);

      nodeFree(condition);
      nodeFree(initializer);
      stabFree(environmentPop(env));
      return NULL;
    }
  } else {
    prev(unparsed, &peek);
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

  Node *condition = parseExpression(entry, unparsed, env, NULL);
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
    Node *value = parseExpression(entry, unparsed, env, NULL);

    Token semi;
    next(unparsed, &semi);
    if (semi.type != TT_SEMI) {
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
 * @param start first ID in type (or null if none)
 *
 * @returns node or null on error
 */
static Node *parseVarDefnStmt(FileListEntry *entry, Node *unparsed,
                              Environment *env, Node *start) {
  Node *typeNode = parseType(entry, unparsed, env, start);
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
      case TT_ASSIGN: {
        // has initializer
        Node *initializer =
            parseAssignmentExpression(entry, unparsed, env, NULL);
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

  for (size_t idx = 0; idx < names->size; ++idx) {
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
 * @param start first ID in expression or NULL if none exists
 *
 * @returns node or null on error
 */
static Node *parseExpressionStmt(FileListEntry *entry, Node *unparsed,
                                 Environment *env, Node *start) {
  Node *expression = parseExpression(entry, unparsed, env, start);
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
 * parses an enum decl (within a function)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseEnumDecl(FileListEntry *entry, Node *unparsed,
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

  Vector *constantNames = vectorCreate();
  Vector *constantValues = vectorCreate();
  bool done = false;
  while (!done) {
    Token peek;
    next(unparsed, &peek);
    switch (peek.type) {
      case TT_ID: {
        // start of a constant line
        vectorInsert(constantNames, idNodeCreate(&peek));

        next(unparsed, &peek);
        switch (peek.type) {
          case TT_ASSIGN: {
            // has an extended int literal
            Node *literal = parseExtendedIntLiteral(entry, unparsed, env);
            if (literal == NULL) {
              panicEnum(unparsed);

              vectorInsert(constantValues, NULL);
              continue;
            }
            vectorInsert(constantValues, literal);

            next(unparsed, &peek);
            switch (peek.type) {
              case TT_COMMA: {
                // end of this constant
                break;
              }
              case TT_RBRACE: {
                // end of the whole enum
                done = true;
                break;
              }
              default: {
                errorExpectedString(entry, "a comma or a right brace", &peek);

                prev(unparsed, &peek);
                panicEnum(unparsed);
                continue;
              }
            }

            break;
          }
          case TT_COMMA: {
            // end of this constant
            vectorInsert(constantValues, NULL);
            break;
          }
          case TT_RBRACE: {
            // end of the whole enum
            vectorInsert(constantValues, NULL);
            done = true;
            break;
          }
          default: {
            errorExpectedString(
                entry, "a comma, an equals sign, or a right brace", &peek);

            prev(unparsed, &peek);
            panicEnum(unparsed);
            continue;
          }
        }
        break;
      }
      case TT_RBRACE: {
        done = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or an enumeration constant",
                            &peek);

        prev(unparsed, &peek);
        panicStmt(unparsed);

        nodeFree(name);
        nodeVectorFree(constantNames);
        nodeVectorFree(constantValues);
        return NULL;
      }
    }
  }

  if (constantNames->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one enumeration constant in "
            "a enumeration declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    panicStmt(unparsed);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  Token semicolon;
  next(unparsed, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    prev(unparsed, &semicolon);
    panicStmt(unparsed);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  Node *body = enumDeclNodeCreate(start, name, constantNames, constantValues);
  SymbolTableEntry *existing =
      hashMapGet(environmentTop(env), name->data.id.id);
  if (existing != NULL) {
    if (existing->kind == SK_OPAQUE) {
      // overwrite the opaque
      name->data.id.entry = existing;
      existing->kind = SK_ENUM;
      vectorInit(&existing->data.enumType.constantNames);
      vectorInit(&existing->data.enumType.constantValues);
      finishEnumStab(entry, body, name->data.id.entry, env);
    } else {
      // whoops - this already exists! complain!
      errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                         existing->file, existing->line, existing->character);
    }
  } else {
    // create a new entry
    name->data.id.entry =
        enumStabEntryCreate(entry, start->line, start->character);
    hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
    finishEnumStab(entry, body, name->data.id.entry, env);
  }

  return body;
}

/**
 * parses a typedef decl (within a function)
 *
 * @param entry entry containing this node
 * @param unparsed unparsed node to read from
 * @param env environment to use
 * @param start first token
 *
 * @returns node or null on error
 */
static Node *parseTypedefDecl(FileListEntry *entry, Node *unparsed,
                              Environment *env, Token *start) {
  Node *originalType = parseType(entry, unparsed, env, NULL);
  if (originalType == NULL) {
    panicStmt(unparsed);

    return NULL;
  }

  Node *name = parseId(entry, unparsed);
  if (name == NULL) {
    panicStmt(unparsed);

    nodeFree(originalType);
    return NULL;
  }

  Token semicolon;
  next(unparsed, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    prev(unparsed, &semicolon);
    panicStmt(unparsed);

    nodeFree(originalType);
    nodeFree(name);
    return NULL;
  }

  Node *body = typedefDeclNodeCreate(start, originalType, name);
  SymbolTableEntry *existing =
      hashMapGet(environmentTop(env), name->data.id.id);
  if (existing != NULL) {
    if (existing->kind == SK_OPAQUE) {
      // overwrite the opaque
      name->data.id.entry = existing;
      existing->kind = SK_TYPEDEF;
      finishTypedefStab(entry, body, name->data.id.entry, env);
    } else {
      // whoops - this already exists! complain!
      errorRedeclaration(entry, name->line, name->character, name->data.id.id,
                         existing->file, existing->line, existing->character);
    }
  } else {
    // create a new entry
    name->data.id.entry =
        typedefStabEntryCreate(entry, start->line, start->character);
    hashMapPut(environmentTop(env), name->data.id.id, name->data.id.entry);
    finishTypedefStab(entry, body, name->data.id.entry, env);
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
      return parseVarDefnStmt(entry, unparsed, env, NULL);
    }
    case TT_ID: {
      // maybe varDefn, maybe expressionStmt - disambiguate

      // get the whole ID, if it's scoped
      prev(unparsed, &peek);
      Node *idNode = parseAnyId(entry, unparsed);

      SymbolTableEntry *symbolEntry = environmentLookup(env, idNode, false);
      if (symbolEntry == NULL) {
        nodeFree(idNode);
        panicStmt(unparsed);
        return NULL;
      }

      switch (symbolEntry->kind) {
        case SK_VARIABLE:
        case SK_FUNCTION:
        case SK_ENUMCONST: {
          return parseExpressionStmt(entry, unparsed, env, idNode);
        }
        case SK_OPAQUE:
        case SK_STRUCT:
        case SK_UNION:
        case SK_ENUM:
        case SK_TYPEDEF: {
          return parseVarDefnStmt(entry, unparsed, env, idNode);
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
    case TT_LSQUARE:
    case TT_LIT_INT_0:
    case TT_LIT_INT_B:
    case TT_BAD_BIN:
    case TT_LIT_INT_O:
    case TT_LIT_INT_D:
    case TT_LIT_INT_H:
    case TT_BAD_HEX:
    case TT_LIT_CHAR:
    case TT_BAD_CHAR:
    case TT_LIT_WCHAR:
    case TT_LIT_FLOAT:
    case TT_LIT_DOUBLE:
    case TT_LIT_STRING:
    case TT_BAD_STRING:
    case TT_LIT_WSTRING:
    case TT_TRUE:
    case TT_FALSE:
    case TT_NULL: {
      // unambiguously an expressionStmt
      prev(unparsed, &peek);
      return parseExpressionStmt(entry, unparsed, env, NULL);
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
      return parseEnumDecl(entry, unparsed, env, &peek);
    }
    case TT_TYPEDEF: {
      return parseTypedefDecl(entry, unparsed, env, &peek);
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
        HashMap *stab = body->data.funDefn.argStab;
        environmentPush(&env, stab);

        for (size_t argIdx = 0; argIdx < body->data.funDefn.argTypes->size;
             ++argIdx) {
          Node *argType = body->data.funDefn.argTypes->elements[argIdx];
          Node *argName = body->data.funDefn.argNames->elements[argIdx];
          SymbolTableEntry *stabEntry =
              variableStabEntryCreate(entry, argType->line, argType->character);
          stabEntry->data.variable.type = nodeToType(argType, &env);
          if (stabEntry->data.variable.type == NULL) entry->errored = true;
          SymbolTableEntry *existing = hashMapGet(stab, argName->data.id.id);
          if (existing != NULL) {
            // already exists - complain!
            errorRedeclaration(entry, argName->line, argName->character,
                               argName->data.id.id, existing->file,
                               existing->line, existing->character);
          } else {
            hashMapPut(stab, argName->data.id.id, stabEntry);
          }
        }

        // parse and reference resolve body, replacing it in the original ast
        Node *unparsed = body->data.funDefn.body;
        body->data.funDefn.body = parseCompoundStmt(entry, unparsed, &env);
        nodeFree(unparsed);

        environmentPop(&env);
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