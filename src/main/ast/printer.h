// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Printers for ASTs, to verify correctness

#ifndef TLC_AST_PRINTER_H_
#define TLC_AST_PRINTER_H_

#include "ast/ast.h"

// prints the structure of a node, in function format
void printNodeStructure(Node const *);

// prints the original code, modulo whitespace and ignored syntactic elements
void printNode(Node const *);

#endif  // TLC_AST_PRINTER_H_