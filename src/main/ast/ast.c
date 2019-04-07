// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of AST nodes

#include "ast/ast.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

// NodeList
NodeList *nodeListCreate(void) {
  NodeList *list = malloc(sizeof(NodeList));
  list->size = 0;
  list->capacity = 1;
  list->elements = malloc(sizeof(Node *));
  return list;
}

void nodeListInsert(NodeList *list, Node *node) {
  if (list->size == list->capacity) {
    list->capacity *= 2;
    list->elements = realloc(list->elements, list->capacity * sizeof(Node *));
  }
  list->elements[list->size++] = node;
}

void nodeListDestroy(NodeList *list) {
  for (size_t idx = 0; idx < list->size; idx++) {
    nodeDestroy(list->elements[idx]);
  }
  free(list->elements);
  free(list);
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
Node *programNodeCreate(size_t line, size_t character, Node *module,
                        NodeList *imports, NodeList *bodies) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_PROGRAM;
  node->data.program.module = module;
  node->data.program.imports = imports;
  node->data.program.bodies = bodies;
  return node;
}

Node *moduleNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_MODULE;
  node->data.module.id = id;
  return node;
}
Node *importNodeCreate(size_t line, size_t character, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_IMPORT;
  node->data.import.id = id;
  return node;
}
Node *funDeclNodeCreate(size_t line, size_t character, Node *returnType,
                        Node *id, NodeList *paramTypes) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FUNDECL;
  node->data.funDecl.returnType = returnType;
  node->data.funDecl.id = id;
  node->data.funDecl.paramTypes = paramTypes;
  return node;
}
Node *varDeclNodeCreate(size_t line, size_t character, Node *type,
                        NodeList *ids) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_VARDECL;
  node->data.varDecl.type = type;
  node->data.varDecl.ids = ids;
  return node;
}
Node *structDeclNodeCreate(size_t line, size_t character, Node *id,
                           NodeList *decls) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_STRUCTDECL;
  node->data.structDecl.id = id;
  node->data.structDecl.decls = decls;
  return node;
}
Node *unionDeclNodeCreate(size_t line, size_t character, Node *id,
                          NodeList *opts) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_UNIONDECL;
  node->data.unionDecl.id = id;
  node->data.unionDecl.opts = opts;
  return node;
}
Node *enumDeclNodeCreate(size_t line, size_t character, Node *id,
                         NodeList *elements) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ENUMDECL;
  node->data.enumDecl.id = id;
  node->data.enumDecl.elements = elements;
  return node;
}
Node *typedefNodeCreate(size_t line, size_t character, Node *type, Node *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_TYPEDEFDECL;
  node->data.typedefDecl.type = type;
  node->data.typedefDecl.id = id;
  return node;
}
Node *functionNodeCreate(size_t line, size_t character, Node *returnType,
                         Node *id, NodePairList *formals, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FUNCTION;
  node->data.function.returnType = returnType;
  node->data.function.id = id;
  node->data.function.formals = formals;
  node->data.function.body = body;
  return node;
}
Node *compoundStmtNodeCreate(size_t line, size_t character,
                             NodeList *statements) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_COMPOUNDSTMT;
  node->data.compoundStmt.statements = statements;
  return node;
}
Node *ifStmtNodeCreate(size_t line, size_t character, Node *condition,
                       Node *thenStmt, Node *elseStmt) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_IFSTMT;
  node->data.ifStmt.condition = condition;
  node->data.ifStmt.thenStmt = thenStmt;
  node->data.ifStmt.elseStmt = elseStmt;
  return node;
}
Node *whileStmtNodeCreate(size_t line, size_t character, Node *condition,
                          Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_WHILESTMT;
  node->data.whileStmt.condition = condition;
  node->data.whileStmt.body = body;
  return node;
}
Node *doWhileStmtNodeCreate(size_t line, size_t character, Node *body,
                            Node *condition) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_DOWHILESTMT;
  node->data.doWhileStmt.body = body;
  node->data.doWhileStmt.condition = condition;
  return node;
}
Node *forStmtNodeCreate(size_t line, size_t character, Node *initialize,
                        Node *condition, Node *update, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FORSTMT;
  node->data.forStmt.initialize = initialize;
  node->data.forStmt.condition = condition;
  node->data.forStmt.update = update;
  node->data.forStmt.body = body;
  return node;
}
Node *switchStmtNodeCreate(size_t line, size_t character, Node *onWhat,
                           NodeList *cases) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SWITCHSTMT;
  node->data.switchStmt.onWhat = onWhat;
  node->data.switchStmt.cases = cases;
  return node;
}
Node *numCaseNodeCreate(size_t line, size_t character, Node *constVal,
                        Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_NUMCASE;
  node->data.numCase.constVal = constVal;
  node->data.numCase.body = body;
  return node;
}
Node *defaultCaseNodeCreate(size_t line, size_t character, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_DEFAULTCASE;
  node->data.defaultCase.body = body;
  return node;
}
Node *breakStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_BREAKSTMT;
  return node;
}
Node *continueStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONTINUESTMT;
  return node;
}
Node *returnStmtNodeCreate(size_t line, size_t character, Node *value) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_RETURNSTMT;
  node->data.returnStmt.value = value;
  return node;
}
Node *varDeclStmtNodeCreate(size_t line, size_t character, Node *type,
                            NodePairList *idValuePairs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_VARDECLSTMT;
  node->data.varDeclStmt.type = type;
  node->data.varDeclStmt.idValuePairs = idValuePairs;
  return node;
}
Node *asmStmtNodeCreate(size_t line, size_t character, Node *assembly) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ASMSTMT;
  node->data.asmStmt.assembly = assembly;
  return node;
}
Node *expressionStmtNodeCreate(size_t line, size_t character,
                               Node *expression) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_EXPRESSIONSTMT;
  node->data.expressionStmt.expression = expression;
  return node;
}
Node *nullStmtNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_NULLSTMT;
  return node;
}
Node *seqExpNodeCreate(size_t line, size_t character, Node *first,
                       Node *second) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SEQEXP;
  node->data.seqExp.first = first;
  node->data.seqExp.second = second;
  return node;
}
Node *binOpExpNodeCreate(size_t line, size_t character, BinOpType op, Node *lhs,
                         Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_BINOPEXP;
  node->data.binopExp.op = op;
  node->data.binopExp.lhs = lhs;
  node->data.binopExp.rhs = rhs;
  return node;
}
Node *unOpExpNodeCreate(size_t line, size_t character, UnOpType op,
                        Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_UNOPEXP;
  node->data.unOpExp.op = op;
  node->data.unOpExp.target = target;
  return node;
}
Node *compOpExpNodeCreate(size_t line, size_t character, CompOpType op,
                          Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_COMPOPEXP;
  node->data.compOpExp.op = op;
  node->data.compOpExp.lhs = lhs;
  node->data.compOpExp.rhs = rhs;
  return node;
}
Node *landAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                              Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LANDASSIGNEXP;
  node->data.landAssignExp.lhs = lhs;
  node->data.landAssignExp.rhs = rhs;
  return node;
}
Node *lorAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                             Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LORASSIGNEXP;
  node->data.lorAssignExp.lhs = lhs;
  node->data.lorAssignExp.rhs = rhs;
  return node;
}
Node *ternaryExpNodeCreate(size_t line, size_t character, Node *condition,
                           Node *thenExp, Node *elseExp) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_TERNARYEXP;
  node->data.ternaryExp.condition = condition;
  node->data.ternaryExp.thenExp = thenExp;
  node->data.ternaryExp.elseExp = elseExp;
  return node;
}
Node *landExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LANDEXP;
  node->data.landExp.lhs = lhs;
  node->data.landExp.rhs = rhs;
  return node;
}
Node *lorExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LOREXP;
  node->data.lorExp.lhs = lhs;
  node->data.lorExp.rhs = rhs;
  return node;
}
Node *structAccessExpNodeCreate(size_t line, size_t character, Node *base,
                                Node *element) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_STRUCTACCESSEXP;
  node->data.structAccessExp.base = base;
  node->data.structAccessExp.element = element;
  return node;
}
Node *structPtrAccessExpNodeCreate(size_t line, size_t character, Node *base,
                                   Node *element) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_STRUCTPTRACCESSEXP;
  node->data.structPtrAccessExp.base = base;
  node->data.structPtrAccessExp.element = element;
  return node;
}
Node *fnCallExpNodeCreate(size_t line, size_t character, Node *who,
                          NodeList *args) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FNCALLEXP;
  node->data.fnCallExp.who = who;
  node->data.fnCallExp.args = args;
  return node;
}
Node *idExpNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_IDEXP;
  node->data.idExp.id = id;
  return node;
}

// Constant parsing
static uint8_t charToHex(char c) {
  if (isdigit(c)) {
    return (uint8_t)(c - '0');
  } else if (isupper(c)) {
    return (uint8_t)(c - 'A' + 0xA);
  } else {
    return (uint8_t)(c - 'a' + 0xA);
  }
}
static void parseInt(Node *node, char *value) {
  enum {
    START,
    SEEN_SIGN,
    SEEN_ZERO,
    IS_ZERO,
    IS_BINARY,
    IS_OCTAL,
    IS_DECIMAL,
    IS_HEX,
  } state = START;  // this uses a DFA to parse the int
  size_t length = strlen(value);

  enum {
    POSITIVE,
    NEGATIVE,
    UNSIGNED,
  } sign = UNSIGNED;
  bool overflow = false;

  uint64_t acc = 0;

  for (size_t idx = 0; idx <= length; idx++) {
    char current = value[idx];
    switch (state) {
      case START: {
        switch (current) {
          case '-': {
            sign = NEGATIVE;
            state = SEEN_SIGN;
            break;
          }
          case '+': {
            sign = POSITIVE;
            state = SEEN_SIGN;
            break;
          }
          case '0': {
            state = SEEN_ZERO;
            break;
          }
          default: {
            state = IS_DECIMAL;  // must be [0-9] based on lex
            idx--;               // back up, keep going
            break;
          }
        }
        break;
      }
      case SEEN_SIGN: {
        switch (current) {
          case '0': {
            state = SEEN_ZERO;
            break;
          }
          default: {
            state = IS_DECIMAL;  // must be [0-9] based on lex
            idx--;               // back up, keep going
            break;
          }
        }
        break;
      }
      case SEEN_ZERO: {
        switch (current) {
          case '\0': {
            state = IS_ZERO;
            idx--;  // back up, keep going
            break;
          }
          case 'x': {
            state = IS_HEX;
            break;
          }
          case 'b': {
            state = IS_BINARY;
            break;
          }
          default: {
            state = IS_OCTAL;  // must be [0-7] based on lex
            idx--;             // back up, keep going
            break;
          }
        }
        break;
      }
      case IS_ZERO: {
        acc = 0;
        idx = length;  // be done
        break;
      }
      case IS_BINARY: {
        for (; idx < length; idx++) {
          size_t oldAcc = acc;

          acc *= 2;
          acc += (uint64_t)(value[idx] - '0');

          if (acc < oldAcc) {  // overflowed!
            overflow = true;
            break;
          }
        }

        idx = length;  // be done
        break;
      }
      case IS_OCTAL: {
        for (; idx < length; idx++) {
          size_t oldAcc = acc;

          acc *= 8;
          acc += (uint64_t)(value[idx] - '0');

          if (acc < oldAcc) {  // overflowed!
            overflow = true;
            break;
          }
        }

        idx = length;  // be done
        break;
      }
      case IS_DECIMAL: {
        for (; idx < length; idx++) {
          size_t oldAcc = acc;

          acc *= 10;
          acc += (uint64_t)(value[idx] - '0');

          if (acc < oldAcc) {  // overflowed!
            overflow = true;
            break;
          }
        }

        idx = length;  // be done
        break;
      }
      case IS_HEX: {
        for (; idx < length; idx++) {
          current = value[idx];
          size_t oldAcc = acc;

          acc *= 16;
          acc += charToHex(current);

          if (acc < oldAcc) {  // overflowed!
            overflow = true;
            break;
          }
        }

        idx = length;  // be done
        break;
      }
    }
  }

  if (overflow) {
    node->data.constExp.type = CTYPE_RANGE_ERROR;
  } else {
    switch (sign) {
      case UNSIGNED: {
        if (acc <= UBYTE_MAX) {
          node->data.constExp.type = CTYPE_UBYTE;
          node->data.constExp.value.ubyteVal = (uint8_t)acc;
        } else if (acc <= UINT_MAX) {
          node->data.constExp.type = CTYPE_UINT;
          node->data.constExp.value.uintVal = (uint32_t)acc;
        } else if (acc <= ULONG_MAX) {
          node->data.constExp.type = CTYPE_ULONG;
          node->data.constExp.value.ulongVal = acc;
        } else {
          node->data.constExp.type = CTYPE_RANGE_ERROR;
        }
        break;
      }
      case POSITIVE: {
        if (acc <= BYTE_MAX) {
          node->data.constExp.type = CTYPE_BYTE;
          node->data.constExp.value.byteVal = (int8_t)acc;
        } else if (acc <= INT_MAX) {
          node->data.constExp.type = CTYPE_INT;
          node->data.constExp.value.intVal = (int32_t)acc;
        } else if (acc <= LONG_MAX) {
          node->data.constExp.type = CTYPE_LONG;
          node->data.constExp.value.longVal = (int64_t)acc;
        } else {
          node->data.constExp.type = CTYPE_RANGE_ERROR;
        }
        break;
      }
      case NEGATIVE: {
        if (acc <= BYTE_MIN) {
          node->data.constExp.type = CTYPE_BYTE;
          node->data.constExp.value.byteVal = (int8_t)-acc;
        } else if (acc <= INT_MIN) {
          node->data.constExp.type = CTYPE_INT;
          node->data.constExp.value.intVal = (int32_t)-acc;
        } else if (acc <= LONG_MIN) {
          node->data.constExp.type = CTYPE_LONG;
          node->data.constExp.value.longVal = (int64_t)-acc;
        } else {
          node->data.constExp.type = CTYPE_RANGE_ERROR;
        }
        break;
      }
    }
  }
}
static void parseFloat(Node *node, char *value) {
  // TODO: write this
}
static void parseString(Node *node, char *value) {
  node->data.constExp.type = CTYPE_STRING;
  node->data.constExp.value.stringVal = malloc(1);
  // TODO: write this
}
static void parseChar(Node *node, char *value) {
  // value is at least 3 chars long, by lex
  node->data.constExp.type = CTYPE_CHAR;

  if (value[1] == '\\') {  // special character
    switch (value[2]) {
      case '\'':
      case '\\': {
        node->data.constExp.value.charVal = (uint8_t)value[2];
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
        node->data.constExp.value.charVal =
            (uint8_t)((charToHex(value[3]) << 4) + charToHex(value[4]));
        break;
      }
    }
  } else {
    node->data.constExp.value.charVal = (uint8_t)value[1];
  }
}
static void parseWString(Node *node, char *value) {
  node->data.constExp.type = CTYPE_WSTRING;
  node->data.constExp.value.stringVal = malloc(1);
  // TODO: write this
}
static void parseWChar(Node *node, char *value) {
  // value is at least 4 chars long, by lex
  node->data.constExp.type = CTYPE_WCHAR;

  if (value[1] == '\\') {  // special character
    switch (value[2]) {
      case '\'':
      case '\\': {
        node->data.constExp.value.wcharVal = (uint32_t)value[2];
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
        node->data.constExp.value.wcharVal =
            (uint32_t)((charToHex(value[3]) << 4) + charToHex(value[4]));
        break;
      }
      case 'u': {
        node->data.constExp.value.wcharVal = 0;
        for (size_t idx = 3; idx < 3 + 8; idx++) {
          node->data.constExp.value.wcharVal <<= 4;
          node->data.constExp.value.wcharVal |= (uint32_t)charToHex(value[idx]);
        }
      }
    }
  } else {
    node->data.constExp.value.wcharVal = (uint32_t)value[1];
  }
}
Node *constExpNodeCreate(size_t line, size_t character, ConstTypeHint hint,
                         char *value) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONSTEXP;
  switch (hint) {
    case TYPEHINT_INT: {
      parseInt(node, value);
      break;
    }
    case TYPEHINT_FLOAT: {
      parseFloat(node, value);
      break;
    }
    case TYPEHINT_STRING: {
      parseString(node, value);
      break;
    }
    case TYPEHINT_CHAR: {
      parseChar(node, value);
      break;
    }
    case TYPEHINT_WSTRING: {
      parseWString(node, value);
      break;
    }
    case TYPEHINT_WCHAR: {
      parseWChar(node, value);
      break;
    }
  }
  free(value);
  return node;
}

// More constructors
Node *aggregateInitExpNodeCreate(size_t line, size_t character,
                                 NodeList *elements) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_AGGREGATEINITEXP;
  node->data.aggregateInitExp.elements = elements;
  return node;
}
Node *constTrueNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONSTEXP;
  node->data.constExp.type = CTYPE_BOOL;
  node->data.constExp.value.boolVal = true;
  return node;
}
Node *constFalseNodeCreate(size_t line, size_t character) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONSTEXP;
  node->data.constExp.type = CTYPE_BOOL;
  node->data.constExp.value.boolVal = false;
  return node;
}
Node *castExpNodeCreate(size_t line, size_t character, Node *toWhat,
                        Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CASTEXP;
  node->data.castExp.toWhat = toWhat;
  node->data.castExp.target = target;
  return node;
}
Node *sizeofTypeExpNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SIZEOFTYPEEXP;
  node->data.sizeofTypeExp.target;
  return node;
}
Node *sizeofExpExpNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SIZEOFEXPEXP;
  node->data.sizeofExpExp.target;
  return node;
}
Node *keywordTypeNodeCreate(size_t line, size_t character, TypeKeyword type) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_KEYWORDTYPE;
  node->data.keywordType.type = type;
  return node;
}
Node *idTypeNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_IDTYPE;
  node->data.idType.id = id;
  return node;
}
Node *constTypeNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONSTTYPE;
  node->data.constType.target = target;
  return node;
}
Node *arrayTypeNodeCreate(size_t line, size_t character, Node *element,
                          Node *size) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ARRAYTYPE;
  node->data.arrayType.element = element;
  node->data.arrayType.size = size;
  return node;
}
Node *ptrTypeNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_PTRTYPE;
  node->data.ptrType.target = target;
  return node;
}
Node *fnPtrTypeNodeCreate(size_t line, size_t character, Node *returnType,
                          NodeList *argTypes) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FNPTRTYPE;
  node->data.fnPtrType.returnType = returnType;
  node->data.fnPtrType.argTypes = argTypes;
  return node;
}
Node *idNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ID;
  node->data.id.id = id;
  return node;
}

// Destructor
static void freeIfNotNull(void *ptr) {
  if (ptr != NULL) free(ptr);
}
void nodeDestroy(Node *node) {
  if (node == NULL)
    return;  // base case: might be asked internally to free NULL.

  // free contents
  switch (node->type) {
    case TYPE_PROGRAM: {
      nodeDestroy(node->data.program.module);
      nodeListDestroy(node->data.program.imports);
      nodeListDestroy(node->data.program.bodies);
      break;
    }
    case TYPE_MODULE: {
      nodeDestroy(node->data.module.id);
      break;
    }
    case TYPE_IMPORT: {
      nodeDestroy(node->data.import.id);
      break;
    }
    case TYPE_FUNDECL: {
      nodeDestroy(node->data.funDecl.returnType);
      nodeDestroy(node->data.funDecl.id);
      nodeListDestroy(node->data.funDecl.paramTypes);
      break;
    }
    case TYPE_VARDECL: {
      nodeDestroy(node->data.varDecl.type);
      nodeListDestroy(node->data.varDecl.ids);
      break;
    }
    case TYPE_STRUCTDECL: {
      nodeDestroy(node->data.structDecl.id);
      nodeListDestroy(node->data.structDecl.decls);
      break;
    }
    case TYPE_UNIONDECL: {
      nodeDestroy(node->data.unionDecl.id);
      nodeListDestroy(node->data.unionDecl.opts);
      break;
    }
    case TYPE_ENUMDECL: {
      nodeDestroy(node->data.enumDecl.id);
      nodeListDestroy(node->data.enumDecl.elements);
      break;
    }
    case TYPE_TYPEDEFDECL: {
      nodeDestroy(node->data.typedefDecl.type);
      nodeDestroy(node->data.typedefDecl.id);
      break;
    }
    case TYPE_FUNCTION: {
      nodeDestroy(node->data.function.returnType);
      nodeDestroy(node->data.function.id);
      nodePairListDestroy(node->data.function.formals);
      nodeDestroy(node->data.function.body);
      break;
    }
    case TYPE_COMPOUNDSTMT: {
      nodeListDestroy(node->data.compoundStmt.statements);
      break;
    }
    case TYPE_IFSTMT: {
      nodeDestroy(node->data.ifStmt.condition);
      nodeDestroy(node->data.ifStmt.thenStmt);
      nodeDestroy(node->data.ifStmt.elseStmt);
      break;
    }
    case TYPE_WHILESTMT: {
      nodeDestroy(node->data.whileStmt.condition);
      nodeDestroy(node->data.whileStmt.body);
      break;
    }
    case TYPE_DOWHILESTMT: {
      nodeDestroy(node->data.doWhileStmt.body);
      nodeDestroy(node->data.doWhileStmt.condition);
      break;
    }
    case TYPE_FORSTMT: {
      nodeDestroy(node->data.forStmt.initialize);
      nodeDestroy(node->data.forStmt.condition);
      nodeDestroy(node->data.forStmt.update);
      nodeDestroy(node->data.forStmt.body);
      break;
    }
    case TYPE_SWITCHSTMT: {
      nodeDestroy(node->data.switchStmt.onWhat);
      nodeListDestroy(node->data.switchStmt.cases);
      break;
    }
    case TYPE_NUMCASE: {
      nodeDestroy(node->data.numCase.constVal);
      nodeDestroy(node->data.numCase.body);
      break;
    }
    case TYPE_DEFAULTCASE: {
      nodeDestroy(node->data.defaultCase.body);
      break;
    }
    case TYPE_BREAKSTMT:
      break;
    case TYPE_CONTINUESTMT:
      break;
    case TYPE_RETURNSTMT: {
      nodeDestroy(node->data.returnStmt.value);
      break;
    }
    case TYPE_VARDECLSTMT: {
      nodeDestroy(node->data.varDeclStmt.type);
      nodePairListDestroy(node->data.varDeclStmt.idValuePairs);
      break;
    }
    case TYPE_ASMSTMT: {
      nodeDestroy(node->data.asmStmt.assembly);
      break;
    }
    case TYPE_EXPRESSIONSTMT: {
      nodeDestroy(node->data.expressionStmt.expression);
      break;
    }
    case TYPE_NULLSTMT:
      break;
    case TYPE_SEQEXP: {
      nodeDestroy(node->data.seqExp.first);
      nodeDestroy(node->data.seqExp.second);
      break;
    }
    case TYPE_BINOPEXP: {
      nodeDestroy(node->data.binopExp.lhs);
      nodeDestroy(node->data.binopExp.rhs);
      break;
    }
    case TYPE_UNOPEXP: {
      nodeDestroy(node->data.unOpExp.target);
      break;
    }
    case TYPE_COMPOPEXP: {
      nodeDestroy(node->data.compOpExp.lhs);
      nodeDestroy(node->data.compOpExp.rhs);
      break;
    }
    case TYPE_LANDASSIGNEXP: {
      nodeDestroy(node->data.landAssignExp.lhs);
      nodeDestroy(node->data.landAssignExp.rhs);
      break;
    }
    case TYPE_LORASSIGNEXP: {
      nodeDestroy(node->data.lorAssignExp.lhs);
      nodeDestroy(node->data.lorAssignExp.rhs);
      break;
    }
    case TYPE_TERNARYEXP: {
      nodeDestroy(node->data.ternaryExp.condition);
      nodeDestroy(node->data.ternaryExp.thenExp);
      nodeDestroy(node->data.ternaryExp.elseExp);
      break;
    }
    case TYPE_LANDEXP: {
      nodeDestroy(node->data.landExp.lhs);
      nodeDestroy(node->data.landExp.rhs);
      break;
    }
    case TYPE_LOREXP: {
      nodeDestroy(node->data.lorExp.lhs);
      nodeDestroy(node->data.lorExp.rhs);
      break;
    }
    case TYPE_STRUCTACCESSEXP: {
      nodeDestroy(node->data.structAccessExp.base);
      nodeDestroy(node->data.structAccessExp.element);
      break;
    }
    case TYPE_STRUCTPTRACCESSEXP: {
      nodeDestroy(node->data.structPtrAccessExp.base);
      nodeDestroy(node->data.structPtrAccessExp.element);
      break;
    }
    case TYPE_FNCALLEXP: {
      nodeDestroy(node->data.fnCallExp.who);
      nodeListDestroy(node->data.fnCallExp.args);
      break;
    }
    case TYPE_IDEXP: {
      free(node->data.idExp.id);
      break;
    }
    case TYPE_CONSTEXP: {
      switch (node->data.constExp.type) {
        case CTYPE_STRING: {
          free(node->data.constExp.value.stringVal);
          break;
        }
        case CTYPE_WSTRING: {
          free(node->data.constExp.value.wstringVal);
          break;
        }
        case CTYPE_UBYTE:
        case CTYPE_BYTE:
        case CTYPE_UINT:
        case CTYPE_INT:
        case CTYPE_ULONG:
        case CTYPE_LONG:
        case CTYPE_FLOAT:
        case CTYPE_DOUBLE:
        case CTYPE_CHAR:
        case CTYPE_WCHAR:
        case CTYPE_BOOL:
        case CTYPE_RANGE_ERROR:
          break;
      }
      break;
    }
    case TYPE_AGGREGATEINITEXP: {
      nodeListDestroy(node->data.aggregateInitExp.elements);
      break;
    }
    case TYPE_CASTEXP: {
      nodeDestroy(node->data.castExp.toWhat);
      nodeDestroy(node->data.castExp.target);
      break;
    }
    case TYPE_SIZEOFTYPEEXP: {
      nodeDestroy(node->data.sizeofTypeExp.target);
      break;
    }
    case TYPE_SIZEOFEXPEXP: {
      nodeDestroy(node->data.sizeofExpExp.target);
      break;
    }
    case TYPE_KEYWORDTYPE:
      break;
    case TYPE_IDTYPE: {
      free(node->data.idType.id);
      break;
    }
    case TYPE_CONSTTYPE: {
      nodeDestroy(node->data.constType.target);
      break;
    }
    case TYPE_ARRAYTYPE: {
      nodeDestroy(node->data.arrayType.element);
      nodeDestroy(node->data.arrayType.size);
      break;
    }
    case TYPE_PTRTYPE: {
      nodeDestroy(node->data.ptrType.target);
      break;
    }
    case TYPE_FNPTRTYPE: {
      nodeDestroy(node->data.fnPtrType.returnType);
      nodeListDestroy(node->data.fnPtrType.argTypes);
      break;
    }
    case TYPE_ID: {
      free(node->data.id.id);
      break;
    }
  }

  free(node);
}