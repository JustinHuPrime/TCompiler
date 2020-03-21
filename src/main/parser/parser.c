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
    "an equal sign",
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

// constructors

/** create an initialized vector */
static Vector *createVector(void) {
  Vector *v = malloc(sizeof(Vector));
  vectorInit(v);
  return v;
}
/** create an uninitialized node */
static Node *createNode(NodeType type, size_t line, size_t character) {
  Node *n = malloc(sizeof(Node));
  n->type = type;
  n->line = line;
  n->character = character;
  return n;
}
/** create a file node */
static Node *createFile(Node *module, Vector *imports, Vector *bodies) {
  Node *n = createNode(NT_FILE, module->line, module->character);
  n->data.file.module = module;
  n->data.file.imports = imports;
  n->data.file.bodies = bodies;
  hashMapInit(&n->data.file.stab);
  return n;
}
/** create a module node */
static Node *createModule(Token *keyword, Node *id) {
  Node *n = createNode(NT_MODULE, keyword->line, keyword->character);
  n->data.module.id = id;
  return n;
}
/** create an import node */
static Node *createImport(Token *keyword, Node *id) {
  Node *n = createNode(NT_IMPORT, keyword->line, keyword->character);
  n->data.import.id = id;
  n->data.import.referenced = NULL;
  return n;
}

// funDefn
// varDefn

// funDecl
// varDecl
static Node *createOpaqueDecl(Token *keyword, Node *name) {
  Node *n = createNode(NT_OPAQUEDECL, keyword->line, keyword->character);
  n->data.opaqueDecl.name = name;
  return n;
}
// structDecl
// unionDecl
// enumDecl
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
 * reads tokens until the end of a top-level form
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

/**
 * parses an ID or scoped ID
 *
 * does not do error recovery
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAnyId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
            entry->inputFile, idToken.line, idToken.character,
            TOKEN_NAMES[TT_ID], TOKEN_NAMES[idToken.type]);
    entry->errored = true;
    unLex(entry, &idToken);
    return NULL;
  }

  // maybe it's a scoped id?
  Token scope;
  lex(entry, &scope);
  if (scope.type != TT_SCOPE) {
    // not a scoped id
    unLex(entry, &scope);
    return createId(&idToken);
  } else {
    // scoped id - saw scope
    Vector *components = createVector();

    vectorInsert(components, createId(&idToken));
    while (true) {
      // expect an id, add it to the node
      lex(entry, &idToken);
      if (idToken.type != TT_ID) {
        fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
                entry->inputFile, idToken.line, idToken.character,
                TOKEN_NAMES[TT_ID], TOKEN_NAMES[idToken.type]);
        entry->errored = true;
        unLex(entry, &idToken);

        // assume it was a garbage scope operator
        if (components->size == 1) {
          Node *retval = components->elements[0];
          free(components->elements);
          free(components);
          return retval;
        } else {
          return createScopedId(components);
        }
      } else {
        vectorInsert(components, createId(&idToken));
      }

      // if there's a scope, keep going, else return
      lex(entry, &scope);
      if (scope.type != TT_SCOPE) {
        unLex(entry, &scope);
        return createScopedId(components);
      }
    }
  }
}

/**
 * parses an ID (not scoped)
 *
 * does not do error recovery
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
            entry->inputFile, idToken.line, idToken.character,
            TOKEN_NAMES[TT_ID], TOKEN_NAMES[idToken.type]);
    entry->errored = true;
    unLex(entry, &idToken);
    return NULL;
  }

  return createId(&idToken);
}

/**
 * parses a type
 *
 * does not do error recovery
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseType(FileListEntry *entry) {
  return NULL;
  // TODO: write this
}

/**
 * parses a module line
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseModule(FileListEntry *entry) {
  Token moduleKeyword;
  lex(entry, &moduleKeyword);

  if (moduleKeyword.type != TT_MODULE) {
    fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
            entry->inputFile, moduleKeyword.line, moduleKeyword.character,
            TOKEN_NAMES[TT_MODULE], TOKEN_NAMES[moduleKeyword.type]);
    entry->errored = true;
    unLex(entry, &moduleKeyword);
    panicTopLevel(entry);
    return NULL;
  }

  Node *id = parseAnyId(entry);
  if (id == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
            entry->inputFile, semicolon.line, semicolon.character,
            TOKEN_NAMES[TT_SEMI], TOKEN_NAMES[semicolon.type]);
    entry->errored = true;
    unLex(entry, &semicolon);
    panicTopLevel(entry);
  }

  return createModule(&moduleKeyword, id);
}

/**
 * parses a set of imports
 *
 * never fatally errors
 *
 * @param entry entry to lex from
 * @param imports Vector of Nodes to fill in
 */
static Vector *parseImports(FileListEntry *entry) {
  Vector *imports = createVector();
  while (true) {
    Token importKeyword;
    lex(entry, &importKeyword);

    if (importKeyword.type != TT_IMPORT) {
      // it's the end of the imports
      unLex(entry, &importKeyword);
      return imports;
    } else {
      Node *id = parseAnyId(entry);
      if (id == NULL) {
        panicTopLevel(entry);
        continue;
      }

      Token semicolon;
      lex(entry, &semicolon);
      if (semicolon.type != TT_SEMI) {
        fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
                entry->inputFile, semicolon.line, semicolon.character,
                TOKEN_NAMES[TT_SEMI], TOKEN_NAMES[semicolon.type]);
        entry->errored = true;
        unLex(entry, &semicolon);
        panicTopLevel(entry);
      }

      vectorInsert(imports, createImport(&importKeyword, id));
    }
  }
}

/**
 * parses a set of decl file bodies
 *
 * never fatally errors, and consumes the EOF
 *
 * @param entry entry to lex from
 * @param bodies Vector of Nodes to fill in
 */
static Vector *parseDeclBodies(FileListEntry *entry) {
  Vector *bodies = createVector();
  while (true) {
    Token start;
    lex(entry, &start);

    switch (start.type) {
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
      case TT_ID: {
        // TODO: write this
        // is a function or a variable declaration
      }
      case TT_OPAQUE: {
        Node *id = parseId(entry);
        if (id == NULL) {
          panicTopLevel(entry);
          break;
        }

        Token semicolon;
        lex(entry, &semicolon);
        if (semicolon.type != TT_SEMI) {
          fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
                  entry->inputFile, semicolon.line, semicolon.character,
                  TOKEN_NAMES[TT_SEMI], TOKEN_NAMES[semicolon.type]);
          entry->errored = true;
          unLex(entry, &semicolon);
          panicTopLevel(entry);
        }

        vectorInsert(bodies, createOpaqueDecl(&start, id));
        break;
      }
      case TT_STRUCT: {
        break;  // TODO: write this
      }
      case TT_UNION: {
        break;  // TODO: write this
      }
      case TT_ENUM: {
        break;  // TODO: write this
      }
      case TT_TYPEDEF: {
        Node *originalType = parseType(entry);
        if (originalType == NULL) {
          panicTopLevel(entry);
          break;
        }

        Node *name = parseId(entry);
        if (name == NULL) {
          nodeUninit(originalType);
          free(originalType);
          panicTopLevel(entry);
          break;
        }

        Token semicolon;
        lex(entry, &semicolon);
        if (semicolon.type != TT_SEMI) {
          fprintf(stderr, "%s:%zu:%zu: error: expected %s, but found %s\n",
                  entry->inputFile, semicolon.line, semicolon.character,
                  TOKEN_NAMES[TT_SEMI], TOKEN_NAMES[semicolon.type]);
          entry->errored = true;
          unLex(entry, &semicolon);
          panicTopLevel(entry);
        }

        vectorInsert(bodies, createTypedefDecl(&start, originalType, name));
        break;
      }
      case TT_EOF: {
        // reached end of file
        return bodies;
      }
      default: {
        // unexpected token
        fprintf(stderr,
                "%s:%zu:%zu: error: expected a declaration, but found %s\n",
                entry->inputFile, start.line, start.character,
                TOKEN_NAMES[start.type]);
        entry->errored = true;
        panicTopLevel(entry);
        break;
      }
    }
  }
}

/**
 * parses a set of code file bodies
 *
 * never fatally errors, and consumes the EOF
 *
 * @param entry entry to lex from
 * @param bodies Vector of Nodes to fill in
 */
static Vector *parseCodeBodies(FileListEntry *entry) {
  Vector *bodies = createVector();
  // TODO: write this
  return bodies;
}

/**
 * parses a file, phase one
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseFile(FileListEntry *entry) {
  Node *module = parseModule(entry);

  Vector *imports = parseImports(entry);

  Vector *bodies = createVector();
  if (entry->isCode)
    bodies = parseCodeBodies(entry);
  else
    bodies = parseDeclBodies(entry);

  if (module == NULL) {
    // fatal error - free report error
    vectorUninit(imports, (void (*)(void *))nodeUninit);
    free(imports);
    vectorUninit(bodies, (void (*)(void *))nodeUninit);
    free(bodies);
    return NULL;
  } else {
    return createFile(module, imports, bodies);
  }
}

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

  // pass two - generate symbol tables (part 1)
  // pass three - resolve imports and parse unparsed nodes

  return 0;
}