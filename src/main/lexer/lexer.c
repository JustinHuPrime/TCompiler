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
 * @param state LexerState to draw line and character from
 * @param token token to initialize
 * @param type type of token
 * @param string additional data, may be null, depends on type
 */
static void tokenInit(LexerState *state, Token *token, TokenType type,
                      char *string) {
  token->type = type;
  token->line = state->line;
  token->character = state->character;
  token->string = string;

  // consistency check
  assert(token->string == NULL ||
         (token->type >= TT_ID && token->type <= TT_LIT_FLOAT));
}

void tokenUninit(Token *token) { free(token->string); }

int lexerStateInit(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  state->character = 0;
  state->line = 0;
  state->pushedBack = 0;

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
  state->current = state->map =
      mmap(NULL, (size_t)statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (state->map == (void *)-1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }

  state->length = (size_t)statbuf.st_size;

  return 0;
}

/**
 * gets a character from the lexer, returns '\x04' if end of file
 */
static char get(LexerState *state) {
  if (state->current >= state->map + state->length)
    return '\x04';
  else
    return *state->current++;
}
/**
 * returns n characters to the lexer
 * must match with a get - i.e. may not put before beginning
 */
static void put(LexerState *state, size_t n) {
  state->current -= n;
  assert(state->current >= state->map);
}
/**
 * consumes whitespace while updating the entry
 * @param entry entry to munch from
 */
static int lexWhitespace(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  bool whitespace = true;
  while (whitespace) {
    char c = get(state);
    switch (c) {
      case ' ':
      case '\t': {
        state->character += 1;
        break;
      }
      case '\n': {
        state->character = 1;
        state->line++;
        break;
      }
      case '\r': {
        state->character = 1;
        state->line++;
        // handle cr-lf
        char lf = get(state);
        if (lf != '\n') put(state, 1);
        break;
      }
      case '/': {
        char next = get(state);
        switch (next) {
          case '/': {
            // line comment
            state->character += 2;

            bool inComment = true;
            while (inComment) {
              char commentChar = get(state);
              switch (commentChar) {
                case '\x04': {
                  // eof
                  inComment = false;
                  put(state, 1);
                  break;
                }
                case '\n': {
                  inComment = false;
                  state->character = 1;
                  state->line += 1;
                  break;
                }
                case '\r': {
                  inComment = false;
                  state->character = 1;
                  state->line += 1;
                  // handle cr-lf
                  char lf = get(state);
                  if (lf != '\n') put(state, 1);
                  break;
                }
                default: {
                  // much that
                  state->character += 1;
                  break;
                }
              }
            }
            break;
          }
          case '*': {
            state->character += 2;

            bool inComment = true;
            while (inComment) {
              char commentChar = get(state);
              switch (commentChar) {
                case '\x04': {
                  fprintf(stderr,
                          "%s:%zu:%zu: error: unterminated block comment\n",
                          entry->inputFile, state->line, state->character);
                  return -1;
                }
                case '\n': {
                  state->character = 1;
                  state->line += 1;
                  break;
                }
                case '\r': {
                  state->character = 1;
                  state->line += 1;
                  // handle cr-lf
                  char lf = get(state);
                  if (lf != '\n') put(state, 1);
                  break;
                }
                case '*': {
                  // maybe the end?
                  char slash = get(state);
                  state->character += 2;
                  inComment = slash != '/';
                  break;
                }
                default: {
                  state->character += 1;
                  break;
                }
              }
            }
            break;
          }
          default: {
            // not a comment, return initial slash and second char
            put(state, 2);
            whitespace = false;
            break;
          }
        }
        break;
      }
      default: {
        // not whitespace anymore
        put(state, 1);
        whitespace = false;
        break;
      }
    }
  }

  return 0;
}

/** lexes a number */
static int lexNumber(FileListEntry *entry, Token *token) {
  // TODO: write this
  return -1;
}

int lex(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  // munch whitespace
  if (lexWhitespace(entry) != 0) return -1;
  // return a token
  char c = get(state);
  switch (c) {
    // EOF
    case '\x04': {
      tokenInit(state, token, TT_EOF, NULL);
      return 0;
    }

    // punctuation
    case ';': {
      tokenInit(state, token, TT_SEMI, NULL);
      state->character += 1;
      return 0;
    }
    case ',': {
      tokenInit(state, token, TT_COMMA, NULL);
      state->character += 1;
      return 0;
    }
    case '(': {
      tokenInit(state, token, TT_LPAREN, NULL);
      state->character += 1;
      return 0;
    }
    case ')': {
      tokenInit(state, token, TT_RPAREN, NULL);
      state->character += 1;
      return 0;
    }
    case '[': {
      tokenInit(state, token, TT_LSQUARE, NULL);
      state->character += 1;
      return 0;
    }
    case ']': {
      tokenInit(state, token, TT_RSQUARE, NULL);
      state->character += 1;
      return 0;
    }
    case '{': {
      tokenInit(state, token, TT_LBRACE, NULL);
      state->character += 1;
      return 0;
    }
    case '}': {
      tokenInit(state, token, TT_RBRACE, NULL);
      state->character += 1;
      return 0;
    }
    case '.': {
      tokenInit(state, token, TT_DOT, NULL);
      state->character += 1;
      return 0;
    }
    case '-': {
      char next = get(state);
      switch (next) {
        case '>': {
          // ->
          tokenInit(state, token, TT_ARROW, NULL);
          state->character += 2;
          return 0;
        }
        case '-': {
          // --
          tokenInit(state, token, TT_DEC, NULL);
          state->character += 2;
          return 0;
        }
        case '=': {
          // -=
          tokenInit(state, token, TT_SUBASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          if (next >= '0' && next <= '9') {
            // number, return - and zero
            put(state, 2);
            return lexNumber(entry, token);
          } else {
            // just -
            put(state, 1);
            tokenInit(state, token, TT_MINUS, NULL);
            state->character += 1;
            return 0;
          }
        }
      }
    }
    case '+': {
      char next = get(state);
      switch (next) {
        case '+': {
          // ++
          tokenInit(state, token, TT_INC, NULL);
          state->character += 2;
          return 0;
        }
        case '=': {
          // +=
          tokenInit(state, token, TT_ADDASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          if (next >= '0' && next <= '9') {
            // number, return + and zero
            put(state, 2);
            return lexNumber(entry, token);
          } else {
            // just +
            put(state, 1);
            tokenInit(state, token, TT_PLUS, NULL);
            state->character += 1;
            return 0;
          }
        }
      }
    }
    case '*': {
      char next = get(state);
      switch (next) {
        case '=': {
          // *=
          tokenInit(state, token, TT_MULASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just *
          put(state, 1);
          tokenInit(state, token, TT_STAR, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '&': {
      char next1 = get(state);
      switch (next1) {
        case '&': {
          // &&
          char next2 = get(state);
          switch (next2) {
            case '=': {
              // &&=
              tokenInit(state, token, TT_LANDASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just &&
              put(state, 1);
              tokenInit(state, token, TT_LAND, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // &=
          tokenInit(state, token, TT_ANDASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just &
          put(state, 1);
          tokenInit(state, token, TT_AMP, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '!': {
      char next = get(state);
      switch (next) {
        case '=': {
          // !=
          tokenInit(state, token, TT_NEQ, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just !
          put(state, 1);
          tokenInit(state, token, TT_BANG, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '~': {
      tokenInit(state, token, TT_TILDE, NULL);
      state->character += 1;
      return 0;
    }
    case '/': {
      // note - comments dealt with in whitespace
      char next = get(state);
      switch (next) {
        case '=': {
          // /=
          tokenInit(state, token, TT_DIVASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just /
          put(state, 1);
          tokenInit(state, token, TT_SLASH, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '%': {
      char next = get(state);
      switch (next) {
        case '=': {
          // %=
          tokenInit(state, token, TT_MODASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just %
          put(state, 1);
          tokenInit(state, token, TT_PERCENT, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '<': {
      char next1 = get(state);
      switch (next1) {
        case '<': {
          // <<
          char next2 = get(state);
          switch (next2) {
            case '=': {
              tokenInit(state, token, TT_LSHIFTASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just <<
              put(state, 1);
              tokenInit(state, token, TT_LSHIFT, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // <=
          char next2 = get(state);
          switch (next2) {
            case '>': {
              // <=>
              tokenInit(state, token, TT_SPACESHIP, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just <=
              put(state, 1);
              tokenInit(state, token, TT_LTEQ, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        default: {
          // just <
          put(state, 1);
          tokenInit(state, token, TT_LANGLE, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '|': {
      char next1 = get(state);
      switch (next1) {
        case '|': {
          // ||
          char next2 = get(state);
          switch (next2) {
            case '=': {
              // ||=
              tokenInit(state, token, TT_LORASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just ||
              put(state, 1);
              tokenInit(state, token, TT_LOR, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // |=
          tokenInit(state, token, TT_ORASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just |
          put(state, 1);
          tokenInit(state, token, TT_BAR, NULL);
          state->character += 1;
          return 0;
        }
      }
    }

    // everything else
    default: {
      // TODO: handle unexpected char
      return -1;
    }
  }
}

void unLex(FileListEntry *entry, Token const *token) {
  // only one token of lookahead is allowed
  LexerState *state = &entry->lexerState;
  assert(!state->pushedBack);
  state->pushedBack = true;
  memcpy(&state->previous, token, sizeof(Token));
}

void lexerStateUninit(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  munmap((void *)state->map, state->length);
  if (state->pushedBack) tokenUninit(&state->previous);
}