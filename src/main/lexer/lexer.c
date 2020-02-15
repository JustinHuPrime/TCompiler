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

#include "lexer/lexer.h"

#include "fileList.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * initializes a token
 *
 * @param lexerState LexerState to draw line and character from
 * @param token token to initialize
 * @param type type of token
 * @param string additional data, may be null, depends on type
 */
static void tokenInit(LexerState *lexerState, Token *token, TokenType type,
                      char *string) {
  token->type = type;
  token->line = lexerState->line;
  token->character = lexerState->character;
  token->string = string;

  // consistency check
  assert(token->string == NULL ||
         (token->type >= TT_ID && token->type <= TT_LIT_FLOAT));
}

void tokenUninit(Token *token) { free(token->string); }

int lexerStateInit(FileListEntry *entry) {
  LexerState *lexerState = &entry->lexerState;
  lexerState->character = 0;
  lexerState->line = 0;
  lexerState->pushedBack = 0;

  // try to map the file
  int fd = open(entry->inputFile, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }
  struct stat statbuf;
  if (fstat(fd, &statbuf) != 0) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    close(fd);
    return -1;
  }
  lexerState->current = lexerState->map =
      mmap(NULL, (size_t)statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (lexerState->map == (void *)-1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }

  lexerState->length = (size_t)statbuf.st_size;

  return 0;
}

/**
 * gets a character from the lexer, returns '\x04' if end of file
 */
static char get(LexerState *lexerState) {
  if (lexerState->current >= lexerState->map + lexerState->length)
    return '\x04';
  else
    return *lexerState->current++;
}
/**
 * returns n characters to the lexer
 * must match with a get - i.e. may not put before beginning
 */
static char put(LexerState *lexerState, size_t n) {
  lexerState->current -= n;
  assert(lexerState->current >= lexerState->map);
}
/**
 * consumes whitespace while updating the entry
 * @param entry entry to munch from
 */
static void lexWhitespace(FileListEntry *entry) {}

int lex(FileListEntry *entry, Token *token) {
  // munch whitespace
  lexWhitespace(entry);
  // return a token
}

void unLex(FileListEntry *entry, Token const *token) {
  // only one token of lookahead is allowed
  LexerState *lexerState = &entry->lexerState;
  assert(!lexerState->pushedBack);
  lexerState->pushedBack = true;
  memcpy(&lexerState->previous, token, sizeof(Token));
}

void lexerStateUninit(FileListEntry *entry) {
  LexerState *lexerState = &entry->lexerState;
  munmap((void *)lexerState->map, lexerState->length);
  if (lexerState->pushedBack) tokenUninit(&lexerState->previous);
}