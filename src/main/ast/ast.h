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

// A "polymorphic" AST node, and lists of nodes

#ifndef TLC_AST_AST_H_
#define TLC_AST_AST_H_

#include "typecheck/symbolTable.h"
#include "util/container/vector.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Node;

// specialization of vector
typedef Vector NodeList;
// constructor
NodeList *nodeListCreate(void);
// inserts a pointer into the list. The list now owns the pointer
void nodeListInsert(NodeList *, struct Node *);
// destructor
void nodeListDestroy(NodeList *);

// A list of pairs of nodes, vector-style
typedef struct {
  size_t size;
  size_t capacity;
  struct Node **firstElements;
  struct Node **secondElements;
} NodePairList;
// constructor
NodePairList *nodePairListCreate(void);
// inserts a pair of pointers into the list. The list now owns both pointers
void nodePairListInsert(NodePairList *, struct Node *, struct Node *);
// destructor
void nodePairListDestroy(NodePairList *);

// A list of triples of nodes, vector-style
typedef struct {
  size_t size;
  size_t capacity;
  struct Node **firstElements;
  struct Node **secondElements;
  struct Node **thirdElements;
} NodeTripleList;
// constructor
NodeTripleList *nodeTripleListCreate(void);
// inserts a pair of pointers into the list. The list now owns both pointers
void nodeTripleListInsert(NodeTripleList *, struct Node *, struct Node *,
                          struct Node *);
// destructor
void nodeTripleListDestroy(NodeTripleList *);

// tag for the specialized type of the AST node
typedef enum {
  NT_FILE,
  NT_MODULE,
  NT_IMPORT,
  NT_FUNDECL,
  NT_FIELDDECL,
  NT_STRUCTDECL,
  NT_STRUCTFORWARDDECL,
  NT_UNIONDECL,
  NT_UNIONFORWARDDECL,
  NT_ENUMDECL,
  NT_ENUMFORWARDDECL,
  NT_TYPEDEFDECL,
  NT_VARDECL,
  NT_FUNCTION,
  NT_COMPOUNDSTMT,
  NT_IFSTMT,
  NT_WHILESTMT,
  NT_DOWHILESTMT,
  NT_FORSTMT,
  NT_SWITCHSTMT,
  NT_NUMCASE,
  NT_DEFAULTCASE,
  NT_BREAKSTMT,
  NT_CONTINUESTMT,
  NT_RETURNSTMT,
  NT_ASMSTMT,
  NT_EXPRESSIONSTMT,
  NT_NULLSTMT,
  NT_SEQEXP,
  NT_BINOPEXP,
  NT_UNOPEXP,
  NT_COMPOPEXP,
  NT_LANDASSIGNEXP,
  NT_LORASSIGNEXP,
  NT_TERNARYEXP,
  NT_LANDEXP,
  NT_LOREXP,
  NT_STRUCTACCESSEXP,
  NT_STRUCTPTRACCESSEXP,
  NT_FNCALLEXP,
  NT_IDEXP,
  NT_CONSTEXP,
  NT_AGGREGATEINITEXP,
  NT_ENUMCONSTEXP,
  NT_CASTEXP,
  NT_SIZEOFTYPEEXP,
  NT_SIZEOFEXPEXP,
  NT_KEYWORDTYPE,
  NT_IDTYPE,
  NT_CONSTTYPE,
  NT_ARRAYTYPE,
  NT_PTRTYPE,
  NT_FNPTRTYPE,
  NT_ID,
} NodeType;
// Type of a simple binop (land, lor, and derivatives are complex, like ternary)
typedef enum {
  BO_ASSIGN,
  BO_MULASSIGN,
  BO_DIVASSIGN,
  BO_MODASSIGN,
  BO_ADDASSIGN,
  BO_SUBASSIGN,
  BO_LSHIFTASSIGN,
  BO_LRSHIFTASSIGN,
  BO_ARSHIFTASSIGN,
  BO_BITANDASSIGN,
  BO_BITXORASSIGN,
  BO_BITORASSIGN,
  BO_BITAND,
  BO_BITOR,
  BO_BITXOR,
  BO_SPACESHIP,  // technically not a comparison - doesn't produce bool
  BO_LSHIFT,
  BO_LRSHIFT,
  BO_ARSHIFT,
  BO_ADD,
  BO_SUB,
  BO_MUL,
  BO_DIV,
  BO_MOD,
  BO_ARRAYACCESS,
} BinOpType;
// Type of a comparison op
typedef enum {
  CO_EQ,
  CO_NEQ,
  CO_LT,
  CO_GT,
  CO_LTEQ,
  CO_GTEQ,
} CompOpType;
// Type of a unary op
typedef enum {
  UO_DEREF,
  UO_ADDROF,
  UO_PREINC,
  UO_PREDEC,
  UO_UPLUS,
  UO_NEG,
  UO_LNOT,
  UO_BITNOT,
  UO_POSTINC,
  UO_POSTDEC,
} UnOpType;
// type of a const
typedef enum {
  CT_UBYTE,
  CT_BYTE,
  CT_USHORT,
  CT_SHORT,
  CT_UINT,
  CT_INT,
  CT_ULONG,
  CT_LONG,
  CT_FLOAT,
  CT_DOUBLE,
  CT_STRING,
  CT_CHAR,
  CT_WSTRING,
  CT_WCHAR,
  CT_BOOL,
  CT_RANGE_ERROR,
} ConstType;
char const *constTypeToString(ConstType);
// built-in type
typedef enum {
  TK_VOID,
  TK_UBYTE,
  TK_BYTE,
  TK_CHAR,
  TK_USHORT,
  TK_SHORT,
  TK_UINT,
  TK_INT,
  TK_WCHAR,
  TK_ULONG,
  TK_LONG,
  TK_FLOAT,
  TK_DOUBLE,
  TK_BOOL,
} TypeKeyword;

// the actual node definition
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      struct Node *module;
      NodeList *imports;
      NodeList *bodies;
      char const *filename;
    } file;

    struct {
      struct Node *id;
    } module;
    struct {
      struct Node *id;
    } import;

    struct {
      struct Node *returnType;
      struct Node *id;
      NodePairList *params;  // <type, literal>
    } funDecl;
    struct {
      struct Node *type;
      NodeList *ids;
    } fieldDecl;
    struct {
      struct Node *id;
      NodeList *decls;
    } structDecl;
    struct {
      struct Node *id;
    } structForwardDecl;
    struct {
      struct Node *id;
      NodeList *opts;
    } unionDecl;
    struct {
      struct Node *id;
    } unionForwardDecl;
    struct {
      struct Node *id;
      NodeList *elements;
    } enumDecl;
    struct {
      struct Node *id;
    } enumForwardDecl;
    struct {
      struct Node *type;
      struct Node *id;
    } typedefDecl;

    struct {
      struct Node *returnType;
      struct Node *id;
      NodeTripleList *formals;  // <type, id (nullable), literal (nullable)>
      struct Node *body;
      SymbolTable *localSymbols;
    } function;

    struct {
      NodeList *statements;
      SymbolTable *localSymbols;
    } compoundStmt;
    struct {
      struct Node *condition;
      struct Node *thenStmt;
      struct Node *elseStmt;  // nullable
    } ifStmt;
    struct {
      struct Node *condition;
      struct Node *body;
    } whileStmt;
    struct {
      struct Node *body;
      struct Node *condition;
    } doWhileStmt;
    struct {
      struct Node *initialize;
      struct Node *condition;
      struct Node *update;
      struct Node *body;
      SymbolTable *localSymbols;
    } forStmt;
    struct {
      struct Node *onWhat;
      NodeList *cases;
    } switchStmt;
    struct {
      NodeList *constVals;
      struct Node *body;
    } numCase;
    struct {
      struct Node *body;
    } defaultCase;
    struct {
      struct Node *value;  // nullable
    } returnStmt;
    struct {
      struct Node *type;
      NodePairList *idValuePairs;  // pair of id, value (nullable)
    } varDecl;
    struct {
      struct Node *assembly;
    } asmStmt;
    struct {
      struct Node *expression;
    } expressionStmt;

    struct {
      struct Node *first;
      struct Node *rest;
    } seqExp;
    struct {
      BinOpType op;
      struct Node *lhs;
      struct Node *rhs;
    } binopExp;
    struct {
      UnOpType op;
      struct Node *target;
    } unOpExp;
    struct {
      CompOpType op;
      struct Node *lhs;
      struct Node *rhs;
    } compOpExp;
    struct {
      struct Node *lhs;
      struct Node *rhs;
    } landAssignExp;
    struct {
      struct Node *lhs;
      struct Node *rhs;
    } lorAssignExp;
    struct {
      struct Node *condition;
      struct Node *thenExp;
      struct Node *elseExp;
    } ternaryExp;
    struct {
      struct Node *lhs;
      struct Node *rhs;
    } landExp;
    struct {
      struct Node *lhs;
      struct Node *rhs;
    } lorExp;
    struct {
      struct Node *base;
      struct Node *element;
    } structAccessExp;
    struct {
      struct Node *base;
      struct Node *element;
    } structPtrAccessExp;
    struct {
      struct Node *who;
      NodeList *args;
    } fnCallExp;
    struct {
      char *id;
    } idExp;
    struct {
      ConstType type;
      union {
        uint8_t ubyteVal;
        int8_t byteVal;
        uint16_t ushortVal;
        int16_t shortVal;
        uint32_t uintVal;
        int32_t intVal;
        uint64_t ulongVal;
        int64_t longVal;
        uint32_t floatBits;
        uint64_t doubleBits;
        uint8_t *stringVal;
        uint8_t charVal;
        uint32_t *wstringVal;
        uint32_t wcharVal;
        bool boolVal;
      } value;
    } constExp;
    struct {
      NodeList *elements;
    } aggregateInitExp;
    struct {
      char *id;
    } enumConstExp;
    struct {
      struct Node *toWhat;
      struct Node *target;
    } castExp;
    struct {
      struct Node *target;
    } sizeofTypeExp;
    struct {
      struct Node *target;
    } sizeofExpExp;

    struct {
      TypeKeyword type;
    } typeKeyword;
    struct {
      char *id;
    } idType;
    struct {
      struct Node *target;
    } constType;
    struct {
      struct Node *element;
      struct Node *size;
    } arrayType;
    struct {
      struct Node *target;
    } ptrType;
    struct {
      struct Node *returnType;
      NodeList *argTypes;
    } fnPtrType;

    struct {
      char *id;
    } id;
  } data;
} Node;

// absolute value of maximum for some data type
extern uint64_t const UBYTE_MAX;
extern uint64_t const BYTE_MAX;
extern uint64_t const BYTE_MIN;
extern uint64_t const UINT_MAX;
extern uint64_t const INT_MAX;
extern uint64_t const INT_MIN;
extern uint64_t const ULONG_MAX;
extern uint64_t const LONG_MAX;
extern uint64_t const LONG_MIN;

// constructors
// Note that all pointers should be owning pointers
Node *fileNodeCreate(size_t line, size_t character, Node *module,
                     NodeList *imports, NodeList *bodyParts,
                     char const *filename);
Node *moduleNodeCreate(size_t line, size_t character, Node *moduleId);
Node *importNodeCreate(size_t line, size_t character, Node *importId);
Node *funDeclNodeCreate(size_t line, size_t character, Node *returnType,
                        Node *functionId, NodePairList *args);
Node *fieldDeclNodeCreate(size_t line, size_t character, Node *varType,
                          NodeList *ids);
Node *structDeclNodeCreate(size_t line, size_t character, Node *structId,
                           NodeList *elements);
Node *structForwardDeclNodeCreate(size_t line, size_t character,
                                  Node *structId);
Node *unionDeclNodeCreate(size_t line, size_t character, Node *unionId,
                          NodeList *opts);
Node *unionForwardDeclNodeCreate(size_t line, size_t character, Node *unionId);
Node *enumDeclNodeCreate(size_t line, size_t character, Node *enumId,
                         NodeList *elements);
Node *enumForwardDeclNodeCreate(size_t line, size_t character, Node *enumId);
Node *typedefNodeCreate(size_t line, size_t character, Node *type, Node *newId);
Node *functionNodeCreate(size_t line, size_t character, Node *returnType,
                         Node *functionId, NodeTripleList *args, Node *body);
Node *varDeclNodeCreate(size_t line, size_t character, Node *type,
                        NodePairList *idValuePairs);
Node *compoundStmtNodeCreate(size_t line, size_t character, NodeList *stmts);
Node *ifStmtNodeCreate(size_t line, size_t character, Node *condition,
                       Node *thenCase, Node *elseCase);
Node *whileStmtNodeCreate(size_t line, size_t character, Node *condition,
                          Node *body);
Node *doWhileStmtNodeCreate(size_t line, size_t character, Node *condition,
                            Node *body);
Node *forStmtNodeCreate(size_t line, size_t character, Node *initializer,
                        Node *condition, Node *update, Node *body);
Node *switchStmtNodeCreate(size_t line, size_t character, Node *switchedOn,
                           NodeList *cases);
Node *numCaseNodeCreate(size_t line, size_t character, NodeList *values,
                        Node *body);
Node *defaultCaseNodeCreate(size_t line, size_t character, Node *body);
Node *breakStmtNodeCreate(size_t line, size_t character);
Node *continueStmtNodeCreate(size_t line, size_t character);
Node *returnStmtNodeCreate(size_t line, size_t character, Node *value);
Node *asmStmtNodeCreate(size_t line, size_t character, Node *asmString);
Node *expressionStmtNodeCreate(size_t line, size_t character, Node *expression);
Node *nullStmtNodeCreate(size_t line, size_t character);
Node *seqExpNodeCreate(size_t line, size_t character, Node *first, Node *rest);
Node *binOpExpNodeCreate(size_t line, size_t character, BinOpType, Node *lhs,
                         Node *rhs);
Node *unOpExpNodeCreate(size_t line, size_t character, UnOpType, Node *target);
Node *compOpExpNodeCreate(size_t line, size_t character, CompOpType, Node *lhs,
                          Node *rhs);
Node *landAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                              Node *rhs);
Node *lorAssignExpNodeCreate(size_t line, size_t character, Node *lhs,
                             Node *rhs);
Node *ternaryExpNodeCreate(size_t line, size_t character, Node *condition,
                           Node *trueCase, Node *falseCase);
Node *landExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs);
Node *lorExpNodeCreate(size_t line, size_t character, Node *lhs, Node *rhs);
Node *structAccessExpNodeCreate(size_t line, size_t character, Node *base,
                                Node *elementId);
Node *structPtrAccessExpNodeCreate(size_t line, size_t character, Node *basePtr,
                                   Node *elementId);
Node *fnCallExpNodeCreate(size_t line, size_t character, Node *function,
                          NodeList *args);
Node *idExpNodeCreate(size_t line, size_t character, char *idString);
Node *constZeroIntExpNodeCreate(size_t line, size_t character,
                                char *constantString);
Node *constBinaryIntExpNodeCreate(size_t line, size_t character,
                                  char *constantString);
Node *constOctalIntExpNodeCreate(size_t line, size_t character,
                                 char *constantString);
Node *constDecimalIntExpNodeCreate(size_t line, size_t character,
                                   char *constantString);
Node *constHexadecimalIntExpNodeCreate(size_t line, size_t character,
                                       char *constantString);
Node *constFloatExpNodeCreate(size_t line, size_t character,
                              char *constantString);
Node *constCharExpNodeCreate(size_t line, size_t character,
                             char *constantString);
Node *constStringExpNodeCreate(size_t line, size_t character,
                               char *constantString);
Node *constWCharExpNodeCreate(size_t line, size_t character,
                              char *constantString);
Node *constWStringExpNodeCreate(size_t line, size_t character,
                                char *constantString);
Node *aggregateInitExpNodeCreate(size_t line, size_t character,
                                 NodeList *elements);
Node *enumConstExpNodeCreate(size_t line, size_t character, char *constString);
Node *constTrueNodeCreate(size_t line, size_t character);
Node *constFalseNodeCreate(size_t line, size_t character);
Node *castExpNodeCreate(size_t line, size_t character, Node *type,
                        Node *target);
Node *sizeofTypeExpNodeCreate(size_t line, size_t character, Node *target);
Node *sizeofExpExpNodeCreate(size_t line, size_t character, Node *target);
Node *typeKeywordNodeCreate(size_t line, size_t character, TypeKeyword type);
Node *idTypeNodeCreate(size_t line, size_t character, char *idString);
Node *constTypeNodeCreate(size_t line, size_t character, Node *target);
Node *arrayTypeNodeCreate(size_t line, size_t character, Node *target,
                          Node *size);
Node *ptrTypeNodeCreate(size_t line, size_t character, Node *target);
Node *fnPtrTypeNodeCreate(size_t line, size_t character, Node *returnType,
                          NodeList *argTypes);
Node *idNodeCreate(size_t line, size_t character, char *idString);

// Destructor
void nodeDestroy(Node *);

#endif  // TLC_AST_AST_H_