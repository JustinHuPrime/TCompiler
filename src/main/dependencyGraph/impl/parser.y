// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

%code top {
#include "dependencyGraph/impl/parser.tab.h"
  
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void dgerror(DGLTYPE *, void *, const char *);
}

%code requires {
}

%code provides {
  #define YY_DECL                             \
    int dglex (DGSTYPE *dglval, DGLTYPE *dglloc, void *scanner)
  YY_DECL;
  #undef YY_DECL
}

%define parse.error verbose
%define api.prefix {dg}
%define api.pure full

%locations

%lex-param { void *scanner }
%parse-param { void *scanner }

%union {
  int tokenID;
  char* tokenString;
  char invalidChar;
}

// keywords
%token <tokenID>
       KWD_MODULE KWD_USING

// punctuation
%token <tokenID>
       P_SEMI

// identifiers
%token <tokenString>
       T_ID

// error case
%token <invalidChar>
       E_INVALIDCHAR

// internal nodes

// precedence goes from least to most tightly bound

%start module
%%
module: module_name imports body
        {  }
      ;
module_name: KWD_MODULE T_ID P_SEMI
             {  }
           ;

imports: {  }
       | imports import
         {  }
       ;
import: KWD_USING T_ID P_SEMI
        {  }
      ;

body: 
%%
void dgerror(YYLTYPE *location, void *scanner, const char* msg) {
  fprintf(stderr, "%s\n", msg);
}