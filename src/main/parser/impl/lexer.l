%option reentrant
%option bison-bridge bison-locations
%option warn never-interactive case-sensitive noyywrap
%option prefix="ast"
%{
/*
Copyright 2019 Justin Hu

This file is part of the T Language Compiler.
*/

#include "parser/impl/parser.tab.h"

#include <string.h>

bool isType(char const *);
%}
%%
 /*
 Copyright 2019 Justin Hu

 This file is part of the T Language Compiler.
 */

 /* whitespace is ignored */
[ \n\r\t]               ;
"//".*                  ;
"/*"([^*]|\*[^\/])*"*/" ;

 /* keywords */
module   { return KWD_MODULE; }
using    { return KWD_USING; }
struct   { return KWD_STRUCT; }
union    { return KWD_UNION; }
enum     { return KWD_ENUM; }
typedef  { return KWD_TYPEDEF; }
if       { return KWD_IF; }
else     { return KWD_ELSE; }
while    { return KWD_WHILE; }
do       { return KWD_DO; }
for      { return KWD_FOR; }
switch   { return KWD_SWITCH; }
case     { return KWD_CASE; }
default  { return KWD_DEFAULT; }
break    { return KWD_BREAK; }
continue { return KWD_CONTINUE; }
return   { return KWD_RETURN; }
asm      { return KWD_ASM; }
true     { return KWD_TRUE; }
false    { return KWD_FALSE; }
cast     { return KWD_CAST; }
sizeof   { return KWD_SIZEOF; }
void     { return KWD_VOID; }
ubyte    { return KWD_UBYTE; }
byte     { return KWD_BYTE; }
uint     { return KWD_UINT; }
int      { return KWD_INT; }
ulong    { return KWD_ULONG; }
long     { return KWD_LONG; }
float    { return KWD_FLOAT; }
double   { return KWD_DOUBLE; }
bool     { return KWD_BOOL; }
const    { return KWD_CONST; }

 /* punctuation */
";"    { return P_SEMI; }
","    { return P_COMMA; }
"("    { return P_LPAREN; }
")"    { return P_RPAREN; }
"["    { return P_LSQUARE; }
"]"    { return P_RSQUARE; }
"{"    { return P_LBRACE; }
"}"    { return P_RBRACE; }
"."    { return P_DOT; }
"->"   { return P_ARROW; }
"++"   { return P_PLUSPLUS; }
"--"   { return P_MINUSMINUS; }
"*"    { return P_STAR; }
"&"    { return P_AMPERSAND; }
"+"    { return P_PLUS; }
"-"    { return P_MINUS; }
"!"    { return P_BANG; }
"~"    { return P_TILDE; }
"/"    { return P_SLASH; }
"%"    { return P_PERCENT; }
"<<"   { return P_LSHIFT; }
">>"   { return P_LRSHIFT; }
">>>"  { return P_ARSHIFT; }
"<=>"  { return P_SPACESHIP; }
"<"    { return P_LANGLE; }
">"    { return P_RANGLE; }
"<="   { return P_LTEQ; }
">="   { return P_GTEQ; }
"=="   { return P_EQ; }
"!="   { return P_NEQ; }
"|"    { return P_PIPE; }
"^"    { return P_CARET; }
"&&"   { return P_LAND; }
"||"   { return P_LOR; }
"?"    { return P_QUESTION; }
":"    { return P_COLON; }
"="    { return P_ASSIGN; }
"*="   { return P_MULASSIGN; }
"/="   { return P_DIVASSIGN; }
"%="   { return P_MODASSIGN; }
"+="   { return P_ADDASSIGN; }
"-="   { return P_SUBASSIGN; }
"<<="  { return P_LSHIFTASSIGN; }
">>="  { return P_LRSHIFTASSIGN; }
">>>=" { return P_ARSHIFTASSIGN; }
"&="   { return P_BITANDASSIGN; }
"^="   { return P_BITXORASSIGN; }
"|="   { return P_BITORASSIGN; }
"&&="  { return P_LANDASSIGN; }
"||="  { return P_LORASSIGN; }

[a-zA-Z_][a-zA-Z0-9_]*                           { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                   return isType(yylval->tokenString) ? T_TYPE_ID : T_ID; } // check the symbol table to determine what type this is
[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*) { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                   return isType(yylval->tokenString) ? T_SCOPED_TYPE_ID : T_SCOPED_ID; }

[+-]?(0x[0-9a-fA-F]+|0b[01]+|0[0-7]+|[1-9][0-9]*|0)                                                                                      { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_INTLITERAL; }
[+-]?[0-9]+\.[0-9]+                                                                                                                      { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_FLOATLITERAL; }
\"(([0-9a-zA-Z !'#$%&\(\)*+,-\./:;>=<?@\[\]^_`\{\|\}~]|"\\\""|"\\n"|"\\r"|"\\t"|"\\0"|"\\"|"\\x"[0-9a-fA-F]{2}))*\"                      { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_STRINGLITERAL; }
'([0-9a-zA-Z !"#$%&\(\)*+,-\./:;>=<?@\[\]^_`\{\|\}~]|"\\'"|"\\n"|"\\r"|"\\t"|"\\0"|"\\"|"\\x"[0-9a-fA-F]{2})'                            { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_CHARLITERAL; }
\"(([0-9a-zA-Z !'#$%&\(\)*+,-\./:;>=<?@\[\]^_`\{\|\}~]|"\\\""|"\\n"|"\\r"|"\\t"|"\\0"|"\\"|"\\x"[0-9a-fA-F]{2}|"\\u"[0-9a-fA-F]{8}))*\"w { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_WSTRINGLITERAL; }
'([0-9a-zA-Z !"#$%&\(\)*+,-\./:;>=<?@\[\]^_`\{\|\}~]|"\\'"|"\\n"|"\\r"|"\\t"|"\\0"|"\\"|"\\x"[0-9a-fA-F]{2}|"\\u"[0-9a-fA-F]{8})'w       { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                                                                                                           return L_WCHARLITERAL; }

. { yylval->invalidChar = yytext[0];
    return E_INVALIDCHAR; }
%%

bool isType(char const *id) { return false; }