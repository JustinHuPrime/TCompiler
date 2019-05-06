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

  lexerInfoDestroy(info);

  keywordMapDestroy(keywords);
  reportDestroy(report);
}