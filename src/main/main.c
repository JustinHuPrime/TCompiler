// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#include "ast/ast.h"
#include "ast/printer.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Expected one argument - file to process.\n");
    exit(EXIT_FAILURE);
  }

  Node *ast;

  int parseStatus = parse(argv[1], &ast);
  if (parseStatus < 0) {
    fprintf(stderr, "Parsing error number %d.\n", -parseStatus);
    exit(EXIT_FAILURE);
  }

  printf("Structure:\n");
  printNodeStructure(ast);
  printf("\n");

  printf("Pretty print:\n");
  printNode(ast);

  nodeDestroy(ast);

  exit(EXIT_SUCCESS);
}