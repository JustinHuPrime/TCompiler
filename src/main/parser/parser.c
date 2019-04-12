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
  FILE *inFile = fopen(filename, "r");
  if (inFile == NULL) return PARSE_EIO;

  yyscan_t scanner;
  astlex_init(&scanner);
  astset_in(inFile, scanner);

  *ast = NULL;
  if (astparse(scanner, ast) == 1) {
    astlex_destroy(scanner);
    fclose(inFile);
    return PARSE_EPARSE;
  }

  fclose(inFile);
  astlex_destroy(scanner);
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