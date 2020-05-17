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

#include "parser/topLevel.h"

#include "util/conversions.h"

#include <stdio.h>
#include <stdlib.h>

// utility functions and data

/** array between token type (as int) and token name */
static char const *const TOKEN_NAMES[] = {
    "the end of file",
    "the keyword 'module'",
    "the keyword 'import'",
    "the keyword 'opaque'",
    "the keyword 'struct'",
    "the keyword 'union'",
    "the keyword 'enum'",
    "the keyword 'typedef'",
    "the keyword 'if'",
    "the keyword 'else'",
    "the keyword 'while'",
    "the keyword 'do'",
    "the keyword 'for'",
    "the keyword 'switch'",
    "the keyword 'case'",
    "the keyword 'default'",
    "the keyword 'break'",
    "the keyword 'continue'",
    "the keyword 'return'",
    "the keyword 'asm'",
    "the keyword 'cast'",
    "the keyword 'sizeof'",
    "the keyword 'true'",
    "the keyword 'false'",
    "the keyword 'null'",
    "the keyword 'void'",
    "the keyword 'ubyte'",
    "the keyword 'byte'",
    "the keyword 'char'",
    "the keyword 'ushort'",
    "the keyword 'short'",
    "the keyword 'uint'",
    "the keyword 'int'",
    "the keyword 'wchar'",
    "the keyword 'ulong'",
    "the keyword 'long'",
    "the keyword 'float'",
    "the keyword 'double'",
    "the keyword 'bool'",
    "the keyword 'const'",
    "the keyword 'volatile'",
    "a semicolon",
    "a comma",
    "a left parenthesis",
    "a right parenthesis",
    "a left square bracket",
    "a right square bracket",
    "a left brace",
    "a right brace",
    "a period",
    "a structure dereference operator",
    "an increment operator",
    "a decrement operator",
    "an asterisk",
    "an ampersand",
    "a plus sign",
    "a minus sign",
    "an exclaimation mark",
    "a tilde",
    "a compound negation-assignment operator",
    "a compound logical-not-assignment operator",
    "a compound bitwise-not-assignment operator",
    "a slash",
    "a percent sign",
    "a left shift operator",
    "an arithmetic-right-shift operator",
    "a logical-right-shift operator",
    "a three way comparison operator",
    "a left angle bracket",
    "a right angle bracket",
    "a less-than-or-equal-to operator",
    "a greater-than-or-equal-to operator",
    "an equal-to operator",
    "a not-equal-to operator",
    "a pipe",
    "a caret",
    "a logical-and operator",
    "a logical-or operator",
    "a question mark",
    "a colon",
    "an equals sign",
    "a compound multiplication-assignment operator",
    "a compound division-assignment operator",
    "a compound modulo-assignment operator",
    "a compound addition-assignment operator",
    "a compound subtraction-assignment operator",
    "a compound left-shift-assignment operator",
    "a compound arithmetic-right-shift-assignment operator",
    "a compound logical-right-shift-assignment operator",
    "a compound bitwise-and-assignment operator",
    "a compound bitwise-exclusive-or-assignment operator",
    "a compound bitwise-or-assignment-operator",
    "a compound logical-and-assignment-operator",
    "a compound logical-or-assignment-operator",
    "a scope-resolution operator",
    "an identifier",
    "a string literal",
    "a wide string literal",
    "a character literal",
    "a wide character literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "a floating-point literal",
    "a floating-point literal",
    "a string literal",
    "a character literal",
    "an integer literal",
    "an integer literal",
};

/**
 * prints an error complaining about a wrong token, specifying what token it
 * should have been
 *
 * @param entry entry to attribute the error to
 * @param expected TokenType expected
 * @param actual actual token
 */
static void errorExpectedToken(FileListEntry *entry, TokenType expected,
                               Token *actual) {
  fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
          entry->inputFilename, actual->line, actual->character,
          TOKEN_NAMES[expected], TOKEN_NAMES[actual->type]);
  entry->errored = true;
}

/**
 * prints an error complaining about a wrong token, specifying what it should
 * have been, as a string
 *
 * @param entry entry to attribute the error to
 * @param expected string describing the expected tokens
 * @param actual actual token
 */
static void errorExpectedString(FileListEntry *entry, char const *expected,
                                Token *actual) {
  fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
          entry->inputFilename, actual->line, actual->character, expected,
          TOKEN_NAMES[actual->type]);
  entry->errored = true;
}

/**
 * prints an error complaining about a too-large integral value
 *
 * @param entry entry to attribute the error to
 * @param token the bad token
 */
static void errorIntOverflow(FileListEntry *entry, Token *token) {
  fprintf(stderr, "%s:%zu:%zu: error: integer constant is too large\n",
          entry->inputFilename, token->line, token->character);
  entry->errored = true;
}

// calling conventions:
// a context-ignorant parser shall unLex as much as it can if an error happens
// (usually one token)
// a context-aware parser shall unLex as much as it can before panicking.
//
// when a failure happens, the handler always has the same patterns:
//  - error message
//  - unLex and/or panic
//  - cleanup
//  - return NULL

// panics

/**
 * reads tokens until a top-level form boundary
 *
 * semicolons are consumed, EOFs, and the start of a top level form are left
 *
 * @param entry entry to lex from
 */
static void panicTopLevel(FileListEntry *entry) {
  while (true) {
    Token token;
    lex(entry, &token);
    switch (token.type) {
      case TT_SEMI: {
        return;
      }
      case TT_MODULE:
      case TT_IMPORT:
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
        unLex(entry, &token);
        return;
      }
      default: {
        tokenUninit(&token);
        break;
      }
    }
  }
}

// parsing

// context-ignorant parsers
// these don't do error recovery

// common elements

/**
 * parses an ID or scoped ID
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAnyId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);
  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
    unLex(entry, &idToken);
    return NULL;
  }

  // maybe it's a scoped id?
  Token scope;
  lex(entry, &scope);
  if (scope.type != TT_SCOPE) {
    // not a scoped id
    unLex(entry, &scope);
    return idNodeCreate(&idToken);
  } else {
    // scoped id - saw scope
    Vector *components = vectorCreate();

    vectorInsert(components, idNodeCreate(&idToken));
    while (true) {
      // expect an id, add it to the node
      lex(entry, &idToken);
      if (idToken.type != TT_ID) {
        errorExpectedToken(entry, TT_ID, &idToken);

        unLex(entry, &idToken);

        nodeVectorFree(components);
        return NULL;
      } else {
        vectorInsert(components, idNodeCreate(&idToken));
      }

      // if there's a scope, keep going, else return
      lex(entry, &scope);
      if (scope.type != TT_SCOPE) {
        unLex(entry, &scope);
        return scopedIdNodeCreate(components);
      }
    }
  }
}

/**
 * parses a scoped ID
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseScopedId(FileListEntry *entry) {
  Vector *components = vectorCreate();
  while (true) {
    Token peek;
    // expect an id, add it to the node
    lex(entry, &peek);
    if (peek.type != TT_ID) {
      errorExpectedToken(entry, TT_ID, &peek);

      unLex(entry, &peek);

      nodeVectorFree(components);
      return NULL;
    } else {
      vectorInsert(components, idNodeCreate(&peek));
    }

    // if there's a scope, keep going, else return
    lex(entry, &peek);
    if (peek.type != TT_SCOPE) {
      unLex(entry, &peek);

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
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
    unLex(entry, &idToken);
    return NULL;
  }

  return idNodeCreate(&idToken);
}

/**
 * parses an extended int literal
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseExtendedIntLiteral(FileListEntry *entry) {
  Token peek;
  lex(entry, &peek);
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
      Node *n = sizedIntegerLiteralNodeCreate( &peek, sign, magnitude);
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
      Node *n = sizedIntegerLiteralNodeCreate( &peek, sign, magnitude);
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
      Node *n = sizedIntegerLiteralNodeCreate( &peek, sign, magnitude);
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
      Node *n = sizedIntegerLiteralNodeCreate( &peek, sign, magnitude);
      if (n == NULL) errorIntOverflow(entry, &peek);
      return n;
    }
    case TT_BAD_CHAR:
    case TT_BAD_BIN:
    case TT_BAD_HEX: {
      return NULL;
    }
    case TT_ID: {
      unLex(entry, &peek);
      return parseScopedId(entry);
    }
    default: {
      errorExpectedString(entry, "an exended integer literal", &peek);

      unLex(entry, &peek);

      return NULL;
    }
  }
}

static Node *parseLiteral(FileListEntry *entry);
/**
 * parses an aggregate initializer
 *
 * @param entry entry to lex from
 * @param start first token in aggregate init
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAggregateInitializer(FileListEntry *entry, Token *start) {
  Vector *literals = vectorCreate();
  while (true) {
    Token peek;
    lex(entry, &peek);
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
      case TT_BAD_STRING:
      case TT_BAD_CHAR:
      case TT_BAD_BIN:
      case TT_BAD_HEX:
      case TT_ID:
      case TT_LSQUARE: {
        // this is the start of a field
        unLex(entry, &peek);
        Node *literal = parseLiteral(entry);
        if (literal == NULL) {
          nodeVectorFree(literals);
          return NULL;
        }
        vectorInsert(literals, literal);
        break;
      }
      case TT_RSQUARE: {
        // end of the init
        Node *n = literalNodeCreate(LT_AGGREGATEINIT, start);
        n->data.literal.value.aggregateInitVal = literals;
        return n;
      }
      default: {
        errorExpectedString(entry, "a right square bracket or a literal",
                            &peek);

        unLex(entry, &peek);

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
static Node *parseLiteral(FileListEntry *entry) {
  Token peek;
  lex(entry, &peek);
  switch (peek.type) {
    case TT_LIT_CHAR:
    case TT_LIT_WCHAR:
    case TT_LIT_INT_B:
    case TT_LIT_INT_O:
    case TT_LIT_INT_0:
    case TT_LIT_INT_D:
    case TT_LIT_INT_H:
    case TT_BAD_CHAR:
    case TT_BAD_BIN:
    case TT_BAD_HEX:
    case TT_ID: {
      unLex(entry, &peek);
      return parseExtendedIntLiteral(entry);
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
      n->data.literal.value.doubleBits = bits;
      return n;
    }
    case TT_LIT_FLOAT: {
      uint32_t bits = floatStringToBits(peek.string);
      Node *n = literalNodeCreate(LT_FLOAT, &peek);
      n->data.literal.value.floatBits = bits;
      return n;
    }
    case TT_BAD_STRING: {
      return NULL;
    }
    case TT_LSQUARE: {
      // aggregate initializer
      return parseAggregateInitializer(entry, &peek);
    }
    default: {
      errorExpectedString(entry, "a literal", &peek);

      unLex(entry, &peek);

      return NULL;
    }
  }
}

/**
 * parses a type
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseType(FileListEntry *entry) {
  Node *type;

  Token start;
  lex(entry, &start);
  switch (start.type) {
    case TT_VOID: {
      type = keywordTypeNodeCreate(TK_VOID, &start);
      break;
    }
    case TT_UBYTE: {
      type = keywordTypeNodeCreate(TK_UBYTE, &start);
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
      unLex(entry, &start);
      type = parseAnyId(entry);
      if (type == NULL) return NULL;
      break;
    }
    default: {
      errorExpectedString(entry, "a type", &start);

      unLex(entry, &start);

      return NULL;
    }
  }

  while (true) {
    Token next1;
    lex(entry, &next1);
    switch (next1.type) {
      case TT_CONST: {
        type = modifiedTypeNodeCreate(TM_CONST, type);
        break;
      }
      case TT_VOLATILE: {
        type = modifiedTypeNodeCreate(TM_VOLATILE, type);
        break;
      }
      case TT_LSQUARE: {
        Node *size = parseExtendedIntLiteral(entry);
        if (size == NULL) {
          nodeFree(type);

          return NULL;
        }

        Token rsquare;
        lex(entry, &rsquare);
        if (rsquare.type != TT_RSQUARE) {
          errorExpectedToken(entry, TT_RSQUARE, &rsquare);

          unLex(entry, &rsquare);

          nodeFree(type);
          nodeFree(size);
          return NULL;
        }

        type = arrayTypeNodeCreate(type, size);
        return NULL;
      }
      case TT_STAR: {
        type = modifiedTypeNodeCreate(TM_POINTER, type);
        break;
      }
      case TT_LPAREN: {
        Vector *argTypes = vectorCreate();
        Vector *argNames = vectorCreate();
        bool doneArgs = false;

        Token peek;
        lex(entry, &peek);
        if (peek.type == TT_RPAREN)
          doneArgs = true;
        else
          unLex(entry, &peek);
        while (!doneArgs) {
          Token next2;
          lex(entry, &next2);
          switch (next2.type) {
            case TT_VOID:
            case TT_UBYTE:
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
              unLex(entry, &start);
              Node *argType = parseType(entry);
              if (argType == NULL) {
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                nodeFree(type);
                return NULL;
              }
              vectorInsert(argTypes, argType);

              Token id;
              lex(entry, &id);
              if (id.type == TT_ID)
                // has an identifier - ignore this
                tokenUninit(&id);
              else
                unLex(entry, &id);

              Token next3;
              lex(entry, &next3);
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

                  unLex(entry, &next3);

                  nodeVectorFree(argTypes);
                  nodeFree(type);
                  return NULL;
                }
              }
              break;
            }
            default: {
              errorExpectedString(entry, "a type", &next2);

              unLex(entry, &next2);

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
        unLex(entry, &next1);
        return type;
      }
    }
  }
}

// expressions

// context-aware parsers
// these do error recovery

// top level stuff

/**
 * parses a module line
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseModule(FileListEntry *entry) {
  Token moduleKeyword;
  lex(entry, &moduleKeyword);
  if (moduleKeyword.type != TT_MODULE) {
    errorExpectedToken(entry, TT_MODULE, &moduleKeyword);

    unLex(entry, &moduleKeyword);
    panicTopLevel(entry);

    return NULL;
  }

  Node *id = parseAnyId(entry);
  if (id == NULL) {
    panicTopLevel(entry);

    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(id);
    return NULL;
  }

  return moduleNodeCreate(&moduleKeyword, id);
}

/**
 * parses a single import
 *
 * @param entry entry to lex from
 * @param importKeyword import keyword
 * @returns import AST node or NULL if errored
 */
static Node *parseImport(FileListEntry *entry, Token *importKeyword) {
  Node *id = parseAnyId(entry);
  if (id == NULL) {
    panicTopLevel(entry);

    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(id);
    return NULL;
  }

  return importNodeCreate(importKeyword, id);
}

/**
 * parses a set of imports
 *
 * never fatally errors
 *
 * @param entry entry to lex from
 * @returns list of imports
 */
static Vector *parseImports(FileListEntry *entry) {
  Vector *imports = vectorCreate();
  while (true) {
    Token importKeyword;
    lex(entry, &importKeyword);

    if (importKeyword.type != TT_IMPORT) {
      // it's the end of the imports
      unLex(entry, &importKeyword);
      return imports;
    } else {
      Node *import = parseImport(entry, &importKeyword);
      if (import != NULL) vectorInsert(imports, import);
    }
  }
}

/**
 * finishes parsing a variable declaration
 *
 * @param entry entry to lex from
 * @param type type of the var decl
 * @param names vector of names, partially filled
 */
static Node *finishVarDecl(FileListEntry *entry, Node *type, Vector *names) {
  while (true) {
    Node *id = parseId(entry);
    if (id == NULL) {
      panicTopLevel(entry);

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
    }

    Token next;
    lex(entry, &next);
    switch (next.type) {
      case TT_COMMA: {
        // continue;
        break;
      }
      case TT_SEMI: {
        // done
        return varDeclNodeCreate(type, names);
      }
      default: {
        errorExpectedString(entry, "a comma or a semicolon", &next);

        unLex(entry, &next);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        return NULL;
      }
    }
  }
}

/**
 * finishes parsing a function declaration
 *
 * @param entry entry to lex from
 * @param returnType return type of function
 * @param name name of the function
 */
static Node *finishFunDecl(FileListEntry *entry, Node *returnType, Node *name) {
  Vector *argTypes = vectorCreate();
  Vector *argNames = vectorCreate();
  Vector *argDefaults = vectorCreate();

  bool doneArgs = false;
  Token peek;
  lex(entry, &peek);
  if (peek.type == TT_RPAREN)
    doneArgs = true;
  else
    unLex(entry, &peek);
  while (!doneArgs) {
    lex(entry, &peek);
    switch (peek.type) {
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
      case TT_ID: {
        // start of an arg decl
        unLex(entry, &peek);
        Node *type = parseType(entry);
        if (type == NULL) {
          panicTopLevel(entry);

          nodeFree(returnType);
          nodeFree(name);
          nodeVectorFree(argTypes);
          nodeVectorFree(argNames);
          nodeVectorFree(argDefaults);
          return NULL;
        }
        vectorInsert(argTypes, type);

        lex(entry, &peek);
        switch (peek.type) {
          case TT_ID: {
            // id - arg decl continues
            vectorInsert(argNames, idNodeCreate(&peek));

            lex(entry, &peek);
            switch (peek.type) {
              case TT_EQ: {
                // has a literal - arg decl continues
                Node *literal = parseLiteral(entry);
                if (literal == NULL) {
                  unLex(entry, &peek);
                  panicTopLevel(entry);

                  nodeFree(returnType);
                  nodeFree(name);
                  nodeVectorFree(argTypes);
                  nodeVectorFree(argNames);
                  nodeVectorFree(argDefaults);
                  return NULL;
                }
                vectorInsert(argDefaults, literal);

                lex(entry, &peek);
                switch (peek.type) {
                  case TT_COMMA: {
                    // done this arg decl;
                    break;
                  }
                  case TT_RPAREN: {
                    // done all arg decls
                    doneArgs = true;
                    break;
                  }
                  default: {
                    errorExpectedString(entry, "a comma or a right parenthesis",
                                        &peek);

                    unLex(entry, &peek);
                    panicTopLevel(entry);

                    nodeFree(returnType);
                    nodeFree(name);
                    nodeVectorFree(argTypes);
                    nodeVectorFree(argNames);
                    nodeVectorFree(argDefaults);
                    return NULL;
                  }
                }
                break;
              }
              case TT_COMMA: {
                // done this arg decl
                vectorInsert(argDefaults, NULL);
                break;
              }
              case TT_RPAREN: {
                // done all arg decls
                vectorInsert(argDefaults, NULL);
                doneArgs = true;
                break;
              }
              default: {
                errorExpectedString(
                    entry, "an equals sign, a comma, or a right parenthesis",
                    &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(returnType);
                nodeFree(name);
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                nodeVectorFree(argDefaults);
                return NULL;
              }
            }
            break;
          }
          case TT_COMMA: {
            // done this arg decl
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            break;
          }
          case TT_RPAREN: {
            // done all arg decls
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            doneArgs = true;
            break;
          }
          default: {
            errorExpectedString(entry, "an id, a comma, or a right parenthesis",
                                &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(returnType);
            nodeFree(name);
            nodeVectorFree(argTypes);
            nodeVectorFree(argNames);
            nodeVectorFree(argDefaults);
            return NULL;
          }
        }
        break;
      }
      default: {
        errorExpectedString(entry, "a type", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        nodeVectorFree(argDefaults);
        return NULL;
      }
    }
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(returnType);
    nodeFree(name);
    nodeVectorFree(argTypes);
    nodeVectorFree(argNames);
    nodeVectorFree(argDefaults);
    return NULL;
  }

  return funDeclNodeCreate(returnType, name, argTypes, argNames, argDefaults);
}

/**
 * parses a function or variable declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseFunOrVarDecl(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Node *id = parseId(entry);
  if (id == NULL) {
    panicTopLevel(entry);

    nodeFree(type);
    return NULL;
  }

  Token next;
  lex(entry, &next);
  switch (next.type) {
    case TT_SEMI: {
      // var decl, ends here
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      return varDeclNodeCreate(type, names);
    }
    case TT_COMMA: {
      // var decl, continued
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      return finishVarDecl(entry, type, names);
    }
    case TT_LPAREN: {
      // func decl, continued
      return finishFunDecl(entry, type, id);
    }
    default: {
      errorExpectedString(entry, "a semicolon, comma, or a left paren", &next);

      unLex(entry, &next);
      panicTopLevel(entry);

      nodeFree(type);
      nodeFree(id);
      return NULL;
    }
  }
}

/**
 * finishes parsing a variable definition
 *
 * @param entry entry to lex from
 * @param type type of the definition
 * @param names vector of names in the definition
 * @param initializers vector of initializers in the definition
 * @param hasLiteral does this defn have a literal after its first name?
 * @returns definition, or NULL if fatal error
 */
static Node *finishVarDefn(FileListEntry *entry, Node *type, Vector *names,
                           Vector *initializers, bool hasLiteral) {
  if (hasLiteral) {
    Node *literal = parseLiteral(entry);
    if (literal == NULL) {
      panicTopLevel(entry);

      nodeFree(type);
      nodeVectorFree(names);
      nodeVectorFree(initializers);
      return NULL;
    }
    vectorInsert(initializers, literal);

    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_COMMA: {
        // declaration continues
        break;
      }
      case TT_SEMI: {
        // end of declaration
        return varDefnNodeCreate(type, names, initializers);
      }
      default: {
        errorExpectedString(entry, "a comma or a semicolon", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        nodeVectorFree(initializers);
        return NULL;
      }
    }
  }

  while (true) {
    Node *id = parseId(entry);
    if (id == NULL) {
      panicTopLevel(entry);

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
    }

    Token next;
    lex(entry, &next);
    switch (next.type) {
      case TT_EQ: {
        // has initializer
        Node *literal = parseLiteral(entry);
        if (literal == NULL) {
          panicTopLevel(entry);

          nodeFree(type);
          nodeVectorFree(names);
          nodeVectorFree(initializers);
          return NULL;
        }
        vectorInsert(initializers, literal);

        Token peek;
        lex(entry, &peek);
        switch (peek.type) {
          case TT_COMMA: {
            // declaration continues
            break;
          }
          case TT_SEMI: {
            // end of declaration
            return varDefnNodeCreate(type, names, initializers);
          }
          default: {
            errorExpectedString(entry, "a comma or a semicolon", &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(type);
            nodeVectorFree(names);
            nodeVectorFree(initializers);
            return NULL;
          }
        }
        break;
      }
      case TT_COMMA: {
        // continue declaration
        vectorInsert(initializers, NULL);
        break;
      }
      case TT_SEMI: {
        // done
        return varDefnNodeCreate(type, names, initializers);
      }
      default: {
        errorExpectedString(entry, "a comma, a semicolon, or an equals sign",
                            &next);

        unLex(entry, &next);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        nodeVectorFree(initializers);
        return NULL;
      }
    }
  }
}

/**
 * makes a function body unparsed
 *
 * only cares about curly braces
 *
 * @param entry entry to lex from
 * @param start first open brace
 * @returns unparsed node, or NULL if fatal error
 */
static Node *parseFuncBody(FileListEntry *entry, Token *start) {
  Vector *tokens = vectorCreate();

  size_t levels = 1;
  while (levels > 0) {
    Token *token = malloc(sizeof(Token));
    lex(entry, token);
    switch (token->type) {
      case TT_LBRACE: {
        levels++;
        break;
      }
      case TT_RBRACE: {
        levels--;
        break;
      }
      default: { break; }
    }
    vectorInsert(tokens, token);
  }
  return unparsedNodeCreate(tokens);
}

/**
 * finishes parsing a function definition or declaration
 *
 * @param entry entry to lex from
 * @param returnType parsed return type
 * @param name name of the function
 * @returns definition, declaration, or NULL if fatal error
 */
static Node *finishFunDeclOrDefn(FileListEntry *entry, Node *returnType,
                                 Node *name) {
  Vector *argTypes = vectorCreate();
  Vector *argNames = vectorCreate();
  Vector *argDefaults = vectorCreate();

  bool doneArgs = false;
  Token peek;
  lex(entry, &peek);
  if (peek.type == TT_RPAREN)
    doneArgs = true;
  else
    unLex(entry, &peek);
  while (!doneArgs) {
    lex(entry, &peek);
    switch (peek.type) {
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
      case TT_ID: {
        // start of an arg decl
        unLex(entry, &peek);
        Node *type = parseType(entry);
        if (type == NULL) {
          panicTopLevel(entry);

          nodeFree(returnType);
          nodeFree(name);
          nodeVectorFree(argTypes);
          nodeVectorFree(argNames);
          nodeVectorFree(argDefaults);
          return NULL;
        }
        vectorInsert(argTypes, type);

        lex(entry, &peek);
        switch (peek.type) {
          case TT_ID: {
            // id - arg decl continues
            vectorInsert(argNames, idNodeCreate(&peek));

            lex(entry, &peek);
            switch (peek.type) {
              case TT_EQ: {
                // has a literal - arg decl continues
                Node *literal = parseLiteral(entry);
                if (literal == NULL) {
                  unLex(entry, &peek);
                  panicTopLevel(entry);

                  nodeFree(returnType);
                  nodeFree(name);
                  nodeVectorFree(argTypes);
                  nodeVectorFree(argNames);
                  nodeVectorFree(argDefaults);
                  return NULL;
                }
                vectorInsert(argDefaults, literal);

                lex(entry, &peek);
                switch (peek.type) {
                  case TT_COMMA: {
                    // done this arg decl;
                    break;
                  }
                  case TT_RPAREN: {
                    // done all arg decls
                    doneArgs = true;
                    break;
                  }
                  default: {
                    errorExpectedString(entry, "a comma or a right parenthesis",
                                        &peek);

                    unLex(entry, &peek);
                    panicTopLevel(entry);

                    nodeFree(returnType);
                    nodeFree(name);
                    nodeVectorFree(argTypes);
                    nodeVectorFree(argNames);
                    nodeVectorFree(argDefaults);
                    return NULL;
                  }
                }
                break;
              }
              case TT_COMMA: {
                // done this arg decl
                vectorInsert(argDefaults, NULL);
                break;
              }
              case TT_RPAREN: {
                // done all arg decls
                vectorInsert(argDefaults, NULL);
                doneArgs = true;
                break;
              }
              default: {
                errorExpectedString(
                    entry, "an equals sign, a comma, or a right parenthesis",
                    &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(returnType);
                nodeFree(name);
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                nodeVectorFree(argDefaults);
                return NULL;
              }
            }
            break;
          }
          case TT_COMMA: {
            // done this arg decl
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            break;
          }
          case TT_RPAREN: {
            // done all arg decls
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            doneArgs = true;
            break;
          }
          default: {
            errorExpectedString(entry, "an id, a comma, or a right parenthesis",
                                &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(returnType);
            nodeFree(name);
            nodeVectorFree(argTypes);
            nodeVectorFree(argNames);
            nodeVectorFree(argDefaults);
            return NULL;
          }
        }
        break;
      }
      default: {
        errorExpectedString(entry, "a type", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        nodeVectorFree(argDefaults);
        return NULL;
      }
    }
  }

  lex(entry, &peek);
  switch (peek.type) {
    case TT_SEMI: {
      return funDeclNodeCreate(returnType, name, argTypes, argNames,
                               argDefaults);
    }
    case TT_LBRACE: {
      Node *body = parseFuncBody(entry, &peek);
      if (body == NULL) {
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        nodeVectorFree(argDefaults);
        return NULL;
      }
      return funDefnNodeCreate(returnType, name, argTypes, argNames,
                               argDefaults, body);
    }
    default: {
      errorExpectedString(entry, "a semicolon or a left brace", &peek);

      unLex(entry, &peek);
      panicTopLevel(entry);

      nodeFree(returnType);
      nodeFree(name);
      nodeVectorFree(argTypes);
      nodeVectorFree(argNames);
      nodeVectorFree(argDefaults);
      return NULL;
    }
  }
}

/**
 * parses a function declaration, or a variable declaration or definition
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration, definition, or null if fatal error
 */
static Node *parseFunOrVarDeclOrDefn(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Node *id = parseId(entry);
  if (id == NULL) {
    panicTopLevel(entry);

    nodeFree(type);
    return NULL;
  }

  Token next;
  lex(entry, &next);
  switch (next.type) {
    case TT_SEMI: {
      // var defn, ends here
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return varDefnNodeCreate(type, names, initializers);
    }
    case TT_COMMA: {
      // var defn, continued
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return finishVarDefn(entry, type, names, initializers, false);
    }
    case TT_EQ: {
      // var defn, continued with initializer
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return finishVarDefn(entry, type, names, initializers, true);
    }
    case TT_LPAREN: {
      // func decl or defn, continued
      return finishFunDeclOrDefn(entry, type, id);
    }
    default: {
      errorExpectedString(entry, "a semicolon, comma, or a left paren", &next);

      unLex(entry, &next);
      panicTopLevel(entry);

      nodeFree(type);
      nodeFree(id);
      return NULL;
    }
  }
}

/**
 * parses an opaque declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseOpaqueDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    return NULL;
  }

  return opaqueDeclNodeCreate(start, name);
}

/**
 * parses a field or option declaration
 *
 * does not do error recovery, unLexes and returns null on an error
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseFieldOrOptionDecl(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    return NULL;
  }

  Vector *names = vectorCreate();
  bool done = false;
  while (!done) {
    Token id;
    lex(entry, &id);
    if (id.type != TT_ID) {
      errorExpectedToken(entry, TT_ID, &id);

      unLex(entry, &id);

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
    }

    vectorInsert(names, idNodeCreate(&id));

    Token peek;
    lex(entry, &peek);
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

        unLex(entry, &peek);

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

/**
 * parses a struct declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseStructDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    unLex(entry, &lbrace);
    panicTopLevel(entry);

    nodeFree(name);
    return NULL;
  }

  Vector *fields = vectorCreate();
  bool doneFields = false;
  while (!doneFields) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
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
      case TT_ID: {
        // this is the start of a field
        Node *field = parseFieldOrOptionDecl(entry, &peek);
        if (field == NULL) {
          panicTopLevel(entry);

          nodeFree(name);
          nodeVectorFree(fields);
          return NULL;
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

        unLex(entry, &peek);
        panicTopLevel(entry);

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
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(fields);
    return NULL;
  }

  return structDeclNodeCreate(start, name, fields);
}

/**
 * parses a union declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseUnionDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    unLex(entry, &lbrace);
    panicTopLevel(entry);

    nodeFree(name);
    return NULL;
  }

  Vector *options = vectorCreate();
  bool doneOptions = false;
  while (!doneOptions) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
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
      case TT_ID: {
        // this is the start of an option
        Node *option = parseFieldOrOptionDecl(entry, &peek);
        if (option == NULL) {
          panicTopLevel(entry);

          nodeFree(name);
          nodeVectorFree(options);
          return NULL;
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

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(name);
        nodeVectorFree(options);
        return NULL;
      }
    }
  }

  if (options->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one option in a union "
            "declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeFree(name);
    nodeVectorFree(options);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(options);
    return NULL;
  }

  return unionDeclNodeCreate(start, name, options);
}

/**
 * parses a enum declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseEnumDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);

    unLex(entry, &lbrace);
    panicTopLevel(entry);

    nodeFree(name);
    return NULL;
  }

  Vector *constantNames = vectorCreate();
  Vector *constantValues = vectorCreate();
  bool doneConstants = false;
  while (!doneConstants) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_ID: {
        // this is the start of a constant line
        vectorInsert(constantNames, idNodeCreate(&peek));

        lex(entry, &peek);
        switch (peek.type) {
          case TT_EQ: {
            // has an extended int literal
            Node *literal = parseExtendedIntLiteral(entry);
            if (literal == NULL) {
              panicTopLevel(entry);

              nodeFree(name);
              nodeVectorFree(constantNames);
              nodeVectorFree(constantValues);
              return NULL;
            }
            vectorInsert(constantValues, literal);

            lex(entry, &peek);
            switch (peek.type) {
              case TT_COMMA: {
                // end of this constant
                break;
              }
              case TT_RBRACE: {
                // end of the whole enum
                doneConstants = true;
                break;
              }
              default: {
                errorExpectedString(entry, "a comma or a right brace", &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(name);
                nodeVectorFree(constantNames);
                nodeVectorFree(constantValues);
                return NULL;
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
            doneConstants = true;
            break;
          }
          default: {
            errorExpectedString(
                entry, "a comma, an equals sign, or a right brace", &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(name);
            nodeVectorFree(constantNames);
            nodeVectorFree(constantValues);
            return NULL;
          }
        }
        break;
      }
      case TT_RBRACE: {
        doneConstants = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or an enumeration constant",
                            &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

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

    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  return enumDeclNodeCreate(start, name, constantNames, constantValues);
}

/**
 * parses a typedef declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseTypedefDecl(FileListEntry *entry, Token *start) {
  Node *originalType = parseType(entry);
  if (originalType == NULL) {
    panicTopLevel(entry);

    return NULL;
  }

  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);

    nodeFree(originalType);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(originalType);
    nodeFree(name);
    return NULL;
  }

  return typedefDeclNodeCreate(start, originalType, name);
}

/**
 * parses a set of file bodies
 *
 * never fatally errors, and consumes the EOF
 * Is aware of the code-file-ness of the entry
 *
 * @param entry entry to lex from
 * @returns Vector of bodies
 */
static Vector *parseBodies(FileListEntry *entry) {
  Vector *bodies = vectorCreate();
  while (true) {
    Token start;
    lex(entry, &start);

    switch (start.type) {
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
      case TT_ID: {
        Node *decl;
        if (entry->isCode)
          decl = parseFunOrVarDeclOrDefn(entry, &start);
        else
          decl = parseFunOrVarDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_OPAQUE: {
        Node *decl = parseOpaqueDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_STRUCT: {
        Node *decl = parseStructDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_UNION: {
        Node *decl = parseUnionDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_ENUM: {
        Node *decl = parseEnumDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_TYPEDEF: {
        Node *decl = parseTypedefDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_EOF: {
        // reached end of file
        return bodies;
      }
      default: {
        // unexpected token
        errorExpectedString(entry, "a declaration", &start);
        unLex(entry, &start);
        panicTopLevel(entry);
        continue;
      }
    }
  }
}

Node *parseFile(FileListEntry *entry) {
  Node *module = parseModule(entry);
  Vector *imports = parseImports(entry);
  Vector *bodies = parseBodies(entry);

  if (module == NULL) {
    // fatal error in the module
    nodeVectorFree(imports);
    nodeVectorFree(bodies);
    return NULL;
  } else {
    return fileNodeCreate(module, imports, bodies);
  }
}