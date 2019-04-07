// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of the parser interface and utilities

#include "parser/parser.h"

#include "ast/ast.h"

int const PARSE_OK = 0;
int const PARSE_EIO = 1;
int const PARSE_EPARSE = 2;

int parse(char const *filename, Node **ast) {
  FILE *inFile = fopen(filename, "r");

  if (inFile == NULL) return PARSE_EIO;

  yyin = inFile;
  *ast = NULL;
  if (yyparse(ast) == 1) return PARSE_EPARSE;

  if (fclose(inFile) == EOF) return PARSE_EIO;

  return PARSE_OK;
}

bool isType(char const *id) { return false; }