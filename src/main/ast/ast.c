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

#include "ast/ast.h"

#include "lexer/lexer.h"

#include <stdlib.h>

/**
 * deinits and frees a symbol table entry
 *
 * @param e entry to free, not null
 */
static void stabEntryFree(SymbolTableEntry *e) {
  stabEntryDeinit(e);
  free(e);
}

/**
 * deinits a symbol table
 *
 * @param t table to deinit
 */
static void stabUninit(HashMap *t) {
  hashMapUninit(t, (void (*)(void *))stabEntryFree);
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