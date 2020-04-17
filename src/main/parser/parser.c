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

#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"

#include "fileList.h"
#include "internalError.h"
#include "numericSizing.h"
#include "options.h"
#include "util/container/stringBuilder.h"
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
          entry->inputFile, actual->line, actual->character,
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
          entry->inputFile, actual->line, actual->character, expected,
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
          entry->inputFile, token->line, token->character);
  entry->errored = true;
}

// constructors

/** create an initialized, empty vector */
static Vector *createVector(void) {
  Vector *v = malloc(sizeof(Vector));
  vectorInit(v);
  return v;
}
/** create a partially initialized node */
static Node *createNode(NodeType type, size_t line, size_t character) {
  Node *n = malloc(sizeof(Node));
  n->type = type;
  n->line = line;
  n->character = character;
  return n;
}

static Node *createFile(Node *module, Vector *imports, Vector *bodies) {
  Node *n = createNode(NT_FILE, module->line, module->character);
  n->data.file.module = module;
  n->data.file.imports = imports;
  n->data.file.bodies = bodies;
  hashMapInit(&n->data.file.stab);
  return n;
}
static Node *createModule(Token *keyword, Node *id) {
  Node *n = createNode(NT_MODULE, keyword->line, keyword->character);
  n->data.module.id = id;
  return n;
}
static Node *createImport(Token *keyword, Node *id) {
  Node *n = createNode(NT_IMPORT, keyword->line, keyword->character);
  n->data.import.id = id;
  n->data.import.referenced = NULL;
  return n;
}

static Node *createFunDefn(Node *returnType, Node *name, Vector *argTypes,
                           Vector *argNames, Vector *argDefaults, Node *body) {
  Node *n = createNode(NT_FUNDEFN, returnType->line, returnType->character);
  n->data.funDefn.returnType = returnType;
  n->data.funDefn.name = name;
  n->data.funDefn.argTypes = argTypes;
  n->data.funDefn.argNames = argNames;
  n->data.funDefn.argDefaults = argDefaults;
  n->data.funDefn.body = body;
  hashMapInit(&n->data.funDefn.stab);
  return n;
}
static Node *createVarDefn(Node *type, Vector *names, Vector *initializers) {
  Node *n = createNode(NT_VARDEFN, type->line, type->character);
  n->data.varDefn.type = type;
  n->data.varDefn.names = names;
  n->data.varDefn.initializers = initializers;
  return n;
}

static Node *createFunDecl(Node *returnType, Node *name, Vector *argTypes,
                           Vector *argNames, Vector *argDefaults) {
  Node *n = createNode(NT_FUNDECL, returnType->line, returnType->character);
  n->data.funDecl.returnType = returnType;
  n->data.funDecl.name = name;
  n->data.funDecl.argTypes = argTypes;
  n->data.funDecl.argNames = argNames;
  n->data.funDecl.argDefaults = argDefaults;
  return n;
}
static Node *createVarDecl(Node *type, Vector *names) {
  Node *n = createNode(NT_VARDECL, type->line, type->character);
  n->data.varDecl.type = type;
  n->data.varDecl.names = names;
  return n;
}
static Node *createOpaqueDecl(Token *keyword, Node *name) {
  Node *n = createNode(NT_OPAQUEDECL, keyword->line, keyword->character);
  n->data.opaqueDecl.name = name;
  return n;
}
static Node *createStructDecl(Token *keyword, Node *name, Vector *fields) {
  Node *n = createNode(NT_STRUCTDECL, keyword->line, keyword->character);
  n->data.structDecl.name = name;
  n->data.structDecl.fields = fields;
  return n;
}
static Node *createUnionDecl(Token *keyword, Node *name, Vector *options) {
  Node *n = createNode(NT_UNIONDECL, keyword->line, keyword->character);
  n->data.unionDecl.name = name;
  n->data.unionDecl.options = options;
  return n;
}
static Node *createEnumDecl(Token *keyword, Node *name, Vector *constantNames,
                            Vector *constantValues) {
  Node *n = createNode(NT_ENUMDECL, keyword->line, keyword->character);
  n->data.enumDecl.name = name;
  n->data.enumDecl.constantNames = constantNames;
  n->data.enumDecl.constantValues = constantValues;
  return n;
}
static Node *createTypedefDecl(Token *keyword, Node *originalType, Node *name) {
  Node *n = createNode(NT_TYPEDEFDECL, keyword->line, keyword->character);
  n->data.typedefDecl.originalType = originalType;
  n->data.typedefDecl.name = name;
  return n;
}

static Node *createCompoundStmt(Token *lbrace, Vector *stmts) {
  Node *n = createNode(NT_COMPOUNDSTMT, lbrace->line, lbrace->character);
  n->data.compoundStmt.stmts = stmts;
  hashMapInit(&n->data.compoundStmt.stab);
  return n;
}
static Node *createIfStmt(Token *keyword, Node *predicate, Node *consequent,
                          Node *alternative) {
  Node *n = createNode(NT_IFSTMT, keyword->line, keyword->character);
  n->data.ifStmt.predicate = predicate;
  n->data.ifStmt.consequent = consequent;
  n->data.ifStmt.alternative = alternative;
  return n;
}
static Node *createWhileStmt(Token *keyword, Node *condition, Node *body) {
  Node *n = createNode(NT_WHILESTMT, keyword->line, keyword->character);
  n->data.whileStmt.condition = condition;
  n->data.whileStmt.body = body;
  return n;
}
static Node *createDoWhileStmt(Token *keyword, Node *body, Node *condition) {
  Node *n = createNode(NT_DOWHILESTMT, keyword->line, keyword->character);
  n->data.doWhileStmt.body = body;
  n->data.doWhileStmt.condition = condition;
  return n;
}
static Node *createForStmt(Token *keyword, Node *initializer, Node *condition,
                           Node *increment, Node *body) {
  Node *n = createNode(NT_FORSTMT, keyword->line, keyword->character);
  n->data.forStmt.initializer = initializer;
  n->data.forStmt.condition = condition;
  n->data.forStmt.increment = increment;
  n->data.forStmt.body = body;
  hashMapInit(&n->data.forStmt.stab);
  return n;
}
static Node *createSwitchStmt(Token *keyword, Node *condition, Vector *cases) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  n->data.switchStmt.condition = condition;
  n->data.switchStmt.cases = cases;
  return n;
}
static Node *createBreatkStmt(Token *keyword) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  return n;
}
static Node *createContinueStmt(Token *keyword) {
  Node *n = createNode(NT_CONTINUESTMT, keyword->line, keyword->character);
  return n;
}
static Node *createReturnStmt(Token *keyword, Node *value) {
  Node *n = createNode(NT_RETURNSTMT, keyword->line, keyword->character);
  n->data.returnStmt.value = value;
  return n;
}
static Node *asmStmt(Token *keyword, Node *assembly) {
  Node *n = createNode(NT_ASMSTMT, keyword->line, keyword->character);
  n->data.asmStmt.assembly = assembly;
  return n;
}
static Node *createVarDefnStmt(Node *type, Vector *names,
                               Vector *initializers) {
  Node *n = createNode(NT_VARDEFNSTMT, type->line, type->character);
  n->data.varDefnStmt.type = type;
  n->data.varDefnStmt.names = names;
  n->data.varDefnStmt.initializers = initializers;
  return n;
}
static Node *createExpressionStmt(Node *expression) {
  Node *n =
      createNode(NT_EXPRESSIONSTMT, expression->line, expression->character);
  n->data.expressionStmt.expression = expression;
  return n;
}
static Node *createNullStmt(Token *semicolon) {
  Node *n = createNode(NT_NULLSTMT, semicolon->line, semicolon->character);
  return n;
}

static Node *createSwitchCase(Token *keyword, Vector *values, Node *body) {
  Node *n = createNode(NT_SWITCHCASE, keyword->line, keyword->character);
  n->data.switchCase.values = values;
  n->data.switchCase.body = body;
  return n;
}
static Node *createSwitchDefault(Token *keyword, Node *body) {
  Node *n = createNode(NT_SWITCHDEFAULT, keyword->line, keyword->character);
  n->data.switchDefault.body = body;
  return n;
}

static Node *createBinOpExp(BinOpType op, Node *lhs, Node *rhs) {
  Node *n = createNode(NT_BINOPEXP, lhs->line, lhs->character);
  n->data.binOpExp.op = op;
  n->data.binOpExp.lhs = lhs;
  n->data.binOpExp.rhs = rhs;
  return n;
}
static Node *createTernaryExp(Node *predicate, Node *consequent,
                              Node *alternative) {
  Node *n = createNode(NT_TERNARYEXP, predicate->line, predicate->character);
  n->data.ternaryExp.predicate = predicate;
  n->data.ternaryExp.consequent = consequent;
  n->data.ternaryExp.alternative = alternative;
  return n;
}
static Node *createPrefixUnOpExp(UnOpType op, Token *opToken, Node *target) {
  Node *n = createNode(NT_UNOPEXP, opToken->line, opToken->character);
  n->data.unOpExp.op = op;
  n->data.unOpExp.target = target;
  return n;
}
static Node *createPostfixUnOpExp(UnOpType op, Node *target) {
  Node *n = createNode(NT_UNOPEXP, target->line, target->character);
  n->data.unOpExp.op = op;
  n->data.unOpExp.target = target;
  return n;
}
static Node *createFunCallExp(Node *function, Vector *arguments) {
  Node *n = createNode(NT_FUNCALLEXP, function->line, function->character);
  n->data.funCallExp.function = function;
  n->data.funCallExp.arguments = arguments;
  return n;
}

static Node *createLiteralNode(LiteralType type, Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = type;
  return n;
}
static Node *createCharLiteralNode(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_CHAR;
  char *string = t->string;

  if (string[0] == '\\') {
    // escape sequence
    switch (string[1]) {
      case 'n': {
        n->data.literal.value.charVal = charToU8('\n');
        break;
      }
      case 'r': {
        n->data.literal.value.charVal = charToU8('\r');
        break;
      }
      case 't': {
        n->data.literal.value.charVal = charToU8('\t');
        break;
      }
      case '0': {
        n->data.literal.value.charVal = charToU8('\0');
        break;
      }
      case '\\': {
        n->data.literal.value.charVal = charToU8('\\');
        break;
      }
      case 'x': {
        n->data.literal.value.charVal = (uint8_t)((nybbleToU8(string[2]) << 4) +
                                                  (nybbleToU8(string[3]) << 0));
        break;
      }
      case '\'': {
        n->data.literal.value.charVal = charToU8('\'');
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "bad char literal string passed to createCharLiteralNode");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.value.charVal = charToU8(string[0]);
  }

  tokenUninit(t);
  return n;
}
static Node *createWcharLiteralNode(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_WCHAR;
  char *string = t->string;

  if (string[0] == '\\') {
    // escape sequence
    switch (string[1]) {
      case 'n': {
        n->data.literal.value.wcharVal = charToU8('\n');
        break;
      }
      case 'r': {
        n->data.literal.value.wcharVal = charToU8('\r');
        break;
      }
      case 't': {
        n->data.literal.value.wcharVal = charToU8('\t');
        break;
      }
      case '0': {
        n->data.literal.value.wcharVal = charToU8('\0');
        break;
      }
      case '\\': {
        n->data.literal.value.wcharVal = charToU8('\\');
        break;
      }
      case 'x': {
        n->data.literal.value.wcharVal = (uint8_t)(
            (nybbleToU8(string[2]) << 4) + (nybbleToU8(string[3]) << 0));
        break;
      }
      case 'u': {
        n->data.literal.value.wcharVal = 0;
        for (size_t idx = 0; idx < 8; idx++) {
          n->data.literal.value.wcharVal <<= 4;
          n->data.literal.value.wcharVal += nybbleToU8(string[idx + 2]);
        }
        break;
      }
      case '\'': {
        n->data.literal.value.wcharVal = charToU8('\'');
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "bad wchar literal string passed to createWcharLiteralNode");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.value.wcharVal = charToU8(string[0]);
  }

  tokenUninit(t);
  return n;
}
static Node *createStringLiteralNode(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_STRING;
  char *string = t->string;

  TStringBuilder sb;
  tstringBuilderInit(&sb);
  for (; *string != '\0'; string++) {
    if (*string == '\\') {
      string++;
      // escape sequence
      switch (*string) {
        case 'n': {
          tstringBuilderPush(&sb, charToU8('\n'));
          break;
        }
        case 'r': {
          tstringBuilderPush(&sb, charToU8('\r'));
          break;
        }
        case 't': {
          tstringBuilderPush(&sb, charToU8('\t'));
          break;
        }
        case '0': {
          tstringBuilderPush(&sb, charToU8('\0'));
          break;
        }
        case '\\': {
          tstringBuilderPush(&sb, charToU8('\\'));
          break;
        }
        case 'x': {
          char high = *string++;
          char low = *string;
          tstringBuilderPush(
              &sb, (uint8_t)((nybbleToU8(high) << 4) + (nybbleToU8(low) << 0)));
          break;
        }
        case '\'': {
          tstringBuilderPush(&sb, charToU8('\''));
          break;
        }
        default: {
          error(__FILE__, __LINE__,
                "bad string literal string passed to createStringLiteralNode");
        }
      }
    } else {
      // not an escape statement
      tstringBuilderPush(&sb, charToU8(*string));
    }
  }

  n->data.literal.value.stringVal = tstringBuilderData(&sb);
  tstringBuilderUninit(&sb);
  tokenUninit(t);
  return n;
}
static Node *createWstringLiteralNode(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_WSTRING;
  char *string = t->string;

  TWStringBuilder sb;
  twstringBuilderInit(&sb);
  for (; *string != '\0'; string++) {
    if (*string == '\\') {
      string++;
      // escape sequence
      switch (*string) {
        case 'n': {
          twstringBuilderPush(&sb, charToU8('\n'));
          break;
        }
        case 'r': {
          twstringBuilderPush(&sb, charToU8('\r'));
          break;
        }
        case 't': {
          twstringBuilderPush(&sb, charToU8('\t'));
          break;
        }
        case '0': {
          twstringBuilderPush(&sb, charToU8('\0'));
          break;
        }
        case '\\': {
          twstringBuilderPush(&sb, charToU8('\\'));
          break;
        }
        case 'x': {
          char high = *string++;
          char low = *string;
          twstringBuilderPush(
              &sb, (uint8_t)((nybbleToU8(high) << 4) + (nybbleToU8(low) << 0)));
          break;
        }
        case 'u': {
          uint32_t value = 0;
          for (size_t idx = 0; idx < 8; idx++, string++) {
            value <<= 4;
            value += nybbleToU8(*string);
          }
          twstringBuilderPush(&sb, value);
          break;
        }
        case '\'': {
          twstringBuilderPush(&sb, charToU8('\''));
          break;
        }
        default: {
          error(
              __FILE__, __LINE__,
              "bad wstring literal string passed to createWstringLiteralNode");
        }
      }
    } else {
      // not an escape statement
      twstringBuilderPush(&sb, charToU8(*string));
    }
  }

  n->data.literal.value.wstringVal = twstringBuilderData(&sb);
  twstringBuilderUninit(&sb);
  tokenUninit(t);
  return n;
}
static Node *createSizedIntegerLiteral(FileListEntry *entry, Token *t,
                                       int8_t sign, uint64_t magnitude) {
  if (sign == 0) {
    // is unsigned
    if (magnitude <= UBYTE_MAX) {
      Node *n = createLiteralNode(LT_UBYTE, t);
      n->data.literal.value.ubyteVal = (uint8_t)magnitude;
      return n;
    } else if (magnitude <= USHORT_MAX) {
      Node *n = createLiteralNode(LT_USHORT, t);
      n->data.literal.value.ushortVal = (uint16_t)magnitude;
      return n;
    } else if (magnitude <= UINT_MAX) {
      Node *n = createLiteralNode(LT_UINT, t);
      n->data.literal.value.uintVal = (uint32_t)magnitude;
      return n;
    } else if (magnitude <= ULONG_MAX) {
      Node *n = createLiteralNode(LT_ULONG, t);
      n->data.literal.value.ulongVal = (uint64_t)magnitude;
      return n;
    } else {
      error(__FILE__, __LINE__,
            "magnitude larger than ULONG_MAX passed to "
            "createSizedIntegerLiteral (CPU bug?)");
    }
  } else if (sign == -1) {
    // negative
    if (magnitude <= BYTE_MIN) {
      Node *n = createLiteralNode(LT_BYTE, t);
      n->data.literal.value.byteVal = (int8_t)-magnitude;
      return n;
    } else if (magnitude <= SHORT_MIN) {
      Node *n = createLiteralNode(LT_SHORT, t);
      n->data.literal.value.shortVal = (int16_t)-magnitude;
      return n;
    } else if (magnitude <= INT_MIN) {
      Node *n = createLiteralNode(LT_INT, t);
      n->data.literal.value.intVal = (int32_t)-magnitude;
      return n;
    } else if (magnitude <= LONG_MIN) {
      Node *n = createLiteralNode(LT_LONG, t);
      n->data.literal.value.longVal = (int64_t)-magnitude;
      return n;
    } else {
      // user-side size error
      errorIntOverflow(entry, t);
      tokenUninit(t);
      return NULL;
    }
  } else if (sign == 1) {
    // positive
    if (magnitude <= BYTE_MAX) {
      Node *n = createLiteralNode(LT_BYTE, t);
      n->data.literal.value.byteVal = (int8_t)magnitude;
      return n;
    } else if (magnitude <= SHORT_MAX) {
      Node *n = createLiteralNode(LT_SHORT, t);
      n->data.literal.value.shortVal = (int16_t)magnitude;
      return n;
    } else if (magnitude <= INT_MAX) {
      Node *n = createLiteralNode(LT_INT, t);
      n->data.literal.value.intVal = (int32_t)magnitude;
      return n;
    } else if (magnitude <= LONG_MAX) {
      Node *n = createLiteralNode(LT_LONG, t);
      n->data.literal.value.longVal = (int64_t)magnitude;
      return n;
    } else {
      // user-side size error
      errorIntOverflow(entry, t);
      tokenUninit(t);
      return NULL;
    }
  } else {
    error(__FILE__, __LINE__,
          "invalid sign passed to createSizedIntegerLiteral");
  }
}

static Node *createKeywordType(TypeKeyword keyword, Token *keywordToken) {
  Node *n =
      createNode(NT_KEYWORDTYPE, keywordToken->line, keywordToken->character);
  n->data.keywordType.keyword = keyword;
  return n;
}
static Node *createModifiedType(TypeModifier modifier, Node *baseType) {
  Node *n = createNode(NT_MODIFIEDTYPE, baseType->line, baseType->character);
  n->data.modifiedType.modifier = modifier;
  n->data.modifiedType.baseType = baseType;
  return n;
}
static Node *createArrayType(Node *baseType, Node *size) {
  Node *n = createNode(NT_ARRAYTYPE, baseType->line, baseType->character);
  n->data.arrayType.baseType = baseType;
  n->data.arrayType.size = size;
  return n;
}
static Node *createFunPtrType(Node *returnType, Vector *argTypes,
                              Vector *argNames) {
  Node *n = createNode(NT_FUNPTRTYPE, returnType->line, returnType->character);
  n->data.funPtrType.returnType = returnType;
  n->data.funPtrType.argTypes = argTypes;
  n->data.funPtrType.argNames = argNames;
  return n;
}

static Node *createScopedId(Vector *components) {
  Node *first = components->elements[0];
  Node *n = createNode(NT_SCOPEDID, first->line, first->character);
  n->data.scopedId.components = components;
  return n;
}
static Node *createId(Token *id) {
  Node *n = createNode(NT_ID, id->line, id->character);
  n->data.id.id = id->string;
  return n;
}

// panics

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

/**
 * reads tokens until a probable statement-level form boundary
 *
 * semicolons are consumed, EOFs, and the start of a statement (excluding
 * expressions) are left
 *
 * @param entry entry to lex from
 */
static void panicStmt(FileListEntry *entry) {
  while (true) {
    Token token;
    lex(entry, &token);
    switch (token.type) {
      case TT_SEMI: {
        return;
      }
      case TT_LBRACE:
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

/**
 * parses an ID or scoped ID
 *
 * does not do error recovery, unlexes on an error
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
    return createId(&idToken);
  } else {
    // scoped id - saw scope
    Vector *components = createVector();

    vectorInsert(components, createId(&idToken));
    while (true) {
      // expect an id, add it to the node
      lex(entry, &idToken);
      if (idToken.type != TT_ID) {
        errorExpectedToken(entry, TT_ID, &idToken);

        unLex(entry, &idToken);

        nodeVectorFree(components);
        return NULL;
      } else {
        vectorInsert(components, createId(&idToken));
      }

      // if there's a scope, keep going, else return
      lex(entry, &scope);
      if (scope.type != TT_SCOPE) {
        unLex(entry, &scope);
        return createScopedId(components);
      }
    }
  }
}

/**
 * parses a scoped ID
 *
 * does not do error recovery, unlexes on an error
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseScopedId(FileListEntry *entry) {
  Vector *components = createVector();
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
      vectorInsert(components, createId(&peek));
    }

    // if there's a scope, keep going, else return
    lex(entry, &peek);
    if (peek.type != TT_SCOPE) {
      unLex(entry, &peek);

      if (components->size >= 2) {
        return createScopedId(components);
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
 * does not do error recovery, unlexes if bad thing happened
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

  return createId(&idToken);
}

/**
 * parses an extended int literal
 *
 * does not do error recovery, unlexes if errored
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseExtendedIntLiteral(FileListEntry *entry) {
  Token peek;
  lex(entry, &peek);
  switch (peek.type) {
    case TT_LIT_CHAR: {
      return createCharLiteralNode(&peek);
    }
    case TT_LIT_WCHAR: {
      return createWcharLiteralNode(&peek);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
 * does not do error recovery, unlexes if errored
 *
 * @param entry entry to lex from
 * @param start first token in aggregate init
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAggregateInitializer(FileListEntry *entry, Token *start) {
  Vector *literals = createVector();
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
        Node *n = createLiteralNode(LT_AGGREGATEINIT, start);
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
 * does not do error recovery, unlexes if errored
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseLiteral(FileListEntry *entry) {
  Token peek;
  lex(entry, &peek);
  switch (peek.type) {
    case TT_LIT_STRING: {
      return createStringLiteralNode(&peek);
    }
    case TT_LIT_WSTRING: {
      return createWstringLiteralNode(&peek);
    }
    case TT_LIT_CHAR: {
      return createCharLiteralNode(&peek);
    }
    case TT_LIT_WCHAR: {
      return createWcharLiteralNode(&peek);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
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
      return createSizedIntegerLiteral(entry, &peek, sign, magnitude);
    }
    case TT_LIT_DOUBLE: {
      uint64_t bits = doubleStringToBits(peek.string);
      Node *n = createLiteralNode(LT_DOUBLE, &peek);
      n->data.literal.value.doubleBits = bits;
      return n;
    }
    case TT_LIT_FLOAT: {
      uint32_t bits = floatStringToBits(peek.string);
      Node *n = createLiteralNode(LT_FLOAT, &peek);
      n->data.literal.value.floatBits = bits;
      return n;
    }
    case TT_BAD_STRING:
    case TT_BAD_CHAR:
    case TT_BAD_BIN:
    case TT_BAD_HEX: {
      return NULL;
    }
    case TT_ID: {
      unLex(entry, &peek);
      return parseScopedId(entry);
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
 * does not do error recovery
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseType(FileListEntry *entry) {
  return NULL;  // TODO: write this
}

// context-aware parsers

// expressions

// statements

static Node *parseStmt(FileListEntry *entry, Token *start);
/**
 * parses a compound statement
 *
 * @param entry entry to lex from
 * @param lbrace first token
 * @returns ast node or null if fatal error happened
 */
static Node *parseCompoundStmt(FileListEntry *entry, Token *lbrace) {
  Vector *stmts = createVector();
  return NULL;  // TODO: write this
}

/**
 * parses any statement
 *
 * @param entry entry to lex from
 * @param start first token of statement
 * @returns ast node or null if fatal error happened
 */
static Node *parseStmt(FileListEntry *entry, Token *start) {
  return NULL;  // TODO: write this
}

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

  return createModule(&moduleKeyword, id);
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

  return createImport(importKeyword, id);
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
  Vector *imports = createVector();
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
        return createVarDecl(type, names);
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
  Vector *argTypes = createVector();
  Vector *argNames = createVector();
  Vector *argDefaults = createVector();

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
            vectorInsert(argNames, createId(&peek));

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

  return createFunDecl(returnType, name, argTypes, argNames, argDefaults);
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
      Vector *names = createVector();
      vectorInsert(names, id);
      return createVarDecl(type, names);
    }
    case TT_COMMA: {
      // var decl, continued
      Vector *names = createVector();
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
        return createVarDefn(type, names, initializers);
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
            return createVarDefn(type, names, initializers);
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
        return createVarDefn(type, names, initializers);
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
 * finishes parsing a function definition or declaration
 *
 * @param entry entry to lex from
 * @param returnType parsed return type
 * @param name name of the function
 * @returns definition, declaration, or NULL if fatal error
 */
static Node *finishFunDeclOrDefn(FileListEntry *entry, Node *returnType,
                                 Node *name) {
  Vector *argTypes = createVector();
  Vector *argNames = createVector();
  Vector *argDefaults = createVector();

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
            vectorInsert(argNames, createId(&peek));

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
      return createFunDecl(returnType, name, argTypes, argNames, argDefaults);
    }
    case TT_LBRACE: {
      Node *body = parseCompoundStmt(entry, &peek);
      if (body == NULL) {
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        nodeVectorFree(argDefaults);
        return NULL;
      }
      return createFunDefn(returnType, name, argTypes, argNames, argDefaults,
                           body);
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
      Vector *names = createVector();
      vectorInsert(names, id);
      Vector *initializers = createVector();
      vectorInsert(initializers, NULL);
      return createVarDefn(type, names, initializers);
    }
    case TT_COMMA: {
      // var defn, continued
      Vector *names = createVector();
      vectorInsert(names, id);
      Vector *initializers = createVector();
      vectorInsert(initializers, NULL);
      return finishVarDefn(entry, type, names, initializers, false);
    }
    case TT_EQ: {
      // var defn, continued with initializer
      Vector *names = createVector();
      vectorInsert(names, id);
      Vector *initializers = createVector();
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

  return createOpaqueDecl(start, name);
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

  Vector *names = createVector();
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

    vectorInsert(names, createId(&id));

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

  return createVarDecl(type, names);
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

  Vector *fields = createVector();
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
            entry->inputFile, lbrace.line, lbrace.character);
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

  return createStructDecl(start, name, fields);
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

  Vector *options = createVector();
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
            entry->inputFile, lbrace.line, lbrace.character);
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

  return createUnionDecl(start, name, options);
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

  Vector *constantNames = createVector();
  Vector *constantValues = createVector();
  bool doneConstants = false;
  while (!doneConstants) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_ID: {
        // this is the start of a constant line
        vectorInsert(constantNames, createId(&peek));

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
            entry->inputFile, lbrace.line, lbrace.character);
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

  return createEnumDecl(start, name, constantNames, constantValues);
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

  return createTypedefDecl(start, originalType, name);
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
  Vector *bodies = createVector();
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

/**
 * parses a file, phase one
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if error happened
 */
static Node *parseFile(FileListEntry *entry) {
  Node *module = parseModule(entry);
  Vector *imports = parseImports(entry);
  Vector *bodies = parseBodies(entry);

  if (module == NULL) {
    // fatal error in the module
    nodeVectorFree(imports);
    nodeVectorFree(bodies);
    return NULL;
  } else {
    return createFile(module, imports, bodies);
  }
}

int parse(void) {
  int retval = 0;
  bool errored = false; /**< has any part of the whole thing errored */

  lexerInitMaps();

  // pass one - parse and gather top-level names, leaving some nodes as unparsed
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      errored = true;
      continue;
    }

    fileList.entries[idx].program = parseFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;

    lexerStateUninit(&fileList.entries[idx]);
  }

  lexerUninitMaps();

  if (errored) {
    // at least one produced NULL - clean up and report that
    for (size_t idx = 0; idx < fileList.size; idx++)
      nodeFree(fileList.entries[idx].program);
    return -1;
  }

  // pass two - generate symbol tables
  // pass three - resolve imports and parse unparsed nodes

  return 0;
}