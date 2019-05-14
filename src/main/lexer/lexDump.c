// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// Implementation of lexDump

#include "lexer/lexDump.h"

#include <stdio.h>
#include <stdlib.h>

#include "lexer/lexer.h"

static void lexDumpOne(Report *report, KeywordMap *keywords,
                       char const *filename) {
  LexerInfo *info = lexerInfoCreate(filename, keywords);
  TokenType type;
  printf("%s:\n", filename);
  do {
    TokenInfo tokenInfo;
    type = lex(report, info, &tokenInfo);
    switch (type) {
      // errors
      case TT_ERR: {
        printf("%s:%zu:%zu: ERR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_EOF: {
        printf("%s:%zu:%zu: EOF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INVALID: {
        printf("%s:%zu:%zu: INVALID_CHAR(%c)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.invalidChar);
        break;
      }
      case TT_EMPTY_SQUOTE: {
        printf("%s:%zu:%zu: EMPTY_SQUOTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INVALID_ESCAPE: {
        printf("%s:%zu:%zu: INVALID_ESCAPE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_NOT_WIDE: {
        printf("%s:%zu:%zu: NOT_WIDE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MULTICHAR_CHAR: {
        printf("%s:%zu:%zu: MULTICHAR_CHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      // keywords
      case TT_MODULE: {
        printf("%s:%zu:%zu: MODULE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_USING: {
        printf("%s:%zu:%zu: USING\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_STRUCT: {
        printf("%s:%zu:%zu: STRUCT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UNION: {
        printf("%s:%zu:%zu: UNION\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ENUM: {
        printf("%s:%zu:%zu: ENUM\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TYPEDEF: {
        printf("%s:%zu:%zu: TYPEDEF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_IF: {
        printf("%s:%zu:%zu: IF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ELSE: {
        printf("%s:%zu:%zu: ELSE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_WHILE: {
        printf("%s:%zu:%zu: WHILE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DO: {
        printf("%s:%zu:%zu: DO\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FOR: {
        printf("%s:%zu:%zu: FOR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SWITCH: {
        printf("%s:%zu:%zu: SWITCH\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CASE: {
        printf("%s:%zu:%zu: CASE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DEFAULT: {
        printf("%s:%zu:%zu: DEFAULT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BREAK: {
        printf("%s:%zu:%zu: BREAK\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CONTINUE: {
        printf("%s:%zu:%zu: CONTINUE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RETURN: {
        printf("%s:%zu:%zu: RETURN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ASM: {
        printf("%s:%zu:%zu: ASM\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TRUE: {
        printf("%s:%zu:%zu: TRUE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FALSE: {
        printf("%s:%zu:%zu: FALSE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CAST: {
        printf("%s:%zu:%zu: CAST\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SIZEOF: {
        printf("%s:%zu:%zu: SIZEOF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_VOID: {
        printf("%s:%zu:%zu: VOID\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UBYTE: {
        printf("%s:%zu:%zu: UBYTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BYTE: {
        printf("%s:%zu:%zu: BYTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CHAR: {
        printf("%s:%zu:%zu: CHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UINT: {
        printf("%s:%zu:%zu: UINT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INT: {
        printf("%s:%zu:%zu: INT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_WCHAR: {
        printf("%s:%zu:%zu: WCHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ULONG: {
        printf("%s:%zu:%zu: ULONG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LONG: {
        printf("%s:%zu:%zu: LONG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FLOAT: {
        printf("%s:%zu:%zu: FLOAT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DOUBLE: {
        printf("%s:%zu:%zu: DOUBLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BOOL: {
        printf("%s:%zu:%zu: BOOL\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CONST: {
        printf("%s:%zu:%zu: CONST\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      // punctuation
      case TT_SEMI: {
        printf("%s:%zu:%zu: SEMI\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_COMMA: {
        printf("%s:%zu:%zu: COMMA\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LPAREN: {
        printf("%s:%zu:%zu: LPAREN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RPAREN: {
        printf("%s:%zu:%zu: RPAREN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSQUARE: {
        printf("%s:%zu:%zu: LSQUARE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RSQUARE: {
        printf("%s:%zu:%zu: RSQUARE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LBRACE: {
        printf("%s:%zu:%zu: LBRACE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RBRACE: {
        printf("%s:%zu:%zu: RBRACE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DOT: {
        printf("%s:%zu:%zu: DOT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARROW: {
        printf("%s:%zu:%zu: ARROW\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PLUSPLUS: {
        printf("%s:%zu:%zu: PLUSPLUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MINUSMINUS: {
        printf("%s:%zu:%zu: MINUSMINUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_STAR: {
        printf("%s:%zu:%zu: STAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_AMPERSAND: {
        printf("%s:%zu:%zu: AMPERSAND\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PLUS: {
        printf("%s:%zu:%zu: PLUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MINUS: {
        printf("%s:%zu:%zu: MINUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BANG: {
        printf("%s:%zu:%zu: BANG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TILDE: {
        printf("%s:%zu:%zu: TILDE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SLASH: {
        printf("%s:%zu:%zu: SLASH\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PERCENT: {
        printf("%s:%zu:%zu: PERCENT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSHIFT: {
        printf("%s:%zu:%zu: LSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LRSHIFT: {
        printf("%s:%zu:%zu: LRSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARSHIFT: {
        printf("%s:%zu:%zu: ARSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SPACESHIP: {
        printf("%s:%zu:%zu: SPACESHIP\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LANGLE: {
        printf("%s:%zu:%zu: LANGLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RANGLE: {
        printf("%s:%zu:%zu: RANGLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LTEQ: {
        printf("%s:%zu:%zu: LTEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_GTEQ: {
        printf("%s:%zu:%zu: GTEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_EQ: {
        printf("%s:%zu:%zu: EQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_NEQ: {
        printf("%s:%zu:%zu: NEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PIPE: {
        printf("%s:%zu:%zu: PIPE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CARET: {
        printf("%s:%zu:%zu: CARET\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LAND: {
        printf("%s:%zu:%zu: LAND\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LOR: {
        printf("%s:%zu:%zu: LOR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_QUESTION: {
        printf("%s:%zu:%zu: QUESTION\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_COLON: {
        printf("%s:%zu:%zu: COLON\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ASSIGN: {
        printf("%s:%zu:%zu: ASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MULASSIGN: {
        printf("%s:%zu:%zu: MULASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DIVASSIGN: {
        printf("%s:%zu:%zu: DIVASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MODASSIGN: {
        printf("%s:%zu:%zu: MODASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ADDASSIGN: {
        printf("%s:%zu:%zu: ADDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SUBASSIGN: {
        printf("%s:%zu:%zu: SUBASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSHIFTASSIGN: {
        printf("%s:%zu:%zu: LSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LRSHIFTASSIGN: {
        printf("%s:%zu:%zu: LRSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARSHIFTASSIGN: {
        printf("%s:%zu:%zu: ARSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITANDASSIGN: {
        printf("%s:%zu:%zu: BITANDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITXORASSIGN: {
        printf("%s:%zu:%zu: BITXORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITORASSIGN: {
        printf("%s:%zu:%zu: BITORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LANDASSIGN: {
        printf("%s:%zu:%zu: LANDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LORASSIGN: {
        printf("%s:%zu:%zu: LORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ID: {
        printf("%s:%zu:%zu: ID(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_SCOPED_ID: {
        printf("%s:%zu:%zu: SCOPED_ID(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_0: {
        printf("%s:%zu:%zu: LITERALINT_0(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_B: {
        printf("%s:%zu:%zu: LITERALINT_B(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_D: {
        printf("%s:%zu:%zu: LITERALINT_D(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_O: {
        printf("%s:%zu:%zu: LITERALINT_O(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_H: {
        printf("%s:%zu:%zu: LITERALINT_H(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALFLOAT: {
        printf("%s:%zu:%zu: LITERALFLOAT(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALSTRING: {
        printf("%s:%zu:%zu: LITERALSTRING(\"%s\")\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALCHAR: {
        printf("%s:%zu:%zu: LITERALCHAR(\'%s\')\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALWSTRING: {
        printf("%s:%zu:%zu: LITERALWSTRING(\"%s\"w)\n", filename,
               tokenInfo.line, tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALWCHAR: {
        printf("%s:%zu:%zu: LITERALWCHAR(\'%s\'w)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
    }
  } while (type != TT_EOF && type != TT_ERR);
}

void lexDump(Report *report, FileList *files) {
  KeywordMap *keywords = keywordMapCreate();
  for (size_t idx = 0; idx < files->decls->size; idx++)
    lexDumpOne(report, keywords, files->decls->elements[idx]);
  for (size_t idx = 0; idx < files->codes->size; idx++)
    lexDumpOne(report, keywords, files->codes->elements[idx]);
  keywordMapDestroy(keywords);
}