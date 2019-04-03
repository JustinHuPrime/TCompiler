%code top {
  #include "ast/ast.h"
  
  #include <stdio.h>

  void yyerror(Node**, const char *);
  int yylex(void);
}

%code requires {
  #include "ast/ast.h"
}

%define parse.error verbose

%locations

%parse-param {Node **ast}

%union {
  int tokenID;
  char* tokenString;
  char invalidChar;
}

// keywords
%token <tokenID>
       KWD_MODULE KWD_USING KWD_STRUCT KWD_TYPEDEF KWD_IF KWD_ELSE KWD_WHILE
       KWD_DO KWD_FOR KWD_SWITCH KWD_CASE KWD_DEFAULT KWD_BREAK KWD_CONTINUE
       KWD_RETURN KWD_ASM KWD_SIZEOF KWD_VOID KWD_UBYTE KWD_BYTE
       KWD_UINT KWD_INT KWD_ULONG KWD_LONG KWD_FLOAT KWD_DOUBLE KWD_BOOL
       KWD_CONST KWD_TRUE KWD_FALSE KWD_CAST

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

// identifier
%token <tokenString>
       T_ID T_SCOPED_ID T_TYPE_ID T_SCOPED_TYPE_ID

// literals
%token <tokenString>
       L_INTLITERAL L_FLOATLITERAL L_STRINGLITERAL L_CHARLITERAL L_WSTRINGLITERAL L_WCHARLITERAL

%token <invalidChar>
       E_INVALIDCHAR

// other precedence (expressions) should go above this
%right O_IF KWD_ELSE

%start module
%%
module: module_name imports body_parts ;
module_name: KWD_MODULE scoped_identifier P_SEMI ;

imports:
       | imports import
       ;
import: KWD_USING scoped_identifier P_SEMI ;

body_parts:
          | body_parts body_part
          ;
body_part: function | declaration ;

declaration: function_declaration
           | variable_declaration
           | struct_declaration
           | typedef_declaration
           ;
function_declaration: type T_ID P_LPAREN function_arg_list P_RPAREN P_SEMI ;
variable_declaration: type T_ID variable_declaration_chain P_SEMI ;
variable_declaration_chain: T_ID
                          | variable_declaration_chain P_COMMA T_ID
                          ;
struct_declaration: KWD_STRUCT T_ID P_LBRACE P_RBRACE struct_declaration_chain P_SEMI ;
struct_declaration_chain:
                        | struct_declaration_chain variable_declaration
                        ;
typedef_declaration: KWD_TYPEDEF type T_ID ;

function: type T_ID P_LPAREN function_arg_list P_RPAREN compound_statement ;

function_arg_list:
                 | function_arg_list_nonempty
                 ;
function_arg_list_nonempty: type opt_id
                          | function_arg_list_nonempty P_COMMA type opt_id
                          ;
opt_id:
      | T_ID
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
         | P_SEMI
         ;

compound_statement: P_LBRACE statements P_RBRACE ;
statements:
          | statements statement
          ;

if_statement: KWD_IF P_LPAREN expression P_RPAREN statement maybe_else ;
maybe_else: %prec O_IF
          | KWD_ELSE statement
          ;

while_statement: KWD_WHILE P_LPAREN expression P_RPAREN statement ;
do_while_statement: KWD_DO statement KWD_WHILE P_LPAREN statement P_RPAREN ;

for_statement: KWD_FOR P_LPAREN for_init expression P_SEMI expression P_RPAREN statement ;
for_init: variable_declaration_statement
        | expression P_SEMI
        ;

switch_statement: KWD_SWITCH P_LPAREN expression P_RPAREN P_LBRACE switch_bodies P_RBRACE ;
switch_bodies:
             | switch_bodies switch_body
             ;
switch_body: switch_case_body
           | switch_default_body
           ;
switch_case_body: KWD_CASE L_INTLITERAL P_COLON compound_statement ;
switch_default_body: KWD_DEFAULT P_COLON compound_statement ;

break_statement: KWD_BREAK P_SEMI ;
continue_statement: KWD_CONTINUE P_SEMI ;

return_statement: KWD_RETURN opt_expression P_SEMI ;
opt_expression:
              | expression
              ;

variable_declaration_statement: type T_ID variable_declaration_statement_chain P_SEMI ;
variable_declaration_statement_chain: T_ID maybe_assign
                                    | variable_declaration_statement_chain P_COMMA T_ID maybe_assign
                                    ;
maybe_assign:
            | P_ASSIGN assignment_expression
            ;

asm_statement: KWD_ASM L_STRINGLITERAL ;

expression: assignment_expression
          | expression P_COMMA assignment_expression
          ;
assignment_expression: ternary_expression
                     | assignment_expression P_ASSIGN ternary_expression
                     | assignment_expression P_MULASSIGN ternary_expression
                     | assignment_expression P_DIVASSIGN ternary_expression
                     | assignment_expression P_MODASSIGN ternary_expression
                     | assignment_expression P_ADDASSIGN ternary_expression
                     | assignment_expression P_SUBASSIGN ternary_expression
                     | assignment_expression P_LSHIFTASSIGN ternary_expression
                     | assignment_expression P_LRSHIFTASSIGN ternary_expression
                     | assignment_expression P_ARSHIFTASSIGN ternary_expression
                     | assignment_expression P_BITANDASSIGN ternary_expression
                     | assignment_expression P_BITXORASSIGN ternary_expression
                     | assignment_expression P_BITORASSIGN ternary_expression
                     | assignment_expression P_LANDASSIGN ternary_expression
                     | assignment_expression P_LORASSIGN ternary_expression
                     ;
ternary_expression: logical_expression
                  | ternary_expression P_QUESTION expression P_COLON logical_expression
                  ;
logical_expression: bitwise_expression
                  | logical_expression P_LAND bitwise_expression
                  | logical_expression P_LOR bitwise_expression
                  ;
bitwise_expression: equality_expression
                  | bitwise_expression P_AMPERSAND equality_expression
                  | bitwise_expression P_PIPE equality_expression
                  | bitwise_expression P_CARET equality_expression
                  ;
equality_expression: comparison_expression
                   | equality_expression P_EQ comparison_expression
                   | equality_expression P_NEQ comparison_expression
                   ;
comparison_expression: spaceship_expression
                     | comparison_expression P_LANGLE spaceship_expression
                     | comparison_expression P_RANGLE spaceship_expression
                     | comparison_expression P_LTEQ spaceship_expression
                     | comparison_expression P_GTEQ spaceship_expression
                     ;
spaceship_expression: shift_expression
                    | spaceship_expression P_SPACESHIP shift_expression
                    ;
shift_expression: addition_expression
                | shift_expression P_LSHIFT addition_expression
                | shift_expression P_LRSHIFT addition_expression
                | shift_expression P_ARSHIFT addition_expression
                ;
addition_expression: multiplication_expression
                   | addition_expression P_PLUS multiplication_expression
                   | addition_expression P_MINUS multiplication_expression
                   ;
multiplication_expression: prefix_expression
                         | multiplication_expression P_STAR prefix_expression
                         | multiplication_expression P_SLASH prefix_expression
                         | multiplication_expression P_PERCENT prefix_expression
                         ;
prefix_expression: postfix_expression
                 | P_STAR prefix_expression
                 | P_AMPERSAND prefix_expression
                 | P_PLUSPLUS prefix_expression
                 | P_MINUSMINUS prefix_expression
                 | P_PLUS prefix_expression
                 | P_MINUS prefix_expression
                 | P_BANG prefix_expression
                 | P_TILDE prefix_expression
                 ;
postfix_expression: primary_expression
                  | postfix_expression P_DOT T_ID
                  | postfix_expression P_ARROW T_ID
                  | postfix_expression P_LPAREN argument_list P_RPAREN
                  | postfix_expression P_LSQUARE expression P_RSQUARE
                  | postfix_expression P_PLUSPLUS
                  | postfix_expression P_MINUSMINUS
                  ;
primary_expression: scoped_identifier
                  | L_INTLITERAL
                  | L_FLOATLITERAL
                  | L_STRINGLITERAL
                  | L_CHARLITERAL
                  | L_WSTRINGLITERAL
                  | L_WCHARLITERAL
                  | KWD_TRUE
                  | KWD_FALSE
                  | KWD_CAST P_LSQUARE type P_RSQUARE P_LPAREN expression P_RPAREN
                  | KWD_SIZEOF P_LPAREN expression P_RPAREN
                  | KWD_SIZEOF P_LPAREN type P_RPAREN
                  | P_LPAREN expression P_RPAREN
                  ;
argument_list:
             | argument_list_nonempty
             ;
argument_list_nonempty: assignment_expression
                      | argument_list_nonempty P_COMMA assignment_expression
                      ;

type: KWD_VOID
    | KWD_UBYTE
    | KWD_BYTE
    | KWD_UINT
    | KWD_INT
    | KWD_ULONG
    | KWD_LONG
    | KWD_FLOAT
    | KWD_DOUBLE
    | KWD_BOOL
    | scoped_type_identifier
    | type KWD_CONST
    | type P_LSQUARE L_INTLITERAL P_RSQUARE
    | type P_STAR
    | P_LPAREN P_STAR type P_RPAREN P_LPAREN function_ptr_arg_list P_RPAREN
    ;
function_ptr_arg_list:
                     | function_ptr_arg_list_nonempty
                     ;
function_ptr_arg_list_nonempty: type
                              | function_ptr_arg_list_nonempty P_COMMA type
                              ;

scoped_identifier: T_SCOPED_ID
                 | T_ID
                 ;
scoped_type_identifier: T_SCOPED_TYPE_ID
                      | T_TYPE_ID
                      ;
%%

void yyerror(Node** ast, const char* msg) {
  fprintf(stderr, "%s\n", msg);
}