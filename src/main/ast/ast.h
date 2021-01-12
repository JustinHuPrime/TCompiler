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

/**
 * @file
 * abstract syntax tree definition
 */

#ifndef TLC_AST_AST_H_
#define TLC_AST_AST_H_

#include <stddef.h>

#include "ast/environment.h"
#include "ast/symbolTable.h"
#include "lexer/lexer.h"
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
  UO_PARENS, /**< included only for line/character tracking, no semantic effects
              */
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
  LT_AGGREGATEINIT,
} LiteralType;

// type modifier list is shared from symbolTable.h
// type keyword list is shared from symbolTable.h

/** an AST node */
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      HashMap *stab;       /**< symbol table for file */
      struct Node *module; /**< NT_MODULE */
      Vector *imports;     /**< vector of Nodes, each is an NT_IMPORT */
      Vector
          *bodies; /**< vector of Nodes, each is a definition or declaration */
    } file;

    struct {
      struct Node *id; /**< NT_SCOPEDID or NT_ID */
    } module;
    struct {
      struct Node *id;                 /**< NT_SCOPEDID or NT_ID */
      FileListEntry const *referenced; /**< File that's referenced */
    } import;

    struct {
      struct Node *returnType; /**< type */
      struct Node *name;       /**< NT_ID */
      Vector *argTypes;        /**< vector of Nodes, each is a type */
      Vector *argNames;  /**< vector of nullable Nodes, each is an NT_ID */
      HashMap *argStab;  /**< symbol table for arguments */
      struct Node *body; /**< NT_COMPOUNDSTMT or NT_UNPARSED */
    } funDefn;
    struct {
      struct Node *type;    /**< type */
      Vector *names;        /**< vector of Nodes, each is an NT_ID */
      Vector *initializers; /**< vector of nullable Nodes, each is a literal */
    } varDefn;

    struct {
      struct Node *returnType; /**< type */
      struct Node *name;       /**< NT_ID */
      Vector *argTypes;        /**< vector of Nodes, each is a type */
      Vector *argNames; /**< vector of nullable Nodes, each is an NT_ID */
    } funDecl;
    struct {
      struct Node *type; /**< type */
      Vector *names;     /**< vector of Nodes, each is an NT_ID */
    } varDecl;
    struct {
      struct Node *name; /**< NT_ID */
    } opaqueDecl;
    struct {
      struct Node *name; /**< NT_ID */
      Vector *fields;    /**< vector of Nodes, each is an NT_VARDECL */
    } structDecl;
    struct {
      struct Node *name; /**< NT_ID */
      Vector *options;   /**< vector of Nodes, each is an NT_VARDECL */
    } unionDecl;
    struct {
      struct Node *name;      /**< NT_ID */
      Vector *constantNames;  /**< vector of Nodes, each is an NT_ID */
      Vector *constantValues; /**< vector of nullable Nodes, each is an extended
                                int literal */
    } enumDecl;
    struct {
      struct Node *originalType; /**< type */
      struct Node *name;         /**< NT_ID */
    } typedefDecl;

    struct {
      HashMap *stab; /**< symbol table for this scope */
      Vector *stmts; /**< vector of Nodes, each is a statement */
    } compoundStmt;
    struct {
      struct Node *predicate;   /**< expression */
      struct Node *consequent;  /**< statement */
      HashMap *consequentStab;  /**< symbol table */
      struct Node *alternative; /**< nullable statement */
      HashMap *alternativeStab; /**< nullable symbol table */
    } ifStmt;
    struct {
      struct Node *condition; /**< expression */
      struct Node *body;      /**< statement */
      HashMap *bodyStab;      /**< symbol table */
    } whileStmt;
    struct {
      struct Node *body;      /**< statement */
      HashMap *bodyStab;      /**< symbol table */
      struct Node *condition; /**< expression */
    } doWhileStmt;
    struct {
      HashMap *loopStab; /**< symbol table */
      struct Node *
          initializer; /**< NT_VARDEFNSTMT, NT_EXPRESSIONSTMT, or NT_NULLSTMT */
      struct Node *condition; /**< expression */
      struct Node *increment; /**< nullable expression */
      struct Node *body;      /**< statement */
      HashMap *bodyStab;      /**< symbol table */
    } forStmt;
    struct {
      struct Node *condition; /**< expression */
      Vector *cases;          /**< vector of Nodes, each either NT_SWITCHCASE or
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
      Vector *names;     /**< vector of Nodes, each is an NT_ID */
      Vector
          *initializers; /**< vector of nullable Nodes, each is an expression */
    } varDefnStmt;
    struct {
      struct Node *expression; /**< expression */
    } expressionStmt;
    // struct { // has no data
    // } nullStmt;

    struct {
      Vector *values; /**< vector of Nodes, each is an extended int literal */
      struct Node *body; /**< statement */
      HashMap *bodyStab; /**< symbol table for the body */
    } switchCase;
    struct {
      struct Node *body; /**< statement */
      HashMap *bodyStab; /**< symbol table for the body */
    } switchDefault;

    struct {
      BinOpType op;
      struct Node *lhs;
      struct Node *rhs;
    } binOpExp;
    struct {
      struct Node *predicate;   /**< expression */
      struct Node *consequent;  /**< expression */
      struct Node *alternative; /**< expression */
    } ternaryExp;
    struct {
      UnOpType op;
      struct Node *target;
    } unOpExp;
    struct {
      struct Node *function; /**< expression */
      Vector *arguments;     /**< vector of Nodes, each is an expression */
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
        Vector *aggregateInitVal; /**< vector of Nodes, each is an NT_LITERAL or
                                    an NT_SCOPEDID (enumeration constant) */
      } data;
    } literal;

    struct {
      TypeKeyword keyword;
    } keywordType;
    struct {
      TypeModifier modifier;
      struct Node *baseType; /**< type */
    } modifiedType;
    struct {
      struct Node *baseType; /**< type */
      struct Node *size;     /**< extended int literal */
    } arrayType;
    struct {
      struct Node *returnType; /**< type */
      Vector *argTypes;        /**< vector of Nodes, each is a type */
      Vector *argNames; /**< vector of nullable Nodes, each is an NT_ID */
    } funPtrType;

    struct {
      Vector *components; /**< vector of Nodes, each is an NT_ID. At least 2
                             long. these nodes have null stab entry variables */
      SymbolTableEntry *entry; /**< non-owning reference to the stab entry, if
                                  any, this references. Nullable */
    } scopedId;
    struct {
      char *id;
      SymbolTableEntry *entry; /**< non-owning reference to the stab entry, if
                                  any, this references. Nullable */
    } id;

    struct {
      Vector *tokens; /**< vector of Tokens */
      size_t curr;    /**< current token (for lexing-ish purposes) */
    } unparsed;
  } data;
} Node;

/**
 * Node constructors - these create and return initialized nodes
 */
Node *fileNodeCreate(Node *module, Vector *imports, Vector *bodies);
Node *moduleNodeCreate(Token const *keyword, Node *id);
Node *importNodeCreate(Token const *keyword, Node *id);
Node *funDefnNodeCreate(Node *returnType, Node *name, Vector *argTypes,
                        Vector *argNames, Node *body);
Node *varDefnNodeCreate(Node *type, Vector *names, Vector *initializers);
Node *funDeclNodeCreate(Node *returnType, Node *name, Vector *argTypes,
                        Vector *argNames);
Node *varDeclNodeCreate(Node *type, Vector *names);
Node *opaqueDeclNodeCreate(Token const *keyword, Node *name);
Node *structDeclNodeCreate(Token const *keyword, Node *name, Vector *fields);
Node *unionDeclNodeCreate(Token const *keyword, Node *name, Vector *options);
Node *enumDeclNodeCreate(Token const *keyword, Node *name,
                         Vector *constantNames, Vector *constantValues);
Node *typedefDeclNodeCreate(Token const *keyword, Node *originalType,
                            Node *name);
Node *compoundStmtNodeCreate(Token const *lbrace, Vector *stmts, HashMap *stab);
Node *ifStmtNodeCreate(Token const *keyword, Node *predicate, Node *consequent,
                       HashMap *consequentStab, Node *alternative,
                       HashMap *alternativeStab);
Node *whileStmtNodeCreate(Token const *keyword, Node *condition, Node *body,
                          HashMap *bodyStab);
Node *doWhileStmtNodeCreate(Token const *keyword, Node *body, HashMap *bodyStab,
                            Node *condition);
Node *forStmtNodeCreate(Token const *keyword, HashMap *loopStab,
                        Node *initializer, Node *condition, Node *increment,
                        Node *body, HashMap *bodyStab);
Node *switchStmtNodeCreate(Token const *keyword, Node *condition,
                           Vector *cases);
Node *breakStmtNodeCreate(Token const *keyword);
Node *continueStmtNodeCreate(Token const *keyword);
Node *returnStmtNodeCreate(Token const *keyword, Node *value);
Node *asmStmtNodeCreate(Token const *keyword, Node *assembly);
Node *varDefnStmtNodeCreate(Node *type, Vector *names, Vector *initializers);
Node *expressionStmtNodeCreate(Node *expression);
Node *nullStmtNodeCreate(Token const *semicolon);
Node *switchCaseNodeCreate(Token const *keyword, Vector *values, Node *body,
                           HashMap *bodyStab);
Node *switchDefaultNodeCreate(Token const *keyword, Node *body,
                              HashMap *bodyStab);
Node *binOpExpNodeCreate(BinOpType op, Node *lhs, Node *rhs);
Node *ternaryExpNodeCreate(Node *predicate, Node *consequent,
                           Node *alternative);
Node *prefixUnOpExpNodeCreate(UnOpType op, Token const *opToken, Node *target);
Node *postfixUnOpExpNodeCreate(UnOpType op, Node *target);
Node *funCallExpNodeCreate(Node *function, Vector *arguments);
Node *literalNodeCreate(LiteralType type, Token const *t);
Node *charLiteralNodeCreate(Token *t);
Node *wcharLiteralNodeCreate(Token *t);
Node *stringLiteralNodeCreate(Token *t);
Node *wstringLiteralNodeCreate(Token *t);
Node *sizedIntegerLiteralNodeCreate(Token *t, int8_t sign, uint64_t magnitude);
Node *keywordTypeNodeCreate(TypeKeyword keyword, Token const *keywordToken);
Node *modifiedTypeNodeCreate(TypeModifier modifier, Node *baseType);
Node *arrayTypeNodeCreate(Node *baseType, Node *size);
Node *funPtrTypeNodeCreate(Node *returnType, Vector *argTypes,
                           Vector *argNames);
Node *scopedIdNodeCreate(Vector *components);
Node *idNodeCreate(Token *id);
Node *unparsedNodeCreate(Vector *tokens);

/**
 * creates a stringified version of a scoped id or plain id
 *
 * @param id id to stringify
 * @returns stringified version of id
 */
char *stringifyId(Node *id);

/**
 * generates a Type * from a Node *, resolving references using the given
 * environment
 *
 * @param n node to use
 * @param env environment to use
 *
 * @returns type or null on error
 */
Type *nodeToType(Node *n, Environment *env);

/**
 * equality predicate for scoped ids and plain ids (may compare scoped with
 * plain as well)
 */
bool nameNodeEqual(Node *a, Node *b);

/**
 * equality predicate for plain or scoped id and scoped id (second must be
 * scoped)
 *
 * compare a with all but last dropCount elements in b
 *
 * @param a base to compare with
 * @param b name to compare after dropping dropCount
 * @param dropCount number of elements to drop from b
 */
bool nameNodeEqualWithDrop(Node *a, Node *b, size_t dropCount);

/**
 * de-inits and frees a node
 *
 * @param n node to free, may be null
 */
void nodeFree(Node *n);

/**
 * deinits a vector of nodes
 *
 * @param v vector to deinit, can have null elements, may not itself be null
 */
void nodeVectorFree(Vector *v);

#endif  // TLC_AST_AST_H_