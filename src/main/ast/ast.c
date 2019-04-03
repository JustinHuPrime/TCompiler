#include "ast/ast.h"

#include <stdlib.h>

Node *nodeCreate(size_t line, size_t character) {
  Node *node = malloc(sizeof(Node));
  node->line = line;
  node->character = character;
  return node;
}
Node *programNodeCreate(size_t line, size_t character, Node *module,
                        size_t numImports, Node **imports, size_t numBodies,
                        Node **bodies) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_PROGRAM;
  node->data.program.module = module;
  node->data.program.numImports = numImports;
  node->data.program.imports = imports;
  node->data.program.numBodies = numBodies;
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
                        Node *id, size_t numParamTypes, Node **paramTypes) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FUNDECL;
  node->data.funDecl.returnType = returnType;
  node->data.funDecl.id = id;
  node->data.funDecl.numParamTypes = numParamTypes;
  node->data.funDecl.paramTypes = paramTypes;
  return node;
}
Node *varDeclNodeCreate(size_t line, size_t character, Node *type,
                        size_t numIds, Node **ids) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_VARDECL;
  node->data.varDecl.type = type;
  node->data.varDecl.numIds = numIds;
  node->data.varDecl.ids = ids;
  return node;
}
Node *structDeclNodeCreate(size_t line, size_t character, Node *id,
                           size_t numDecls, Node **decls) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_STRUCTDECL;
  node->data.structDecl.id = id;
  node->data.structDecl.numDecls = numDecls;
  node->data.structDecl.decls = decls;
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
                         Node *id, size_t numFormals, Node **formalTypes,
                         Node **formalIds, Node *body) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FUNCTION;
  node->data.function.returnType = returnType;
  node->data.function.id = id;
  node->data.function.numFormals = numFormals;
  node->data.function.formalTypes = formalTypes;
  node->data.function.formalIds = formalIds;
  node->data.function.body = body;
  return node;
}
Node *compoundStmtNodeCreate(size_t line, size_t character,
                             size_t numStatements, Node **statements) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_COMPOUNDSTMT;
  node->data.compoundStmt.numStatements = numStatements;
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
  // TODO: implement desugaring
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FORSTMT;
  node->data.forStmt.initialize = initialize;
  node->data.forStmt.condition = condition;
  node->data.forStmt.update = update;
  node->data.forStmt.body = body;
  return node;
}
Node *switchStmtNodeCreate(size_t line, size_t character, Node *onWhat,
                           size_t numCases, Node **cases) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SWITCHSTMT;
  node->data.switchStmt.onWhat = onWhat;
  node->data.switchStmt.numCases = numCases;
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
                            size_t numIds, Node **ids, Node **values) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_VARDECLSTMT;
  node->data.varDeclStmt.type = type;
  node->data.varDeclStmt.numIds = numIds;
  node->data.varDeclStmt.ids = ids;
  node->data.varDeclStmt.values = values;
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
  // TODO: implement desugaring to ternaryExp
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LANDASSIGNEXP;
  node->data.landAssignExp.lhs = lhs;
  node->data.landAssignExp.rhs = rhs;
  return node;
}
Node *lorAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                             Node *rhs) {
  // TODO: implement desugaring to ternaryExp
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
  // TODO: implement desugaring to ternaryExp
  Node *node = nodeCreate(line, character);
  node->type = TYPE_LANDEXP;
  node->data.landExp.lhs = lhs;
  node->data.landExp.rhs = rhs;
  return node;
}
Node *lorExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs) {
  // TODO: implement desugaring to ternaryExp
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
                          size_t numArgs, Node **args) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FNCALLEXP;
  node->data.fnCallExp.who = who;
  node->data.fnCallExp.numArgs = numArgs;
  node->data.fnCallExp.args = args;
  return node;
}
Node *idExpNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_IDEXP;
  node->data.idExp.id = id;
  return node;
}
Node *constExpNodeCreate(size_t line, size_t character, char *value) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_CONSTEXP;
  // node->data.constExp
  // TODO: parse the value
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
Node *sizeofExpNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_SIZEOFEXP;
  node->data.sizeofExp.target;
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
                          char *size) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ARRAYTYPE;
  node->data.arrayType.element = element;
  // node->data.arrayType.size
  // TODO: parse size
  return node;
}
Node *ptrTypeNodeCreate(size_t line, size_t character, Node *target) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_PTRTYPE;
  node->data.ptrType.target = target;
  return node;
}
Node *fnPtrTypeNodeCreate(size_t line, size_t character, Node *returnType,
                          size_t numArgTypes, Node **argTypes) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_FNPTRTYPE;
  node->data.fnPtrType.returnType = returnType;
  node->data.fnPtrType.numArgTypes = numArgTypes;
  node->data.fnPtrType.argTypes = argTypes;
  return node;
}
Node *idNodeCreate(size_t line, size_t character, char *id) {
  Node *node = nodeCreate(line, character);
  node->type = TYPE_ID;
  node->data.id.id = id;
  return node;
}

void freeIfNotNull(void *ptr) {
  if (ptr != NULL) free(ptr);
}

void nodeDestroy(Node *node) {
  if (node == NULL) return;

  switch (node->type) {
    case TYPE_PROGRAM: {
      nodeDestroy(node->data.program.module);
      for (size_t idx = 0; idx < node->data.program.numImports; idx++)
        nodeDestroy(node->data.program.imports[idx]);
      freeIfNotNull(node->data.program.imports);
      for (size_t idx = 0; idx < node->data.program.numBodies; idx++)
        nodeDestroy(node->data.program.bodies[idx]);
      freeIfNotNull(node->data.program.bodies);
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
      for (size_t idx = 0; idx < node->data.funDecl.numParamTypes; idx++)
        nodeDestroy(node->data.funDecl.paramTypes[idx]);
      free(node->data.funDecl.paramTypes);
      break;
    }
    case TYPE_VARDECL: {
      nodeDestroy(node->data.varDecl.type);
      for (size_t idx = 0; idx < node->data.varDecl.numIds; idx++)
        nodeDestroy(node->data.varDecl.ids[idx]);
      freeIfNotNull(node->data.varDecl.ids);
      break;
    }
    case TYPE_STRUCTDECL: {
      nodeDestroy(node->data.structDecl.id);
      for (size_t idx = 0; idx < node->data.structDecl.numDecls; idx++)
        nodeDestroy(node->data.structDecl.decls[idx]);
      freeIfNotNull(node->data.structDecl.decls);
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
      for (size_t idx = 0; idx < node->data.function.numFormals; idx++) {
        nodeDestroy(node->data.function.formalTypes[idx]);
        nodeDestroy(node->data.function.formalIds[idx]);
      }
      freeIfNotNull(node->data.function.formalTypes);
      freeIfNotNull(node->data.function.formalIds);
      nodeDestroy(node->data.function.body);
      break;
    }
    case TYPE_COMPOUNDSTMT: {
      for (size_t idx = 0; idx < node->data.compoundStmt.numStatements; idx++)
        nodeDestroy(node->data.compoundStmt.statements[idx]);
      freeIfNotNull(node->data.compoundStmt.statements);
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
      for (size_t idx = 0; idx < node->data.switchStmt.numCases; idx++)
        nodeDestroy(node->data.switchStmt.cases[idx]);
      freeIfNotNull(node->data.switchStmt.cases);
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
      for (size_t idx = 0; idx < node->data.varDeclStmt.numIds; idx++) {
        nodeDestroy(node->data.varDeclStmt.ids[idx]);
        nodeDestroy(node->data.varDeclStmt.values[idx]);
      }
      freeIfNotNull(node->data.varDeclStmt.ids);
      freeIfNotNull(node->data.varDeclStmt.values);
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
      for (size_t idx = 0; idx < node->data.fnCallExp.numArgs; idx++)
        nodeDestroy(node->data.fnCallExp.args[idx]);
      freeIfNotNull(node->data.fnCallExp.args);
      break;
    }
    case TYPE_IDEXP: {
      free(node->data.idExp.id);
      break;
    }
    case TYPE_CONSTEXP:
      break;
    case TYPE_CASTEXP: {
      nodeDestroy(node->data.castExp.toWhat);
      nodeDestroy(node->data.castExp.target);
      break;
    }
    case TYPE_SIZEOFEXP: {
      nodeDestroy(node->data.sizeofExp.target);
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
      break;
    }
    case TYPE_PTRTYPE: {
      nodeDestroy(node->data.ptrType.target);
      break;
    }
    case TYPE_FNPTRTYPE: {
      nodeDestroy(node->data.fnPtrType.returnType);
      for (size_t idx = 0; idx < node->data.fnPtrType.numArgTypes; idx++)
        nodeDestroy(node->data.fnPtrType.argTypes[idx]);
      freeIfNotNull(node->data.fnPtrType.argTypes);
      break;
    }
    case TYPE_ID: {
      free(node->data.id.id);
      break;
    }
  }

  free(node);
}