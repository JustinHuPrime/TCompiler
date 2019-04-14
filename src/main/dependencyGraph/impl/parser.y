// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

%code top {
#include "dependencyGraph/impl/parser.tab.h"
  
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void dgerror(DGLTYPE *, void *, ModuleInfo *, const char *);
}

%code requires {
#include "dependencyGraph/grapher.h"
}

%code provides {
  #define YY_DECL\
    int dglex (DGSTYPE *dglval, DGLTYPE *dglloc, void *scanner)
  YY_DECL;
  #undef YY_DECL
}

%define parse.error verbose
%define api.prefix {dg}
%define api.pure full

%locations

%lex-param { void *scanner }
%parse-param { void *scanner } { ModuleInfo *info }

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
%type <tokenString>
      import

// precedence goes from least to most tightly bound

%start module
%%
module: module_name imports body ;
module_name: KWD_MODULE T_ID P_SEMI
             { info->moduleName = $T_ID;
               info->moduleLine = (size_t)@T_ID.first_line;
               info->moduleColumn = (size_t)@T_ID.first_column; }
           ;

imports:
       | imports import
         { moduleInfoAddDependency(info, $import, (size_t)@import.first_line, (size_t)@import.first_column); }
       ;
import: KWD_USING T_ID P_SEMI
        { $$ = $T_ID; }
      ;

body: { YYACCEPT; } ;

%%
void dgerror(YYLTYPE *location, void *scanner, ModuleInfo *info, const char* msg) {
  fprintf(stderr, "%s\n", msg);
}