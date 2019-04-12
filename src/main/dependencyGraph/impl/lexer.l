%option reentrant
%option bison-bridge bison-locations
%option warn never-interactive case-sensitive noyywrap
%option prefix="dg"
%{
/*
Copyright 2019 Justin Hu

This file is part of the T Language Compiler.
*/

#include "dependencyGraph/impl/parser.tab.h"

#include <string.h>
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

 /* punctuation */
";"    { return P_SEMI; }

[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*) { yylval->tokenString = strcpy(malloc((size_t)yyleng + 1), yytext);
                                                   return T_ID; }

. { yylval->invalidChar = yytext[0];
    return E_INVALIDCHAR; }
%%