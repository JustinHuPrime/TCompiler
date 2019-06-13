// Copyright 2019 Justin Hu
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

// Tests for the lexer

#include "lexer/lexer.h"

#include "tests.h"
#include "util/functional.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

void keywordMapTest(TestStatus *status) {
  KeywordMap *keywords = keywordMapCreate();

  test(status, "[lexer] [keywordMap] keywordMap isn't empty",
       keywords->size != 0);
  test(status, "[lexer] [keywordMap] keywordMap has a keyword",
       keywordMapGet(keywords, "return") != NULL);
  test(status, "[lexer] [keywordMap] keywordMap doesn't have non-keywords",
       keywordMapGet(keywords, "foo") == NULL);

  keywordMapDestroy(keywords);
}

void lexerTest(TestStatus *status) {
  TokenInfo tokenInfo;

  Report *report = reportCreate();
  KeywordMap *keywords = keywordMapCreate();

  LexerInfo *info =
      lexerInfoCreate("testFiles/lexer/lexerTestBasic.tc", keywords);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] basic file token one is module",
       tokenInfo.type == TT_MODULE);
  test(status, "[lexer] [lex] basic file token one is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token one is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] basic file token two is id",
       tokenInfo.type == TT_ID);
  test(status, "[lexer] [lex] basic file token two is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token two is at char 8",
       tokenInfo.character == 8);
  test(status, "[lexer] [lex] basic file token two is 'foo'",
       strcmp("foo", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] basic file token three is semicolon",
       tokenInfo.type == TT_SEMI);
  test(status, "[lexer] [lex] basic file token three is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token three is at char 11",
       tokenInfo.character == 11);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] basic file token four is eof",
       tokenInfo.type == TT_EOF);
  test(status, "[lexer] [lex] basic file token four is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token four is at char 12",
       tokenInfo.character == 12);

  lexerInfoDestroy(info);

  info = lexerInfoCreate("testFiles/lexer/lexerTestComprehensive.tc", keywords);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 1 is module",
       tokenInfo.type == TT_MODULE);
  test(status, "[lexer] [lex] comprehensive file token 1 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 1 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 2 is import",
       tokenInfo.type == TT_IMPORT);
  test(status, "[lexer] [lex] comprehensive file token 2 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 2 is at char 8",
       tokenInfo.character == 8);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 3 is struct",
       tokenInfo.type == TT_STRUCT);
  test(status, "[lexer] [lex] comprehensive file token 3 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 3 is at char 15",
       tokenInfo.character == 15);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 4 is union",
       tokenInfo.type == TT_UNION);
  test(status, "[lexer] [lex] comprehensive file token 4 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 4 is at char 22",
       tokenInfo.character == 22);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 5 is enum",
       tokenInfo.type == TT_ENUM);
  test(status, "[lexer] [lex] comprehensive file token 5 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 5 is at char 28",
       tokenInfo.character == 28);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 6 is typedef",
       tokenInfo.type == TT_TYPEDEF);
  test(status, "[lexer] [lex] comprehensive file token 6 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 6 is at char 33",
       tokenInfo.character == 33);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 7 is if",
       tokenInfo.type == TT_IF);
  test(status, "[lexer] [lex] comprehensive file token 7 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 7 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 8 is else",
       tokenInfo.type == TT_ELSE);
  test(status, "[lexer] [lex] comprehensive file token 8 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 8 is at char 4",
       tokenInfo.character == 4);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 9 is while",
       tokenInfo.type == TT_WHILE);
  test(status, "[lexer] [lex] comprehensive file token 9 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 9 is at char 9",
       tokenInfo.character == 9);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 10 is do",
       tokenInfo.type == TT_DO);
  test(status, "[lexer] [lex] comprehensive file token 10 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 10 is at char 15",
       tokenInfo.character == 15);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 11 is for",
       tokenInfo.type == TT_FOR);
  test(status, "[lexer] [lex] comprehensive file token 11 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 11 is at char 18",
       tokenInfo.character == 18);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 12 is switch",
       tokenInfo.type == TT_SWITCH);
  test(status, "[lexer] [lex] comprehensive file token 12 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 12 is at char 22",
       tokenInfo.character == 22);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 13 is case",
       tokenInfo.type == TT_CASE);
  test(status, "[lexer] [lex] comprehensive file token 13 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 13 is at char 29",
       tokenInfo.character == 29);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 14 is default",
       tokenInfo.type == TT_DEFAULT);
  test(status, "[lexer] [lex] comprehensive file token 14 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 14 is at char 34",
       tokenInfo.character == 34);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 15 is break",
       tokenInfo.type == TT_BREAK);
  test(status, "[lexer] [lex] comprehensive file token 15 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 15 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 16 is continue",
       tokenInfo.type == TT_CONTINUE);
  test(status, "[lexer] [lex] comprehensive file token 16 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 16 is at char 7",
       tokenInfo.character == 7);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 17 is return",
       tokenInfo.type == TT_RETURN);
  test(status, "[lexer] [lex] comprehensive file token 17 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 17 is at char 16",
       tokenInfo.character == 16);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 18 is asm",
       tokenInfo.type == TT_ASM);
  test(status, "[lexer] [lex] comprehensive file token 18 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 18 is at char 23",
       tokenInfo.character == 23);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 19 is true",
       tokenInfo.type == TT_TRUE);
  test(status, "[lexer] [lex] comprehensive file token 19 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 19 is at char 27",
       tokenInfo.character == 27);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 20 is false",
       tokenInfo.type == TT_FALSE);
  test(status, "[lexer] [lex] comprehensive file token 20 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 20 is at char 32",
       tokenInfo.character == 32);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 21 is cast",
       tokenInfo.type == TT_CAST);
  test(status, "[lexer] [lex] comprehensive file token 21 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 21 is at char 38",
       tokenInfo.character == 38);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 22 is sizeof",
       tokenInfo.type == TT_SIZEOF);
  test(status, "[lexer] [lex] comprehensive file token 22 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 22 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 23 is void",
       tokenInfo.type == TT_VOID);
  test(status, "[lexer] [lex] comprehensive file token 23 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 23 is at char 8",
       tokenInfo.character == 8);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 24 is ubyte",
       tokenInfo.type == TT_UBYTE);
  test(status, "[lexer] [lex] comprehensive file token 24 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 24 is at char 13",
       tokenInfo.character == 13);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 25 is byte",
       tokenInfo.type == TT_BYTE);
  test(status, "[lexer] [lex] comprehensive file token 25 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 25 is at char 19",
       tokenInfo.character == 19);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 26 is char",
       tokenInfo.type == TT_CHAR);
  test(status, "[lexer] [lex] comprehensive file token 26 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 26 is at char 24",
       tokenInfo.character == 24);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 27 is uint",
       tokenInfo.type == TT_UINT);
  test(status, "[lexer] [lex] comprehensive file token 27 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 27 is at char 29",
       tokenInfo.character == 29);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 28 is int",
       tokenInfo.type == TT_INT);
  test(status, "[lexer] [lex] comprehensive file token 28 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 28 is at char 34",
       tokenInfo.character == 34);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 29 is wchar",
       tokenInfo.type == TT_WCHAR);
  test(status, "[lexer] [lex] comprehensive file token 29 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 29 is at char 38",
       tokenInfo.character == 38);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 30 is ulong",
       tokenInfo.type == TT_ULONG);
  test(status, "[lexer] [lex] comprehensive file token 30 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 30 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 31 is long",
       tokenInfo.type == TT_LONG);
  test(status, "[lexer] [lex] comprehensive file token 31 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 31 is at char 7",
       tokenInfo.character == 7);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 32 is float",
       tokenInfo.type == TT_FLOAT);
  test(status, "[lexer] [lex] comprehensive file token 32 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 32 is at char 12",
       tokenInfo.character == 12);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 33 is double",
       tokenInfo.type == TT_DOUBLE);
  test(status, "[lexer] [lex] comprehensive file token 33 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 33 is at char 18",
       tokenInfo.character == 18);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 34 is bool",
       tokenInfo.type == TT_BOOL);
  test(status, "[lexer] [lex] comprehensive file token 34 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 34 is at char 25",
       tokenInfo.character == 25);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 35 is const",
       tokenInfo.type == TT_CONST);
  test(status, "[lexer] [lex] comprehensive file token 35 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 35 is at char 30",
       tokenInfo.character == 30);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 36 is semi",
       tokenInfo.type == TT_SEMI);
  test(status, "[lexer] [lex] comprehensive file token 36 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 36 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 37 is comma",
       tokenInfo.type == TT_COMMA);
  test(status, "[lexer] [lex] comprehensive file token 37 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 37 is at char 2",
       tokenInfo.character == 2);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 38 is lparen",
       tokenInfo.type == TT_LPAREN);
  test(status, "[lexer] [lex] comprehensive file token 38 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 38 is at char 3",
       tokenInfo.character == 3);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 39 is rparen",
       tokenInfo.type == TT_RPAREN);
  test(status, "[lexer] [lex] comprehensive file token 39 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 39 is at char 4",
       tokenInfo.character == 4);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 40 is lsquare",
       tokenInfo.type == TT_LSQUARE);
  test(status, "[lexer] [lex] comprehensive file token 40 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 40 is at char 5",
       tokenInfo.character == 5);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 41 is rsquare",
       tokenInfo.type == TT_RSQUARE);
  test(status, "[lexer] [lex] comprehensive file token 41 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 41 is at char 6",
       tokenInfo.character == 6);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 42 is lbrace",
       tokenInfo.type == TT_LBRACE);
  test(status, "[lexer] [lex] comprehensive file token 42 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 42 is at char 7",
       tokenInfo.character == 7);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 43 is rbrace",
       tokenInfo.type == TT_RBRACE);
  test(status, "[lexer] [lex] comprehensive file token 43 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 43 is at char 8",
       tokenInfo.character == 8);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 44 is dot",
       tokenInfo.type == TT_DOT);
  test(status, "[lexer] [lex] comprehensive file token 44 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 44 is at char 9",
       tokenInfo.character == 9);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 45 is arrow",
       tokenInfo.type == TT_ARROW);
  test(status, "[lexer] [lex] comprehensive file token 45 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 45 is at char 1",
       tokenInfo.character == 1);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 46 is plusplus",
       tokenInfo.type == TT_PLUSPLUS);
  test(status, "[lexer] [lex] comprehensive file token 46 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 46 is at char 3",
       tokenInfo.character == 3);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 47 is minusminus",
       tokenInfo.type == TT_MINUSMINUS);
  test(status, "[lexer] [lex] comprehensive file token 47 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 47 is at char 5",
       tokenInfo.character == 5);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 48 is star",
       tokenInfo.type == TT_STAR);
  test(status, "[lexer] [lex] comprehensive file token 48 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 48 is at char 7",
       tokenInfo.character == 7);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 49 is ampersand",
       tokenInfo.type == TT_AMPERSAND);
  test(status, "[lexer] [lex] comprehensive file token 49 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 49 is at char 8",
       tokenInfo.character == 8);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 50 is plus",
       tokenInfo.type == TT_PLUS);
  test(status, "[lexer] [lex] comprehensive file token 50 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 50 is at char 9",
       tokenInfo.character == 9);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 51 is minus",
       tokenInfo.type == TT_MINUS);
  test(status, "[lexer] [lex] comprehensive file token 51 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 51 is at char 10",
       tokenInfo.character == 10);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 52 is bang",
       tokenInfo.type == TT_BANG);
  test(status, "[lexer] [lex] comprehensive file token 52 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 52 is at char 11",
       tokenInfo.character == 11);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 53 is tilde",
       tokenInfo.type == TT_TILDE);
  test(status, "[lexer] [lex] comprehensive file token 53 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 53 is at char 12",
       tokenInfo.character == 12);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 54 is slash",
       tokenInfo.type == TT_SLASH);
  test(status, "[lexer] [lex] comprehensive file token 54 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 54 is at char 13",
       tokenInfo.character == 13);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 55 is percent",
       tokenInfo.type == TT_PERCENT);
  test(status, "[lexer] [lex] comprehensive file token 55 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 55 is at char 14",
       tokenInfo.character == 14);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 56 is lshift",
       tokenInfo.type == TT_LSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 56 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 56 is at char 15",
       tokenInfo.character == 15);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 57 is lrshift",
       tokenInfo.type == TT_LRSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 57 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 57 is at char 17",
       tokenInfo.character == 17);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 58 is arshift",
       tokenInfo.type == TT_ARSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 58 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 58 is at char 20",
       tokenInfo.character == 20);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 59 is spaceship",
       tokenInfo.type == TT_SPACESHIP);
  test(status, "[lexer] [lex] comprehensive file token 59 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 59 is at char 22",
       tokenInfo.character == 22);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 60 is langle",
       tokenInfo.type == TT_LANGLE);
  test(status, "[lexer] [lex] comprehensive file token 60 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 60 is at char 25",
       tokenInfo.character == 25);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 61 is rangle",
       tokenInfo.type == TT_RANGLE);
  test(status, "[lexer] [lex] comprehensive file token 61 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 61 is at char 26",
       tokenInfo.character == 26);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 62 is lteq",
       tokenInfo.type == TT_LTEQ);
  test(status, "[lexer] [lex] comprehensive file token 62 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 62 is at char 27",
       tokenInfo.character == 27);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 63 is gteq",
       tokenInfo.type == TT_GTEQ);
  test(status, "[lexer] [lex] comprehensive file token 63 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 63 is at char 33",
       tokenInfo.character == 33);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 64 is eq",
       tokenInfo.type == TT_EQ);
  test(status, "[lexer] [lex] comprehensive file token 64 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 64 is at char 35",
       tokenInfo.character == 35);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 65 is neq",
       tokenInfo.type == TT_NEQ);
  test(status, "[lexer] [lex] comprehensive file token 65 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 65 is at char 37",
       tokenInfo.character == 37);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 66 is pipe",
       tokenInfo.type == TT_PIPE);
  test(status, "[lexer] [lex] comprehensive file token 66 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 66 is at char 39",
       tokenInfo.character == 39);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 67 is caret",
       tokenInfo.type == TT_CARET);
  test(status, "[lexer] [lex] comprehensive file token 67 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 67 is at char 40",
       tokenInfo.character == 40);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 68 is land",
       tokenInfo.type == TT_LAND);
  test(status, "[lexer] [lex] comprehensive file token 68 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 68 is at char 41",
       tokenInfo.character == 41);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 69 is lor",
       tokenInfo.type == TT_LOR);
  test(status, "[lexer] [lex] comprehensive file token 69 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 69 is at char 43",
       tokenInfo.character == 43);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 70 is question",
       tokenInfo.type == TT_QUESTION);
  test(status, "[lexer] [lex] comprehensive file token 70 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 70 is at char 45",
       tokenInfo.character == 45);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 71 is colon",
       tokenInfo.type == TT_COLON);
  test(status, "[lexer] [lex] comprehensive file token 71 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 71 is at char 46",
       tokenInfo.character == 46);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 72 is assign",
       tokenInfo.type == TT_ASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 72 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 72 is at char 47",
       tokenInfo.character == 47);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 73 is mulassign",
       tokenInfo.type == TT_MULASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 73 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 73 is at char 48",
       tokenInfo.character == 48);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 74 is divassign",
       tokenInfo.type == TT_DIVASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 74 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 74 is at char 50",
       tokenInfo.character == 50);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 75 is modassign",
       tokenInfo.type == TT_MODASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 75 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 75 is at char 52",
       tokenInfo.character == 52);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 76 is addassign",
       tokenInfo.type == TT_ADDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 76 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 76 is at char 54",
       tokenInfo.character == 54);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 77 is subassign",
       tokenInfo.type == TT_SUBASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 77 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 77 is at char 56",
       tokenInfo.character == 56);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 78 is lshiftassign",
       tokenInfo.type == TT_LSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 78 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 78 is at char 58",
       tokenInfo.character == 58);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 79 is lrshiftassign",
       tokenInfo.type == TT_LRSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 79 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 79 is at char 61",
       tokenInfo.character == 61);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 80 is arshiftassign",
       tokenInfo.type == TT_ARSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 80 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 80 is at char 65",
       tokenInfo.character == 65);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 81 is bitandassign",
       tokenInfo.type == TT_BITANDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 81 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 81 is at char 68",
       tokenInfo.character == 68);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 82 is bitxorassign",
       tokenInfo.type == TT_BITXORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 82 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 82 is at char 70",
       tokenInfo.character == 70);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 83 is bitorassign",
       tokenInfo.type == TT_BITORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 83 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 83 is at char 72",
       tokenInfo.character == 72);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 84 is landassign",
       tokenInfo.type == TT_LANDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 84 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 84 is at char 74",
       tokenInfo.character == 74);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 85 is lorassign",
       tokenInfo.type == TT_LORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 85 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 85 is at char 77",
       tokenInfo.character == 77);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 86 is id",
       tokenInfo.type == TT_ID);
  test(status, "[lexer] [lex] comprehensive file token 86 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 86 is at char 80",
       tokenInfo.character == 80);
  test(status, "[lexer] [lex] comprehensive file token 86 is 'id'",
       strcmp("id", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 87 is scoped id",
       tokenInfo.type == TT_SCOPED_ID);
  test(status, "[lexer] [lex] comprehensive file token 87 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 87 is at char 102",
       tokenInfo.character == 102);
  test(
      status,
      "[lexer] [lex] comprehensive file token 87 is 'scoped::id::withCapitals'",
      strcmp("scoped::id::withCapitals", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 88 is literal zero",
       tokenInfo.type == TT_LITERALINT_0);
  test(status, "[lexer] [lex] comprehensive file token 88 is at line 11",
       tokenInfo.line == 11);
  test(status, "[lexer] [lex] comprehensive file token 88 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 88 is '0'",
       strcmp("0", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 89 is literal binary",
       tokenInfo.type == TT_LITERALINT_B);
  test(status, "[lexer] [lex] comprehensive file token 89 is at line 12",
       tokenInfo.line == 12);
  test(status, "[lexer] [lex] comprehensive file token 89 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 89 is '0b101'",
       strcmp("0b101", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 90 is literal octal",
       tokenInfo.type == TT_LITERALINT_O);
  test(status, "[lexer] [lex] comprehensive file token 90 is at line 13",
       tokenInfo.line == 13);
  test(status, "[lexer] [lex] comprehensive file token 90 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 90 is '0135'",
       strcmp("0135", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 91 is literal decimal",
       tokenInfo.type == TT_LITERALINT_D);
  test(status, "[lexer] [lex] comprehensive file token 91 is at line 14",
       tokenInfo.line == 14);
  test(status, "[lexer] [lex] comprehensive file token 91 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 91 is '123'",
       strcmp("123", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status,
       "[lexer] [lex] comprehensive file token 92 is literal hexadecimal",
       tokenInfo.type == TT_LITERALINT_H);
  test(status, "[lexer] [lex] comprehensive file token 92 is at line 15",
       tokenInfo.line == 15);
  test(status, "[lexer] [lex] comprehensive file token 92 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 92 is '0xfF1'",
       strcmp("0xfF1", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status,
       "[lexer] [lex] comprehensive file token 93 is literal floating point",
       tokenInfo.type == TT_LITERALFLOAT);
  test(status, "[lexer] [lex] comprehensive file token 93 is at line 16",
       tokenInfo.line == 16);
  test(status, "[lexer] [lex] comprehensive file token 93 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 93 is '3.1415'",
       strcmp("3.1415", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 94 is literal string",
       tokenInfo.type == TT_LITERALSTRING);
  test(status, "[lexer] [lex] comprehensive file token 94 is at line 17",
       tokenInfo.line == 17);
  test(status, "[lexer] [lex] comprehensive file token 94 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 94 is 'string'",
       strcmp("string", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 95 is literal char",
       tokenInfo.type == TT_LITERALCHAR);
  test(status, "[lexer] [lex] comprehensive file token 95 is at line 18",
       tokenInfo.line == 18);
  test(status, "[lexer] [lex] comprehensive file token 95 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 95 is 'c'",
       strcmp("c", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 96 is literal wstring",
       tokenInfo.type == TT_LITERALWSTRING);
  test(status, "[lexer] [lex] comprehensive file token 96 is at line 19",
       tokenInfo.line == 19);
  test(status, "[lexer] [lex] comprehensive file token 96 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 96 is 'wstring'",
       strcmp("wstring", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 97 is literal wchar",
       tokenInfo.type == TT_LITERALWCHAR);
  test(status, "[lexer] [lex] comprehensive file token 97 is at line 20",
       tokenInfo.line == 20);
  test(status, "[lexer] [lex] comprehensive file token 97 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 97 is 'c'",
       strcmp("c", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 98 is literal decimal",
       tokenInfo.type == TT_LITERALINT_D);
  test(status, "[lexer] [lex] comprehensive file token 98 is at line 21",
       tokenInfo.line == 21);
  test(status, "[lexer] [lex] comprehensive file token 98 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 98 is '+1'",
       strcmp("+1", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 99 is literal decimal",
       tokenInfo.type == TT_LITERALINT_D);
  test(status, "[lexer] [lex] comprehensive file token 99 is at line 22",
       tokenInfo.line == 22);
  test(status, "[lexer] [lex] comprehensive file token 99 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 99 is '-2'",
       strcmp("-2", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 100 is literal string",
       tokenInfo.type == TT_LITERALSTRING);
  test(status, "[lexer] [lex] comprehensive file token 100 is at line 23",
       tokenInfo.line == 23);
  test(status, "[lexer] [lex] comprehensive file token 100 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 100 is '\\xaF'",
       strcmp("\\xaF", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 101 is literal wchar",
       tokenInfo.type == TT_LITERALWCHAR);
  test(status, "[lexer] [lex] comprehensive file token 101 is at line 23",
       tokenInfo.line == 24);
  test(status, "[lexer] [lex] comprehensive file token 101 is at char 1",
       tokenInfo.character == 1);
  test(status, "[lexer] [lex] comprehensive file token 101 is '\\u12ab34CD'",
       strcmp("\\u12ab34CD", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 192 is eof",
       tokenInfo.type == TT_EOF);
  test(status, "[lexer] [lex] comprehensive file token 102 is at line 21",
       tokenInfo.line == 26);
  test(status, "[lexer] [lex] comprehensive file token 102 is at char 1",
       tokenInfo.character == 1);

  lexerInfoDestroy(info);

  info = lexerInfoCreate("testFiles/lexer/lexerTestCorner.tc", keywords);

  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] id before colon is plain id",
       tokenInfo.type == TT_ID);
  free(tokenInfo.data.string);
  lex(info, report, &tokenInfo);
  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] invalid characters are caught",
       tokenInfo.type == TT_INVALID);
  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] bad escape in string is caught",
       tokenInfo.type == TT_INVALID_ESCAPE);
  lex(info, report, &tokenInfo);
  test(status, "[lexer] [lex] bad escape in char is caught",
       tokenInfo.type == TT_INVALID_ESCAPE);

  lexerInfoDestroy(info);

  keywordMapDestroy(keywords);
  reportDestroy(report);
}