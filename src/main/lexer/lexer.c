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

void tokenInit(Token *token, TokenType type, size_t line, size_t character,
               char *string) {
  token->type = type;
  token->line = line;
  token->character = character;
  token->string = string;

  // consistency check
  assert(token->string == NULL ||
         (token->type >= TT_ID && token->type <= TT_LIT_FLOAT));
}

void tokenUninit(Token *token) {
  // if the token is known to have a string, free it
  if (token->type >= TT_ID && token->type <= TT_LIT_FLOAT) free(token->string);
}

int lexerStateInit(FileListEntry *entry) {
  entry->lexerState.character = 0;
  entry->lexerState.line = 0;
  entry->lexerState.pushedBack = 0;

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
  entry->lexerState.current = entry->lexerState.map =
      mmap(NULL, (size_t)statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (entry->lexerState.map == (void *)-1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }

  entry->lexerState.length = (size_t)statbuf.st_size;

  return 0;
}

int lex(FileListEntry *entry, Token *token) {}

void unLex(FileListEntry *entry, Token const *token) {
  // only one token of lookahead is allowed
  assert(!entry->lexerState.pushedBack);
  entry->lexerState.pushedBack = true;
  memcpy(&entry->lexerState.previous, token, sizeof(Token));
}

void lexerStateUninit(FileListEntry *entry) {
  munmap((void *)entry->lexerState.map, entry->lexerState.length);
  if (entry->lexerState.pushedBack) tokenUninit(&entry->lexerState.previous);
}