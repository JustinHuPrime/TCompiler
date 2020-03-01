// Copyright 2020 Justin Hu
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

/**
 * @file
 * abstract syntax tree definition
 */

#ifndef TLC_PARSER_AST_H_
#define TLC_PARSER_AST_H_

#include <stddef.h>

#include "util/container/vector.h"

/** the type of an AST node */
typedef enum {
  NT_FILE,

  NT_MODULE,
  NT_IMPORT,

  NT_FUNDEFN,
  NT_VARDEFN,

  NT_FUNDECL,
  NT_VARDECL,
  NT_OPAQUEDECL,
  NT_STRUCTDECL,
  NT_UNIONDECL,
  NT_ENUMDECL,
  NT_TYPEDEFDECL,

  NT_COMPOUNDSTMT,
  NT_IFSTMT,
  NT_WHILESTMT,
  NT_DOWHILESTMT,
  NT_FORSTMT,
  NT_SWITCHSTMT,
  NT_BREAKSTMT,
  NT_CONTINUESTMT,
  NT_RETURNSTMT,
  NT_ASMSTMT,
  NT_VARDEFNSTMT,
  NT_EXPRESSIONSTMT,
  NT_NULLSTMT,

  NT_SWITCHCASE,
  NT_SWITCHDEFAULT,

  NT_BINOPEXP, /**< node for a generalized syntactic binary operation */
  NT_TERNARYEXP,
  NT_UNOPEXP, /**< node for a generalized syntactic unary operation */
  NT_FUNCALLEXP,

  NT_LITERAL,

  NT_KEYWORDTYPE,
  NT_MODIFIEDTYPE, /**< node for a generalized simple modified type */
  NT_ARRAYTYPE,
  NT_FUNPTRTYPE,

  NT_SCOPEDID,
  NT_ID,

  NT_UNPARSED, /**< tokens representing a variable declaration, definition,
                      definition statement, or expression that are yet to be
                      parsed*/
} NodeType;

/** the type of a syntactic binary operation */
typedef enum {
  BO_SEQ,
  BO_ASSIGN,
  BO_MULASSIGN,
  BO_DIVASSIGN,
  BO_ADDASSIGN,
  BO_SUBASSIGN,
  BO_LSHIFTASSIGN,
  BO_ARSHIFTASSIGN,
  BO_LRSHIFTASSIGN,
  BO_BITANDASSIGN,
  BO_BITXORASSIGN,
  BO_BITORASSIGN,
  BO_LANDASSIGN,
  BO_LORASSIGN,
  BO_LAND,
  BO_LOR,
  BO_BITAND,
  BO_BITOR,
  BO_BITXOR,
  BO_EQ,
  BO_NEQ,
  BO_LT,
  BO_GT,
  BO_LTEQ,
  BO_GTEQ,
  BO_SPACESHIP,
  BO_LSHIFT,
  BO_ARSHIFT,
  BO_LRSHIFT,
  BO_ADD,
  BO_SUB,
  BO_MUL,
  BO_DIV,
  BO_MOD,
  BO_FIELD,
  BO_PTRFIELD,
  BO_ARRAY,
  BO_CAST,
} BinOpType;

/** the type of a syntactic unary operation */
typedef enum {
  UO_DEREF,
  UO_ADDROF,
  UO_PREINC,
  UO_PREDEC,
  UO_NEG,
  UO_LNOT,
  UO_BITNOT,
  UO_POSTINC,
  UO_POSTDEC,
  UO_NEGASSIGN,
  UO_LNOTASSIGN,
  UO_BITNOTASSIGN,
  UO_SIZEOFEXP,  /**< sizeof operator applied to an expression */
  UO_SIZEOFTYPE, /**< sizeof operator applied to a type */
} UnOpType;

/** the type of a literal */
typedef enum {
  LT_UBYTE,
  LT_BYTE,
  LT_USHORT,
  LT_SHORT,
  LT_UINT,
  LT_INT,
  LT_ULONG,
  LT_LONG,
  LT_FLOAT,
  LT_DOUBLE,
  LT_STRING,
  LT_CHAR,
  LT_WSTRING,
  LT_WCHAR,
  LT_BOOL,
  LT_NULL,
  LT_ENUMCONST,
  LT_AGGREGATEINIT,
} LiteralType;

/** the type of a simple type modifier */
typedef enum {
  TMT_CONST,
  TMT_VOLATILE,
  TMT_POINTER,
} TypeModifierType;

/** the type of a type keword */
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

/** an AST node */
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      struct Node *module; /**< NT_MODULE */
      Vector imports;      /**< vector of Nodes, each is an NT_IMPORT */
      Vector body; /**< vector of Nodes, each is a definition or declaration */
    } file;

    struct {
      struct Node *id; /**< NT_SCOPEDID */
    } module;
    struct {
      struct Node *id; /**< NT_SCOPEDID */
    } import;

    struct {
      struct Node *returnType; /**< type */
      struct Node *funName;    /**< NT_ID */
      Vector argTypes;         /**< vector of Nodes, each is a type */
      Vector argNames;    /**< vector of nullable Nodes, each is an NT_ID */
      Vector argLiterals; /**< vector of nullable Nodes, each is a literal */
      struct Node *body;  /**< NT_COMPOUNDSTMT */
    } funDefn;
    struct {
      struct Node *type;   /**< type */
      Vector names;        /**< vector of Nodes, each is an NT_ID */
      Vector initializers; /**< vector of nullable Nodes, each is a literal */
    } varDefn;

    struct {
      struct Node *returnType; /**< type */
      struct Node *funName;    /**< NT_ID */
      Vector argTypes;         /**< vector of Nodes, each is a type */
      Vector argNames;    /**< vector of nullable Nodes, each is an NT_ID */
      Vector argLiterals; /**< vector of nullable Nodes, each is a literal */
    } funDecl;
    struct {
      struct Node *type; /**< type */
      Vector names;      /**< vector of Nodes, each is an NT_ID */
    } varDecl;
    struct {
      struct Node *name; /**< NT_ID */
    } opaqueDecl;
    struct {
      struct Node *name; /**< NT_ID */
      Vector fields;     /**< vector of Nodes, each is an NT_VARDECL */
    } structDecl;
    struct {
      struct Node *name; /**< NT_ID */
      Vector options;    /**< vector of Nodes, each is an NT_VARDECL */
    } unionDecl;
    struct {
      struct Node *name;     /**< NT_ID */
      Vector constantNames;  /**< vector of Nodes, each is an NT_ID */
      Vector constantValues; /**< vector of nullable Nodes, each is an extended
                                int literal */
    } enumDecl;
    struct {
      struct Node *originalType; /**< type */
      struct Node *name;         /**< NT_ID */
    } typedefDecl;

    struct {
      Vector stmts; /**< vector of Nodes, each is a statement */
    } compoundStmt;
    struct {
      struct Node *predicate;   /**< expression */
      struct Node *consequent;  /**< statement */
      struct Node *alternative; /**< nullable statement */
    } ifStmt;
    struct {
      struct Node *condition; /**< expression */
      struct Node *body;      /**< statement */
    } whileStmt;
    struct {
      struct Node *body;      /**< expression */
      struct Node *condition; /**< statement */
    } doWhileStmt;
    struct {
      struct Node *
          initializer; /**< NT_VARDEFNSTMT, NT_EXPRESSIONSTMT, or NT_NULLSTMT */
      struct Node *condition; /**< expression */
      struct Node *increment; /**< nullable */
      struct Node *body;      /**< statement */
    } forStmt;
    struct {
      struct Node *condition; /**< expression */
      Vector cases;           /**< vector of Nodes, each either NT_SWITCHCASE or
                                 NT_SWITCHDEFAULT */
    } switchStmt;
    // struct { // has no data
    // } breakStmt;
    // struct { // has no data
    // } continueStmt;
    struct {
      struct Node *value; /**< nullable */
    } returnStmt;
    struct {
      struct Node *assembly; /**< string literal */
    } asmStmt;
    struct {
      struct Node *type; /**< type */
      Vector names;      /**< vector of Nodes, each is an NT_ID */
      Vector
          initializers; /**< vector of nullable Nodes, each is an expression */
    } varDefnStmt;
    struct {
      struct Node *expression; /**< expression */
    } expressionStmt;
    // struct { // has no data
    // } nullStmt;

    struct {
      Vector values; /**< vector of Nodes, each is an extended int literal */
      struct Node *body; /**< statement */
    } switchCase;
    struct {
      struct Node *body; /**< statement */
    } switchDefault;

    struct {
      BinOpType op;
      struct Node *lhs;
      struct Node *rhs;
    } binopExp;
    struct {
      struct Node *predicate;   /**< expression */
      struct Node *consequent;  /**< expression */
      struct Node *alternative; /**< expression */
    } ternaryExp;
    struct {
      UnOpType op;
      struct Node *target;
    } unopExp;
    struct {
      struct Node *function; /**< expression */
      Vector arguments;      /**< vector of Nodes, each is an expression */
    } funCallExp;

    struct {
      LiteralType type;
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
        // LT_NULL has no additional data
        bool boolVal;
        struct Node *enumConstVal;
        Vector aggregateInitVal; /**< vector of Nodes, each is an NT_LITERAL or
                                    an NT_SCOPEDID (enumeration constant) */
      } value;
    } literal;

    struct {
      TypeKeyword keyword;
    } keywordType;
    struct {
      TypeModifierType modifier;
      struct Node *baseType; /**< type */
    } modifiedType;
    struct {
      struct Node *baseType; /**< type */
      struct Node *size;     /**< extended int literal */
    } arrayType;
    struct {
      struct Node *returnType; /**< type */
      Vector argTypes;         /**< vector of Nodes, each is a type */
      Vector argNames; /**< vector of nullable Nodes, each is an NT_ID */
    } funPtrType;

    struct {
      Vector components; /**< vector of Nodes, each is an NT_ID */
    } scopedId;
    struct {
      char *id;
    } id;

    struct {
      Vector tokens; /**< vector of Tokens */
    } unparsed;
  } data;
} Node;

/**
 * deinitializes a node
 *
 * @param n Node to deinitialize - not null
 */
void nodeDeinit(Node *n);

#endif  // TLC_PARSER_AST_H_