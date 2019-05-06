// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for the lexer

#include "lexer/lexer.h"

#include "tests.h"
#include "util/functional.h"
#include "util/hash.h"

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
  Report *report = reportCreate();
  KeywordMap *keywords = keywordMapCreate();

  LexerInfo *info = lexerInfoCreate("testFiles/lexerTestBasic.tc", keywords);

  TokenInfo tokenInfo;
  TokenType tokenType;

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
  keywordMapDestroy(keywords);
  reportDestroy(report);
}