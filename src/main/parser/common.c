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

#include "parser/common.h"

#include <stdio.h>

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

// error reporters

void errorExpectedString(FileListEntry *entry, char const *expected,
                         Token *actual) {
  fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
          entry->inputFilename, actual->line, actual->character, expected,
          TOKEN_NAMES[actual->type]);
  entry->errored = true;
}
void errorExpectedToken(FileListEntry *entry, TokenType expected,
                        Token *actual) {
  errorExpectedString(entry, TOKEN_NAMES[expected], actual);
}

// panics

void panicStructOrUnion(FileListEntry *entry) {
  while (true) {
    Token token;
    lex(entry, &token);
    switch (token.type) {
      case TT_SEMI: {
        return;
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
      case TT_BOOL:
      case TT_ID:
      case TT_EOF:
      case TT_RBRACE: {
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

void panicEnum(FileListEntry *entry) {
  while (true) {
    Token token;
    lex(entry, &token);
    switch (token.type) {
      case TT_COMMA: {
        return;
      }
      case TT_EOF:
      case TT_RBRACE: {
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

// context ignorant parser bits

Node *parseAnyId(FileListEntry *entry) {
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

Node *parseScopedId(FileListEntry *entry) {
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

Node *parseId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
    unLex(entry, &idToken);
    return NULL;
  }

  return idNodeCreate(&idToken);
}

Node *parseExtendedIntLiteral(FileListEntry *entry) {
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

Node *parseAggregateInitializer(FileListEntry *entry, Token *start) {
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
        n->data.literal.data.aggregateInitVal = literals;
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

Node *parseLiteral(FileListEntry *entry) {
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
      n->data.literal.data.doubleBits = bits;
      return n;
    }
    case TT_LIT_FLOAT: {
      uint32_t bits = floatStringToBits(peek.string);
      Node *n = literalNodeCreate(LT_FLOAT, &peek);
      n->data.literal.data.floatBits = bits;
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

Node *parseType(FileListEntry *entry) {
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

Node *parseFieldOrOptionDecl(FileListEntry *entry, Token *start) {
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