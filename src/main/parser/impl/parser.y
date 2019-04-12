// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

%code top {
#include "parser/impl/parser.tab.h"
  
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void asterror(ASTLTYPE *, void *, Node**, const char *);
}

%code requires {
#include "ast/ast.h"
}

%code provides {
  #define YY_DECL\
    int astlex (ASTSTYPE *astlval, ASTLTYPE *astlloc, void *scanner)
  YY_DECL;
  #undef YY_DECL
}

%define parse.error verbose
%define api.prefix {ast}
%define api.pure full

%locations

%lex-param { void *scanner }
%parse-param { void *scanner } { Node **ast }

%union {
  int tokenID;
  char* tokenString;
  char invalidChar;
  Node* ast;
  NodeList *list;
  NodePairList *pairList;
}

// keywords
%token <tokenID>
       KWD_MODULE KWD_USING KWD_STRUCT KWD_UNION KWD_ENUM KWD_TYPEDEF KWD_IF
       KWD_ELSE KWD_WHILE KWD_DO KWD_FOR KWD_SWITCH KWD_CASE KWD_DEFAULT
       KWD_BREAK KWD_CONTINUE KWD_RETURN KWD_ASM KWD_SIZEOF KWD_VOID KWD_UBYTE
       KWD_BYTE KWD_UINT KWD_INT KWD_ULONG KWD_LONG KWD_FLOAT KWD_DOUBLE
       KWD_BOOL KWD_CONST KWD_TRUE KWD_FALSE KWD_CAST

// punctuation
%token <tokenID>
       P_SEMI P_COMMA P_LPAREN P_RPAREN P_LSQUARE P_RSQUARE P_LBRACE P_RBRACE
       P_DOT P_ARROW P_PLUSPLUS P_MINUSMINUS P_STAR P_AMPERSAND P_PLUS P_MINUS
       P_BANG P_TILDE P_SLASH P_PERCENT P_LSHIFT P_LRSHIFT P_ARSHIFT
       P_SPACESHIP P_LANGLE P_RANGLE P_LTEQ P_GTEQ P_EQ P_NEQ P_PIPE P_CARET
       P_LAND P_LOR P_QUESTION P_COLON P_ASSIGN P_MULASSIGN P_DIVASSIGN
       P_MODASSIGN P_ADDASSIGN P_SUBASSIGN P_LSHIFTASSIGN P_LRSHIFTASSIGN
       P_ARSHIFTASSIGN P_BITANDASSIGN P_BITXORASSIGN P_BITORASSIGN P_LANDASSIGN
       P_LORASSIGN

// identifiers
%token <tokenString>
       T_ID T_SCOPED_ID T_TYPE_ID T_SCOPED_TYPE_ID

// literals
%token <tokenString>
       L_INTLITERAL L_FLOATLITERAL L_STRINGLITERAL L_CHARLITERAL L_WSTRINGLITERAL L_WCHARLITERAL

// error case
%token <invalidChar>
       E_INVALIDCHAR

// internal nodes
%type <ast>
      module_name import body_part function declaration function_declaration
      variable_declaration struct_declaration struct_forward_declaration
      union_declaration union_forward_declaration enum_declaration
      enum_forward_declaration typedef_declaration type compound_statement
      statement opt_id maybe_else expression if_statement while_statement
      do_while_statement for_init for_statement switch_body switch_statement
      switch_case_body switch_default_body break_statement continue_statement
      opt_expression return_statement variable_declaration_statement
      opt_assign assignment_expression asm_statement ternary_expression
      logical_expression bitwise_expression equality_expression
      comparison_expression spaceship_expression shift_expression
      addition_expression multiplication_expression prefix_expression
      postfix_expression primary_expression aggregate_initializer literal
%type <list>
      imports body_parts struct_declaration_chain
      union_declaration_chain enum_declaration_chain variable_declaration_chain
      statements switch_bodies argument_list argument_list_nonempty
      function_ptr_arg_list function_ptr_arg_list_nonempty literal_list
      literal_list_nonempty
%type <pairList>
      function_arg_list function_arg_list_nonempty
      variable_declaration_statement_chain
%type <tokenString>
      scoped_type_identifier scoped_identifier

// precedence goes from least to most tightly bound
%right O_IF KWD_ELSE
%right P_ASSIGN P_MULASSIGN P_DIVASSIGN P_MODASSIGN P_ADDASSIGN P_SUBASSIGN
       P_LSHIFTASSIGN P_LRSHIFTASSIGN P_ARSHIFTASSIGN P_BITANDASSIGN
       P_BITXORASSIGN P_BITORASSIGN P_LANDASSIGN P_LORASSIGN

%start module
%%
module: module_name imports body_parts
        { *ast = programNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $module_name, $imports, $body_parts); }
      ;
module_name: KWD_MODULE scoped_identifier P_SEMI
             { $$ = moduleNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@scoped_identifier.first_line, (size_t)@scoped_identifier.first_column, $scoped_identifier)); }
           ;

imports: { $$ = nodeListCreate(); }
       | imports import
         { $$ = $1;
           nodeListInsert($$, $import); }
       ;
import: KWD_USING scoped_identifier P_SEMI
        { $$ = importNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@scoped_identifier.first_line, (size_t)@scoped_identifier.first_column, $scoped_identifier)); }
      ;

body_parts: { $$ = nodeListCreate(); }
          | body_parts body_part
            { $$ = $1;
              nodeListInsert($$, $body_part); }
          ;
body_part: variable_declaration_statement
         | function
         | declaration
         ;

declaration: function_declaration
           | variable_declaration
           | struct_declaration
           | struct_forward_declaration
           | union_declaration
           | union_forward_declaration
           | enum_declaration
           | enum_forward_declaration
           | typedef_declaration
           ;
function_declaration: type T_ID P_LPAREN function_arg_list P_RPAREN P_SEMI
                      { NodeList *list = malloc(sizeof(NodeList));
                        list->size = $function_arg_list->size;
                        list->capacity = $function_arg_list->capacity;
                        list->elements = $function_arg_list->firstElements;
                        for(size_t idx = 0; idx < $function_arg_list->size; idx++) {
                          nodeDestroy($function_arg_list->secondElements[idx]);
                        }
                        free($function_arg_list->secondElements);
                        $$ = funDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), list); }
                    ;
                          ;
variable_declaration: type T_ID variable_declaration_chain P_SEMI
                      { $$ = varDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, $variable_declaration_chain); }
                    ;
variable_declaration_chain: T_ID
                            { $$ = nodeListCreate();
                              nodeListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                          | variable_declaration_chain P_COMMA T_ID
                            { $$ = $1;
                              nodeListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                          ;
struct_declaration: KWD_STRUCT T_ID P_LBRACE struct_declaration_chain P_RBRACE P_SEMI
                    { $$ = structDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $struct_declaration_chain); }
                  ;
struct_declaration_chain: variable_declaration
                          { $$ = nodeListCreate();
                            nodeListInsert($$, $variable_declaration); }
                        | struct_declaration_chain variable_declaration
                          { $$ = $1;
                            nodeListInsert($$, $variable_declaration); }
                        ;
struct_forward_declaration: KWD_STRUCT T_ID P_SEMI
                            { $$ = structForwardDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                          ;
union_declaration: KWD_UNION T_ID P_LBRACE union_declaration_chain P_RBRACE P_SEMI
                    { $$ = unionDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $union_declaration_chain); }
                  ;
union_declaration_chain: variable_declaration
                         { $$ = nodeListCreate();
                            nodeListInsert($$, $variable_declaration); }
                       | union_declaration_chain variable_declaration
                         { $$ = $1;
                            nodeListInsert($$, $variable_declaration); }
                        ;
union_forward_declaration: KWD_UNION T_ID P_SEMI
                            { $$ = unionForwardDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                          ;
enum_declaration: KWD_ENUM T_ID P_LBRACE enum_declaration_chain opt_comma P_RBRACE P_SEMI
                  { $$ = enumDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $enum_declaration_chain); }
                ;
enum_declaration_chain: T_ID
                        { $$ = nodeListCreate();
                          nodeListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                      | enum_declaration_chain P_COMMA T_ID
                        { $$ = $1;
                          nodeListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                      ;
opt_comma:
         | P_COMMA
         ;
enum_forward_declaration: KWD_ENUM T_ID P_SEMI
                            { $$ = enumForwardDeclNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                          ;
typedef_declaration: KWD_TYPEDEF type T_ID 
                     { $$ = typedefNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                   ;

function: type T_ID P_LPAREN function_arg_list P_RPAREN compound_statement
          { $$ = functionNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $function_arg_list, $compound_statement); }
        ;
function_arg_list: { $$ = nodePairListCreate(); }
                 | function_arg_list_nonempty
                 ;
function_arg_list_nonempty: type opt_id
                            { $$ = nodePairListCreate();
                              nodePairListInsert($$, $type, $opt_id); }
                          | function_arg_list_nonempty P_COMMA type opt_id
                            { $$ = $1;
                              nodePairListInsert($$, $type, $opt_id); }
                          ;
opt_id: { $$ = NULL; }
      | T_ID
        { $$ = idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID); }
      ;

statement: compound_statement
         | if_statement
         | while_statement
         | do_while_statement
         | for_statement
         | switch_statement
         | break_statement
         | continue_statement
         | return_statement
         | variable_declaration_statement
         | asm_statement
         | expression P_SEMI
           { $$ = expressionStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression); }
         | P_SEMI
           { $$ = nullStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
         ;

compound_statement: P_LBRACE statements P_RBRACE 
                    { $$ = compoundStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $statements); }
                  ;
statements: { $$ = nodeListCreate(); }
          | statements statement
            { $$ = $1;
              nodeListInsert($$, $statement); }
          ;

if_statement: KWD_IF P_LPAREN expression P_RPAREN statement maybe_else
              { $$ = ifStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression, $statement, $maybe_else); } ;
maybe_else: %prec O_IF { $$ = NULL; }
          | KWD_ELSE statement { $$ = $statement; }
          ;

while_statement: KWD_WHILE P_LPAREN expression P_RPAREN statement 
                 { $$ = whileStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression, $statement); }
               ;
do_while_statement: KWD_DO statement KWD_WHILE P_LPAREN expression P_RPAREN 
                    { $$ = doWhileStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $statement, $expression); }
                  ;

for_statement: KWD_FOR P_LPAREN for_init expression P_SEMI expression P_RPAREN statement 
               { $$ = forStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $for_init, $4, $6, $statement); }
             ;
for_init: variable_declaration_statement
        | expression P_SEMI
          { $$ = expressionStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression); }
        ;

switch_statement: KWD_SWITCH P_LPAREN expression P_RPAREN P_LBRACE switch_bodies P_RBRACE 
                  { $$ = switchStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression, $switch_bodies); }
                ;
switch_bodies: { $$ = nodeListCreate(); }
             | switch_bodies switch_body
               { $$ = $1;
                 nodeListInsert($$, $switch_body); }
             ;
switch_body: switch_case_body
           | switch_default_body
           ;
switch_case_body: KWD_CASE L_INTLITERAL P_COLON compound_statement
                  { $$ = numCaseNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, constExpNodeCreate((size_t)@L_INTLITERAL.first_line, (size_t)@L_INTLITERAL.first_column, TYPEHINT_INT, $L_INTLITERAL), $compound_statement); }
                ;
switch_default_body: KWD_DEFAULT P_COLON compound_statement
                     { $$ = defaultCaseNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $compound_statement); }
                   ;

break_statement: KWD_BREAK P_SEMI
                 { $$ = breakStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
               ;
continue_statement: KWD_CONTINUE P_SEMI
                    { $$ = continueStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
                  ;

return_statement: KWD_RETURN opt_expression P_SEMI
                  { $$ = returnStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $opt_expression); }
                ;
opt_expression: { $$ = NULL; }
              | expression
                { $$ = $expression; }
              ;

variable_declaration_statement: type variable_declaration_statement_chain P_SEMI
                                { $$ = varDeclStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, $variable_declaration_statement_chain); }
                              ;
variable_declaration_statement_chain: T_ID opt_assign
                                      { $$ = nodePairListCreate();
                                        nodePairListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $opt_assign); }
                                    | variable_declaration_statement_chain P_COMMA T_ID opt_assign
                                      { $$ = $1;
                                        nodePairListInsert($$, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID), $opt_assign); }
                                    ;
opt_assign: { $$ = NULL; }
            | P_ASSIGN assignment_expression
              { $$ = $assignment_expression; }
            ;

asm_statement: KWD_ASM L_STRINGLITERAL
               { $$ = asmStmtNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, constExpNodeCreate((size_t)@L_STRINGLITERAL.first_line, (size_t)@L_STRINGLITERAL.first_column, TYPEHINT_STRING, $L_STRINGLITERAL)); }
             ;

expression: assignment_expression
          | expression P_COMMA assignment_expression
            { $$ = seqExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, $assignment_expression); }
          ;
assignment_expression: ternary_expression
                     | assignment_expression P_ASSIGN ternary_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_MULASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_MULASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_DIVASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_DIVASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_MODASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_MODASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_ADDASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ADDASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_SUBASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_SUBASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_LSHIFTASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LSHIFTASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_LRSHIFTASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LRSHIFTASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_ARSHIFTASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ARSHIFTASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_BITANDASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITANDASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_BITXORASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITXORASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_BITORASSIGN assignment_expression
                       { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITORASSIGN, $ternary_expression, $3); }
                     | ternary_expression P_LANDASSIGN assignment_expression
                       { $$ = landAssignExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $ternary_expression, $3); }
                     | ternary_expression P_LORASSIGN assignment_expression
                       { $$ = lorAssignExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $ternary_expression, $3); }
                     ;
ternary_expression: logical_expression
                  | logical_expression P_QUESTION expression P_COLON ternary_expression
                    { $$ = ternaryExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $logical_expression, $expression, $5); }
                  ;
logical_expression: bitwise_expression
                  | logical_expression P_LAND bitwise_expression
                    { $$ = landExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, $bitwise_expression); }
                  | logical_expression P_LOR bitwise_expression
                    { $$ = lorExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, $bitwise_expression); }
                  ;
bitwise_expression: equality_expression
                  | bitwise_expression P_AMPERSAND equality_expression
                    { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITAND, $1, $equality_expression); }
                  | bitwise_expression P_PIPE equality_expression
                    { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITOR, $1, $equality_expression); }
                  | bitwise_expression P_CARET equality_expression
                    { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITOR, $1, $equality_expression); }
                  ;
equality_expression: comparison_expression
                   | equality_expression P_EQ comparison_expression
                     { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_EQ, $1, $comparison_expression); }
                   | equality_expression P_NEQ comparison_expression
                     { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_NEQ, $1, $comparison_expression); }
                   ;
comparison_expression: spaceship_expression
                     | comparison_expression P_LANGLE spaceship_expression
                       { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LT, $1, $spaceship_expression); }
                     | comparison_expression P_RANGLE spaceship_expression
                       { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_GT, $1, $spaceship_expression); }
                     | comparison_expression P_LTEQ spaceship_expression
                       { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LTEQ, $1, $spaceship_expression); }
                     | comparison_expression P_GTEQ spaceship_expression
                       { $$ = compOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_GTEQ, $1, $spaceship_expression); }
                     ;
spaceship_expression: shift_expression
                    | spaceship_expression P_SPACESHIP shift_expression
                      { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_SPACESHIP, $1, $shift_expression); }
                    ;
shift_expression: addition_expression
                | shift_expression P_LSHIFT addition_expression
                  { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LSHIFT, $1, $addition_expression); }
                | shift_expression P_LRSHIFT addition_expression
                  { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LRSHIFT, $1, $addition_expression); }
                | shift_expression P_ARSHIFT addition_expression
                  { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ARSHIFT, $1, $addition_expression); }
                ;
addition_expression: multiplication_expression
                   | addition_expression P_PLUS multiplication_expression
                     { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ADD, $1, $multiplication_expression); }
                   | addition_expression P_MINUS multiplication_expression
                     { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_SUB, $1, $multiplication_expression); }
                   ;
multiplication_expression: prefix_expression
                         | multiplication_expression P_STAR prefix_expression
                           { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_MUL, $1, $prefix_expression); }
                         | multiplication_expression P_SLASH prefix_expression
                           { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_DIV, $1, $prefix_expression); }
                         | multiplication_expression P_PERCENT prefix_expression
                           { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_MOD, $1, $prefix_expression); }
                         ;
prefix_expression: postfix_expression
                 | P_STAR prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_DEREF, $2); }
                 | P_AMPERSAND prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ADDROF, $2); }
                 | P_PLUSPLUS prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_PREINC, $2); }
                 | P_MINUSMINUS prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_PREDEC, $2); }
                 | P_PLUS prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_UPLUS, $2); }
                 | P_MINUS prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_NEG, $2); }
                 | P_BANG prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_LNOT, $2); }
                 | P_TILDE prefix_expression
                   { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_BITNOT, $2); }
                 ;
postfix_expression: primary_expression
                  | postfix_expression P_DOT T_ID
                    { $$ = structAccessExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                  | postfix_expression P_ARROW T_ID
                    { $$ = structPtrAccessExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, idNodeCreate((size_t)@T_ID.first_line, (size_t)@T_ID.first_column, $T_ID)); }
                  | postfix_expression P_LPAREN argument_list P_RPAREN
                    { $$ = fnCallExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, $argument_list); }
                  | postfix_expression P_LSQUARE expression P_RSQUARE
                    { $$ = binOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_ARRAYACCESS, $1, $expression); }
                  | postfix_expression P_PLUSPLUS
                    { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_POSTINC, $1); }
                  | postfix_expression P_MINUSMINUS
                    { $$ = unOpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, OP_POSTDEC, $1); }
                  ;
primary_expression: scoped_identifier
                    { $$ = idExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $scoped_identifier); }
                  | L_INTLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_INT, $L_INTLITERAL); }
                  | L_FLOATLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_FLOAT, $L_FLOATLITERAL); }
                  | L_STRINGLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_STRING, $L_STRINGLITERAL); }
                  | L_CHARLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_CHAR, $L_CHARLITERAL); }
                  | L_WSTRINGLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_WSTRING, $L_WSTRINGLITERAL); }
                  | L_WCHARLITERAL
                    { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_WCHAR, $L_WCHARLITERAL); }
                  | KWD_TRUE
                    { $$ = constTrueNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
                  | KWD_FALSE
                    { $$ = constFalseNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
                  | aggregate_initializer
                  | KWD_CAST P_LSQUARE type P_RSQUARE P_LPAREN expression P_RPAREN
                    { $$ = castExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type, $expression); }
                  | KWD_SIZEOF P_LPAREN expression P_RPAREN
                    { $$ = sizeofExpExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $expression); }
                  | KWD_SIZEOF P_LPAREN type P_RPAREN
                    { $$ = sizeofTypeExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $type); }
                  | P_LPAREN expression P_RPAREN
                    { $$ = $expression; } 
                  ;
argument_list: { $$ = nodeListCreate(); }
             | argument_list_nonempty
             ;
argument_list_nonempty: assignment_expression
                        { $$ = nodeListCreate();
                          nodeListInsert($$, $assignment_expression); }
                      | argument_list_nonempty P_COMMA assignment_expression
                        { $$ = $1;
                          nodeListInsert($$, $assignment_expression); }
                      ;
aggregate_initializer: P_LANGLE literal_list P_RANGLE
                       { $$ = aggregateInitExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $literal_list); }
                     ;
literal_list: { $$ = nodeListCreate(); }
            | literal_list_nonempty
            ;
literal_list_nonempty: literal
                       { $$ = nodeListCreate();
                         nodeListInsert($$, $literal); }
                     | literal_list_nonempty P_COMMA literal
                       { $$ = $1;
                         nodeListInsert($$, $literal); }
                     ;
literal: L_INTLITERAL
         { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_INT, $L_INTLITERAL); }
       | L_FLOATLITERAL
         { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_FLOAT, $L_FLOATLITERAL); }
       | L_STRINGLITERAL
         { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_STRING, $L_STRINGLITERAL); }
       | L_CHARLITERAL
           { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_CHAR, $L_CHARLITERAL); }
       | L_WSTRINGLITERAL
         { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_WSTRING, $L_WSTRINGLITERAL); }
       | L_WCHARLITERAL
         { $$ = constExpNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEHINT_WCHAR, $L_WCHARLITERAL); }
       | KWD_TRUE
         { $$ = constTrueNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
       | KWD_FALSE
         { $$ = constFalseNodeCreate((size_t)@$.first_line, (size_t)@$.first_column); }
       | aggregate_initializer
       ;

type: KWD_VOID
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_VOID); }
    | KWD_UBYTE
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_UBYTE); }
    | KWD_BYTE
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_BYTE); }
    | KWD_UINT
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_UINT); }
    | KWD_INT
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_INT); }
    | KWD_ULONG
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_ULONG); }
    | KWD_LONG
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_LONG); }
    | KWD_FLOAT
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_FLOAT); }
    | KWD_DOUBLE
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_DOUBLE); }
    | KWD_BOOL
      { $$ = keywordTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, TYPEKWD_BOOL); }
    | scoped_type_identifier
      { $$ = idTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $scoped_type_identifier); }
    | type KWD_CONST
      { $$ = constTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1); }
    | type P_LSQUARE L_INTLITERAL P_RSQUARE
      { $$ = arrayTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, constExpNodeCreate((size_t)@L_INTLITERAL.first_line, (size_t)@L_INTLITERAL.first_column, TYPEHINT_INT, $L_INTLITERAL)); }
    | type P_STAR
      { $$ = ptrTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1); }
    | type P_LPAREN function_ptr_arg_list P_RPAREN
      { $$ = fnPtrTypeNodeCreate((size_t)@$.first_line, (size_t)@$.first_column, $1, $function_ptr_arg_list); }
    ;
function_ptr_arg_list: { $$ = nodeListCreate(); }
                     | function_ptr_arg_list_nonempty
                     ;
function_ptr_arg_list_nonempty: type
                                { $$ = nodeListCreate();
                                  nodeListInsert($$, $type); }
                              | function_ptr_arg_list_nonempty P_COMMA type
                                { $$ = $1;
                                  nodeListInsert($$, $type); }
                              ;

scoped_identifier: T_SCOPED_ID
                 | T_ID
                 ;
scoped_type_identifier: T_SCOPED_TYPE_ID
                      | T_TYPE_ID
                      ;
%%
void asterror(YYLTYPE *location, void *scanner, Node** ast, const char* msg) {
  fprintf(stderr, "%s\n", msg);
}