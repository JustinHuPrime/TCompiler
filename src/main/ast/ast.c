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

#include "internalError.h"
#include "lexer/lexer.h"
#include "numericSizing.h"
#include "util/container/stringBuilder.h"
#include "util/conversions.h"
#include "util/format.h"

#include <stdlib.h>
#include <string.h>

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
  n->data.file.module = module;
  n->data.file.imports = imports;
  n->data.file.bodies = bodies;
  hashMapInit(&n->data.file.stab);
  return n;
}
Node *moduleNodeCreate(Token *keyword, Node *id) {
  Node *n = createNode(NT_MODULE, keyword->line, keyword->character);
  n->data.module.id = id;
  return n;
}
Node *importNodeCreate(Token *keyword, Node *id) {
  Node *n = createNode(NT_IMPORT, keyword->line, keyword->character);
  n->data.import.id = id;
  n->data.import.referenced = NULL;
  return n;
}

Node *funDefnNodeCreate(Node *returnType, Node *name, Vector *argTypes,
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
Node *varDefnNodeCreate(Node *type, Vector *names, Vector *initializers) {
  Node *n = createNode(NT_VARDEFN, type->line, type->character);
  n->data.varDefn.type = type;
  n->data.varDefn.names = names;
  n->data.varDefn.initializers = initializers;
  return n;
}

Node *funDeclNodeCreate(Node *returnType, Node *name, Vector *argTypes,
                        Vector *argNames, Vector *argDefaults) {
  Node *n = createNode(NT_FUNDECL, returnType->line, returnType->character);
  n->data.funDecl.returnType = returnType;
  n->data.funDecl.name = name;
  n->data.funDecl.argTypes = argTypes;
  n->data.funDecl.argNames = argNames;
  n->data.funDecl.argDefaults = argDefaults;
  return n;
}
Node *varDeclNodeCreate(Node *type, Vector *names) {
  Node *n = createNode(NT_VARDECL, type->line, type->character);
  n->data.varDecl.type = type;
  n->data.varDecl.names = names;
  return n;
}
Node *opaqueDeclNodeCreate(Token *keyword, Node *name) {
  Node *n = createNode(NT_OPAQUEDECL, keyword->line, keyword->character);
  n->data.opaqueDecl.name = name;
  return n;
}
Node *structDeclNodeCreate(Token *keyword, Node *name, Vector *fields) {
  Node *n = createNode(NT_STRUCTDECL, keyword->line, keyword->character);
  n->data.structDecl.name = name;
  n->data.structDecl.fields = fields;
  return n;
}
Node *unionDeclNodeCreate(Token *keyword, Node *name, Vector *options) {
  Node *n = createNode(NT_UNIONDECL, keyword->line, keyword->character);
  n->data.unionDecl.name = name;
  n->data.unionDecl.options = options;
  return n;
}
Node *enumDeclNodeCreate(Token *keyword, Node *name, Vector *constantNames,
                         Vector *constantValues) {
  Node *n = createNode(NT_ENUMDECL, keyword->line, keyword->character);
  n->data.enumDecl.name = name;
  n->data.enumDecl.constantNames = constantNames;
  n->data.enumDecl.constantValues = constantValues;
  return n;
}
Node *typedefDeclNodeCreate(Token *keyword, Node *originalType, Node *name) {
  Node *n = createNode(NT_TYPEDEFDECL, keyword->line, keyword->character);
  n->data.typedefDecl.originalType = originalType;
  n->data.typedefDecl.name = name;
  return n;
}

Node *compoundStmtNodeCreate(Token *lbrace, Vector *stmts) {
  Node *n = createNode(NT_COMPOUNDSTMT, lbrace->line, lbrace->character);
  n->data.compoundStmt.stmts = stmts;
  hashMapInit(&n->data.compoundStmt.stab);
  return n;
}
Node *ifStmtNodeCreate(Token *keyword, Node *predicate, Node *consequent,
                       Node *alternative) {
  Node *n = createNode(NT_IFSTMT, keyword->line, keyword->character);
  n->data.ifStmt.predicate = predicate;
  n->data.ifStmt.consequent = consequent;
  n->data.ifStmt.alternative = alternative;
  return n;
}
Node *whileStmtNodeCreate(Token *keyword, Node *condition, Node *body) {
  Node *n = createNode(NT_WHILESTMT, keyword->line, keyword->character);
  n->data.whileStmt.condition = condition;
  n->data.whileStmt.body = body;
  return n;
}
Node *doWhileStmtNodeCreate(Token *keyword, Node *body, Node *condition) {
  Node *n = createNode(NT_DOWHILESTMT, keyword->line, keyword->character);
  n->data.doWhileStmt.body = body;
  n->data.doWhileStmt.condition = condition;
  return n;
}
Node *forStmtNodeCreate(Token *keyword, Node *initializer, Node *condition,
                        Node *increment, Node *body) {
  Node *n = createNode(NT_FORSTMT, keyword->line, keyword->character);
  n->data.forStmt.initializer = initializer;
  n->data.forStmt.condition = condition;
  n->data.forStmt.increment = increment;
  n->data.forStmt.body = body;
  hashMapInit(&n->data.forStmt.stab);
  return n;
}
Node *switchStmtNodeCreate(Token *keyword, Node *condition, Vector *cases) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  n->data.switchStmt.condition = condition;
  n->data.switchStmt.cases = cases;
  return n;
}
Node *breakStmtNodeCreate(Token *keyword) {
  Node *n = createNode(NT_SWITCHSTMT, keyword->line, keyword->character);
  return n;
}
Node *continueStmtNodeCreate(Token *keyword) {
  Node *n = createNode(NT_CONTINUESTMT, keyword->line, keyword->character);
  return n;
}
Node *returnStmtNodeCreate(Token *keyword, Node *value) {
  Node *n = createNode(NT_RETURNSTMT, keyword->line, keyword->character);
  n->data.returnStmt.value = value;
  return n;
}
Node *asmStmtNodeCreate(Token *keyword, Node *assembly) {
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
Node *nullStmtNodeCreate(Token *semicolon) {
  Node *n = createNode(NT_NULLSTMT, semicolon->line, semicolon->character);
  return n;
}

Node *switchCaseNodeCreate(Token *keyword, Vector *values, Node *body) {
  Node *n = createNode(NT_SWITCHCASE, keyword->line, keyword->character);
  n->data.switchCase.values = values;
  n->data.switchCase.body = body;
  return n;
}
Node *switchDefaultNodeCreate(Token *keyword, Node *body) {
  Node *n = createNode(NT_SWITCHDEFAULT, keyword->line, keyword->character);
  n->data.switchDefault.body = body;
  return n;
}

Node *binOpExpNodeCreate(BinOpType op, Node *lhs, Node *rhs) {
  Node *n = createNode(NT_BINOPEXP, lhs->line, lhs->character);
  n->data.binOpExp.op = op;
  n->data.binOpExp.lhs = lhs;
  n->data.binOpExp.rhs = rhs;
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
Node *prefixUnOpExpNodeCreate(UnOpType op, Token *opToken, Node *target) {
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

Node *literalNodeCreate(LiteralType type, Token *t) {
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
              "bad char literal string passed to charLiteralNodeCreate");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.value.charVal = charToU8(string[0]);
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
              "bad wchar literal string passed to wcharLiteralNodeCreate");
      }
    }
  } else {
    // not an escape statement
    n->data.literal.value.wcharVal = charToU8(string[0]);
  }

  tokenUninit(t);
  return n;
}
Node *stringLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_STRING;

  TStringBuilder sb;
  tstringBuilderInit(&sb);
  for (char *string = t->string; *string != '\0'; string++) {
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
                "bad string literal string passed to stringLiteralNodeCreate");
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
Node *wstringLiteralNodeCreate(Token *t) {
  Node *n = createNode(NT_LITERAL, t->line, t->character);
  n->data.literal.type = LT_WSTRING;

  TWStringBuilder sb;
  twstringBuilderInit(&sb);
  for (char *string = t->string; *string != '\0'; string++) {
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
              "bad wstring literal string passed to wstringLiteralNodeCreate");
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
Node *sizedIntegerLiteralNodeCreate(Token *t, int8_t sign, uint64_t magnitude) {
  if (sign == 0) {
    // is unsigned
    if (magnitude <= UBYTE_MAX) {
      Node *n = literalNodeCreate(LT_UBYTE, t);
      n->data.literal.value.ubyteVal = (uint8_t)magnitude;
      return n;
    } else if (magnitude <= USHORT_MAX) {
      Node *n = literalNodeCreate(LT_USHORT, t);
      n->data.literal.value.ushortVal = (uint16_t)magnitude;
      return n;
    } else if (magnitude <= UINT_MAX) {
      Node *n = literalNodeCreate(LT_UINT, t);
      n->data.literal.value.uintVal = (uint32_t)magnitude;
      return n;
    } else if (magnitude <= ULONG_MAX) {
      Node *n = literalNodeCreate(LT_ULONG, t);
      n->data.literal.value.ulongVal = (uint64_t)magnitude;
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
      n->data.literal.value.byteVal = (int8_t)-magnitude;
      return n;
    } else if (magnitude <= SHORT_MIN) {
      Node *n = literalNodeCreate(LT_SHORT, t);
      n->data.literal.value.shortVal = (int16_t)-magnitude;
      return n;
    } else if (magnitude <= INT_MIN) {
      Node *n = literalNodeCreate(LT_INT, t);
      n->data.literal.value.intVal = (int32_t)-magnitude;
      return n;
    } else if (magnitude <= LONG_MIN) {
      Node *n = literalNodeCreate(LT_LONG, t);
      n->data.literal.value.longVal = (int64_t)-magnitude;
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
      n->data.literal.value.byteVal = (int8_t)magnitude;
      return n;
    } else if (magnitude <= SHORT_MAX) {
      Node *n = literalNodeCreate(LT_SHORT, t);
      n->data.literal.value.shortVal = (int16_t)magnitude;
      return n;
    } else if (magnitude <= INT_MAX) {
      Node *n = literalNodeCreate(LT_INT, t);
      n->data.literal.value.intVal = (int32_t)magnitude;
      return n;
    } else if (magnitude <= LONG_MAX) {
      Node *n = literalNodeCreate(LT_LONG, t);
      n->data.literal.value.longVal = (int64_t)magnitude;
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

Node *keywordTypeNodeCreate(TypeKeyword keyword, Token *keywordToken) {
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
  return n;
}

char *stringifyId(Node *id) {
  switch (id->type) {
    case NT_SCOPEDID: {
      Vector const *components = id->data.scopedId.components;
      Node const *first = components->elements[0];
      char *stringified = strdup(first->data.id.id);
      for (size_t idx = 1; idx < components->size; idx++) {
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
    default: { error(__FILE__, __LINE__, "attempted to stringify non-id"); }
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
        return modifiedTypeCreate(
            TM_CONST,
            modifiedTypeCreate(
                TM_VOLATILE,
                nodeToType(
                    n->data.modifiedType.baseType->data.modifiedType.baseType,
                    env)));
      } else {
        return modifiedTypeCreate(
            n->data.modifiedType.modifier,
            nodeToType(n->data.modifiedType.baseType, env));
      }
    }
    case NT_ARRAYTYPE: {
      // TODO: write this
    }
    case NT_FUNPTRTYPE: {
      // TODO: write this
    }
    case NT_SCOPEDID: {
      // TODO: write this
    }
    case NT_ID: {
      // TODO: write this
    }
    default: { error(__FILE__, __LINE__, "non-type node encountered"); }
  }
}

bool nameNodeEqual(Node *a, Node *b) {
  if (a->type != b->type) return false;

  if (a->type == NT_ID) {
    return strcmp(a->data.id.id, b->data.id.id) == 0;
  } else {
    if (a->data.scopedId.components->size != b->data.scopedId.components->size)
      return false;

    for (size_t idx = 0; idx < a->data.scopedId.components->size; idx++) {
      char const *aComponent = a->data.scopedId.components->elements[idx];
      char const *bComponent = b->data.scopedId.components->elements[idx];
      if (strcmp(aComponent, bComponent) != 0) return false;
    }
    return true;
  }
}

void nodeUninit(Node *n) {
  if (n == NULL) return;
  switch (n->type) {
    case NT_FILE: {
      stabUninit(&n->data.file.stab);
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
      stabUninit(&n->data.funDefn.stab);
      nodeFree(n->data.funDefn.returnType);
      nodeFree(n->data.funDefn.name);
      nodeVectorFree(n->data.funDefn.argTypes);
      nodeVectorFree(n->data.funDefn.argNames);
      nodeVectorFree(n->data.funDefn.argDefaults);
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
      nodeVectorFree(n->data.funDecl.argDefaults);
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
      stabUninit(&n->data.compoundStmt.stab);
      nodeVectorFree(n->data.compoundStmt.stmts);
      break;
    }
    case NT_IFSTMT: {
      nodeFree(n->data.ifStmt.predicate);
      nodeFree(n->data.ifStmt.consequent);
      nodeFree(n->data.ifStmt.alternative);
      break;
    }
    case NT_WHILESTMT: {
      nodeFree(n->data.whileStmt.condition);
      nodeFree(n->data.whileStmt.body);
      break;
    }
    case NT_DOWHILESTMT: {
      nodeFree(n->data.doWhileStmt.body);
      nodeFree(n->data.doWhileStmt.condition);
      break;
    }
    case NT_FORSTMT: {
      stabUninit(&n->data.forStmt.stab);
      nodeFree(n->data.forStmt.initializer);
      nodeFree(n->data.forStmt.condition);
      nodeFree(n->data.forStmt.increment);
      nodeFree(n->data.forStmt.body);
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
          free(n->data.literal.value.stringVal);
          break;
        }
        case LT_WSTRING: {
          free(n->data.literal.value.wstringVal);
          break;
        }
        case LT_AGGREGATEINIT: {
          nodeVectorFree(n->data.literal.value.aggregateInitVal);
          break;
        }
        default: { break; }
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
      vectorUninit(n->data.unparsed.tokens, (void (*)(void *))tokenUninit);
      free(n->data.unparsed.tokens);
      break;
    }
  }
}

void nodeFree(Node *n) {
  nodeUninit(n);
  free(n);
}

void nodeVectorFree(Vector *v) {
  vectorUninit(v, (void (*)(void *))nodeFree);
  free(v);
}