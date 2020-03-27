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

#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"

#include "fileList.h"
#include "internalError.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>

// utility functions and data

/** array between token type (as int) and token name */
static char const *const TOKEN_NAMES[] = {
    "the end of file",
    "the keyword 'module'",
    "the keyword 'import'",
    "the keyword 'opaque'",
    "the keyword 'struct'",
    "the keyword 'union'",
    "the keyword 'enum'",
    "the keyword 'typedef'",
    "the keyword 'if'",
    "the keyword 'else'",
    "the keyword 'while'",
    "the keyword 'do'",
    "the keyword 'for'",
    "the keyword 'switch'",
    "the keyword 'case'",
    "the keyword 'default'",
    "the keyword 'break'",
    "the keyword 'continue'",
    "the keyword 'return'",
    "the keyword 'asm'",
    "the keyword 'cast'",
    "the keyword 'sizeof'",
    "the keyword 'true'",
    "the keyword 'false'",
    "the keyword 'null'",
    "the keyword 'void'",
    "the keyword 'ubyte'",
    "the keyword 'byte'",
    "the keyword 'char'",
    "the keyword 'ushort'",
    "the keyword 'short'",
    "the keyword 'uint'",
    "the keyword 'int'",
    "the keyword 'wchar'",
    "the keyword 'ulong'",
    "the keyword 'long'",
    "the keyword 'float'",
    "the keyword 'double'",
    "the keyword 'bool'",
    "the keyword 'const'",
    "the keyword 'volatile'",
    "a semicolon",
    "a comma",
    "a left parenthesis",
    "a right parenthesis",
    "a left square bracket",
    "a right square bracket",
    "a left brace",
    "a right brace",
    "a period",
    "a structure dereference operator",
    "an increment operator",
    "a decrement operator",
    "an asterisk",
    "an ampersand",
    "a plus sign",
    "a minus sign",
    "an exclaimation mark",
    "a tilde",
    "a compound negation-assignment operator",
    "a compound logical-not-assignment operator",
    "a compound bitwise-not-assignment operator",
    "a slash",
    "a percent sign",
    "a left shift operator",
    "an arithmetic-right-shift operator",
    "a logical-right-shift operator",
    "a three way comparison operator",
    "a left angle bracket",
    "a right angle bracket",
    "a less-than-or-equal-to operator",
    "a greater-than-or-equal-to operator",
    "an equal-to operator",
    "a not-equal-to operator",
    "a pipe",
    "a caret",
    "a logical-and operator",
    "a logical-or operator",
    "a question mark",
    "a colon",
    "an equals sign",
    "a compound multiplication-assignment operator",
    "a compound division-assignment operator",
    "a compound modulo-assignment operator",
    "a compound addition-assignment operator",
    "a compound subtraction-assignment operator",
    "a compound left-shift-assignment operator",
    "a compound arithmetic-right-shift-assignment operator",
    "a compound logical-right-shift-assignment operator",
    "a compound bitwise-and-assignment operator",
    "a compound bitwise-exclusive-or-assignment operator",
    "a compound bitwise-or-assignment-operator",
    "a compound logical-and-assignment-operator",
    "a compound logical-or-assignment-operator",
    "a scope-resolution operator",
    "an identifier",
    "a string literal",
    "a wide string literal",
    "a character literal",
    "a wide character literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "an integer literal",
    "a floating-point literal",
    "a floating-point literal",
    "a string literal",
    "a character literal",
    "an integer literal",
    "an integer literal",
};

/**
 * prints an error complaining about a wrong token, specifying what token it
 * should have been
 *
 * @param entry entry to attribute the error to
 * @param expected TokenType expected
 * @param actual actual token
 */
static void errorExpectedToken(FileListEntry *entry, TokenType expected,
                               Token *actual) {
  fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
          entry->inputFile, actual->line, actual->character,
          TOKEN_NAMES[expected], TOKEN_NAMES[actual->type]);
  entry->errored = true;
}

/**
 * prints an error complaining about a wrong token, specifying what it should
 * have been, as a string
 *
 * @param entry entry to attribute the error to
 * @param expected string describing the expected tokens
 * @param actual actual token
 */
static void errorExpectedString(FileListEntry *entry, char const *expected,
                                Token *actual) {
  fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
          entry->inputFile, actual->line, actual->character, expected,
          TOKEN_NAMES[actual->type]);
  entry->errored = true;
}

// constructors

/** create an initialized, empty vector */
static Vector *createVector(void) {
  Vector *v = malloc(sizeof(Vector));
  vectorInit(v);
  return v;
}
/** create a partially initialized node */
static Node *createNode(NodeType type, size_t line, size_t character) {
  Node *n = malloc(sizeof(Node));
  n->type = type;
  n->line = line;
  n->character = character;
  return n;
}

static Node *createFile(Node *module, Vector *imports, Vector *bodies) {
  Node *n = createNode(NT_FILE, module->line, module->character);
  n->data.file.module = module;
  n->data.file.imports = imports;
  n->data.file.bodies = bodies;
  hashMapInit(&n->data.file.stab);
  return n;
}
static Node *createModule(Token *keyword, Node *id) {
  Node *n = createNode(NT_MODULE, keyword->line, keyword->character);
  n->data.module.id = id;
  return n;
}
static Node *createImport(Token *keyword, Node *id) {
  Node *n = createNode(NT_IMPORT, keyword->line, keyword->character);
  n->data.import.id = id;
  n->data.import.referenced = NULL;
  return n;
}

static Node *createFunDefn(Node *returnType, Node *name, Vector *argTypes,
                           Vector *argNames, Vector *argDefaults, Node *body) {
  Node *n = createNode(NT_FUNDEFN, returnType->line, returnType->character);
  n->data.funDefn.returnType = returnType;
  n->data.funDefn.name = name;
  n->data.funDefn.argTypes = argTypes;
  n->data.funDefn.argNames = argNames;
  n->data.funDefn.argDefaults = argDefaults;
  n->data.funDefn.body = body;
  hashMapInit(&n->data.funDefn.stab);
  return n;
}
static Node *createVarDefn(Node *type, Vector *names, Vector *initializers) {
  Node *n = createNode(NT_VARDEFN, type->line, type->character);
  n->data.varDefn.type = type;
  n->data.varDefn.names = names;
  n->data.varDefn.initializers = initializers;
  return n;
}

static Node *createFunDecl(Node *returnType, Node *name, Vector *argTypes,
                           Vector *argNames, Vector *argDefaults) {
  Node *n = createNode(NT_FUNDECL, returnType->line, returnType->character);
  n->data.funDecl.returnType = returnType;
  n->data.funDecl.name = name;
  n->data.funDecl.argTypes = argTypes;
  n->data.funDecl.argNames = argNames;
  n->data.funDecl.argDefaults = argDefaults;
  return n;
}
static Node *createVarDecl(Node *type, Vector *names) {
  Node *n = createNode(NT_VARDECL, type->line, type->character);
  n->data.varDecl.type = type;
  n->data.varDecl.names = names;
  return n;
}
static Node *createOpaqueDecl(Token *keyword, Node *name) {
  Node *n = createNode(NT_OPAQUEDECL, keyword->line, keyword->character);
  n->data.opaqueDecl.name = name;
  return n;
}
static Node *createStructDecl(Token *keyword, Node *name, Vector *fields) {
  Node *n = createNode(NT_STRUCTDECL, keyword->line, keyword->character);
  n->data.structDecl.name = name;
  n->data.structDecl.fields = fields;
  return n;
}
static Node *createUnionDecl(Token *keyword, Node *name, Vector *options) {
  Node *n = createNode(NT_UNIONDECL, keyword->line, keyword->character);
  n->data.unionDecl.name = name;
  n->data.unionDecl.options = options;
  return n;
}
static Node *createEnumDecl(Token *keyword, Node *name, Vector *constantNames,
                            Vector *constantValues) {
  Node *n = createNode(NT_ENUMDECL, keyword->line, keyword->character);
  n->data.enumDecl.name = name;
  n->data.enumDecl.constantNames = constantNames;
  n->data.enumDecl.constantValues = constantValues;
  return n;
}
static Node *createTypedefDecl(Token *keyword, Node *originalType, Node *name) {
  Node *n = createNode(NT_TYPEDEFDECL, keyword->line, keyword->character);
  n->data.typedefDecl.originalType = originalType;
  n->data.typedefDecl.name = name;
  return n;
}

// compoundStmt
// ifStmt
// whileStmt
// doWhileStmt
// forStmt
// switchStmt
// breakStmt
// continueStmt
// returnStmt
// asmStmt
// varDefnStmt
// expressionStmt
// nullStmt

// switchCase
// switchDefault

// binOpExp
// ternaryExp
// unOpExp
// funCallExp

// literal

// keywordType
// modifiedType
// arrayType
// funPtrType

static Node *createScopedId(Vector *components) {
  Node *first = components->elements[0];
  Node *n = createNode(NT_SCOPEDID, first->line, first->character);
  n->data.scopedId.components = components;
  return n;
}
static Node *createId(Token *id) {
  Node *n = createNode(NT_ID, id->line, id->character);
  n->data.id.id = id->string;
  return n;
}

// panics

/**
 * reads tokens until a top-level form boundary
 *
 * semicolons are consumed, EOFs, and the start of a top level form are left
 *
 * @param entry entry to lex from
 */
static void panicTopLevel(FileListEntry *entry) {
  Token token;
  while (true) {
    lex(entry, &token);

    switch (token.type) {
      case TT_SEMI: {
        return;
      }
      case TT_MODULE:
      case TT_IMPORT:
      case TT_VOID:
      case TT_UBYTE:
      case TT_CHAR:
      case TT_USHORT:
      case TT_UINT:
      case TT_INT:
      case TT_WCHAR:
      case TT_ULONG:
      case TT_LONG:
      case TT_FLOAT:
      case TT_DOUBLE:
      case TT_BOOL:
      case TT_ID:
      case TT_OPAQUE:
      case TT_STRUCT:
      case TT_UNION:
      case TT_ENUM:
      case TT_TYPEDEF:
      case TT_EOF: {
        unLex(entry, &token);
        return;
      }
      default: {
        tokenUninit(&token);
        break;
      }
    }
  }
}

// parsing

int parse(void) {
  int retval = 0;
  bool errored = false;

  lexerInitMaps();

  // pass one - parse and gather top-level names, leaving some nodes as unparsed
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) continue;

    fileList.entries[idx].program = parseFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].program == NULL;

    lexerStateUninit(&fileList.entries[idx]);
  }

  lexerUninitMaps();

  if (errored) {
    // at least one produced NULL - clean up and report that
    for (size_t idx = 0; idx < fileList.size; idx++) {
      if (fileList.entries[idx].program != NULL) {
        nodeUninit(fileList.entries[idx].program);
        free(fileList.entries[idx].program);
      }
    }
    return -1;
  }

  // pass two - generate symbol tables
  // pass three - resolve imports and parse unparsed nodes

  return 0;
}