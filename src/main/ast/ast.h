#ifndef TLC_AST_AST_H_
#define TLC_AST_AST_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  TYPE_PROGRAM,
  TYPE_MODULE,
  TYPE_USING,
  TYPE_FUNDECL,
  TYPE_VARDECL,
  TYPE_STRUCTDECL,
  TYPE_TYPEDEF,
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
  TYPE_CASTEXP,
  TYPE_SIZEOFEXP,
  TYPE_KEYWORDTYPE,
  TYPE_IDTYPE,
  TYPE_CONSTTYPE,
  TYPE_ARRAYTYPE,
  TYPE_PTRTYPE,
  TYPE_FNPTRTYPE,
} NodeType;
typedef enum {
  OP_ASSIGN,
  OP_MULASSIGN,
  OP_DIVASSIGN,
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
  OP_SPACESHIP,
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
typedef enum {
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_GT,
  OP_LTEQ,
  OP_GTEQ,
} CompOpType;
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
typedef enum {
  CTYPE_UBYTE,
  CTYPE_BYTE,
  CTYPE_UINT,
  CTYPE_INT,
  CTYPE_ULONG,
  CTYPE_LONG,
  CTYPE_FLOAT,
  CTYPE_DOUBLE,
  CTYPE_CSTRING,
  CTYPE_CHAR,
  CTYPE_WCSTRING,
  CTYPE_WCHAR,
  CTYPE_BOOL,
} ConstType;
typedef enum {
  TYPEKWD_VOID,
  TYPEKWD_UBYTE,
  TYPEKWD_BYTE,
  TYPEKWD_UINT,
  TYPEKWD_INT,
  TYPEKWD_ULONG,
  TYPEKWD_LONG,
  TYPEKWD_FLOAT,
  TYPEKWD_DOUBLE
} TypeKeyword;

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
      char *name;
    } module;
    struct {
      char *name;
    } import;

    struct {
      struct Node *returnType;
      char *name;
      size_t numParamTypes;
      struct Node **paramTypes;
    } funDecl;
    struct {
      struct Node *type;
      size_t numIds;
      char **ids;
    } varDecl;
    struct {
      char *id;
      size_t numElements;
      struct Node **elements;
    } structDecl;
    struct {
      char *id;
      struct Node *type;
    } typedefDecl;

    struct {
      struct Node *returnType;
      char *id;
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
      struct Node *condition;
      struct Node *body;
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
      char **ids;
      struct Node **values;  // nullable elements
    } varDeclStmt;
    struct {
      char *assembly;
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
      struct Node *exp;
    } unopExp;
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
      char *element;
    } structAccessExp;
    struct {
      struct Node *base;
      char *element;
    } structPtrAccessExp;
    struct {
      struct Node *who;
      size_t numArgs;
      struct Node **withWhat;
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
        uint8_t *cstringVal;
        uint8_t charVal;
        uint32_t *wcstringVal;
        uint32_t wcharVal;
        bool boolVal;
      } value;
    } constExp;
    struct {
      struct Node *toWhat;
      struct Node *exp;
    } castExp;
    struct {
      struct Node *target;
    } sizeofExp;

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
      struct Node *target;
      size_t size;
    } arrayType;
    struct {
      struct Node *target;
    } ptrType;
    struct {
      struct Node *returnType;
      size_t numArgTypes;
      struct Node **argTypes;
    } fnPtrType;
  } data;
} Node;
#endif  // TLC_AST_AST_H_