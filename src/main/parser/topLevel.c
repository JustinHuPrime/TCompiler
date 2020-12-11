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

#include "parser/topLevel.h"

#include <stdio.h>
#include <stdlib.h>

#include "parser/common.h"
#include "util/conversions.h"

/**
 * prints an error complaining about a too-large integral value
 *
 * @param entry entry to attribute the error to
 * @param token the bad token
 */
static void errorIntOverflow(FileListEntry *entry, Token *token) {
  fprintf(stderr, "%s:%zu:%zu: error: integer constant is too large\n",
          entry->inputFilename, token->line, token->character);
  entry->errored = true;
}

/**
 * reads tokens until a top-level form boundary
 *
 * semicolons are consumed, EOFs, and the start of a top level form are left
 *
 * @param entry entry to lex from
 */
static void panicTopLevel(FileListEntry *entry) {
  while (true) {
    Token token;
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
// these do error recovery

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

    nodeFree(id);
    return NULL;
  }

  return moduleNodeCreate(&moduleKeyword, id);
}

/**
 * parses a single import
 *
 * @param entry entry to lex from
 * @param importKeyword import keyword
 * @returns import AST node or NULL if errored
 */
static Node *parseImport(FileListEntry *entry, Token *importKeyword) {
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

    nodeFree(id);
    return NULL;
  }

  return importNodeCreate(importKeyword, id);
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
  Vector *imports = vectorCreate();
  while (true) {
    Token importKeyword;
    lex(entry, &importKeyword);

    if (importKeyword.type != TT_IMPORT) {
      // it's the end of the imports
      unLex(entry, &importKeyword);
      return imports;
    } else {
      Node *import = parseImport(entry, &importKeyword);
      if (import != NULL) vectorInsert(imports, import);
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

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
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
        return varDeclNodeCreate(type, names);
      }
      default: {
        errorExpectedString(entry, "a comma or a semicolon", &next);

        unLex(entry, &next);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        return NULL;
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
  Vector *argTypes = vectorCreate();
  Vector *argNames = vectorCreate();

  bool doneArgs = false;
  Token peek;
  lex(entry, &peek);
  if (peek.type == TT_RPAREN)
    doneArgs = true;
  else
    unLex(entry, &peek);
  while (!doneArgs) {
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
        // start of an arg decl
        unLex(entry, &peek);
        Node *type = parseType(entry);
        if (type == NULL) {
          panicTopLevel(entry);

          nodeFree(returnType);
          nodeFree(name);
          nodeVectorFree(argTypes);
          nodeVectorFree(argNames);
          return NULL;
        }
        vectorInsert(argTypes, type);

        lex(entry, &peek);
        switch (peek.type) {
          case TT_ID: {
            // id - arg decl continues
            vectorInsert(argNames, idNodeCreate(&peek));

            lex(entry, &peek);
            switch (peek.type) {
              case TT_COMMA: {
                // done this arg decl
                break;
              }
              case TT_RPAREN: {
                // done all arg decls
                doneArgs = true;
                break;
              }
              default: {
                errorExpectedString(entry, "a comma or a right parenthesis",
                                    &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(returnType);
                nodeFree(name);
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                return NULL;
              }
            }
            break;
          }
          case TT_COMMA: {
            // done this arg decl
            vectorInsert(argNames, NULL);
            break;
          }
          case TT_RPAREN: {
            // done all arg decls
            vectorInsert(argNames, NULL);
            doneArgs = true;
            break;
          }
          default: {
            errorExpectedString(entry, "an id, a comma, or a right parenthesis",
                                &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(returnType);
            nodeFree(name);
            nodeVectorFree(argTypes);
            nodeVectorFree(argNames);
            return NULL;
          }
        }
        break;
      }
      default: {
        errorExpectedString(entry, "a type", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        return NULL;
      }
    }
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(returnType);
    nodeFree(name);
    nodeVectorFree(argTypes);
    nodeVectorFree(argNames);
    return NULL;
  }

  return funDeclNodeCreate(returnType, name, argTypes, argNames);
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
    panicTopLevel(entry);

    nodeFree(type);
    return NULL;
  }

  Token next;
  lex(entry, &next);
  switch (next.type) {
    case TT_SEMI: {
      // var decl, ends here
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      return varDeclNodeCreate(type, names);
    }
    case TT_COMMA: {
      // var decl, continued
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      return finishVarDecl(entry, type, names);
    }
    case TT_LPAREN: {
      // func decl, continued
      return finishFunDecl(entry, type, id);
    }
    default: {
      errorExpectedString(entry, "a semicolon, comma, or a left paren", &next);

      unLex(entry, &next);
      panicTopLevel(entry);

      nodeFree(type);
      nodeFree(id);
      return NULL;
    }
  }
}

/**
 * finishes parsing a variable definition
 *
 * @param entry entry to lex from
 * @param type type of the definition
 * @param names vector of names in the definition
 * @param initializers vector of initializers in the definition
 * @param hasLiteral does this defn have a literal after its first name?
 * @returns definition, or NULL if fatal error
 */
static Node *finishVarDefn(FileListEntry *entry, Node *type, Vector *names,
                           Vector *initializers, bool hasLiteral) {
  if (hasLiteral) {
    Node *literal = parseLiteral(entry);
    if (literal == NULL) {
      panicTopLevel(entry);

      nodeFree(type);
      nodeVectorFree(names);
      nodeVectorFree(initializers);
      return NULL;
    }
    vectorInsert(initializers, literal);

    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_COMMA: {
        // declaration continues
        break;
      }
      case TT_SEMI: {
        // end of declaration
        return varDefnNodeCreate(type, names, initializers);
      }
      default: {
        errorExpectedString(entry, "a comma or a semicolon", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        nodeVectorFree(initializers);
        return NULL;
      }
    }
  }

  while (true) {
    Node *id = parseId(entry);
    if (id == NULL) {
      panicTopLevel(entry);

      nodeFree(type);
      nodeVectorFree(names);
      return NULL;
    }

    Token next;
    lex(entry, &next);
    switch (next.type) {
      case TT_EQ: {
        // has initializer
        Node *literal = parseLiteral(entry);
        if (literal == NULL) {
          panicTopLevel(entry);

          nodeFree(type);
          nodeVectorFree(names);
          nodeVectorFree(initializers);
          return NULL;
        }
        vectorInsert(initializers, literal);

        Token peek;
        lex(entry, &peek);
        switch (peek.type) {
          case TT_COMMA: {
            // declaration continues
            break;
          }
          case TT_SEMI: {
            // end of declaration
            return varDefnNodeCreate(type, names, initializers);
          }
          default: {
            errorExpectedString(entry, "a comma or a semicolon", &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(type);
            nodeVectorFree(names);
            nodeVectorFree(initializers);
            return NULL;
          }
        }
        break;
      }
      case TT_COMMA: {
        // continue declaration
        vectorInsert(initializers, NULL);
        break;
      }
      case TT_SEMI: {
        // done
        return varDefnNodeCreate(type, names, initializers);
      }
      default: {
        errorExpectedString(entry, "a comma, a semicolon, or an equals sign",
                            &next);

        unLex(entry, &next);
        panicTopLevel(entry);

        nodeFree(type);
        nodeVectorFree(names);
        nodeVectorFree(initializers);
        return NULL;
      }
    }
  }
}

/**
 * makes a function body unparsed
 *
 * only cares about curly braces
 *
 * @param entry entry to lex from
 * @param start first open brace
 * @returns unparsed node, or NULL if fatal error
 */
static Node *parseFuncBody(FileListEntry *entry, Token *start) {
  Vector *tokens = vectorCreate();

  size_t levels = 1;
  while (levels > 0) {
    Token *token = malloc(sizeof(Token));
    lex(entry, token);
    switch (token->type) {
      case TT_LBRACE: {
        ++levels;
        break;
      }
      case TT_RBRACE: {
        --levels;
        break;
      }
      default: {
        break;
      }
    }
    vectorInsert(tokens, token);
  }
  return unparsedNodeCreate(tokens);
}

/**
 * finishes parsing a function definition or declaration
 *
 * @param entry entry to lex from
 * @param returnType parsed return type
 * @param name name of the function
 * @returns definition, declaration, or NULL if fatal error
 */
static Node *finishFunDefn(FileListEntry *entry, Node *returnType, Node *name) {
  Vector *argTypes = vectorCreate();
  Vector *argNames = vectorCreate();

  bool doneArgs = false;
  Token peek;
  lex(entry, &peek);
  if (peek.type == TT_RPAREN)
    doneArgs = true;
  else
    unLex(entry, &peek);
  while (!doneArgs) {
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
        // start of an arg decl
        unLex(entry, &peek);
        Node *type = parseType(entry);
        if (type == NULL) {
          panicTopLevel(entry);

          nodeFree(returnType);
          nodeFree(name);
          nodeVectorFree(argTypes);
          nodeVectorFree(argNames);
          return NULL;
        }
        vectorInsert(argTypes, type);

        lex(entry, &peek);
        switch (peek.type) {
          case TT_ID: {
            // id - arg decl continues
            vectorInsert(argNames, idNodeCreate(&peek));

            lex(entry, &peek);
            switch (peek.type) {
              case TT_COMMA: {
                // done this arg decl
                break;
              }
              case TT_RPAREN: {
                // done all arg decls
                doneArgs = true;
                break;
              }
              default: {
                errorExpectedString(
                    entry, "an equals sign, a comma, or a right parenthesis",
                    &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(returnType);
                nodeFree(name);
                nodeVectorFree(argTypes);
                nodeVectorFree(argNames);
                return NULL;
              }
            }
            break;
          }
          case TT_COMMA: {
            // done this arg decl
            vectorInsert(argNames, NULL);
            break;
          }
          case TT_RPAREN: {
            // done all arg decls
            vectorInsert(argNames, NULL);
            doneArgs = true;
            break;
          }
          default: {
            errorExpectedString(entry, "an id, a comma, or a right parenthesis",
                                &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(returnType);
            nodeFree(name);
            nodeVectorFree(argTypes);
            nodeVectorFree(argNames);
            return NULL;
          }
        }
        break;
      }
      default: {
        errorExpectedString(entry, "a type", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(returnType);
        nodeFree(name);
        nodeVectorFree(argTypes);
        nodeVectorFree(argNames);
        return NULL;
      }
    }
  }

  Token lbrace;
  lex(entry, &lbrace);
  if (lbrace.type != TT_LBRACE) {
    errorExpectedString(entry, "a left brace", &peek);

    unLex(entry, &peek);
    panicTopLevel(entry);

    nodeFree(returnType);
    nodeFree(name);
    nodeVectorFree(argTypes);
    nodeVectorFree(argNames);
    return NULL;
  }

  Node *body = parseFuncBody(entry, &peek);
  if (body == NULL) {
    panicTopLevel(entry);

    nodeFree(returnType);
    nodeFree(name);
    nodeVectorFree(argTypes);
    nodeVectorFree(argNames);
    return NULL;
  }

  return funDefnNodeCreate(returnType, name, argTypes, argNames, body);
}

/**
 * parses a function declaration, or a variable declaration or definition
 *
 * @param entry entry to lex from
 * @param start first token
 * @returns declaration, definition, or null if fatal error
 */
static Node *parseFunOrVarDefn(FileListEntry *entry, Token *start) {
  unLex(entry, start);
  Node *type = parseType(entry);
  if (type == NULL) {
    panicTopLevel(entry);
    return NULL;
  }

  Node *id = parseId(entry);
  if (id == NULL) {
    panicTopLevel(entry);

    nodeFree(type);
    return NULL;
  }

  Token next;
  lex(entry, &next);
  switch (next.type) {
    case TT_SEMI: {
      // var defn, ends here
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return varDefnNodeCreate(type, names, initializers);
    }
    case TT_COMMA: {
      // var defn, continued
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return finishVarDefn(entry, type, names, initializers, false);
    }
    case TT_EQ: {
      // var defn, continued with initializer
      Vector *names = vectorCreate();
      vectorInsert(names, id);
      Vector *initializers = vectorCreate();
      vectorInsert(initializers, NULL);
      return finishVarDefn(entry, type, names, initializers, true);
    }
    case TT_LPAREN: {
      // func defn, continued
      return finishFunDefn(entry, type, id);
    }
    default: {
      errorExpectedString(entry, "a semicolon, comma, or a left paren", &next);

      unLex(entry, &next);
      panicTopLevel(entry);

      nodeFree(type);
      nodeFree(id);
      return NULL;
    }
  }
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

    nodeFree(name);
    return NULL;
  }

  return opaqueDeclNodeCreate(start, name);
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

    nodeFree(name);
    return NULL;
  }

  Vector *fields = vectorCreate();
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
        Node *field = parseFieldOrOptionDecl(entry, &peek);
        if (field == NULL) {
          panicTopLevel(
              entry);  // TODO: ought to panic to the end of the struct

          nodeFree(name);
          nodeVectorFree(fields);
          return NULL;
        }
        vectorInsert(fields, field);
        break;
      }
      case TT_RBRACE: {
        doneFields = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or a field", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(name);
        nodeVectorFree(fields);
        return NULL;
      }
    }
  }

  if (fields->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one field in a struct "
            "declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeFree(name);
    nodeVectorFree(fields);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(fields);
    return NULL;
  }

  return structDeclNodeCreate(start, name, fields);
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

    nodeFree(name);
    return NULL;
  }

  Vector *options = vectorCreate();
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
        Node *option = parseFieldOrOptionDecl(entry, &peek);
        if (option == NULL) {
          panicTopLevel(
              entry);  // TODO: ought to panic to the end of the option

          nodeFree(name);
          nodeVectorFree(options);
          return NULL;
        }
        vectorInsert(options, option);
        break;
      }
      case TT_RBRACE: {
        doneOptions = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or an option", &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(name);
        nodeVectorFree(options);
        return NULL;
      }
    }
  }

  if (options->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one option in a union "
            "declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    nodeFree(name);
    nodeVectorFree(options);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(options);
    return NULL;
  }

  return unionDeclNodeCreate(start, name, options);
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

    nodeFree(name);
    return NULL;
  }

  Vector *constantNames = vectorCreate();
  Vector *constantValues = vectorCreate();
  bool doneConstants = false;
  while (!doneConstants) {
    Token peek;
    lex(entry, &peek);
    switch (peek.type) {
      case TT_ID: {
        // this is the start of a constant line
        vectorInsert(constantNames, idNodeCreate(&peek));

        lex(entry, &peek);
        switch (peek.type) {
          case TT_EQ: {
            // has an extended int literal
            Node *literal = parseExtendedIntLiteral(entry);
            if (literal == NULL) {
              panicTopLevel(entry);

              nodeFree(name);
              nodeVectorFree(constantNames);
              nodeVectorFree(constantValues);
              return NULL;
            }
            vectorInsert(constantValues, literal);

            lex(entry, &peek);
            switch (peek.type) {
              case TT_COMMA: {
                // end of this constant
                break;
              }
              case TT_RBRACE: {
                // end of the whole enum
                doneConstants = true;
                break;
              }
              default: {
                errorExpectedString(entry, "a comma or a right brace", &peek);

                unLex(entry, &peek);
                panicTopLevel(entry);

                nodeFree(name);
                nodeVectorFree(constantNames);
                nodeVectorFree(constantValues);
                return NULL;
              }
            }

            break;
          }
          case TT_COMMA: {
            // end of this constant
            vectorInsert(constantValues, NULL);
            break;
          }
          case TT_RBRACE: {
            // end of the whole enum
            vectorInsert(constantValues, NULL);
            doneConstants = true;
            break;
          }
          default: {
            errorExpectedString(
                entry, "a comma, an equals sign, or a right brace", &peek);

            unLex(entry, &peek);
            panicTopLevel(entry);

            nodeFree(name);
            nodeVectorFree(constantNames);
            nodeVectorFree(constantValues);
            return NULL;
          }
        }
        break;
      }
      case TT_RBRACE: {
        doneConstants = true;
        break;
      }
      default: {
        errorExpectedString(entry, "a right brace or an enumeration constant",
                            &peek);

        unLex(entry, &peek);
        panicTopLevel(entry);

        nodeFree(name);
        nodeVectorFree(constantNames);
        nodeVectorFree(constantValues);
        return NULL;
      }
    }
  }

  if (constantNames->size == 0) {
    fprintf(stderr,
            "%s:%zu:%zu: error: expected at least one enumeration constant in "
            "a enumeration declaration\n",
            entry->inputFilename, lbrace.line, lbrace.character);
    entry->errored = true;

    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(name);
    nodeVectorFree(constantNames);
    nodeVectorFree(constantValues);
    return NULL;
  }

  return enumDeclNodeCreate(start, name, constantNames, constantValues);
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
    panicTopLevel(entry);

    nodeFree(originalType);
    return NULL;
  }

  Token semicolon;
  lex(entry, &semicolon);
  if (semicolon.type != TT_SEMI) {
    errorExpectedToken(entry, TT_SEMI, &semicolon);

    unLex(entry, &semicolon);
    panicTopLevel(entry);

    nodeFree(originalType);
    nodeFree(name);
    return NULL;
  }

  return typedefDeclNodeCreate(start, originalType, name);
}

/**
 * parses a set of file bodies
 *
 * never fatally errors, and consumes the EOF
 * Is aware of the code-file-ness of the entry
 *
 * @param entry entry to lex from
 * @returns Vector of bodies
 */
static Vector *parseBodies(FileListEntry *entry) {
  Vector *bodies = vectorCreate();
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
          decl = parseFunOrVarDefn(entry, &start);
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

Node *parseFile(FileListEntry *entry) {
  Node *module = parseModule(entry);
  Vector *imports = parseImports(entry);
  Vector *bodies = parseBodies(entry);

  if (module == NULL) {
    // fatal error in the module
    nodeVectorFree(imports);
    nodeVectorFree(bodies);
    return NULL;
  } else {
    return fileNodeCreate(module, imports, bodies);
  }
}