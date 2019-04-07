// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A "polymorphic" AST node.

#ifndef TLC_AST_AST_H_
#define TLC_AST_AST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// tag for the specialized type of the AST node
typedef enum {
  TYPE_PROGRAM,
  TYPE_MODULE,
  TYPE_IMPORT,
  TYPE_FUNDECL,
  TYPE_VARDECL,
  TYPE_STRUCTDECL,
  TYPE_UNIONDECL,
  TYPE_ENUMDECL,
  TYPE_TYPEDEFDECL,
  TYPE_FUNCTION,
  TYPE_COMPOUNDSTMT,
  TYPE_IFSTMT,
  TYPE_WHILESTMT,
  TYPE_DOWHILESTMT,
  TYPE_FORSTMT,
  TYPE_SWITCHSTMT,
  TYPE_NUMCASE,
  TYPE_DEFAULTCASE,
  TYPE_BREAKSTMT,
  TYPE_CONTINUESTMT,
  TYPE_RETURNSTMT,
  TYPE_VARDECLSTMT,
  TYPE_ASMSTMT,
  TYPE_EXPRESSIONSTMT,
  TYPE_NULLSTMT,
  TYPE_SEQEXP,
  TYPE_BINOPEXP,
  TYPE_UNOPEXP,
  TYPE_COMPOPEXP,
  TYPE_LANDASSIGNEXP,
  TYPE_LORASSIGNEXP,
  TYPE_TERNARYEXP,
  TYPE_LANDEXP,
  TYPE_LOREXP,
  TYPE_STRUCTACCESSEXP,
  TYPE_STRUCTPTRACCESSEXP,
  TYPE_FNCALLEXP,
  TYPE_IDEXP,
  TYPE_CONSTEXP,
  TYPE_AGGREGATEINITEXP,
  TYPE_CASTEXP,
  TYPE_SIZEOFTYPEEXP,
  TYPE_SIZEOFEXPEXP,
  TYPE_KEYWORDTYPE,
  TYPE_IDTYPE,
  TYPE_CONSTTYPE,
  TYPE_ARRAYTYPE,
  TYPE_PTRTYPE,
  TYPE_FNPTRTYPE,
  TYPE_ID,
} NodeType;
// Type of a simple binop (land, lor, and derivatives are complex, like ternary)
typedef enum {
  OP_ASSIGN,
  OP_MULASSIGN,
  OP_DIVASSIGN,
  OP_MODASSIGN,
  OP_ADDASSIGN,
  OP_SUBASSIGN,
  OP_LSHIFTASSIGN,
  OP_LRSHIFTASSIGN,
  OP_ARSHIFTASSIGN,
  OP_BITANDASSIGN,
  OP_BITXORASSIGN,
  OP_BITORASSIGN,
  OP_BITAND,
  OP_BITOR,
  OP_BITXOR,
  OP_SPACESHIP,  // technically not a comparison - doesn't produce bool
  OP_LSHIFT,
  OP_LRSHIFT,
  OP_ARSHIFT,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_ARRAYACCESS,
} BinOpType;
// Type of a comparison op
typedef enum {
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_GT,
  OP_LTEQ,
  OP_GTEQ,
} CompOpType;
// Type of a unary op
typedef enum {
  OP_DEREF,
  OP_ADDROF,
  OP_PREINC,
  OP_PREDEC,
  OP_UPLUS,
  OP_NEG,
  OP_LNOT,
  OP_BITNOT,
  OP_POSTINC,
  OP_POSTDEC,
} UnOpType;
// type of a const
typedef enum {
  CTYPE_UBYTE,
  CTYPE_BYTE,
  CTYPE_UINT,
  CTYPE_INT,
  CTYPE_ULONG,
  CTYPE_LONG,
  CTYPE_FLOAT,
  CTYPE_DOUBLE,
  CTYPE_STRING,
  CTYPE_CHAR,
  CTYPE_WSTRING,
  CTYPE_WCHAR,
  CTYPE_BOOL,
  CTYPE_RANGE_ERROR,
} ConstType;
// general category that a const should be interpreted as - guarenteed by lexer
typedef enum {
  TYPEHINT_INT,
  TYPEHINT_FLOAT,
  TYPEHINT_STRING,
  TYPEHINT_CHAR,
  TYPEHINT_WSTRING,
  TYPEHINT_WCHAR,
} ConstTypeHint;
// built-in type
typedef enum {
  TYPEKWD_VOID,
  TYPEKWD_UBYTE,
  TYPEKWD_BYTE,
  TYPEKWD_UINT,
  TYPEKWD_INT,
  TYPEKWD_ULONG,
  TYPEKWD_LONG,
  TYPEKWD_FLOAT,
  TYPEKWD_DOUBLE,
  TYPEKWD_BOOL,
} TypeKeyword;

// the actual node definition
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      struct Node *module;
      size_t numImports;
      struct Node **imports;
      size_t numBodies;
      struct Node **bodies;
    } program;

    struct {
      struct Node *id;
    } module;
    struct {
      struct Node *id;
    } import;

    struct {
      struct Node *returnType;
      struct Node *id;
      size_t numParamTypes;
      struct Node **paramTypes;
    } funDecl;
    struct {
      struct Node *type;
      size_t numIds;
      struct Node **ids;
    } varDecl;
    struct {
      struct Node *id;
      size_t numDecls;
      struct Node **decls;
    } structDecl;
    struct {
      struct Node *id;
      size_t numOpts;
      struct Node **opts;
    } unionDecl;
    struct {
      struct Node *id;
      size_t numElements;
      struct Node **elements;
    } enumDecl;
    struct {
      struct Node *type;
      struct Node *id;
    } typedefDecl;

    struct {
      struct Node *returnType;
      struct Node *id;
      size_t numFormals;
      struct Node **formalTypes;
      struct Node **formalIds;  // nullable elements
      struct Node *body;
    } function;

    struct {
      size_t numStatements;
      struct Node **statements;
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
    } forStmt;
    struct {
      struct Node *onWhat;
      size_t numCases;
      struct Node **cases;
    } switchStmt;
    struct {
      struct Node *constVal;
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
      size_t numIds;
      struct Node **ids;
      struct Node **values;  // nullable elements
    } varDeclStmt;
    struct {
      struct Node *assembly;
    } asmStmt;
    struct {
      struct Node *expression;
    } expressionStmt;

    struct {
      struct Node *first;
      struct Node *second;
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
      size_t numArgs;
      struct Node **args;
    } fnCallExp;
    struct {
      char *id;
    } idExp;
    struct {
      ConstType type;
      union {
        uint8_t ubyteVal;
        int8_t byteVal;
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
      size_t numElements;
      struct Node **elements;
    } aggregateInitExp;
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
    } keywordType;
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
      size_t numArgTypes;
      struct Node **argTypes;
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
Node *nodeCreate(size_t line, size_t character);  // base constructor
Node *programNodeCreate(size_t line, size_t character, Node *module,
                        size_t numImports, Node **imports, size_t numBodyParts,
                        Node **bodyParts);
Node *moduleNodeCreate(size_t line, size_t character, Node *moduleId);
Node *importNodeCreate(size_t line, size_t character, Node *importId);
Node *funDeclNodeCreate(size_t line, size_t character, Node *returnType,
                        Node *functionId, size_t numArgs, Node **argTypes);
Node *varDeclNodeCreate(size_t line, size_t character, Node *varType,
                        size_t numIds, Node **ids);
Node *structDeclNodeCreate(size_t line, size_t character, Node *structId,
                           size_t numElements, Node **elements);
Node *unionDeclNodeCreate(size_t line, size_t character, Node *unionId,
                          size_t numOpts, Node **opts);
Node *enumDeclNodeCreate(size_t line, size_t character, Node *enumId,
                         size_t numElements, Node **elements);
Node *typedefNodeCreate(size_t line, size_t character, Node *type, Node *newId);
Node *functionNodeCreate(size_t line, size_t character, Node *returnType,
                         Node *functionId, size_t numArgs, Node **argTypes,
                         Node **argNames, Node *body);
Node *compoundStmtNodeCreate(size_t line, size_t character, size_t numStmts,
                             Node **stmts);
Node *ifStmtNodeCreate(size_t line, size_t character, Node *condition,
                       Node *thenCase, Node *elseCase);
Node *whileStmtNodeCreate(size_t line, size_t character, Node *condition,
                          Node *body);
Node *doWhileStmtNodeCreate(size_t line, size_t character, Node *condition,
                            Node *body);
Node *forStmtNodeCreate(size_t line, size_t character, Node *initializer,
                        Node *condition, Node *update, Node *body);
Node *switchStmtNodeCreate(size_t line, size_t character, Node *switchedOn,
                           size_t numCases, Node **cases);
Node *numCaseNodeCreate(size_t line, size_t character, Node *value, Node *body);
Node *defaultCaseNodeCreate(size_t line, size_t character, Node *body);
Node *breakStmtNodeCreate(size_t line, size_t character);
Node *continueStmtNodeCreate(size_t line, size_t character);
Node *returnStmtNodeCreate(size_t line, size_t character, Node *value);
Node *varDeclStmtNodeCreate(size_t line, size_t character, Node *type,
                            size_t numIds, Node **ids, Node **initializers);
Node *asmStmtNodeCreate(size_t line, size_t character, Node *asmString);
Node *expressionStmtNodeCreate(size_t line, size_t character, Node *expression);
Node *nullStmtNodeCreate(size_t line, size_t character);
Node *seqExpNodeCreate(size_t line, size_t character, Node *first,
                       Node *second);
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
Node *fnCallExpNodeCreate(size_t line, size_t character, Node *functionId,
                          size_t numArgs, Node **args);
Node *idExpNodeCreate(size_t line, size_t character, char *idString);
Node *constExpNodeCreate(size_t line, size_t character, ConstTypeHint type,
                         char *constantString);
Node *aggregateInitExpNodeCreate(size_t line, size_t character,
                                 size_t numElements, Node **elements);
Node *constTrueNodeCreate(size_t line, size_t character);
Node *constFalseNodeCreate(size_t line, size_t character);
Node *castExpNodeCreate(size_t line, size_t character, Node *type,
                        Node *target);
Node *sizeofTypeExpNodeCreate(size_t line, size_t character, Node *target);
Node *sizeofExpExpNodeCreate(size_t line, size_t character, Node *target);
Node *keywordTypeNodeCreate(size_t line, size_t character, TypeKeyword type);
Node *idTypeNodeCreate(size_t line, size_t character, char *idString);
Node *constTypeNodeCreate(size_t line, size_t character, Node *target);
Node *arrayTypeNodeCreate(size_t line, size_t character, Node *target,
                          Node *size);
Node *ptrTypeNodeCreate(size_t line, size_t character, Node *target);
Node *fnPtrTypeNodeCreate(size_t line, size_t character, Node *returnType,
                          size_t numArgs, Node **argTypes);
Node *idNodeCreate(size_t line, size_t character, char *idString);

// Destructor
void nodeDestroy(Node *);

#endif  // TLC_AST_AST_H_