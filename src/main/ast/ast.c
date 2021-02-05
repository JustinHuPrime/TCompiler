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

#include "ast/ast.h"

#include <stdlib.h>
#include <string.h>

#include "fileList.h"
#include "internalError.h"
#include "lexer/lexer.h"
#include "numericSizing.h"
#include "util/container/stringBuilder.h"
#include "util/conversions.h"
#include "util/format.h"

BinOpType assignmentTokenToBinop(TokenType token) {
  switch (token) {
    case TT_ASSIGN: {
      return BO_ASSIGN;
    }
    case TT_MULASSIGN: {
      return BO_MULASSIGN;
    }
    case TT_DIVASSIGN: {
      return BO_DIVASSIGN;
    }
    case TT_ADDASSIGN: {
      return BO_ADDASSIGN;
    }
    case TT_SUBASSIGN: {
      return BO_SUBASSIGN;
    }
    case TT_LSHIFTASSIGN: {
      return BO_LSHIFTASSIGN;
    }
    case TT_ARSHIFTASSIGN: {
      return BO_ARSHIFTASSIGN;
    }
    case TT_LRSHIFTASSIGN: {
      return BO_LRSHIFTASSIGN;
    }
    case TT_BITANDASSIGN: {
      return BO_BITANDASSIGN;
    }
    case TT_BITXORASSIGN: {
      return BO_BITXORASSIGN;
    }
    case TT_BITORASSIGN: {
      return BO_BITORASSIGN;
    }
    case TT_LANDASSIGN: {
      return BO_LANDASSIGN;
    }
    case TT_LORASSIGN: {
      return BO_LORASSIGN;
    }
    default: {
      error(__FILE__, __LINE__, "invalid assign binop token given");
    }
  }
}
BinOpType logicalTokenToBinop(TokenType token) {
  switch (token) {
    case TT_LAND: {
      return BO_LAND;
    }
    case TT_LOR: {
      return BO_LOR;
    }
    default: {
      error(__FILE__, __LINE__, "invalid logical binop token given");
    }
  }
}
BinOpType bitwiseTokenToBinop(TokenType token) {
  switch (token) {
    case TT_AMP: {
      return BO_BITAND;
    }
    case TT_BAR: {
      return BO_BITOR;
    }
    case TT_CARET: {
      return BO_BITXOR;
    }
    default: {
      error(__FILE__, __LINE__, "invalid logical binop token given");
    }
  }
}
BinOpType equalityTokenToBinop(TokenType token) {
  switch (token) {
    case TT_EQ: {
      return BO_EQ;
    }
    case TT_NEQ: {
      return BO_NEQ;
    }
    default: {
      error(__FILE__, __LINE__, "invalid equality binop token given");
    }
  }
}
BinOpType comparisonTokenToBinop(TokenType token) {
  switch (token) {
    case TT_LANGLE: {
      return BO_LT;
    }
    case TT_RANGLE: {
      return BO_GT;
    }
    case TT_LTEQ: {
      return BO_LTEQ;
    }
    case TT_GTEQ: {
      return BO_GTEQ;
    }
    default: {
      error(__FILE__, __LINE__, "invalid comparison binop token given");
    }
  }
}
BinOpType shiftTokenToBinop(TokenType token) {
  switch (token) {
    case TT_LSHIFT: {
      return BO_LSHIFT;
    }
    case TT_ARSHIFT: {
      return BO_ARSHIFT;
    }
    case TT_LRSHIFT: {
      return BO_LRSHIFT;
    }
    default: {
      error(__FILE__, __LINE__, "invalid shift binop token given");
    }
  }
}
BinOpType additionTokenToBinop(TokenType token) {
  switch (token) {
    case TT_PLUS: {
      return BO_ADD;
    }
    case TT_MINUS: {
      return BO_SUB;
    }
    default: {
      error(__FILE__, __LINE__, "invalid addition binop token given");
    }
  }
}
BinOpType multiplicationTokenToBinop(TokenType token) {
  switch (token) {
    case TT_STAR: {
      return BO_MUL;
    }
    case TT_SLASH: {
      return BO_DIV;
    }
    case TT_PERCENT: {
      return BO_MOD;
    }
    default: {
      error(__FILE__, __LINE__, "invalid multiplication binop token given");
    }
  }
}
BinOpType accessorTokenToBinop(TokenType token) {
  switch (token) {
    case TT_DOT: {
      return BO_FIELD;
    }
    case TT_ARROW: {
      return BO_PTRFIELD;
    }
    default: {
      error(__FILE__, __LINE__, "invalid accessor binop token given");
    }
  }
}

UnOpType prefixTokenToUnop(TokenType token) {
  switch (token) {
    case TT_STAR: {
      return UO_DEREF;
    }
    case TT_AMP: {
      return UO_ADDROF;
    }
    case TT_INC: {
      return UO_PREINC;
    }
    case TT_DEC: {
      return UO_PREDEC;
    }
    case TT_MINUS: {
      return UO_NEG;
    }
    case TT_BANG: {
      return UO_LNOT;
    }
    case TT_TILDE: {
      return UO_BITNOT;
    }
    default: {
      error(__FILE__, __LINE__, "invalid prefix binop token given");
    }
  }
}
UnOpType postfixTokenToUnop(TokenType token) {
  switch (token) {
    case TT_INC: {
      return UO_POSTINC;
    }
    case TT_DEC: {
      return UO_POSTDEC;
    }
    case TT_NEGASSIGN: {
      return UO_NEGASSIGN;
    }
    case TT_LNOTASSIGN: {
      return UO_LNOTASSIGN;
    }
    case TT_BITNOTASSIGN: {
      return UO_BITNOTASSIGN;
    }
    default: {
      error(__FILE__, __LINE__, "invalid postfix binop token given");
    }
  }
}

/**
 * create a partially initialized node
 *
 * @param type type of node to indicate
 * @param line line to attribute node to
 * @param character character to attribute node to
 */
static Node *createNode(NodeType type, size_t line, size_t character) {
  Node *n = malloc(sizeof(Node));
  n->type = type;
  n->line = line;
  n->character = character;
  return n;
}

Node *fileNodeCreate(Node *module, Vector *imports, Vector *bodies) {
  Node *n = createNode(NT_FILE, module->line, module->character);
  n->data.file.stab = hashMapCreate();
  n->data.file.module = module;
  n->data.file.imports = imports;
  n->data.file.bodies = bodies;
  return n;
}
Node *moduleNodeCreate(Token const *keyword, Node *id) {
  Node *n = createNode(NT_MODULE, keyword->line, keyword->character);
  n->data.module.id = id;
  return n;
}
Node *importNodeCreate(Token const *keyword, Node *id) {
  Node *n = createNode(NT_IMPORT, keyword->line, keyword->character);
  n->data.import.id = id;
  n->data.import.referenced = NULL;
  return n;
}

Node *funDefnNodeCreate(Node *returnType, Node *name, Vector *argTypes,
                        Vector *argNames, Node *body) {
  Node *n = createNode(NT_FUNDEFN, returnType->line, returnType->character);
  n->data.funDefn.returnType = returnType;
  n->data.funDefn.name = name;
  n->data.funDefn.argTypes = argTypes;
  n->data.funDefn.argNames = argNames;
  n->data.funDefn.argStab = hashMapCreate();
  n->data.funDefn.body = body;
  return n;
}
Node *varDefnNodeCreate(Node *type, Vector *names, Vector *initializers) {
  Node *n = createNode(NT_VARDEFN, type->line, type->character);
  n->data.varDefn.type = type;
  n->data.varDefn.names = names;
  n->data.varDefn.initializers = initializers;
  return n;
}

Node *funDeclNodeCreate(Node *returnType, Node *name, Vector *argTypes,
                        Vector *argNames) {
  Node *n = createNode(NT_FUNDECL, returnType->line, returnType->character);
  n->data.funDecl.returnType = returnType;
  n->data.funDecl.name = name;
  n->data.funDecl.argTypes = argTypes;
  n->data.funDecl.argNames = argNames;
  return n;
}
Node *varDeclNodeCreate(Node *type, Vector *names) {
  Node *n = createNode(NT_VARDECL, type->line, type->character);
  n->data.varDecl.type = type;
  n->data.varDecl.names = names;
  return n;
}
Node *opaqueDeclNodeCreate(Token const *keyword, Node *name) {
  Node *n = createNode(NT_OPAQUEDECL, keyword->line, keyword->character);
  n->data.opaqueDecl.name = name;
  return n;
}
Node *structDeclNodeCreate(Token const *keyword, Node *name, Vector *fields) {
  Node *n = createNode(NT_STRUCTDECL, keyword->line, keyword->character);
  n->data.structDecl.name = name;
  n->data.structDecl.fields = fields;
  return n;
}
Node *unionDeclNodeCreate(Token const *keyword, Node *name, Vector *options) {
  Node *n = createNode(NT_UNIONDECL, keyword->line, keyword->character);
  n->data.unionDecl.name = name;
  n->data.unionDecl.options = options;
  return n;
}
Node *enumDeclNodeCreate(Token const *keyword, Node *name,
                         Vector *constantNames, Vector *constantValues) {
  Node *n = createNode(NT_ENUMDECL, keyword->line, keyword->character);
  n->data.enumDecl.name = name;
  n->data.enumDecl.constantNames = constantNames;
  n->data.enumDecl.constantValues = constantValues;
  return n;
}
Node *typedefDeclNodeCreate(Token const *keyword, Node *originalType,
                            Node *name) {
  Node *n = createNode(NT_TYPEDEFDECL, keyword->line, keyword->character);
  n->data.typedefDecl.originalType = originalType;
  n->data.typedefDecl.name = name;
  return n;
}

Node *compoundStmtNodeCreate(Token const *lbrace, Vector *stmts,
                             HashMap *stab) {
  Node *n = createNode(NT_COMPOUNDSTMT, lbrace->line, lbrace->character);
  n->data.compoundStmt.stab = stab;
  n->data.compoundStmt.stmts = stmts;
  return n;
}
Node *ifStmtNodeCreate(Token const *keyword, Node *predicate, Node *consequent,
                       HashMap *consequentStab, Node *alternative,
                       HashMap *alternativeStab) {
  Node *n = createNode(NT_IFSTMT, keyword->line, keyword->character);
  n->data.ifStmt.predicate = predicate;
  n->data.ifStmt.consequent = consequent;
  n->data.ifStmt.consequentStab = consequentStab;
  n->data.ifStmt.alternative = alternative;
  n->data.ifStmt.alternativeStab = alternativeStab;
  return n;
}
Node *whileStmtNodeCreate(Token const *keyword, Node *condition, Node *body,
                          HashMap *bodyStab) {
  Node *n = createNode(NT_WHILESTMT, keyword->line, keyword->character);
  n->data.whileStmt.condition = condition;
  n->data.whileStmt.body = body;
  n->data.whileStmt.bodyStab = bodyStab;
  return n;
}
Node *doWhileStmtNodeCreate(Token const *keyword, Node *body, HashMap *bodyStab,
                            Node *condition) {
  Node *n = createNode(NT_DOWHILESTMT, keyword->line, keyword->character);
  n->data.doWhileStmt.body = body;
  n->data.doWhileStmt.bodyStab = bodyStab;
  n->data.doWhileStmt.condition = condition;
  return n;
}
Node *forStmtNodeCreate(Token const *keyword, HashMap *loopStab,
                        Node *initializer, Node *condition, Node *increment,
                        Node *body, HashMap *bodyStab) {
  Node *n = createNode(NT_FORSTMT, keyword->line, keyword->character);
  n->data.forStmt.loopStab = loopStab;
  n->data.forStmt.initializer = initializer;
  n->data.forStmt.condition = condition;
  n->data.forStmt.increment = increment;
  n->data.forStmt.body = body;
  n->data.forStmt.bodyStab = bodyStab;
  return n;
}
Node *switchStmtNodeCreate(Token const *keyword, Node *condition,
                           Vector *cases) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  n->data.switchStmt.condition = condition;
  n->data.switchStmt.cases = cases;
  return n;
}
Node *breakStmtNodeCreate(Token const *keyword) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  return n;
}
Node *continueStmtNodeCreate(Token const *keyword) {
  Node *n = createNode(NT_CONTINUESTMT, keyword->line, keyword->character);
  return n;
}
Node *returnStmtNodeCreate(Token const *keyword, Node *value) {
  Node *n = createNode(NT_RETURNSTMT, keyword->line, keyword->character);
  n->data.returnStmt.value = value;
  return n;
}
Node *asmStmtNodeCreate(Token const *keyword, Node *assembly) {
  Node *n = createNode(NT_ASMSTMT, keyword->line, keyword->character);
  n->data.asmStmt.assembly = assembly;
  return n;
}
Node *varDefnStmtNodeCreate(Node *type, Vector *names, Vector *initializers) {
  Node *n = createNode(NT_VARDEFNSTMT, type->line, type->character);
  n->data.varDefnStmt.type = type;
  n->data.varDefnStmt.names = names;
  n->data.varDefnStmt.initializers = initializers;
  return n;
}
Node *expressionStmtNodeCreate(Node *expression) {
  Node *n =
      createNode(NT_EXPRESSIONSTMT, expression->line, expression->character);
  n->data.expressionStmt.expression = expression;
  return n;
}
Node *nullStmtNodeCreate(Token const *semicolon) {
  Node *n = createNode(NT_NULLSTMT, semicolon->line, semicolon->character);
  return n;
}

Node *switchCaseNodeCreate(Token const *keyword, Vector *values, Node *body,
                           HashMap *bodyStab) {
  Node *n = createNode(NT_SWITCHCASE, keyword->line, keyword->character);
  n->data.switchCase.values = values;
  n->data.switchCase.body = body;
  n->data.switchCase.bodyStab = bodyStab;
  return n;
}
Node *switchDefaultNodeCreate(Token const *keyword, Node *body,
                              HashMap *bodyStab) {
  Node *n = createNode(NT_SWITCHDEFAULT, keyword->line, keyword->character);
  n->data.switchDefault.body = body;
  n->data.switchDefault.bodyStab = bodyStab;
  return n;
}

Node *binOpExpNodeCreate(BinOpType op, Node *lhs, Node *rhs) {
  Node *n = createNode(NT_BINOPEXP, lhs->line, lhs->character);
  n->data.binOpExp.op = op;
  n->data.binOpExp.lhs = lhs;
  n->data.binOpExp.rhs = rhs;
  return n;
}
Node *castExpNodeCreate(Token const *opToken, Node *type, Node *target) {
  Node *n = createNode(NT_BINOPEXP, opToken->line, opToken->character);
  n->data.binOpExp.op = BO_CAST;
  n->data.binOpExp.lhs = type;
  n->data.binOpExp.rhs = target;
  return n;
}
Node *ternaryExpNodeCreate(Node *predicate, Node *consequent,
                           Node *alternative) {
  Node *n = createNode(NT_TERNARYEXP, predicate->line, predicate->character);
  n->data.ternaryExp.predicate = predicate;
  n->data.ternaryExp.consequent = consequent;
  n->data.ternaryExp.alternative = alternative;
  return n;
}
Node *prefixUnOpExpNodeCreate(UnOpType op, Token const *opToken, Node *target) {
  Node *n = createNode(NT_UNOPEXP, opToken->line, opToken->character);
  n->data.unOpExp.op = op;
  n->data.unOpExp.target = target;
  return n;
}
Node *postfixUnOpExpNodeCreate(UnOpType op, Node *target) {
  Node *n = createNode(NT_UNOPEXP, target->line, target->character);
  n->data.unOpExp.op = op;
  n->data.unOpExp.target = target;
  return n;
}
Node *funCallExpNodeCreate(Node *function, Vector *arguments) {
  Node *n = createNode(NT_FUNCALLEXP, function->line, function->character);
  n->data.funCallExp.function = function;
  n->data.funCallExp.arguments = arguments;
  return n;
}

Node *literalNodeCreate(LiteralType type, Token const *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = type;
  return n;
}
Node *charLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_CHAR;
  char *string = t->string;

  if (string[0] == '\\') {
    // escape sequence
    switch (string[1]) {
      case 'n': {
        n->data.literal.data.charVal = charToU8('\n');
        break;
      }
      case 'r': {
        n->data.literal.data.charVal = charToU8('\r');
        break;
      }
      case 't': {
        n->data.literal.data.charVal = charToU8('\t');
        break;
      }
      case '0': {
        n->data.literal.data.charVal = charToU8('\0');
        break;
      }
      case '\\': {
        n->data.literal.data.charVal = charToU8('\\');
        break;
      }
      case 'x': {
        n->data.literal.data.charVal = (uint8_t)((nybbleToU8(string[2]) << 4) +
                                                 (nybbleToU8(string[3]) << 0));
        break;
      }
      case '\'': {
        n->data.literal.data.charVal = charToU8('\'');
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "bad char literal string passed to charLiteralNodeCreate");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.data.charVal = charToU8(string[0]);
  }

  tokenUninit(t);
  return n;
}
Node *wcharLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_WCHAR;
  char *string = t->string;

  if (string[0] == '\\') {
    // escape sequence
    switch (string[1]) {
      case 'n': {
        n->data.literal.data.wcharVal = charToU8('\n');
        break;
      }
      case 'r': {
        n->data.literal.data.wcharVal = charToU8('\r');
        break;
      }
      case 't': {
        n->data.literal.data.wcharVal = charToU8('\t');
        break;
      }
      case '0': {
        n->data.literal.data.wcharVal = charToU8('\0');
        break;
      }
      case '\\': {
        n->data.literal.data.wcharVal = charToU8('\\');
        break;
      }
      case 'x': {
        n->data.literal.data.wcharVal = (uint8_t)((nybbleToU8(string[2]) << 4) +
                                                  (nybbleToU8(string[3]) << 0));
        break;
      }
      case 'u': {
        n->data.literal.data.wcharVal = 0;
        for (size_t idx = 0; idx < 8; ++idx) {
          n->data.literal.data.wcharVal <<= 4;
          n->data.literal.data.wcharVal += nybbleToU8(string[idx + 2]);
        }
        break;
      }
      case '\'': {
        n->data.literal.data.wcharVal = charToU8('\'');
        break;
      }
      default: {
        error(__FILE__, __LINE__,
              "bad wchar literal string passed to wcharLiteralNodeCreate");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.data.wcharVal = charToU8(string[0]);
  }

  tokenUninit(t);
  return n;
}
Node *stringLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_STRING;

  TStringBuilder sb;
  tstringBuilderInit(&sb);
  for (char *string = t->string; *string != '\0'; ++string) {
    if (*string == '\\') {
      ++string;
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
                "bad string literal string passed to stringLiteralNodeCreate");
        }
      }
    } else {
      // not an escape statement
      tstringBuilderPush(&sb, charToU8(*string));
    }
  }

  n->data.literal.data.stringVal = tstringBuilderData(&sb);
  tstringBuilderUninit(&sb);
  tokenUninit(t);
  return n;
}
Node *wstringLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_WSTRING;

  TWStringBuilder sb;
  twstringBuilderInit(&sb);
  for (char *string = t->string; *string != '\0'; ++string) {
    if (*string == '\\') {
      ++string;
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
          for (size_t idx = 0; idx < 8; ++idx, ++string) {
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
              "bad wstring literal string passed to wstringLiteralNodeCreate");
        }
      }
    } else {
      // not an escape statement
      twstringBuilderPush(&sb, charToU8(*string));
    }
  }

  n->data.literal.data.wstringVal = twstringBuilderData(&sb);
  twstringBuilderUninit(&sb);
  tokenUninit(t);
  return n;
}
Node *sizedIntegerLiteralNodeCreate(Token *t, int8_t sign, uint64_t magnitude) {
  if (sign == 0) {
    // is unsigned
    if (magnitude <= UBYTE_MAX) {
      Node *n = literalNodeCreate(LT_UBYTE, t);
      n->data.literal.data.ubyteVal = (uint8_t)magnitude;
      return n;
    } else if (magnitude <= USHORT_MAX) {
      Node *n = literalNodeCreate(LT_USHORT, t);
      n->data.literal.data.ushortVal = (uint16_t)magnitude;
      return n;
    } else if (magnitude <= UINT_MAX) {
      Node *n = literalNodeCreate(LT_UINT, t);
      n->data.literal.data.uintVal = (uint32_t)magnitude;
      return n;
    } else if (magnitude <= ULONG_MAX) {
      Node *n = literalNodeCreate(LT_ULONG, t);
      n->data.literal.data.ulongVal = (uint64_t)magnitude;
      return n;
    } else {
      error(__FILE__, __LINE__,
            "magnitude larger than ULONG_MAX passed to "
            "sizedIntegerLiteralNodeCreate (CPU bug?)");
    }
  } else if (sign == -1) {
    // negative
    if (magnitude <= BYTE_MIN) {
      Node *n = literalNodeCreate(LT_BYTE, t);
      n->data.literal.data.byteVal = (int8_t)-magnitude;
      return n;
    } else if (magnitude <= SHORT_MIN) {
      Node *n = literalNodeCreate(LT_SHORT, t);
      n->data.literal.data.shortVal = (int16_t)-magnitude;
      return n;
    } else if (magnitude <= INT_MIN) {
      Node *n = literalNodeCreate(LT_INT, t);
      n->data.literal.data.intVal = (int32_t)-magnitude;
      return n;
    } else if (magnitude <= LONG_MIN) {
      Node *n = literalNodeCreate(LT_LONG, t);
      n->data.literal.data.longVal = (int64_t)-magnitude;
      return n;
    } else {
      // user-side size error
      tokenUninit(t);
      return NULL;
    }
  } else if (sign == 1) {
    // positive
    if (magnitude <= BYTE_MAX) {
      Node *n = literalNodeCreate(LT_BYTE, t);
      n->data.literal.data.byteVal = (int8_t)magnitude;
      return n;
    } else if (magnitude <= SHORT_MAX) {
      Node *n = literalNodeCreate(LT_SHORT, t);
      n->data.literal.data.shortVal = (int16_t)magnitude;
      return n;
    } else if (magnitude <= INT_MAX) {
      Node *n = literalNodeCreate(LT_INT, t);
      n->data.literal.data.intVal = (int32_t)magnitude;
      return n;
    } else if (magnitude <= LONG_MAX) {
      Node *n = literalNodeCreate(LT_LONG, t);
      n->data.literal.data.longVal = (int64_t)magnitude;
      return n;
    } else {
      // user-side size error
      tokenUninit(t);
      return NULL;
    }
  } else {
    error(__FILE__, __LINE__,
          "invalid sign passed to sizedIntegerLiteralNodeCreate");
  }
}

Node *keywordTypeNodeCreate(TypeKeyword keyword, Token const *keywordToken) {
  Node *n =
      createNode(NT_KEYWORDTYPE, keywordToken->line, keywordToken->character);
  n->data.keywordType.keyword = keyword;
  return n;
}
Node *modifiedTypeNodeCreate(TypeModifier modifier, Node *baseType) {
  Node *n = createNode(NT_MODIFIEDTYPE, baseType->line, baseType->character);
  n->data.modifiedType.modifier = modifier;
  n->data.modifiedType.baseType = baseType;
  return n;
}
Node *arrayTypeNodeCreate(Node *baseType, Node *size) {
  Node *n = createNode(NT_ARRAYTYPE, baseType->line, baseType->character);
  n->data.arrayType.baseType = baseType;
  n->data.arrayType.size = size;
  return n;
}
Node *funPtrTypeNodeCreate(Node *returnType, Vector *argTypes,
                           Vector *argNames) {
  Node *n = createNode(NT_FUNPTRTYPE, returnType->line, returnType->character);
  n->data.funPtrType.returnType = returnType;
  n->data.funPtrType.argTypes = argTypes;
  n->data.funPtrType.argNames = argNames;
  return n;
}

Node *scopedIdNodeCreate(Vector *components) {
  Node *first = components->elements[0];
  Node *n = createNode(NT_SCOPEDID, first->line, first->character);
  n->data.scopedId.components = components;
  n->data.scopedId.entry = NULL;
  return n;
}
Node *idNodeCreate(Token *id) {
  Node *n = createNode(NT_ID, id->line, id->character);
  n->data.id.id = id->string;
  n->data.id.entry = NULL;
  return n;
}

Node *unparsedNodeCreate(Vector *tokens) {
  Token const *first = tokens->elements[0];
  Node *n = createNode(NT_UNPARSED, first->line, first->character);
  n->data.unparsed.tokens = tokens;
  n->data.unparsed.curr = 0;
  return n;
}

char *stringifyId(Node *id) {
  switch (id->type) {
    case NT_SCOPEDID: {
      Vector const *components = id->data.scopedId.components;
      Node const *first = components->elements[0];
      char *stringified = strdup(first->data.id.id);
      for (size_t idx = 1; idx < components->size; ++idx) {
        char *old = stringified;
        Node const *component = components->elements[idx];
        stringified = format("%s::%s", old, component->data.id.id);
        free(old);
      }

      return stringified;
    }
    case NT_ID: {
      return strdup(id->data.id.id);
    }
    default: {
      error(__FILE__, __LINE__, "attempted to stringify non-id");
    }
  }
}

static void errorNotPositive(Node *n, Environment *env) {
  fprintf(stderr, "%s:%zu:%zu: error: array length must be positive",
          env->currentModuleFile->inputFilename, n->line, n->character);
}
/**
 * gets the uint64_t value of the extended int literal
 *
 * complains to the user if any error occurrs
 *
 * @returns 0 if an error occurred
 */
static uint64_t extendedIntLiteralToValue(Node *n, Environment *env) {
  switch (n->type) {
    case NT_LITERAL: {
      switch (n->data.literal.type) {
        case LT_UBYTE: {
          return n->data.literal.data.ubyteVal;
        }
        case LT_BYTE: {
          if (n->data.literal.data.byteVal <= 0) {
            errorNotPositive(n, env);
            return 0;
          } else {
            return (uint64_t)n->data.literal.data.byteVal;
          }
        }
        case LT_USHORT: {
          return n->data.literal.data.ushortVal;
        }
        case LT_SHORT: {
          if (n->data.literal.data.shortVal <= 0) {
            errorNotPositive(n, env);
            return 0;
          } else {
            return (uint64_t)n->data.literal.data.shortVal;
          }
        }
        case LT_UINT: {
          return n->data.literal.data.uintVal;
        }
        case LT_INT: {
          if (n->data.literal.data.intVal <= 0) {
            errorNotPositive(n, env);
            return 0;
          } else {
            return (uint64_t)n->data.literal.data.intVal;
          }
        }
        case LT_ULONG: {
          return n->data.literal.data.ulongVal;
        }
        case LT_LONG: {
          if (n->data.literal.data.longVal <= 0) {
            errorNotPositive(n, env);
            return 0;
          } else {
            return (uint64_t)n->data.literal.data.longVal;
          }
        }
        case LT_CHAR: {
          return n->data.literal.data.charVal;
        }
        case LT_WCHAR: {
          return n->data.literal.data.wcharVal;
        }
        default: {
          error(__FILE__, __LINE__,
                "bad extended int literal node given to "
                "extendedIntLiteralToValue");
        }
      }
    }
    case NT_SCOPEDID: {
      SymbolTableEntry *enumConst = environmentLookup(env, n, false);
      if (enumConst == NULL) {
        return 0;
      } else if (enumConst->kind != SK_ENUMCONST) {
        fprintf(stderr,
                "%s:%zu:%zu: error: expected an extended integer "
                "literal, found %s\n",
                env->currentModuleFile->inputFilename, n->line, n->character,
                symbolKindToString(enumConst->kind));
        env->currentModuleFile->errored = true;
        return 0;
      }

      if (enumConst->data.enumConst.signedness) {
        // signed - allow only negatives
        if (enumConst->data.enumConst.data.signedValue <= 0) {
          errorNotPositive(n, env);
          return 0;
        } else {
          return (uint64_t)enumConst->data.enumConst.data.signedValue;
        }
      } else {
        return enumConst->data.enumConst.data.unsignedValue;
      }
    }
    default: {
      error(__FILE__, __LINE__,
            "bad extended int literal node given to "
            "extendedIntLiteralToValue");
    }
  }
}
Type *nodeToType(Node *n, Environment *env) {
  switch (n->type) {
    case NT_KEYWORDTYPE: {
      return keywordTypeCreate(n->data.keywordType.keyword);
    }
    case NT_MODIFIEDTYPE: {
      // if it's a volatile type and next thing is a const, produce const
      // volatile instead
      if (n->data.modifiedType.modifier == TM_VOLATILE &&
          n->data.modifiedType.baseType->type == NT_MODIFIEDTYPE &&
          n->data.modifiedType.baseType->data.modifiedType.modifier ==
              TM_CONST) {
        Type *inner = nodeToType(
            n->data.modifiedType.baseType->data.modifiedType.baseType, env);
        if (inner != NULL)
          return modifiedTypeCreate(TM_CONST,
                                    modifiedTypeCreate(TM_VOLATILE, inner));
        else
          return NULL;
      } else {
        Type *inner = nodeToType(n->data.modifiedType.baseType, env);
        if (inner != NULL)
          return modifiedTypeCreate(n->data.modifiedType.modifier, inner);
        else
          return NULL;
      }
    }
    case NT_ARRAYTYPE: {
      Type *base = nodeToType(n->data.arrayType.baseType, env);
      if (base == NULL) return NULL;
      uint64_t length = extendedIntLiteralToValue(n->data.arrayType.size, env);
      if (length == 0) {
        typeFree(base);
        return NULL;
      }
      return arrayTypeCreate(length, base);
    }
    case NT_FUNPTRTYPE: {
      Type *inner = nodeToType(n->data.funPtrType.returnType, env);
      if (inner == NULL) return NULL;

      Type *retval = funPtrTypeCreate(inner);

      for (size_t idx = 0; idx < n->data.funPtrType.argTypes->size; ++idx) {
        Type *argType =
            nodeToType(n->data.funPtrType.argTypes->elements[idx], env);
        if (argType == NULL) {
          typeFree(retval);
          return NULL;
        }

        vectorInsert(&retval->data.funPtr.argTypes, argType);
      }

      return retval;
    }
    case NT_SCOPEDID: {
      SymbolTableEntry *entry = environmentLookup(env, n, false);
      switch (entry->kind) {
        case SK_OPAQUE:
        case SK_STRUCT:
        case SK_UNION:
        case SK_ENUM:
        case SK_TYPEDEF: {
          n->data.scopedId.entry = entry;
          return referenceTypeCreate(entry);
        }
        default: {
          char *idString = stringifyId(n);
          fprintf(stderr, "%s:%zu:%zu: error: '%s' is not a type\n",
                  env->currentModuleFile->inputFilename, n->line, n->character,
                  idString);
          free(idString);
          return NULL;
        }
      }
    }
    case NT_ID: {
      SymbolTableEntry *entry = environmentLookup(env, n, false);
      switch (entry->kind) {
        case SK_OPAQUE:
        case SK_STRUCT:
        case SK_UNION:
        case SK_ENUM:
        case SK_TYPEDEF: {
          n->data.id.entry = entry;
          return referenceTypeCreate(entry);
        }
        default: {
          fprintf(stderr, "%s:%zu:%zu: error: '%s' is not a type\n",
                  env->currentModuleFile->inputFilename, n->line, n->character,
                  n->data.id.id);
          return NULL;
        }
      }
    }
    default: {
      error(__FILE__, __LINE__, "non-type node encountered");
    }
  }
}

bool nameNodeEqual(Node *a, Node *b) {
  if (a->type != b->type) return false;

  if (a->type == NT_ID) {
    return strcmp(a->data.id.id, b->data.id.id) == 0;
  } else {
    if (a->data.scopedId.components->size != b->data.scopedId.components->size)
      return false;

    for (size_t idx = 0; idx < a->data.scopedId.components->size; ++idx) {
      Node *aComponent = a->data.scopedId.components->elements[idx];
      Node *bComponent = b->data.scopedId.components->elements[idx];
      if (strcmp(aComponent->data.id.id, bComponent->data.id.id) != 0)
        return false;
    }
    return true;
  }
}

bool nameNodeEqualWithDrop(Node *a, Node *b, size_t dropCount) {
  size_t compareLength = b->data.scopedId.components->size - dropCount;

  if (a->type == NT_ID) {
    if (compareLength == 1) {
      Node *first = b->data.scopedId.components->elements[0];
      return strcmp(a->data.id.id, first->data.id.id) == 0;
    } else {
      return false;
    }
  } else {
    if (a->data.scopedId.components->size != compareLength) return false;

    for (size_t idx = 0; idx < compareLength; ++idx) {
      Node *aComponent = a->data.scopedId.components->elements[idx];
      Node *bComponent = b->data.scopedId.components->elements[idx];
      if (strcmp(aComponent->data.id.id, bComponent->data.id.id) != 0)
        return false;
    }
    return true;
  }
}

static void nullableTokenFree(Token *token) {
  if (token != NULL) {
    tokenUninit(token);
    free(token);
  }
}
void nodeFree(Node *n) {
  if (n == NULL) return;
  switch (n->type) {
    case NT_FILE: {
      stabFree(n->data.file.stab);
      nodeFree(n->data.file.module);
      nodeVectorFree(n->data.file.imports);
      nodeVectorFree(n->data.file.bodies);
      break;
    }
    case NT_MODULE: {
      nodeFree(n->data.file.module);
      break;
    }
    case NT_IMPORT: {
      nodeFree(n->data.file.module);
      break;
    }
    case NT_FUNDEFN: {
      nodeFree(n->data.funDefn.returnType);
      nodeFree(n->data.funDefn.name);
      nodeVectorFree(n->data.funDefn.argTypes);
      nodeVectorFree(n->data.funDefn.argNames);
      stabFree(n->data.funDefn.argStab);
      nodeFree(n->data.funDefn.body);
      break;
    }
    case NT_VARDEFN: {
      nodeFree(n->data.varDefn.type);
      nodeVectorFree(n->data.varDefn.names);
      nodeVectorFree(n->data.varDefn.initializers);
      break;
    }
    case NT_FUNDECL: {
      nodeFree(n->data.funDecl.returnType);
      nodeFree(n->data.funDecl.name);
      nodeVectorFree(n->data.funDecl.argTypes);
      nodeVectorFree(n->data.funDecl.argNames);
      break;
    }
    case NT_VARDECL: {
      nodeFree(n->data.varDecl.type);
      nodeVectorFree(n->data.varDecl.names);
      break;
    }
    case NT_OPAQUEDECL: {
      nodeFree(n->data.opaqueDecl.name);
      break;
    }
    case NT_STRUCTDECL: {
      nodeFree(n->data.structDecl.name);
      nodeVectorFree(n->data.structDecl.fields);
      break;
    }
    case NT_UNIONDECL: {
      nodeFree(n->data.unionDecl.name);
      nodeVectorFree(n->data.unionDecl.options);
      break;
    }
    case NT_ENUMDECL: {
      nodeFree(n->data.enumDecl.name);
      nodeVectorFree(n->data.enumDecl.constantNames);
      nodeVectorFree(n->data.enumDecl.constantValues);
      break;
    }
    case NT_TYPEDEFDECL: {
      nodeFree(n->data.typedefDecl.originalType);
      nodeFree(n->data.typedefDecl.name);
      break;
    }
    case NT_COMPOUNDSTMT: {
      nodeVectorFree(n->data.compoundStmt.stmts);
      stabFree(n->data.compoundStmt.stab);
      break;
    }
    case NT_IFSTMT: {
      nodeFree(n->data.ifStmt.predicate);
      nodeFree(n->data.ifStmt.consequent);
      stabFree(n->data.ifStmt.consequentStab);
      nodeFree(n->data.ifStmt.alternative);
      stabFree(n->data.ifStmt.alternativeStab);
      break;
    }
    case NT_WHILESTMT: {
      nodeFree(n->data.whileStmt.condition);
      nodeFree(n->data.whileStmt.body);
      stabFree(n->data.whileStmt.bodyStab);
      break;
    }
    case NT_DOWHILESTMT: {
      nodeFree(n->data.doWhileStmt.body);
      nodeFree(n->data.doWhileStmt.condition);
      stabFree(n->data.doWhileStmt.bodyStab);
      break;
    }
    case NT_FORSTMT: {
      stabFree(n->data.forStmt.loopStab);
      nodeFree(n->data.forStmt.initializer);
      nodeFree(n->data.forStmt.condition);
      nodeFree(n->data.forStmt.increment);
      nodeFree(n->data.forStmt.body);
      stabFree(n->data.forStmt.bodyStab);
      break;
    }
    case NT_SWITCHSTMT: {
      nodeFree(n->data.switchStmt.condition);
      nodeVectorFree(n->data.switchStmt.cases);
      break;
    }
    case NT_BREAKSTMT: {
      break;
    }
    case NT_CONTINUESTMT: {
      break;
    }
    case NT_RETURNSTMT: {
      nodeFree(n->data.returnStmt.value);
      break;
    }
    case NT_ASMSTMT: {
      nodeFree(n->data.asmStmt.assembly);
      break;
    }
    case NT_VARDEFNSTMT: {
      nodeFree(n->data.varDefnStmt.type);
      nodeVectorFree(n->data.varDefnStmt.names);
      nodeVectorFree(n->data.varDefnStmt.initializers);
      break;
    }
    case NT_EXPRESSIONSTMT: {
      nodeFree(n->data.expressionStmt.expression);
      break;
    }
    case NT_NULLSTMT: {
      break;
    }
    case NT_SWITCHCASE: {
      nodeVectorFree(n->data.switchCase.values);
      nodeFree(n->data.switchCase.body);
      break;
    }
    case NT_SWITCHDEFAULT: {
      nodeFree(n->data.switchDefault.body);
      break;
    }
    case NT_BINOPEXP: {
      nodeFree(n->data.binOpExp.lhs);
      nodeFree(n->data.binOpExp.rhs);
      break;
    }
    case NT_TERNARYEXP: {
      nodeFree(n->data.ternaryExp.predicate);
      nodeFree(n->data.ternaryExp.consequent);
      nodeFree(n->data.ternaryExp.alternative);
      break;
    }
    case NT_UNOPEXP: {
      nodeFree(n->data.unOpExp.target);
      break;
    }
    case NT_FUNCALLEXP: {
      nodeFree(n->data.funCallExp.function);
      nodeVectorFree(n->data.funCallExp.arguments);
      break;
    }
    case NT_LITERAL: {
      switch (n->data.literal.type) {
        case LT_STRING: {
          free(n->data.literal.data.stringVal);
          break;
        }
        case LT_WSTRING: {
          free(n->data.literal.data.wstringVal);
          break;
        }
        case LT_AGGREGATEINIT: {
          nodeVectorFree(n->data.literal.data.aggregateInitVal);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    case NT_KEYWORDTYPE: {
      break;
    }
    case NT_MODIFIEDTYPE: {
      nodeFree(n->data.modifiedType.baseType);
      break;
    }
    case NT_ARRAYTYPE: {
      nodeFree(n->data.arrayType.baseType);
      nodeFree(n->data.arrayType.size);
      break;
    }
    case NT_FUNPTRTYPE: {
      nodeFree(n->data.funPtrType.returnType);
      nodeVectorFree(n->data.funPtrType.argTypes);
      nodeVectorFree(n->data.funPtrType.argNames);
      break;
    }
    case NT_SCOPEDID: {
      nodeVectorFree(n->data.scopedId.components);
      break;
    }
    case NT_ID: {
      free(n->data.id.id);
      break;
    }
    case NT_UNPARSED: {
      vectorUninit(n->data.unparsed.tokens,
                   (void (*)(void *))nullableTokenFree);
      free(n->data.unparsed.tokens);
      break;
    }
  }
  free(n);
}

void nodeVectorFree(Vector *v) {
  vectorUninit(v, (void (*)(void *))nodeFree);
  free(v);
}