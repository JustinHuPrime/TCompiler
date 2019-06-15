// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// Implementation of AST nodes

#include "ast/ast.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

// NodeList
NodeList *nodeListCreate(void) { return vectorCreate(); }

void nodeListInsert(NodeList *list, Node *node) { vectorInsert(list, node); }

void nodeListDestroy(NodeList *list) {
  vectorDestroy(list, (void (*)(void *))nodeDestroy);
}

// NodePairList
NodePairList *nodePairListCreate(void) {
  NodePairList *list = malloc(sizeof(NodePairList));
  list->size = 0;
  list->capacity = 1;
  list->firstElements = malloc(sizeof(Node *));
  list->secondElements = malloc(sizeof(Node *));
  return list;
}

void nodePairListInsert(NodePairList *list, Node *first, Node *second) {
  if (list->size == list->capacity) {
    list->capacity *= 2;
    list->firstElements =
        realloc(list->firstElements, list->capacity * sizeof(Node *));
    list->secondElements =
        realloc(list->secondElements, list->capacity * sizeof(Node *));
  }
  list->firstElements[list->size] = first;
  list->secondElements[list->size] = second;
  list->size++;
}

void nodePairListDestroy(NodePairList *list) {
  for (size_t idx = 0; idx < list->size; idx++) {
    nodeDestroy(list->firstElements[idx]);
    nodeDestroy(list->secondElements[idx]);
  }
  free(list->firstElements);
  free(list->secondElements);
  free(list);
}

uint64_t const UBYTE_MAX = 255;
uint64_t const BYTE_MAX = 127;
uint64_t const BYTE_MIN = 128;
uint64_t const UINT_MAX = 4294967295;
uint64_t const INT_MAX = 2147483647;
uint64_t const INT_MIN = 2147483648;
uint64_t const ULONG_MAX = 18446744073709551615UL;
uint64_t const LONG_MAX = 9223372036854775807;
uint64_t const LONG_MIN = 9223372036854775808UL;

// Constructors
static Node *nodeCreate(size_t line, size_t character) {
  Node *node = malloc(sizeof(Node));
  node->line = line;
  node->character = character;
  return node;
}
Node *fileNodeCreate(size_t line, size_t character, Node *module,
                     NodeList *imports, NodeList *bodies,
                     char const *filename) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FILE;
  node->data.file.module = module;
  node->data.file.imports = imports;
  node->data.file.bodies = bodies;
  node->data.file.filename = filename;
  return node;
}

Node *moduleNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_MODULE;
  node->data.module.id = id;
  return node;
}
Node *importNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_IMPORT;
  node->data.import.id = id;
  return node;
}
Node *funDeclNodeCreate(size_t line, size_t character, Node *returnType,
                        Node *id, NodeList *paramTypes) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FUNDECL;
  node->data.funDecl.returnType = returnType;
  node->data.funDecl.id = id;
  node->data.funDecl.paramTypes = paramTypes;
  return node;
}
Node *fieldDeclNodeCreate(size_t line, size_t character, Node *type,
                          NodeList *ids) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FIELDDECL;
  node->data.fieldDecl.type = type;
  node->data.fieldDecl.ids = ids;
  return node;
}
Node *structDeclNodeCreate(size_t line, size_t character, Node *id,
                           NodeList *decls) {
  Node *node = nodeCreate(line, character);
  node->type = NT_STRUCTDECL;
  node->data.structDecl.id = id;
  node->data.structDecl.decls = decls;
  return node;
}
Node *structForwardDeclNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_STRUCTFORWARDDECL;
  node->data.structForwardDecl.id = id;
  return node;
}
Node *unionDeclNodeCreate(size_t line, size_t character, Node *id,
                          NodeList *opts) {
  Node *node = nodeCreate(line, character);
  node->type = NT_UNIONDECL;
  node->data.unionDecl.id = id;
  node->data.unionDecl.opts = opts;
  return node;
}
Node *unionForwardDeclNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_UNIONFORWARDDECL;
  node->data.unionForwardDecl.id = id;
  return node;
}
Node *enumDeclNodeCreate(size_t line, size_t character, Node *id,
                         NodeList *elements) {
  Node *node = nodeCreate(line, character);
  node->type = NT_ENUMDECL;
  node->data.enumDecl.id = id;
  node->data.enumDecl.elements = elements;
  return node;
}
Node *enumForwardDeclNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_ENUMFORWARDDECL;
  node->data.enumForwardDecl.id = id;
  return node;
}
Node *typedefNodeCreate(size_t line, size_t character, Node *type, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_TYPEDEFDECL;
  node->data.typedefDecl.type = type;
  node->data.typedefDecl.id = id;
  return node;
}
Node *functionNodeCreate(size_t line, size_t character, Node *returnType,
                         Node *id, NodePairList *formals, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FUNCTION;
  node->data.function.returnType = returnType;
  node->data.function.id = id;
  node->data.function.formals = formals;
  node->data.function.body = body;
  return node;
}
Node *varDeclNodeCreate(size_t line, size_t character, Node *type,
                        NodePairList *idValuePairs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_VARDECL;
  node->data.varDecl.type = type;
  node->data.varDecl.idValuePairs = idValuePairs;
  return node;
}
Node *compoundStmtNodeCreate(size_t line, size_t character,
                             NodeList *statements) {
  Node *node = nodeCreate(line, character);
  node->type = NT_COMPOUNDSTMT;
  node->data.compoundStmt.statements = statements;
  return node;
}
Node *ifStmtNodeCreate(size_t line, size_t character, Node *condition,
                       Node *thenStmt, Node *elseStmt) {
  Node *node = nodeCreate(line, character);
  node->type = NT_IFSTMT;
  node->data.ifStmt.condition = condition;
  node->data.ifStmt.thenStmt = thenStmt;
  node->data.ifStmt.elseStmt = elseStmt;
  return node;
}
Node *whileStmtNodeCreate(size_t line, size_t character, Node *condition,
                          Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = NT_WHILESTMT;
  node->data.whileStmt.condition = condition;
  node->data.whileStmt.body = body;
  return node;
}
Node *doWhileStmtNodeCreate(size_t line, size_t character, Node *body,
                            Node *condition) {
  Node *node = nodeCreate(line, character);
  node->type = NT_DOWHILESTMT;
  node->data.doWhileStmt.body = body;
  node->data.doWhileStmt.condition = condition;
  return node;
}
Node *forStmtNodeCreate(size_t line, size_t character, Node *initialize,
                        Node *condition, Node *update, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FORSTMT;
  node->data.forStmt.initialize = initialize;
  node->data.forStmt.condition = condition;
  node->data.forStmt.update = update;
  node->data.forStmt.body = body;
  return node;
}
Node *switchStmtNodeCreate(size_t line, size_t character, Node *onWhat,
                           NodeList *cases) {
  Node *node = nodeCreate(line, character);
  node->type = NT_SWITCHSTMT;
  node->data.switchStmt.onWhat = onWhat;
  node->data.switchStmt.cases = cases;
  return node;
}
Node *numCaseNodeCreate(size_t line, size_t character, Node *constVal,
                        Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = NT_NUMCASE;
  node->data.numCase.constVal = constVal;
  node->data.numCase.body = body;
  return node;
}
Node *defaultCaseNodeCreate(size_t line, size_t character, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = NT_DEFAULTCASE;
  node->data.defaultCase.body = body;
  return node;
}
Node *breakStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_BREAKSTMT;
  return node;
}
Node *continueStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CONTINUESTMT;
  return node;
}
Node *returnStmtNodeCreate(size_t line, size_t character, Node *value) {
  Node *node = nodeCreate(line, character);
  node->type = NT_RETURNSTMT;
  node->data.returnStmt.value = value;
  return node;
}
Node *asmStmtNodeCreate(size_t line, size_t character, Node *assembly) {
  Node *node = nodeCreate(line, character);
  node->type = NT_ASMSTMT;
  node->data.asmStmt.assembly = assembly;
  return node;
}
Node *expressionStmtNodeCreate(size_t line, size_t character,
                               Node *expression) {
  Node *node = nodeCreate(line, character);
  node->type = NT_EXPRESSIONSTMT;
  node->data.expressionStmt.expression = expression;
  return node;
}
Node *nullStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_NULLSTMT;
  return node;
}
Node *seqExpNodeCreate(size_t line, size_t character, Node *first,
                       Node *second) {
  Node *node = nodeCreate(line, character);
  node->type = NT_SEQEXP;
  node->data.seqExp.first = first;
  node->data.seqExp.second = second;
  return node;
}
Node *binOpExpNodeCreate(size_t line, size_t character, BinOpType op, Node *lhs,
                         Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_BINOPEXP;
  node->data.binopExp.op = op;
  node->data.binopExp.lhs = lhs;
  node->data.binopExp.rhs = rhs;
  return node;
}
Node *unOpExpNodeCreate(size_t line, size_t character, UnOpType op,
                        Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_UNOPEXP;
  node->data.unOpExp.op = op;
  node->data.unOpExp.target = target;
  return node;
}
Node *compOpExpNodeCreate(size_t line, size_t character, CompOpType op,
                          Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_COMPOPEXP;
  node->data.compOpExp.op = op;
  node->data.compOpExp.lhs = lhs;
  node->data.compOpExp.rhs = rhs;
  return node;
}
Node *landAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                              Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_LANDASSIGNEXP;
  node->data.landAssignExp.lhs = lhs;
  node->data.landAssignExp.rhs = rhs;
  return node;
}
Node *lorAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                             Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_LORASSIGNEXP;
  node->data.lorAssignExp.lhs = lhs;
  node->data.lorAssignExp.rhs = rhs;
  return node;
}
Node *ternaryExpNodeCreate(size_t line, size_t character, Node *condition,
                           Node *thenExp, Node *elseExp) {
  Node *node = nodeCreate(line, character);
  node->type = NT_TERNARYEXP;
  node->data.ternaryExp.condition = condition;
  node->data.ternaryExp.thenExp = thenExp;
  node->data.ternaryExp.elseExp = elseExp;
  return node;
}
Node *landExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_LANDEXP;
  node->data.landExp.lhs = lhs;
  node->data.landExp.rhs = rhs;
  return node;
}
Node *lorExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = NT_LOREXP;
  node->data.lorExp.lhs = lhs;
  node->data.lorExp.rhs = rhs;
  return node;
}
Node *structAccessExpNodeCreate(size_t line, size_t character, Node *base,
                                Node *element) {
  Node *node = nodeCreate(line, character);
  node->type = NT_STRUCTACCESSEXP;
  node->data.structAccessExp.base = base;
  node->data.structAccessExp.element = element;
  return node;
}
Node *structPtrAccessExpNodeCreate(size_t line, size_t character, Node *base,
                                   Node *element) {
  Node *node = nodeCreate(line, character);
  node->type = NT_STRUCTPTRACCESSEXP;
  node->data.structPtrAccessExp.base = base;
  node->data.structPtrAccessExp.element = element;
  return node;
}
Node *fnCallExpNodeCreate(size_t line, size_t character, Node *who,
                          NodeList *args) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FNCALLEXP;
  node->data.fnCallExp.who = who;
  node->data.fnCallExp.args = args;
  return node;
}
Node *idExpNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_IDEXP;
  node->data.idExp.id = id;
  return node;
}

// Constant parsing
typedef enum {
  S_POSITIVE,
  S_NEGATIVE,
  S_UNSIGNED,
} Sign;
static uint8_t charToHex(char c) {
  if (isdigit(c)) {
    return (uint8_t)(c - '0');
  } else if (isupper(c)) {
    return (uint8_t)(c - 'A' + 0xA);
  } else {
    return (uint8_t)(c - 'a' + 0xA);
  }
}
static Node *constExpNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CONSTEXP;
  return node;
}
static Sign constIntExpGetSign(char *constantString) {
  switch (constantString[0]) {
    case '+':
      return S_POSITIVE;
    case '-':
      return S_NEGATIVE;
    default:
      return S_UNSIGNED;
  }
}
static Node *constIntExpSetValue(Node *node, Sign sign, bool overflow,
                                 size_t value) {
  if (overflow) {
    node->data.constExp.type = CT_RANGE_ERROR;
  } else {
    switch (sign) {
      case S_UNSIGNED: {
        if (value <= UBYTE_MAX) {
          node->data.constExp.type = CT_UBYTE;
          node->data.constExp.value.ubyteVal = (uint8_t)value;
        } else if (value <= UINT_MAX) {
          node->data.constExp.type = CT_UINT;
          node->data.constExp.value.uintVal = (uint32_t)value;
        } else if (value <= ULONG_MAX) {
          node->data.constExp.type = CT_ULONG;
          node->data.constExp.value.ulongVal = value;
        } else {
          node->data.constExp.type = CT_RANGE_ERROR;
        }
        break;
      }
      case S_POSITIVE: {
        if (value <= BYTE_MAX) {
          node->data.constExp.type = CT_BYTE;
          node->data.constExp.value.byteVal = (int8_t)value;
        } else if (value <= INT_MAX) {
          node->data.constExp.type = CT_INT;
          node->data.constExp.value.intVal = (int32_t)value;
        } else if (value <= LONG_MAX) {
          node->data.constExp.type = CT_LONG;
          node->data.constExp.value.longVal = (int64_t)value;
        } else {
          node->data.constExp.type = CT_RANGE_ERROR;
        }
        break;
      }
      case S_NEGATIVE: {
        if (value <= BYTE_MIN) {
          node->data.constExp.type = CT_BYTE;
          node->data.constExp.value.byteVal = (int8_t)-value;
        } else if (value <= INT_MIN) {
          node->data.constExp.type = CT_INT;
          node->data.constExp.value.intVal = (int32_t)-value;
        } else if (value <= LONG_MIN) {
          node->data.constExp.type = CT_LONG;
          node->data.constExp.value.longVal = (int64_t)-value;
        } else {
          node->data.constExp.type = CT_RANGE_ERROR;
        }
        break;
      }
    }
  }

  return node;
}
Node *constZeroIntExpNodeCreate(size_t line, size_t character,
                                char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);
  free(constantString);
  return constIntExpSetValue(node, sign, false, 0);
}
Node *constBinaryIntExpNodeCreate(size_t line, size_t character,
                                  char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);

  size_t acc = 0;
  bool overflow = false;

  for (constantString += 2 + (sign == S_UNSIGNED ? 0 : 1);
       *constantString != '\0'; constantString++) {
    size_t oldAcc = acc;

    acc *= 2;
    acc += (uint64_t)(*constantString - '0');

    if (acc < oldAcc) {  // overflowed!
      overflow = true;
      break;
    }
  }

  return constIntExpSetValue(node, sign, overflow, acc);
}
Node *constOctalIntExpNodeCreate(size_t line, size_t character,
                                 char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);

  size_t acc = 0;
  bool overflow = false;

  for (constantString += 1 + (sign == S_UNSIGNED ? 0 : 1);
       *constantString != '\0'; constantString++) {
    size_t oldAcc = acc;

    acc *= 8;
    acc += (uint64_t)(*constantString - '0');

    if (acc < oldAcc) {  // overflowed!
      overflow = true;
      break;
    }
  }

  return constIntExpSetValue(node, sign, overflow, acc);
}
Node *constDecimalIntExpNodeCreate(size_t line, size_t character,
                                   char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);

  size_t acc = 0;
  bool overflow = false;

  for (constantString += 2 + (sign == S_UNSIGNED ? 0 : 1);
       *constantString != '\0'; constantString++) {
    size_t oldAcc = acc;

    acc *= 10;
    acc += (uint64_t)(*constantString - '0');

    if (acc < oldAcc) {  // overflowed!
      overflow = true;
      break;
    }
  }

  return constIntExpSetValue(node, sign, overflow, acc);
}
Node *constHexadecimalIntExpNodeCreate(size_t line, size_t character,
                                       char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);

  size_t acc = 0;
  bool overflow = false;

  for (constantString += 2 + (sign == S_UNSIGNED ? 0 : 1);
       *constantString != '\0'; constantString++) {
    size_t oldAcc = acc;

    acc *= 16;
    acc += charToHex(*constantString);

    if (acc < oldAcc) {  // overflowed!
      overflow = true;
      break;
    }
  }

  return constIntExpSetValue(node, sign, overflow, acc);
}
Node *constFloatExpNodeCreate(size_t line, size_t character,
                              char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  Sign sign = constIntExpGetSign(constantString);

  // TODO: write

  return node;
}
Node *constCharExpNodeCreate(size_t line, size_t character,
                             char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  node->data.constExp.type = CT_CHAR;

  if (constantString[1] == '\\') {  // special character
    switch (constantString[2]) {
      case '\'':
      case '\\': {
        node->data.constExp.value.charVal = (uint8_t)constantString[2];
        break;
      }
      case 'n': {
        node->data.constExp.value.charVal = '\n';
        break;
      }
      case 'r': {
        node->data.constExp.value.charVal = '\r';
        break;
      }
      case 't': {
        node->data.constExp.value.charVal = '\t';
        break;
      }
      case '0': {
        node->data.constExp.value.charVal = '\0';
        break;
      }
      case 'x': {
        node->data.constExp.value.charVal = (uint8_t)(
            (charToHex(constantString[3]) << 4) + charToHex(constantString[4]));
        break;
      }
    }
  } else {
    node->data.constExp.value.charVal = (uint8_t)constantString[1];
  }

  return node;
}
Node *constStringExpNodeCreate(size_t line, size_t character,
                               char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  node->data.constExp.type = CT_STRING;

  // TODO: write

  return node;
}
Node *constWCharExpNodeCreate(size_t line, size_t character,
                              char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  node->data.constExp.type = CT_WCHAR;

  if (constantString[1] == '\\') {  // special character
    switch (constantString[2]) {
      case '\'':
      case '\\': {
        node->data.constExp.value.wcharVal = (uint32_t)constantString[2];
        break;
      }
      case 'n': {
        node->data.constExp.value.wcharVal = '\n';
        break;
      }
      case 'r': {
        node->data.constExp.value.wcharVal = '\r';
        break;
      }
      case 't': {
        node->data.constExp.value.wcharVal = '\t';
        break;
      }
      case '0': {
        node->data.constExp.value.wcharVal = '\0';
        break;
      }
      case 'x': {
        node->data.constExp.value.wcharVal = (uint32_t)(
            (charToHex(constantString[3]) << 4) + charToHex(constantString[4]));
        break;
      }
      case 'u': {
        node->data.constExp.value.wcharVal = 0;
        for (size_t idx = 3; idx < 3 + 8; idx++) {
          node->data.constExp.value.wcharVal <<= 4;
          node->data.constExp.value.wcharVal |=
              (uint32_t)charToHex(constantString[idx]);
        }
      }
    }
  } else {
    node->data.constExp.value.wcharVal = (uint32_t)constantString[1];
  }

  return node;
}
Node *constWStringExpNodeCreate(size_t line, size_t character,
                                char *constantString) {
  Node *node = constExpNodeCreate(line, character);
  node->data.constExp.type = CT_WSTRING;

  // TODO: write

  return node;
}

// More constructors
Node *aggregateInitExpNodeCreate(size_t line, size_t character,
                                 NodeList *elements) {
  Node *node = nodeCreate(line, character);
  node->type = NT_AGGREGATEINITEXP;
  node->data.aggregateInitExp.elements = elements;
  return node;
}
Node *constTrueNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CONSTEXP;
  node->data.constExp.type = CT_BOOL;
  node->data.constExp.value.boolVal = true;
  return node;
}
Node *constFalseNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CONSTEXP;
  node->data.constExp.type = CT_BOOL;
  node->data.constExp.value.boolVal = false;
  return node;
}
Node *castExpNodeCreate(size_t line, size_t character, Node *toWhat,
                        Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CASTEXP;
  node->data.castExp.toWhat = toWhat;
  node->data.castExp.target = target;
  return node;
}
Node *sizeofTypeExpNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_SIZEOFTYPEEXP;
  node->data.sizeofTypeExp.target = target;
  return node;
}
Node *sizeofExpExpNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_SIZEOFEXPEXP;
  node->data.sizeofExpExp.target = target;
  return node;
}
Node *typeKeywordNodeCreate(size_t line, size_t character, TypeKeyword type) {
  Node *node = nodeCreate(line, character);
  node->type = NT_KEYWORDTYPE;
  node->data.typeKeyword.type = type;
  return node;
}
Node *idTypeNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_IDTYPE;
  node->data.idType.id = id;
  return node;
}
Node *constTypeNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_CONSTTYPE;
  node->data.constType.target = target;
  return node;
}
Node *arrayTypeNodeCreate(size_t line, size_t character, Node *element,
                          Node *size) {
  Node *node = nodeCreate(line, character);
  node->type = NT_ARRAYTYPE;
  node->data.arrayType.element = element;
  node->data.arrayType.size = size;
  return node;
}
Node *ptrTypeNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = NT_PTRTYPE;
  node->data.ptrType.target = target;
  return node;
}
Node *fnPtrTypeNodeCreate(size_t line, size_t character, Node *returnType,
                          NodeList *argTypes) {
  Node *node = nodeCreate(line, character);
  node->type = NT_FNPTRTYPE;
  node->data.fnPtrType.returnType = returnType;
  node->data.fnPtrType.argTypes = argTypes;
  return node;
}
Node *idNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = NT_ID;
  node->data.id.id = id;
  return node;
}

// Destructor
void nodeDestroy(Node *node) {
  if (node == NULL) return;  // base case: might be asked to free NULL.

  // free contents
  switch (node->type) {
    case NT_FILE: {
      nodeDestroy(node->data.file.module);
      nodeListDestroy(node->data.file.imports);
      nodeListDestroy(node->data.file.bodies);
      break;
    }
    case NT_MODULE: {
      nodeDestroy(node->data.module.id);
      break;
    }
    case NT_IMPORT: {
      nodeDestroy(node->data.import.id);
      break;
    }
    case NT_FUNDECL: {
      nodeDestroy(node->data.funDecl.returnType);
      nodeDestroy(node->data.funDecl.id);
      nodeListDestroy(node->data.funDecl.paramTypes);
      break;
    }
    case NT_FIELDDECL: {
      nodeDestroy(node->data.fieldDecl.type);
      nodeListDestroy(node->data.fieldDecl.ids);
      break;
    }
    case NT_STRUCTDECL: {
      nodeDestroy(node->data.structDecl.id);
      nodeListDestroy(node->data.structDecl.decls);
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      nodeDestroy(node->data.structForwardDecl.id);
      break;
    }
    case NT_UNIONDECL: {
      nodeDestroy(node->data.unionDecl.id);
      nodeListDestroy(node->data.unionDecl.opts);
      break;
    }
    case NT_UNIONFORWARDDECL: {
      nodeDestroy(node->data.unionForwardDecl.id);
      break;
    }
    case NT_ENUMDECL: {
      nodeDestroy(node->data.enumDecl.id);
      nodeListDestroy(node->data.enumDecl.elements);
      break;
    }
    case NT_ENUMFORWARDDECL: {
      nodeDestroy(node->data.enumForwardDecl.id);
      break;
    }
    case NT_TYPEDEFDECL: {
      nodeDestroy(node->data.typedefDecl.type);
      nodeDestroy(node->data.typedefDecl.id);
      break;
    }
    case NT_FUNCTION: {
      nodeDestroy(node->data.function.returnType);
      nodeDestroy(node->data.function.id);
      nodePairListDestroy(node->data.function.formals);
      nodeDestroy(node->data.function.body);
      break;
    }
    case NT_VARDECL: {
      nodeDestroy(node->data.varDecl.type);
      nodePairListDestroy(node->data.varDecl.idValuePairs);
      break;
    }
    case NT_COMPOUNDSTMT: {
      nodeListDestroy(node->data.compoundStmt.statements);
      break;
    }
    case NT_IFSTMT: {
      nodeDestroy(node->data.ifStmt.condition);
      nodeDestroy(node->data.ifStmt.thenStmt);
      nodeDestroy(node->data.ifStmt.elseStmt);
      break;
    }
    case NT_WHILESTMT: {
      nodeDestroy(node->data.whileStmt.condition);
      nodeDestroy(node->data.whileStmt.body);
      break;
    }
    case NT_DOWHILESTMT: {
      nodeDestroy(node->data.doWhileStmt.body);
      nodeDestroy(node->data.doWhileStmt.condition);
      break;
    }
    case NT_FORSTMT: {
      nodeDestroy(node->data.forStmt.initialize);
      nodeDestroy(node->data.forStmt.condition);
      nodeDestroy(node->data.forStmt.update);
      nodeDestroy(node->data.forStmt.body);
      break;
    }
    case NT_SWITCHSTMT: {
      nodeDestroy(node->data.switchStmt.onWhat);
      nodeListDestroy(node->data.switchStmt.cases);
      break;
    }
    case NT_NUMCASE: {
      nodeDestroy(node->data.numCase.constVal);
      nodeDestroy(node->data.numCase.body);
      break;
    }
    case NT_DEFAULTCASE: {
      nodeDestroy(node->data.defaultCase.body);
      break;
    }
    case NT_BREAKSTMT:
      break;
    case NT_CONTINUESTMT:
      break;
    case NT_RETURNSTMT: {
      nodeDestroy(node->data.returnStmt.value);
      break;
    }
    case NT_ASMSTMT: {
      nodeDestroy(node->data.asmStmt.assembly);
      break;
    }
    case NT_EXPRESSIONSTMT: {
      nodeDestroy(node->data.expressionStmt.expression);
      break;
    }
    case NT_NULLSTMT:
      break;
    case NT_SEQEXP: {
      nodeDestroy(node->data.seqExp.first);
      nodeDestroy(node->data.seqExp.second);
      break;
    }
    case NT_BINOPEXP: {
      nodeDestroy(node->data.binopExp.lhs);
      nodeDestroy(node->data.binopExp.rhs);
      break;
    }
    case NT_UNOPEXP: {
      nodeDestroy(node->data.unOpExp.target);
      break;
    }
    case NT_COMPOPEXP: {
      nodeDestroy(node->data.compOpExp.lhs);
      nodeDestroy(node->data.compOpExp.rhs);
      break;
    }
    case NT_LANDASSIGNEXP: {
      nodeDestroy(node->data.landAssignExp.lhs);
      nodeDestroy(node->data.landAssignExp.rhs);
      break;
    }
    case NT_LORASSIGNEXP: {
      nodeDestroy(node->data.lorAssignExp.lhs);
      nodeDestroy(node->data.lorAssignExp.rhs);
      break;
    }
    case NT_TERNARYEXP: {
      nodeDestroy(node->data.ternaryExp.condition);
      nodeDestroy(node->data.ternaryExp.thenExp);
      nodeDestroy(node->data.ternaryExp.elseExp);
      break;
    }
    case NT_LANDEXP: {
      nodeDestroy(node->data.landExp.lhs);
      nodeDestroy(node->data.landExp.rhs);
      break;
    }
    case NT_LOREXP: {
      nodeDestroy(node->data.lorExp.lhs);
      nodeDestroy(node->data.lorExp.rhs);
      break;
    }
    case NT_STRUCTACCESSEXP: {
      nodeDestroy(node->data.structAccessExp.base);
      nodeDestroy(node->data.structAccessExp.element);
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      nodeDestroy(node->data.structPtrAccessExp.base);
      nodeDestroy(node->data.structPtrAccessExp.element);
      break;
    }
    case NT_FNCALLEXP: {
      nodeDestroy(node->data.fnCallExp.who);
      nodeListDestroy(node->data.fnCallExp.args);
      break;
    }
    case NT_IDEXP: {
      free(node->data.idExp.id);
      break;
    }
    case NT_CONSTEXP: {
      switch (node->data.constExp.type) {
        case CT_STRING: {
          free(node->data.constExp.value.stringVal);
          break;
        }
        case CT_WSTRING: {
          free(node->data.constExp.value.wstringVal);
          break;
        }
        case CT_UBYTE:
        case CT_BYTE:
        case CT_UINT:
        case CT_INT:
        case CT_ULONG:
        case CT_LONG:
        case CT_FLOAT:
        case CT_DOUBLE:
        case CT_CHAR:
        case CT_WCHAR:
        case CT_BOOL:
        case CT_RANGE_ERROR:
          break;
      }
      break;
    }
    case NT_AGGREGATEINITEXP: {
      nodeListDestroy(node->data.aggregateInitExp.elements);
      break;
    }
    case NT_CASTEXP: {
      nodeDestroy(node->data.castExp.toWhat);
      nodeDestroy(node->data.castExp.target);
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      nodeDestroy(node->data.sizeofTypeExp.target);
      break;
    }
    case NT_SIZEOFEXPEXP: {
      nodeDestroy(node->data.sizeofExpExp.target);
      break;
    }
    case NT_KEYWORDTYPE:
      break;
    case NT_IDTYPE: {
      free(node->data.idType.id);
      break;
    }
    case NT_CONSTTYPE: {
      nodeDestroy(node->data.constType.target);
      break;
    }
    case NT_ARRAYTYPE: {
      nodeDestroy(node->data.arrayType.element);
      nodeDestroy(node->data.arrayType.size);
      break;
    }
    case NT_PTRTYPE: {
      nodeDestroy(node->data.ptrType.target);
      break;
    }
    case NT_FNPTRTYPE: {
      nodeDestroy(node->data.fnPtrType.returnType);
      nodeListDestroy(node->data.fnPtrType.argTypes);
      break;
    }
    case NT_ID: {
      free(node->data.id.id);
      break;
    }
  }

  free(node);
}