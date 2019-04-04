// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#include "parser/parser.h"

#include "ast/ast.h"

int parse(char const *filename, Node **ast) {
  FILE *inFile = fopen(filename, "r");

  if (inFile == NULL) return -1;

  yyin = inFile;
  *ast = NULL;
  if (yyparse(ast) == 1) return -2;

  if (fclose(inFile) == EOF) return -3;

  return 0;
}