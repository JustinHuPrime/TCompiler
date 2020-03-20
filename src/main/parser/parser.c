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
#include "options.h"

#include <stdio.h>
#include <stdlib.h>

// pointer ownership conventions:
// when returning from any error, pointers must not have any data in them,
// and vectors must not have any partial elements

// utility functions

static char const *tokenTypeToString(TokenType type) {
  // TODO: write this
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
          unLex(entry, &token);
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