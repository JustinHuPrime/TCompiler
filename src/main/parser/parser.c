// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of the parser interface and utilities

#include "parser/parser.h"

#include "ast/ast.h"

int const PARSE_OK = 0;
int const PARSE_EIO = 1;
int const PARSE_EPARSE = 2;
int const PARSE_ESCAN = 3;

int parse(char const *filename, Node **ast) {
  yyscan_t scanner;
  if (yylex_init(&scanner) != 0) return PARSE_ESCAN;

  FILE *inFile = fopen(filename, "r");
  if (inFile == NULL) return PARSE_EIO;
  yyset_in(inFile, scanner);

  *ast = NULL;
  if (yyparse(scanner, ast) == 1) return PARSE_EPARSE;

  if (fclose(inFile) == EOF) return PARSE_EIO;

  yylex_destroy(scanner);
  return PARSE_OK;
}