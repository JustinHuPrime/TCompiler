// Copyright 2019 Justin Hu
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
  TokenType tokenType;

  Report *report = reportCreate();
  KeywordMap *keywords = keywordMapCreate();

  LexerInfo *info = lexerInfoCreate("testFiles/lexerTestBasic.tc", keywords);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] basic file token one is module",
       tokenType == TT_MODULE);
  test(status, "[lexer] [lex] basic file token one is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token one is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] basic file token two is id", tokenType == TT_ID);
  test(status, "[lexer] [lex] basic file token two is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token two is at char 8",
       tokenInfo.character == 8);
  test(status, "[lexer] [lex] basic file token two is 'foo'",
       strcmp("foo", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] basic file token three is semicolon",
       tokenType == TT_SEMI);
  test(status, "[lexer] [lex] basic file token three is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token three is at char 11",
       tokenInfo.character == 11);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] basic file token four is eof",
       tokenType == TT_EOF);
  test(status, "[lexer] [lex] basic file token four is at line 1",
       tokenInfo.line == 1);
  test(status, "[lexer] [lex] basic file token four is at char 12",
       tokenInfo.character == 12);

  lexerInfoDestroy(info);

  info = lexerInfoCreate("testFiles/lexerTestComprehensive.tc", keywords);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 1 is module",
       tokenType == TT_MODULE);
  test(status, "[lexer] [lex] comprehensive file token 1 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 1 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 2 is using",
       tokenType == TT_USING);
  test(status, "[lexer] [lex] comprehensive file token 2 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 2 is at char 8",
       tokenInfo.character == 8);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 3 is struct",
       tokenType == TT_STRUCT);
  test(status, "[lexer] [lex] comprehensive file token 3 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 3 is at char 14",
       tokenInfo.character == 14);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 4 is union",
       tokenType == TT_UNION);
  test(status, "[lexer] [lex] comprehensive file token 4 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 4 is at char 21",
       tokenInfo.character == 21);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 5 is enum",
       tokenType == TT_ENUM);
  test(status, "[lexer] [lex] comprehensive file token 5 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 5 is at char 27",
       tokenInfo.character == 27);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 6 is typedef",
       tokenType == TT_TYPEDEF);
  test(status, "[lexer] [lex] comprehensive file token 6 is at line 2",
       tokenInfo.line == 2);
  test(status, "[lexer] [lex] comprehensive file token 6 is at char 32",
       tokenInfo.character == 32);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 7 is if",
       tokenType == TT_IF);
  test(status, "[lexer] [lex] comprehensive file token 7 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 7 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 8 is else",
       tokenType == TT_ELSE);
  test(status, "[lexer] [lex] comprehensive file token 8 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 8 is at char 4",
       tokenInfo.character == 4);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 9 is while",
       tokenType == TT_WHILE);
  test(status, "[lexer] [lex] comprehensive file token 9 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 9 is at char 9",
       tokenInfo.character == 9);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 10 is do",
       tokenType == TT_DO);
  test(status, "[lexer] [lex] comprehensive file token 10 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 10 is at char 15",
       tokenInfo.character == 15);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 11 is for",
       tokenType == TT_FOR);
  test(status, "[lexer] [lex] comprehensive file token 11 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 11 is at char 18",
       tokenInfo.character == 18);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 12 is switch",
       tokenType == TT_SWITCH);
  test(status, "[lexer] [lex] comprehensive file token 12 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 12 is at char 22",
       tokenInfo.character == 22);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 13 is case",
       tokenType == TT_CASE);
  test(status, "[lexer] [lex] comprehensive file token 13 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 13 is at char 29",
       tokenInfo.character == 29);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 14 is default",
       tokenType == TT_DEFAULT);
  test(status, "[lexer] [lex] comprehensive file token 14 is at line 3",
       tokenInfo.line == 3);
  test(status, "[lexer] [lex] comprehensive file token 14 is at char 34",
       tokenInfo.character == 34);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 15 is break",
       tokenType == TT_BREAK);
  test(status, "[lexer] [lex] comprehensive file token 15 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 15 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 16 is continue",
       tokenType == TT_CONTINUE);
  test(status, "[lexer] [lex] comprehensive file token 16 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 16 is at char 7",
       tokenInfo.character == 7);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 17 is return",
       tokenType == TT_RETURN);
  test(status, "[lexer] [lex] comprehensive file token 17 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 17 is at char 16",
       tokenInfo.character == 16);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 18 is asm",
       tokenType == TT_ASM);
  test(status, "[lexer] [lex] comprehensive file token 18 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 18 is at char 23",
       tokenInfo.character == 23);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 19 is true",
       tokenType == TT_TRUE);
  test(status, "[lexer] [lex] comprehensive file token 19 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 19 is at char 27",
       tokenInfo.character == 27);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 20 is false",
       tokenType == TT_FALSE);
  test(status, "[lexer] [lex] comprehensive file token 20 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 20 is at char 32",
       tokenInfo.character == 32);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 21 is cast",
       tokenType == TT_CAST);
  test(status, "[lexer] [lex] comprehensive file token 21 is at line 4",
       tokenInfo.line == 4);
  test(status, "[lexer] [lex] comprehensive file token 21 is at char 38",
       tokenInfo.character == 38);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 22 is sizeof",
       tokenType == TT_SIZEOF);
  test(status, "[lexer] [lex] comprehensive file token 22 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 22 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 23 is void",
       tokenType == TT_VOID);
  test(status, "[lexer] [lex] comprehensive file token 23 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 23 is at char 8",
       tokenInfo.character == 8);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 24 is ubyte",
       tokenType == TT_UBYTE);
  test(status, "[lexer] [lex] comprehensive file token 24 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 24 is at char 13",
       tokenInfo.character == 13);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 25 is byte",
       tokenType == TT_BYTE);
  test(status, "[lexer] [lex] comprehensive file token 25 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 25 is at char 19",
       tokenInfo.character == 19);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 26 is char",
       tokenType == TT_CHAR);
  test(status, "[lexer] [lex] comprehensive file token 26 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 26 is at char 24",
       tokenInfo.character == 24);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 27 is uint",
       tokenType == TT_UINT);
  test(status, "[lexer] [lex] comprehensive file token 27 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 27 is at char 29",
       tokenInfo.character == 29);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 28 is int",
       tokenType == TT_INT);
  test(status, "[lexer] [lex] comprehensive file token 28 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 28 is at char 34",
       tokenInfo.character == 34);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 29 is wchar",
       tokenType == TT_WCHAR);
  test(status, "[lexer] [lex] comprehensive file token 29 is at line 5",
       tokenInfo.line == 5);
  test(status, "[lexer] [lex] comprehensive file token 29 is at char 38",
       tokenInfo.character == 38);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 30 is ulong",
       tokenType == TT_ULONG);
  test(status, "[lexer] [lex] comprehensive file token 30 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 30 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 31 is long",
       tokenType == TT_LONG);
  test(status, "[lexer] [lex] comprehensive file token 31 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 31 is at char 7",
       tokenInfo.character == 7);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 32 is float",
       tokenType == TT_FLOAT);
  test(status, "[lexer] [lex] comprehensive file token 32 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 32 is at char 12",
       tokenInfo.character == 12);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 33 is double",
       tokenType == TT_DOUBLE);
  test(status, "[lexer] [lex] comprehensive file token 33 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 33 is at char 18",
       tokenInfo.character == 18);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 34 is bool",
       tokenType == TT_BOOL);
  test(status, "[lexer] [lex] comprehensive file token 34 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 34 is at char 25",
       tokenInfo.character == 25);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 35 is const",
       tokenType == TT_CONST);
  test(status, "[lexer] [lex] comprehensive file token 35 is at line 6",
       tokenInfo.line == 6);
  test(status, "[lexer] [lex] comprehensive file token 35 is at char 30",
       tokenInfo.character == 30);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 36 is semi",
       tokenType == TT_SEMI);
  test(status, "[lexer] [lex] comprehensive file token 36 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 36 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 37 is comma",
       tokenType == TT_COMMA);
  test(status, "[lexer] [lex] comprehensive file token 37 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 37 is at char 2",
       tokenInfo.character == 2);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 38 is lparen",
       tokenType == TT_LPAREN);
  test(status, "[lexer] [lex] comprehensive file token 38 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 38 is at char 3",
       tokenInfo.character == 3);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 39 is rparen",
       tokenType == TT_RPAREN);
  test(status, "[lexer] [lex] comprehensive file token 39 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 39 is at char 4",
       tokenInfo.character == 4);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 40 is lsquare",
       tokenType == TT_LSQUARE);
  test(status, "[lexer] [lex] comprehensive file token 40 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 40 is at char 5",
       tokenInfo.character == 5);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 41 is rsquare",
       tokenType == TT_RSQUARE);
  test(status, "[lexer] [lex] comprehensive file token 41 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 41 is at char 6",
       tokenInfo.character == 6);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 42 is lbrace",
       tokenType == TT_LBRACE);
  test(status, "[lexer] [lex] comprehensive file token 42 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 42 is at char 7",
       tokenInfo.character == 7);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 43 is rbrace",
       tokenType == TT_RBRACE);
  test(status, "[lexer] [lex] comprehensive file token 43 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 43 is at char 8",
       tokenInfo.character == 8);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 44 is dot",
       tokenType == TT_DOT);
  test(status, "[lexer] [lex] comprehensive file token 44 is at line 9",
       tokenInfo.line == 9);
  test(status, "[lexer] [lex] comprehensive file token 44 is at char 9",
       tokenInfo.character == 9);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 45 is arrow",
       tokenType == TT_ARROW);
  test(status, "[lexer] [lex] comprehensive file token 45 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 45 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 46 is plusplus",
       tokenType == TT_PLUSPLUS);
  test(status, "[lexer] [lex] comprehensive file token 46 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 46 is at char 3",
       tokenInfo.character == 3);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 47 is minusminus",
       tokenType == TT_MINUSMINUS);
  test(status, "[lexer] [lex] comprehensive file token 47 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 47 is at char 5",
       tokenInfo.character == 5);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 48 is star",
       tokenType == TT_STAR);
  test(status, "[lexer] [lex] comprehensive file token 48 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 48 is at char 7",
       tokenInfo.character == 7);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 49 is ampersand",
       tokenType == TT_AMPERSAND);
  test(status, "[lexer] [lex] comprehensive file token 49 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 49 is at char 8",
       tokenInfo.character == 8);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 50 is plus",
       tokenType == TT_PLUS);
  test(status, "[lexer] [lex] comprehensive file token 50 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 50 is at char 9",
       tokenInfo.character == 9);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 51 is minus",
       tokenType == TT_MINUS);
  test(status, "[lexer] [lex] comprehensive file token 51 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 51 is at char 10",
       tokenInfo.character == 10);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 52 is bang",
       tokenType == TT_BANG);
  test(status, "[lexer] [lex] comprehensive file token 52 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 52 is at char 11",
       tokenInfo.character == 11);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 53 is tilde",
       tokenType == TT_TILDE);
  test(status, "[lexer] [lex] comprehensive file token 53 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 53 is at char 12",
       tokenInfo.character == 12);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 54 is slash",
       tokenType == TT_SLASH);
  test(status, "[lexer] [lex] comprehensive file token 54 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 54 is at char 13",
       tokenInfo.character == 13);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 55 is percent",
       tokenType == TT_PERCENT);
  test(status, "[lexer] [lex] comprehensive file token 55 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 55 is at char 14",
       tokenInfo.character == 14);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 56 is lshift",
       tokenType == TT_LSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 56 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 56 is at char 15",
       tokenInfo.character == 15);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 57 is lrshift",
       tokenType == TT_LRSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 57 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 57 is at char 17",
       tokenInfo.character == 17);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 58 is arshift",
       tokenType == TT_ARSHIFT);
  test(status, "[lexer] [lex] comprehensive file token 58 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 58 is at char 20",
       tokenInfo.character == 20);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 59 is spaceship",
       tokenType == TT_SPACESHIP);
  test(status, "[lexer] [lex] comprehensive file token 59 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 59 is at char 22",
       tokenInfo.character == 22);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 60 is langle",
       tokenType == TT_LANGLE);
  test(status, "[lexer] [lex] comprehensive file token 60 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 60 is at char 25",
       tokenInfo.character == 25);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 61 is rangle",
       tokenType == TT_RANGLE);
  test(status, "[lexer] [lex] comprehensive file token 61 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 61 is at char 26",
       tokenInfo.character == 26);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 62 is lteq",
       tokenType == TT_LTEQ);
  test(status, "[lexer] [lex] comprehensive file token 62 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 62 is at char 27",
       tokenInfo.character == 27);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 63 is gteq",
       tokenType == TT_GTEQ);
  test(status, "[lexer] [lex] comprehensive file token 63 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 63 is at char 33",
       tokenInfo.character == 33);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 64 is eq",
       tokenType == TT_EQ);
  test(status, "[lexer] [lex] comprehensive file token 64 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 64 is at char 35",
       tokenInfo.character == 35);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 65 is neq",
       tokenType == TT_NEQ);
  test(status, "[lexer] [lex] comprehensive file token 65 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 65 is at char 37",
       tokenInfo.character == 37);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 66 is pipe",
       tokenType == TT_PIPE);
  test(status, "[lexer] [lex] comprehensive file token 66 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 66 is at char 39",
       tokenInfo.character == 39);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 67 is caret",
       tokenType == TT_CARET);
  test(status, "[lexer] [lex] comprehensive file token 67 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 67 is at char 40",
       tokenInfo.character == 40);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 68 is land",
       tokenType == TT_LAND);
  test(status, "[lexer] [lex] comprehensive file token 68 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 68 is at char 41",
       tokenInfo.character == 41);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 69 is lor",
       tokenType == TT_LOR);
  test(status, "[lexer] [lex] comprehensive file token 69 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 69 is at char 43",
       tokenInfo.character == 43);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 70 is question",
       tokenType == TT_QUESTION);
  test(status, "[lexer] [lex] comprehensive file token 70 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 70 is at char 45",
       tokenInfo.character == 45);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 71 is colon",
       tokenType == TT_COLON);
  test(status, "[lexer] [lex] comprehensive file token 71 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 71 is at char 46",
       tokenInfo.character == 46);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 72 is assign",
       tokenType == TT_ASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 72 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 72 is at char 47",
       tokenInfo.character == 47);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 73 is mulassign",
       tokenType == TT_MULASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 73 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 73 is at char 48",
       tokenInfo.character == 48);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 74 is divassign",
       tokenType == TT_DIVASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 74 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 74 is at char 50",
       tokenInfo.character == 50);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 75 is modassign",
       tokenType == TT_MODASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 75 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 75 is at char 52",
       tokenInfo.character == 52);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 76 is addassign",
       tokenType == TT_ADDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 76 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 76 is at char 54",
       tokenInfo.character == 54);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 77 is subassign",
       tokenType == TT_SUBASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 77 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 77 is at char 56",
       tokenInfo.character == 56);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 78 is lshiftassign",
       tokenType == TT_LSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 78 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 78 is at char 58",
       tokenInfo.character == 58);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 79 is lrshiftassign",
       tokenType == TT_LRSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 79 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 79 is at char 61",
       tokenInfo.character == 61);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 80 is arshiftassign",
       tokenType == TT_ARSHIFTASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 80 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 80 is at char 65",
       tokenInfo.character == 65);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 81 is bitandassign",
       tokenType == TT_BITANDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 81 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 81 is at char 68",
       tokenInfo.character == 68);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 82 is bitxorassign",
       tokenType == TT_BITXORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 82 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 82 is at char 70",
       tokenInfo.character == 70);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 83 is bitorassign",
       tokenType == TT_BITORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 83 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 83 is at char 72",
       tokenInfo.character == 72);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 84 is landassign",
       tokenType == TT_LANDASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 84 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 84 is at char 74",
       tokenInfo.character == 74);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 85 is lorassign",
       tokenType == TT_LORASSIGN);
  test(status, "[lexer] [lex] comprehensive file token 85 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 85 is at char 77",
       tokenInfo.character == 77);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 86 is id",
       tokenType == TT_ID);
  test(status, "[lexer] [lex] comprehensive file token 86 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 86 is at char 80",
       tokenInfo.character == 80);
  test(status, "[lexer] [lex] comprehensive file token 86 is 'id'",
       strcmp("id", tokenInfo.data.string) == 0);
  free(tokenInfo.data.string);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 87 is scoped id",
       tokenType == TT_SCOPED_ID);
  test(status, "[lexer] [lex] comprehensive file token 87 is at line 10",
       tokenInfo.line == 10);
  test(status, "[lexer] [lex] comprehensive file token 87 is at char 102",
       tokenInfo.character == 102);
  test(status, "[lexer] [lex] comprehensive file token 87 is 'scoped::id'",
       strcmp("scoped::id", tokenInfo.data.string) == 0);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 88 is literal zero",
       tokenType == TT_LITERALINT_0);
  test(status, "[lexer] [lex] comprehensive file token 88 is at line 11",
       tokenInfo.line == 11);
  test(status, "[lexer] [lex] comprehensive file token 88 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 89 is literal binary",
       tokenType == TT_LITERALINT_B);
  test(status, "[lexer] [lex] comprehensive file token 89 is at line 12",
       tokenInfo.line == 12);
  test(status, "[lexer] [lex] comprehensive file token 89 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 90 is literal octal",
       tokenType == TT_LITERALINT_O);
  test(status, "[lexer] [lex] comprehensive file token 90 is at line 13",
       tokenInfo.line == 13);
  test(status, "[lexer] [lex] comprehensive file token 90 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 91 is literal decimal",
       tokenType == TT_LITERALINT_D);
  test(status, "[lexer] [lex] comprehensive file token 91 is at line 14",
       tokenInfo.line == 14);
  test(status, "[lexer] [lex] comprehensive file token 91 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status,
       "[lexer] [lex] comprehensive file token 92 is literal hexadecimal",
       tokenType == TT_LITERALINT_H);
  test(status, "[lexer] [lex] comprehensive file token 92 is at line 15",
       tokenInfo.line == 15);
  test(status, "[lexer] [lex] comprehensive file token 92 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status,
       "[lexer] [lex] comprehensive file token 93 is literal floating point",
       tokenType == TT_LITERALFLOAT);
  test(status, "[lexer] [lex] comprehensive file token 93 is at line 16",
       tokenInfo.line == 16);
  test(status, "[lexer] [lex] comprehensive file token 93 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 94 is literal string",
       tokenType == TT_LITERALSTRING);
  test(status, "[lexer] [lex] comprehensive file token 94 is at line 17",
       tokenInfo.line == 17);
  test(status, "[lexer] [lex] comprehensive file token 94 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 95 is literal char",
       tokenType == TT_LITERALCHAR);
  test(status, "[lexer] [lex] comprehensive file token 95 is at line 18",
       tokenInfo.line == 18);
  test(status, "[lexer] [lex] comprehensive file token 95 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 96 is literal wstring",
       tokenType == TT_LITERALWSTRING);
  test(status, "[lexer] [lex] comprehensive file token 96 is at line 19",
       tokenInfo.line == 19);
  test(status, "[lexer] [lex] comprehensive file token 96 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 97 is literal wchar",
       tokenType == TT_LITERALWCHAR);
  test(status, "[lexer] [lex] comprehensive file token 97 is at line 20",
       tokenInfo.line == 20);
  test(status, "[lexer] [lex] comprehensive file token 97 is at char 1",
       tokenInfo.character == 1);

  tokenType = lex(report, info, &tokenInfo);
  test(status, "[lexer] [lex] comprehensive file token 98 is eof",
       tokenType == TT_EOF);
  test(status, "[lexer] [lex] comprehensive file token 98 is at line 21",
       tokenInfo.line == 21);
  test(status, "[lexer] [lex] comprehensive file token 98 is at char 1",
       tokenInfo.character == 1);

  lexerInfoDestroy(info);

  info = lexerInfoCreate("testFiles/lexerTestCorner.tc", keywords);

  // TODO: write tests for corner case

  tokenType = lex(report, info, &tokenInfo);

  lexerInfoDestroy(info);

  keywordMapDestroy(keywords);
  reportDestroy(report);
}