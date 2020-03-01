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
} NodeType;

/** an AST node */
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      struct Node *module;
      Vector imports; /**< vector of Nodes */
      Vector body;    /**< vector of Nodes */
    } file;

    struct {
      struct Node *id;
    } module;
    struct {
      struct Node *id;
    } import;

    struct {
      struct Node *returnType;
      struct Node *funName;
      Vector argTypes;    /**< vector of Nodes */
      Vector argNames;    /**< vector of nullable Nodes */
      Vector argLiterals; /**< vector of nullable Nodes */
      struct Node *body;
    } funDefn;
    struct {
      struct Node *type;
      Vector names;        /**< vector of Nodes */
      Vector initializers; /**< vector of nullable Nodes */
    } varDefn;

    struct {
      struct Node *returnType;
      struct Node *funName;
      Vector argTypes;    /**< vector of Nodes */
      Vector argNames;    /**< vector of nullable Nodes */
      Vector argLiterals; /**< vector of nullable Nodes */
    } funDecl;
    struct {
      struct Node *type;
      Vector names; /**< vector of Nodes */
    } varDecl;
    struct {
      struct Node *name;
    } opaqueDecl;
    struct {
      struct Node *name;
      Vector fields; /**< vector of Nodes */
    } structDecl;
    struct {
      struct Node *name;
      Vector options; /**< vector of Nodes */
    } unionDecl;
    struct {
      struct Node *name;
      Vector constantNames; /**< vector of Nodes */
    } enumDecl;
    struct {
      struct Node *originalType;
      struct Node *name;
    } typedefDecl;

    struct {
      Vector stmts; /**< vector of Nodes */
    } compoundStmt;
    struct {
      struct Node *predicate;
      struct Node *consequent;
      struct Node *alternative; /**< nullable */
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
      struct Node *initializer;
      struct Node *condition;
      struct Node *increment; /**< nullable */
      struct Node *body;
    } forStmt;
    struct {
      struct Node *condition;
      Vector cases; /**< vector of Nodes */
    } switchStmt;
    // struct { // has no data
    // } breakStmt;
    // struct { // has no data
    // } continueStmt;
    struct {
      struct Node *value; /**< nullable */
    } returnStmt;
    struct {
      struct Node *assembly;
    } asmStmt;
    struct {
      struct Node *type;
      Vector names;        /**< vector of Nodes */
      Vector initializers; /**< vector of nullable Nodes */
    } varDefnStmt;
    struct {
      struct Node *expression;
    } expressionStmt;
    // struct { // has no data
    // } nullStmt;

    struct {
      Vector values; /**< vector of Nodes */
      struct Node *body;
    } switchCase;
    struct {
      struct Node *body;
    } switchDefault;
  } data;
} Node;

/**
 * deinitializes a node
 *
 * @param n Node to deinitialize
 */
void nodeDeinit(Node *n);

#endif  // TLC_PARSER_AST_H_