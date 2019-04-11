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
  yylex_init(&scanner);

  FILE *inFile = fopen(filename, "r");
  if (inFile == NULL) return PARSE_EIO;
  yyset_in(inFile, scanner);

  *ast = NULL;
  if (yyparse(scanner, ast) == 1) return PARSE_EPARSE;

  fclose(inFile);
  yylex_destroy(scanner);
  return PARSE_OK;
}

ModuleNodeTablePair *parseFiles(Report *report, FileList *files) {
  ModuleNodeTablePair *pair = moduleNodeTablePairCreate();

  // while there are unparsed files: {
  // from the list of unparsed files, chose one
  // parse all of its dependencies
  // if you need to parse it, abort - circular dependency error
  // parse it
  // }

  return pair;
}