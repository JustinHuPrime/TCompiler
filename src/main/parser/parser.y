%{
  #include <stdio.h>

  void yyerror(void*, const char *);
  int yylex(void);
%}

%define parse.error verbose

%locations

%parse-param {void *object}

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
       P_LORASSIGN P_COLONCOLON

// identifier
%token <tokenString>
       T_ID T_TYPE_ID

// literals
%token <tokenString>
       L_STRINGLITERAL L_CHARLITERAL L_INTLITERAL L_FLOATLITERAL L_WSTRINGLITERAL L_WCHARLITERAL

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
            | P_ASSIGN expression
            ;

asm_statement: KWD_ASM L_STRINGLITERAL ;

expression: scoped_identifier ;

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

scoped_identifier: T_ID
                 | scoped_identifier P_COLONCOLON T_ID
                 ;
scoped_type_identifier: T_TYPE_ID
                      | scoped_identifier P_COLONCOLON T_TYPE_ID
                      ;
%%

void yyerror(void* ignored, const char* msg) {
  fprintf(stderr, "%s", msg);
}