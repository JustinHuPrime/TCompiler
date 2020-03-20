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

// pointer ownership conventions:
// when returning from any error, pointers must not have any data in them,
// and vectors must not have any partial elements

// utility functions

static char const *tokenTypeToString(TokenType type) {
  switch (type) {
    case TT_EOF:
      return "the end of file";
    case TT_MODULE:
      return "the keyword 'module'";
    case TT_IMPORT:
      return "the keyword 'import'";
    case TT_OPAQUE:
      return "the keyword 'opaque'";
    case TT_STRUCT:
      return "the keyword 'struct'";
    case TT_UNION:
      return "the keyword 'union'";
    case TT_ENUM:
      return "the keyword 'enum'";
    case TT_TYPEDEF:
      return "the keyword 'typedef'";
    case TT_IF:
      return "the keyword 'if'";
    case TT_ELSE:
      return "the keyword 'else'";
    case TT_WHILE:
      return "the keyword 'while'";
    case TT_DO:
      return "the keyword 'do'";
    case TT_FOR:
      return "the keyword 'for'";
    case TT_SWITCH:
      return "the keyword 'switch'";
    case TT_CASE:
      return "the keyword 'case'";
    case TT_DEFAULT:
      return "the keyword 'default'";
    case TT_BREAK:
      return "the keyword 'break'";
    case TT_CONTINUE:
      return "the keyword 'continue'";
    case TT_RETURN:
      return "the keyword 'return'";
    case TT_ASM:
      return "the keyword 'asm'";
    case TT_CAST:
      return "the keyword 'cast'";
    case TT_SIZEOF:
      return "the keyword 'sizeof'";
    case TT_TRUE:
      return "the keyword 'true'";
    case TT_FALSE:
      return "the keyword 'false'";
    case TT_NULL:
      return "the keyword 'null'";
    case TT_VOID:
      return "the keyword 'void'";
    case TT_UBYTE:
      return "the keyword 'ubyte'";
    case TT_BYTE:
      return "the keyword 'byte'";
    case TT_CHAR:
      return "the keyword 'char'";
    case TT_USHORT:
      return "the keyword 'ushort'";
    case TT_SHORT:
      return "the keyword 'short'";
    case TT_UINT:
      return "the keyword 'uint'";
    case TT_INT:
      return "the keyword 'int'";
    case TT_WCHAR:
      return "the keyword 'wchar'";
    case TT_ULONG:
      return "the keyword 'ulong'";
    case TT_LONG:
      return "the keyword 'long'";
    case TT_FLOAT:
      return "the keyword 'float'";
    case TT_DOUBLE:
      return "the keyword 'double'";
    case TT_BOOL:
      return "the keyword 'bool'";
    case TT_CONST:
      return "the keyword 'const'";
    case TT_VOLATILE:
      return "the keyword 'volatile'";
    case TT_SEMI:
      return "a semicolon";
    case TT_COMMA:
      return "a comma";
    case TT_LPAREN:
      return "a left parenthesis";
    case TT_RPAREN:
      return "a right parenthesis";
    case TT_LSQUARE:
      return "a left square bracket";
    case TT_RSQUARE:
      return "a right square bracket";
    case TT_LBRACE:
      return "a left brace";
    case TT_RBRACE:
      return "a right brace";
    case TT_DOT:
      return "a period";
    case TT_ARROW:
      return "a structure dereference operator";
    case TT_INC:
      return "an increment operator";
    case TT_DEC:
      return "a decrement operator";
    case TT_STAR:
      return "an asterisk";
    case TT_AMP:
      return "an ampersand";
    case TT_PLUS:
      return "a plus sign";
    case TT_MINUS:
      return "a minus sign";
    case TT_BANG:
      return "an exclaimation mark";
    case TT_TILDE:
      return "a tilde";
    case TT_NEGASSIGN:
      return "a compound negation-assignment operator";
    case TT_LNOTASSIGN:
      return "a compound logical-not-assignment operator";
    case TT_BITNOTASSIGN:
      return "a compound bitwise-not-assignment operator";
    case TT_SLASH:
      return "a slash";
    case TT_PERCENT:
      return "a percent sign";
    case TT_LSHIFT:
      return "a left shift operator";
    case TT_ARSHIFT:
      return "an arithmetic-right-shift operator";
    case TT_LRSHIFT:
      return "a logical-right-shift operator";
    case TT_SPACESHIP:
      return "a three way comparison operator";
    case TT_LANGLE:
      return "a left angle bracket";
    case TT_RANGLE:
      return "a right angle bracket";
    case TT_LTEQ:
      return "a less-than-or-equal-to operator";
    case TT_GTEQ:
      return "a greater-than-or-equal-to operator";
    case TT_EQ:
      return "an equal-to operator";
    case TT_NEQ:
      return "a not-equal-to operator";
    case TT_BAR:
      return "a pipe";
    case TT_CARET:
      return "a caret";
    case TT_LAND:
      return "a logical-and operator";
    case TT_LOR:
      return "a logical-or operator";
    case TT_QUESTION:
      return "a question mark";
    case TT_COLON:
      return "a colon";
    case TT_ASSIGN:
      return "an equal sign";
    case TT_MULASSIGN:
      return "a compound multiplication-assignment operator";
    case TT_DIVASSIGN:
      return "a compound division-assignment operator";
    case TT_MODASSIGN:
      return "a compound modulo-assignment operator";
    case TT_ADDASSIGN:
      return "a compound addition-assignment operator";
    case TT_SUBASSIGN:
      return "a compound subtraction-assignment operator";
    case TT_LSHIFTASSIGN:
      return "a compound left-shift-assignment operator";
    case TT_ARSHIFTASSIGN:
      return "a compound arithmetic-right-shift-assignment operator";
    case TT_LRSHIFTASSIGN:
      return "a compound logical-right-shift-assignment operator";
    case TT_BITANDASSIGN:
      return "a compound bitwise-and-assignment operator";
    case TT_BITXORASSIGN:
      return "a compound bitwise-exclusive-or-assignment operator";
    case TT_BITORASSIGN:
      return "a compound bitwise-or-assignment-operator";
    case TT_LANDASSIGN:
      return "a compound logical-and-assignment-operator";
    case TT_LORASSIGN:
      return "a compound logical-or-assignment-operator";
    case TT_SCOPE:
      return "a scope-resolution operator";
    case TT_ID:
      return "an identifier";
    case TT_LIT_STRING:
    case TT_BAD_STRING:
      return "a string literal";
    case TT_LIT_WSTRING:
      return "a wide string literal";
    case TT_LIT_CHAR:
    case TT_BAD_CHAR:
      return "a character literal";
    case TT_LIT_WCHAR:
      return "a wide character literal";
    case TT_LIT_INT_0:
    case TT_LIT_INT_B:
    case TT_LIT_INT_O:
    case TT_LIT_INT_D:
    case TT_LIT_INT_H:
    case TT_BAD_BIN:
    case TT_BAD_HEX:
      return "an integer literal";
    case TT_LIT_DOUBLE:
    case TT_LIT_FLOAT:
      return "a floating-point literal";
    default:
      error(__FILE__, __LINE__, "invalid TokenType enum constant encountered");
  }
}

// panics

/**
 * reads tokens until the end of a top-level form
 *
 * semicolons are consumed, EOFs, and the start of a top level form are left
 *
 * @param entry entry to lex from
 * @returns status code: 0 = OK, -1 = fatal error
 */
static int panicTopLevel(FileListEntry *entry) {
  int retval;
  Token token;
  while (true) {
    retval = lex(entry, &token);
    if (retval == -1) {
      return -1;
    } else {
      switch (token.type) {
        case TT_SEMI: {
          tokenUninit(&token);
          return 0;
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
          unLex(entry, &token, retval);
          return 0;
        }
        default: {
          tokenUninit(&token);
          break;
        }
      }
    }
  }
}

// parsing

/**
 * parses either a simple ID or a compound ID
 *
 * @param entry entry to lex from
 * @param id Node to fill in
 * @returns status code: 0 = OK, 1 = recovered error, -1 = fatal error
 */
static int parseAnyId(FileListEntry *entry, Node *id) {
  // TODO: write this
  return -1;
}

/**
 * parses a module line
 *
 * @param entry entry to lex from
 * @param module Node to fill in
 * @returns status code: 0 = OK, 1 = recovered error, -1 = fatal error
 */
static int parseModule(FileListEntry *entry, Node *module) {
  bool errored = false;
  int retval;

  Token moduleKeyword;
  retval = lex(entry, &moduleKeyword);
  if (retval == -1) {
    return retval;
  } else if (retval == 1) {
    errored = true;
  }

  if (moduleKeyword.type != TT_MODULE) {
    // not a module line!
    fprintf(stderr,
            "%s:%zu:%zu: error: expected the module keyword, but found %s\n",
            entry->inputFile, moduleKeyword.line, moduleKeyword.character,
            tokenTypeToString(moduleKeyword.type));
    tokenUninit(&moduleKeyword);
    retval = panicTopLevel(entry);
    if (retval == -1) {
      return retval;
    } else {
      return 1;
    }
  }

  Node *idNode = malloc(sizeof(Node));
  retval = parseAnyId(entry, idNode);
  if (retval == -1) {
    tokenUninit(&moduleKeyword);
    return retval;
  } else if (retval == 1) {
    errored = true;
  }

  if (errored) {
    tokenUninit(&moduleKeyword);
    nodeUninit(idNode);
    free(idNode);
    return 1;
  } else {
    module->type = NT_MODULE;
    module->line = moduleKeyword.line;
    module->character = moduleKeyword.character;
    module->data.module.id = idNode;
    return 0;
  }
}

/**
 * parses a set of import lines
 *
 * @param entry entry to lex from
 * @param imports Vector of Nodes to fill in
 * @returns status code: 0 = OK, 1 = recovered error, -1 = fatal error
 */
static int parseImports(FileListEntry *entry, Vector *imports) {
  // TODO: write this
  return -1;
}

/**
 * parses a set of declaration bodies
 *
 * @param entry entry to lex from
 * @param bodies Vector of Nodes to fill in
 * @returns status code: 0 = OK, 1 = recovered error, -1 = fatal error
 */
static int parseDeclBodies(FileListEntry *entry, Vector *bodies) {
  // TODO: write this
  return -1;
}

/**
 * parses a set of code bodies
 *
 * @param entry entry to lex from
 * @param bodies Vector of Nodes to fill in
 * @returns status code: 0 = OK, 1 = recovered error, -1 = fatal error
 */
static int parseCodeBodies(FileListEntry *entry, Vector *bodies) {
  // TODO: write this
  return -1;
}

/**
 * parses a file, phase one
 *
 * @param entry entry to parse
 * @returns status code: 0 = OK, -1 = error
 */
static int parseFile(FileListEntry *entry) {
  bool errored = false;
  int retval;

  Node *module = malloc(sizeof(Node));
  retval = parseModule(entry, module);
  if (retval == -1) {
    nodeUninit(module);
    free(module);
    return retval;
  } else if (retval == 1) {
    errored = true;
  }

  vectorInit(&entry->program.data.file.imports);
  retval = parseImports(entry, &entry->program.data.file.imports);
  if (retval == -1) {
    nodeUninit(module);
    free(module);
    vectorUninit(&entry->program.data.file.imports,
                 (void (*)(void *))nodeUninit);
    return retval;
  } else if (retval == 1) {
    errored = true;
  }

  vectorInit(&entry->program.data.file.bodies);
  retval = entry->isCode
               ? parseCodeBodies(entry, &entry->program.data.file.bodies)
               : parseDeclBodies(entry, &entry->program.data.file.bodies);
  if (retval == -1) {
    nodeUninit(module);
    free(module);
    vectorUninit(&entry->program.data.file.imports,
                 (void (*)(void *))nodeUninit);
    vectorUninit(&entry->program.data.file.bodies,
                 (void (*)(void *))nodeUninit);
    return retval;
  } else if (retval == 1) {
    errored = true;
  }

  if (errored) {
    nodeUninit(module);
    free(module);
    vectorUninit(&entry->program.data.file.imports,
                 (void (*)(void *))nodeUninit);
    vectorUninit(&entry->program.data.file.bodies,
                 (void (*)(void *))nodeUninit);
    return -1;
  } else {
    entry->program.line = module->line;
    entry->program.character = module->character;
    entry->program.data.file.module = module;
    return 0;
  }
}

int parse(void) {
  int retval = 0;
  size_t badIdx; /**< set for uninitialization in pass one if retval != 0 */

  lexerInitMaps();

  // pass one - parse and gather top-level names, leaving some nodes as unparsed
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      badIdx = idx;
      continue;
    }

    retval = parseFile(&fileList.entries[idx]);
    if (retval != 0) {
      badIdx = idx;
      lexerStateUninit(&fileList.entries[idx]);
      continue;
    }

    lexerStateUninit(&fileList.entries[idx]);
  }

  if (retval != 0) {
    for (size_t idx = 0; idx < badIdx; idx++)
      nodeUninit(&fileList.entries[idx].program);
    return retval;
  }

  lexerUninitMaps();

  // pass two - resolve imports and parse unparsed nodes
  for (size_t idx = 0; idx < fileList.size; idx++) {
    // TODO: write this
  }

  if (retval != 0) {
    for (size_t idx = 0; idx < fileList.size; idx++)
      nodeUninit(&fileList.entries[idx].program);
    return retval;
  }

  return 0;
}