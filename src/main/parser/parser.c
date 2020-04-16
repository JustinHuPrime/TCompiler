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
// The T Language Compiler. If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

/**
 * reads tokens until a field or option boundary
 *
 * semicolons are consumed, right braces, EOFs and the start of a type are left
 *
 * @param entry entry to lex from
 */
static void panicFieldOrOption(FileListEntry *entry) {
  Token token;
  while (true) {
    lex(entry, &token);

    switch (token.type) {
      case TT_SEMI: {
        return;
      }
      case TT_RBRACE:
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

/**
 * reads tokens until an enumeration constant boundary
 *
 * commas are consumed, EOFs and IDs are left
 *
 * @param entry entry to lex from
 */
static void panicEnumEntry(FileListEntry *entry) {
  Token token;
  while (true) {
    lex(entry, &token);

    switch (token.type) {
      case TT_COMMA: {
        return;
      }
      case TT_ID:
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

/**
 * reads tokens until an argument boundary
 *
 * commas are consumed, right parens, EOFs, and the start of a type are left
 *
 * @param entry entry to lex from
 */
static void panicArgDecl(FileListEntry *entry) {
  Token token;
  while (true) {
    lex(entry, &token);

    switch (token.type) {
      case TT_COMMA: {
        return;
      }
      case TT_RPAREN:
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
 * does not do error recovery, unlexes on an error
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseAnyId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
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
        errorExpectedToken(entry, TT_ID, &idToken);
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
 * does not do error recovery, unlexes if bad thing happened
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseId(FileListEntry *entry) {
  Token idToken;
  lex(entry, &idToken);

  if (idToken.type != TT_ID) {
    errorExpectedToken(entry, TT_ID, &idToken);
    unLex(entry, &idToken);
    return NULL;
  }

  return createId(&idToken);
}

/**
 * parses an extended int literal
 *
 * does not do error recovery, unlexes if bad thing happened
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if fatal error happened
 */
static Node *parseExtendedIntLiteral(FileListEntry *entry) {
  return NULL;  // TODO: write this
}

/**
 * parses any literal
 *
 * does not do error recovery, unlexes if bad thing happened
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns AST nde or NULL if fatal error happened
 */
static Node *parseLiteral(FileListEntry *entry, Token *start) {
  return NULL;  // TODO: write this
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
    errorExpectedToken(entry, TT_MODULE, &moduleKeyword);
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
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createModule(&moduleKeyword, id);
  }

  return createModule(&moduleKeyword, id);
}

/**
 * parses a set of imports
 *
 * never fatally errors
 *
 * @param entry entry to lex from
 * @returns list of imports
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
        errorExpectedToken(entry, TT_SEMI, &semicolon);
        unLex(entry, &semicolon);
        panicTopLevel(entry);
        vectorInsert(imports, createImport(&importKeyword, id));
      }

      vectorInsert(imports, createImport(&importKeyword, id));
    }
  }
}

/**
 * finishes parsing a variable declaration
 *
 * @param entry entry to lex from
 * @param type type of the var decl
 * @param names vector of names, partially filled
 */
static Node *finishVarDecl(FileListEntry *entry, Node *type, Vector *names) {
  while (true) {
    Node *id = parseId(entry);
    if (id == NULL) {
      panicTopLevel(entry);
      return createVarDecl(type, names);
    }

    Token next;
    lex(entry, &next);
    switch (next.type) {
      case TT_COMMA: {
        // continue;
        break;
      }
      case TT_SEMI: {
        // done
        return createVarDecl(type, names);
      }
      default: {
        errorExpectedString(entry, "a comma or a semicolon", &next);
        unLex(entry, &next);
        panicTopLevel(entry);
        return createVarDecl(type, names);
      }
    }
  }
}

/**
 * finishes parsing a function declaration
 *
 * @param entry entry to lex from
 * @param returnType return type of function
 * @param name name of the function
 */
static Node *finishFunDecl(FileListEntry *entry, Node *returnType, Node *name) {
  Vector *argTypes = createVector();
  Vector *argNames = createVector();
  Vector *argDefaults = createVector();

  bool doneArgs = false;
  while (!doneArgs) {
    Token peek;
    lex(entry, &peek);

    switch (peek.type) {
      case TT_RPAREN: {
        // done with arguments
        doneArgs = true;
        break;
      }
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
        // start of an argument
        unLex(entry, &peek);
        Node *argType = parseType(entry);
        if (argType == NULL) {
          panicArgDecl(entry);
          break;
        }

        lex(entry, &peek);
        switch (peek.type) {
          case TT_ID: {
            Node *argName = createId(&peek);

            lex(entry, &peek);
            switch (peek.type) {
              case TT_EQ: {
                lex(entry, &peek);
                switch (peek.type) {
                  case TT_LIT_INT_0:
                  case TT_LIT_INT_B:
                  case TT_LIT_INT_O:
                  case TT_LIT_INT_D:
                  case TT_LIT_INT_H:
                  case TT_LIT_STRING:
                  case TT_LIT_CHAR:
                  case TT_LIT_WSTRING:
                  case TT_LIT_WCHAR:
                  case TT_TRUE:
                  case TT_FALSE:
                  case TT_NULL:
                  case TT_ID:
                  case TT_LBRACE:
                  case TT_BAD_BIN:
                  case TT_BAD_HEX:
                  case TT_BAD_CHAR:
                  case TT_BAD_STRING: {
                    Node *argLiteral = parseLiteral(entry, &peek);
                    if (argLiteral == NULL) {
                      panicArgDecl(entry);
                    }
                    vectorInsert(argTypes, argType);
                    vectorInsert(argNames, argName);
                    vectorInsert(argDefaults, argLiteral);

                    Token next;
                    lex(entry, &next);
                    switch (next.type) {
                      case TT_RPAREN: {
                        doneArgs = true;
                        break;
                      }
                      case TT_COMMA: {
                        // keep going
                        break;
                      }
                      default: {
                        errorExpectedString(
                            entry, "a comma or a right parenthesis", &next);
                        unLex(entry, &next);
                        panicArgDecl(entry);
                        break;
                      }
                    }
                    break;
                  }
                  default: {
                    errorExpectedString(entry, "a literal", &peek);
                    unLex(entry, &peek);
                    panicArgDecl(entry);
                    vectorInsert(argTypes, argType);
                    vectorInsert(argNames, argName);
                    vectorInsert(argDefaults, NULL);
                    break;
                  }
                }
                break;
              }
              case TT_RPAREN: {
                doneArgs = true;
                vectorInsert(argTypes, argType);
                vectorInsert(argNames, argName);
                vectorInsert(argDefaults, NULL);
                break;
              }
              case TT_COMMA: {
                vectorInsert(argTypes, argType);
                vectorInsert(argNames, argName);
                vectorInsert(argDefaults, NULL);
                break;
              }
              default: {
                errorExpectedString(entry, "an equal sign or a comma", &peek);
                unLex(entry, &peek);
                panicArgDecl(entry);
                vectorInsert(argTypes, argType);
                vectorInsert(argNames, argName);
                vectorInsert(argDefaults, NULL);
                break;
              }
            }
            break;
          }
          case TT_RPAREN: {
            doneArgs = true;
            vectorInsert(argTypes, argType);
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            break;
          }
          case TT_COMMA: {
            vectorInsert(argTypes, argType);
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            break;
          }
          default: {
            errorExpectedString(
                entry, "an identifier, right parentesis, or a comma", &peek);
            unLex(entry, &peek);
            panicArgDecl(entry);
            vectorInsert(argTypes, argType);
            vectorInsert(argNames, NULL);
            vectorInsert(argDefaults, NULL);
            break;
          }
        }
        break;
      }
      default: {
        errorExpectedString(entry, "a right parenthesis or a type", &peek);
        unLex(entry, &peek);
        return createFunDecl(returnType, name, argTypes, argNames, argDefaults);
      }
    }
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createFunDecl(returnType, name, argTypes, argNames, argDefaults);
  }

  return createFunDecl(returnType, name, argTypes, argNames, argDefaults);
}

/**
 * parses a function or variable declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseFunOrVarDecl(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Node *id = parseId(entry);
  if (id == NULL) {
    nodeUninit(type);
    free(type);
    panicTopLevel(entry);
    return NULL;
  }

  Token next;
  lex(entry, &next);
  switch (next.type) {
    case TT_SEMI: {
      // var decl, ends here
      Vector *names = createVector();
      vectorInsert(names, id);
      return createVarDecl(type, names);
    }
    case TT_COMMA: {
      // var decl, continued
      Vector *names = createVector();
      vectorInsert(names, id);
      return finishVarDecl(entry, type, names);
    }
    case TT_LPAREN: {
      // func decl, continued
      return finishFunDecl(entry, type, id);
    }
    default: {
      errorExpectedString(entry, "a semicolon, comma, or a left paren", &next);
      nodeUninit(id);
      free(id);
      nodeUninit(type);
      free(type);
      unLex(entry, &next);
      panicTopLevel(entry);
      return NULL;
    }
  }
}

/**
 * parses a function declaration, or a variable declaration or definition
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration, definition, or null if fatal error
 */
static Node *parseFunOrVarDeclOrDefn(FileListEntry *entry, Token *start) {
  return NULL;  // TODO: write this
}

/**
 * parses an opaque declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseOpaqueDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createOpaqueDecl(start, name);
  }

  return createOpaqueDecl(start, name);
}

/**
 * parses a field or option declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseFieldOptionDecl(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    panicFieldOrOption(entry);
    return NULL;
  }

  Vector *names = createVector();
  bool done = false;
  while (!done) {
    Token id;
    lex(entry, &id);
    if (id.type != TT_ID) {
      errorExpectedToken(entry, TT_ID, &id);
      unLex(entry, &id);
      panicFieldOrOption(entry);
      break;
    }

    vectorInsert(names, createId(&id));

    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_SEMI: {
        // end of the names
        done = true;
        break;
      }
      case TT_COMMA: {
        // comma between names - do nothing
        break;
      }
      default: {
        errorExpectedString(entry, "a semicolon or a comma", &peek);
        unLex(entry, &peek);
        panicFieldOrOption(entry);
        done = true;
        break;
      }
    }
  }

  if (names->size == 0) {
    nodeUninit(type);
    free(type);
    return NULL;
  }

  return createVarDecl(type, names);
}

/**
 * parses a struct declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseStructDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);
    unLex(entry, &lbrace);
    panicTopLevel(entry);
    nodeUninit(name);
    free(name);
    return NULL;
  }

  Vector *fields = createVector();
  bool doneFields = false;
  while (!doneFields) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
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
        // this is the start of a field
        Node *field = parseFieldOptionDecl(entry, &peek);
        if (field != NULL) vectorInsert(fields, field);
        break;
      }
      case TT_RBRACE: {
        doneFields = true;
        break;
      }
      default: {
        // error! assume this is the end of the fields and of the struct
        errorExpectedString(entry, "a right brace or a field", &peek);
        unLex(entry, &peek);
        panicTopLevel(entry);
        return createStructDecl(start, name, fields);
      }
    }
  }

  if (fields->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one field in a struct "
            "declaration\n",
            entry->inputFile, lbrace.line, lbrace.character);
    entry->errored = true;
    nodeUninit(name);
    free(name);
    vectorUninit(fields, (void (*)(void *))nodeUninit);
    free(fields);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createStructDecl(start, name, fields);
  }

  return createStructDecl(start, name, fields);
}

/**
 * parses a union declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseUnionDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);
    unLex(entry, &lbrace);
    panicTopLevel(entry);
    nodeUninit(name);
    free(name);
    return NULL;
  }

  Vector *options = createVector();
  bool doneOptions = false;
  while (!doneOptions) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
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
        // this is the start of an option
        Node *option = parseFieldOptionDecl(entry, &peek);
        if (option != NULL) vectorInsert(options, option);
        break;
      }
      case TT_RBRACE: {
        doneOptions = true;
        break;
      }
      default: {
        // error! assume this is the end of the options and of the struct
        errorExpectedString(entry, "a right brace or an option", &peek);
        unLex(entry, &peek);
        panicTopLevel(entry);
        return createStructDecl(start, name, options);
      }
    }
  }

  if (options->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one option in a union "
            "declaration\n",
            entry->inputFile, lbrace.line, lbrace.character);
    entry->errored = true;
    nodeUninit(name);
    free(name);
    vectorUninit(options, (void (*)(void *))nodeUninit);
    free(options);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createUnionDecl(start, name, options);
  }

  return createUnionDecl(start, name, options);
}

/**
 * parses a enum declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseEnumDecl(FileListEntry *entry, Token *start) {
  Node *name = parseId(entry);
  if (name == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedToken(entry, TT_LBRACE, &lbrace);
    unLex(entry, &lbrace);
    panicTopLevel(entry);
    nodeUninit(name);
    free(name);
    return NULL;
  }

  Vector *constantNames = createVector();
  Vector *constantValues = createVector();
  bool doneConstants = false;
  while (!doneConstants) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_ID: {
        // this is the start of a constant line
        vectorInsert(constantNames, createId(&peek));

        Token next;
        lex(entry, &next);
        switch (next.type) {
          case TT_EQ: {
            // has an extended int literal
            Node *literal = parseExtendedIntLiteral(entry);
            vectorInsert(constantValues, literal);
            if (literal == NULL) {
              panicEnumEntry(entry);
              break;
            }

            Token comma;
            lex(entry, &comma);
            if (comma.type != TT_COMMA) {
              errorExpectedToken(entry, TT_COMMA, &comma);
              unLex(entry, &comma);
              panicEnumEntry(entry);
              break;
            }

            break;
          }
          case TT_COMMA: {
            // done this
            vectorInsert(constantValues, NULL);
            break;
          }
          default: {
            vectorInsert(constantValues, NULL);
            errorExpectedString(entry, "a comma or an equals sign", &peek);
            unLex(entry, &peek);
            panicEnumEntry(entry);
            break;
          }
        }
        break;
      }
      case TT_RBRACE: {
        doneConstants = true;
        break;
      }
      default: {
        // error! assume this is the end of the fields and of the struct
        errorExpectedString(entry, "a right brace or an enumeration constant",
                            &peek);
        unLex(entry, &peek);
        panicTopLevel(entry);
        return createEnumDecl(start, name, constantNames, constantValues);
      }
    }
  }

  if (constantNames->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one enumeration constant in "
            "a enumeration declaration\n",
            entry->inputFile, lbrace.line, lbrace.character);
    entry->errored = true;
    nodeUninit(name);
    free(name);
    vectorUninit(constantNames, (void (*)(void *))nodeUninit);
    free(constantNames);
    vectorUninit(constantValues, (void (*)(void *))nodeUninit);
    free(constantValues);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createEnumDecl(start, name, constantNames, constantValues);
  }

  return createEnumDecl(start, name, constantNames, constantValues);
}

/**
 * parses a typedef declaration
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration or null if fatal error
 */
static Node *parseTypedefDecl(FileListEntry *entry, Token *start) {
  Node *originalType = parseType(entry);
  if (originalType == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Node *name = parseId(entry);
  if (name == NULL) {
    nodeUninit(originalType);
    free(originalType);
    panicTopLevel(entry);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);
    unLex(entry, &semicolon);
    panicTopLevel(entry);
    return createTypedefDecl(start, originalType, name);
  }

  return createTypedefDecl(start, originalType, name);
}

/**
 * parses a set of file bodies
 *
 * never fatally errors, and consumes the EOF
 * Is sensitive to the code-ness of the entry
 *
 * @param entry entry to lex from
 * @returns Vector of bodies
 */
static Vector *parseBodies(FileListEntry *entry) {
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
        Node *decl;
        if (entry->isCode)
          decl = parseFunOrVarDeclOrDefn(entry, &start);
        else
          decl = parseFunOrVarDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_OPAQUE: {
        Node *decl = parseOpaqueDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_STRUCT: {
        Node *decl = parseStructDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_UNION: {
        Node *decl = parseUnionDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_ENUM: {
        Node *decl = parseEnumDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_TYPEDEF: {
        Node *decl = parseTypedefDecl(entry, &start);
        if (decl != NULL) vectorInsert(bodies, decl);
        break;
      }
      case TT_EOF: {
        // reached end of file
        return bodies;
      }
      default: {
        // unexpected token
        errorExpectedString(entry, "a declaration", &start);
        unLex(entry, &start);
        panicTopLevel(entry);
        continue;
      }
    }
  }
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
  Vector *bodies = parseBodies(entry);

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

  // pass two - generate symbol tables
  // pass three - resolve imports and parse unparsed nodes

  return 0;
}