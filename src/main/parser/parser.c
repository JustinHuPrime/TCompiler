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

// Implementation of the parser

#include "parser/parser.h"

#include "lexer/lexer.h"
#include "symbolTable/typeTable.h"
#include "util/functional.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// output data structures

ModuleAstMap *moduleAstMapCreate(void) { return hashMapCreate(); }
void moduleAstMapInit(ModuleAstMap *map) { hashMapInit(map); }
Node *moduleAstMapGet(ModuleAstMap const *map, char const *key) {
  return hashMapGet(map, key);
}
int moduleAstMapPut(ModuleAstMap *map, char const *key, Node *value) {
  return hashMapPut(map, key, value, (void (*)(void *))nodeDestroy);
}
void moduleAstMapUninit(ModuleAstMap *map) {
  hashMapUninit(map, (void (*)(void *))nodeDestroy);
}
void moduleAstMapDestroy(ModuleAstMap *map) {
  hashMapDestroy(map, (void (*)(void *))nodeDestroy);
}

ModuleAstMapPair *moduleAstMapPairCreate(void) {
  ModuleAstMapPair *pair = malloc(sizeof(ModuleAstMapPair));
  moduleAstMapPairInit(pair);
  return pair;
}
void moduleAstMapPairInit(ModuleAstMapPair *pair) {
  moduleAstMapInit(&pair->decls);
  moduleAstMapInit(&pair->codes);
}
void moduleAstMapPairUninit(ModuleAstMapPair *pair) {
  moduleAstMapUninit(&pair->decls);
  moduleAstMapUninit(&pair->codes);
}
void moduleAstMapPairDestroy(ModuleAstMapPair *pair) {
  moduleAstMapPairInit(pair);
  free(pair);
}

// internal data structures

typedef HashMap ModuleLexerInfoMap;
static void moduleLexerInfoMapInit(ModuleLexerInfoMap *map) {
  hashMapInit(map);
}
static LexerInfo *moduleLexerInfoMapGet(ModuleLexerInfoMap const *map,
                                        char const *key) {
  return hashMapGet(map, key);
}
static int moduleLexerInfoMapPut(ModuleLexerInfoMap *map, char const *key,
                                 LexerInfo *value) {
  return hashMapPut(map, key, value, (void (*)(void *))lexerInfoDestroy);
}
static void moduleLexerInfoMapUninit(ModuleLexerInfoMap *map) {
  hashMapUninit(map, (void (*)(void *))lexerInfoDestroy);
}

typedef HashMap ModuleNodeMap;
static void moduleNodeMapInit(ModuleNodeMap *map) { hashMapInit(map); }
static Node *moduleNodeMapGet(ModuleNodeMap const *map, char const *key) {
  return hashMapGet(map, key);
}
static int moduleNodeMapPut(ModuleNodeMap *map, char const *key, Node *value) {
  return hashMapPut(map, key, value, (void (*)(void *))nodeDestroy);
}
static void moduleNodeMapUninit(ModuleNodeMap *map) {
  hashMapUninit(map, nullDtor);
}

// parsing helpers

// basic helpers
static Node *parseAnyId(Report *report, LexerInfo *info) {
  TokenInfo id;
  lex(info, report, &id);
  if (id.type != TT_ID && id.type != TT_SCOPED_ID) {
    reportError(
        report, "%s:%zu:%zu: error: expected an identifier, but found %s",
        info->filename, id.line, id.character, tokenTypeToString(id.type));
    tokenInfoUninit(&id);
    return NULL;
  }

  return idNodeCreate(id.line, id.character, id.data.string);
}
static Node *parseUnscopedId(Report *report, LexerInfo *info) {
  TokenInfo id;
  lex(info, report, &id);
  if (id.type != TT_ID) {
    reportError(
        report,
        "%s:%zu:%zu: error: expected an unqualified identifier, but found %s",
        info->filename, id.line, id.character, tokenTypeToString(id.type));
    tokenInfoUninit(&id);
    return NULL;
  }

  return idNodeCreate(id.line, id.character, id.data.string);
}
static NodeList *parseUnscopedIdList(Report *report, Options *options,
                                     TypeEnvironment *env, LexerInfo *info) {
  NodeList *ids = nodeListCreate();
  TokenInfo next;

  lex(info, report, &next);
  while (next.type == TT_ID) {
    nodeListInsert(ids,
                   idNodeCreate(next.line, next.character, next.data.string));

    lex(info, report, &next);
    if (next.type != TT_COMMA) {
      break;
    }

    lex(info, report, &next);
  }
  unLex(info, &next);

  return ids;
}
static Node *parseType(Report *, Options *, TypeEnvironment *, LexerInfo *);
static NodeList *parseTypeList(Report *report, Options *options,
                               TypeEnvironment *env, LexerInfo *info) {
  NodeList *types = nodeListCreate();
  TokenInfo peek;

  lex(info, report, &peek);
  while (tokenInfoIsTypeKeyword(&peek) || peek.type == TT_SCOPED_ID ||
         peek.type == TT_ID) {
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      TernaryValue isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == INDETERMINATE) {
        unLex(info, &peek);
        nodeListDestroy(types);
        return NULL;
      } else if (isType == NO) {
        break;
      }
    }
    unLex(info, &peek);

    // is the start of a type!
    Node *type = parseType(report, options, env, info);
    if (type == NULL) {
      nodeListDestroy(types);
      return NULL;
    }
    nodeListInsert(types, type);

    lex(info, report, &peek);
    if (peek.type != TT_COMMA) {
      break;
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return types;
}
static Node *parseIntLiteral(Report *, Options *, TypeEnvironment *,
                             LexerInfo *);
static Node *parseTypeExtensions(Report *report, Options *options,
                                 TypeEnvironment *env, Node *base,
                                 LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_CONST: {
      return parseTypeExtensions(
          report, options, env,
          constTypeNodeCreate(base->line, base->character, base), info);
    }
    case TT_LSQUARE: {
      Node *size = parseIntLiteral(report, options, env, info);

      if (size == NULL) {
        return NULL;
      }

      TokenInfo closeSquare;
      lex(info, report, &closeSquare);
      if (closeSquare.type != TT_RSQUARE) {
        if (!tokenInfoIsLexerError(&closeSquare)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a close square brace to end "
                      "the array type, but found %s",
                      info->filename, closeSquare.line, closeSquare.character,
                      tokenTypeToString(closeSquare.type));
        }
        nodeDestroy(size);
        tokenInfoUninit(&closeSquare);
      }

      return parseTypeExtensions(
          report, options, env,
          arrayTypeNodeCreate(base->line, base->character, base, size), info);
    }
    case TT_STAR: {
      return parseTypeExtensions(
          report, options, env,
          ptrTypeNodeCreate(base->line, base->character, base), info);
    }
    case TT_LPAREN: {
      NodeList *argTypes = parseTypeList(report, options, env, info);

      TokenInfo closeParen;
      lex(info, report, &closeParen);
      if (closeParen.type != TT_RPAREN) {
        if (!tokenInfoIsLexerError(&closeParen)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a close paren to end the "
                      "function pointer type, but found %s",
                      info->filename, closeParen.line, closeParen.character,
                      tokenTypeToString(closeParen.type));
        }
        tokenInfoUninit(&closeParen);
        nodeListDestroy(argTypes);
        return NULL;
      }
      return parseTypeExtensions(
          report, options, env,
          fnPtrTypeNodeCreate(base->line, base->character, base, argTypes),
          info);
    }
    default: {
      unLex(info, &peek);
      return base;
    }
  }
}
static Node *parseType(Report *report, Options *options, TypeEnvironment *env,
                       LexerInfo *info) {
  TokenInfo base;
  lex(info, report, &base);

  if (tokenInfoIsTypeKeyword(&base)) {
    return parseTypeExtensions(
        report, options, env,
        typeKeywordNodeCreate(base.line, base.character,
                              tokenTypeToTypeKeyword(base.type)),
        info);
  } else if (base.type == TT_ID ||
             base.type == TT_SCOPED_ID) {  // must be a type ID to get here
    return parseTypeExtensions(
        report, options, env,
        idTypeNodeCreate(base.line, base.character, base.data.string), info);
  } else {
    if (!tokenInfoIsLexerError(&base)) {
      reportError(report, "%s:%zu:%zu: error: expected a type, but found '%s'",
                  info->filename, base.line, base.character,
                  tokenTypeToString(base.type));
      tokenInfoUninit(&base);
    }
    return NULL;
  }
}

// module
static Node *parseModule(Report *report, LexerInfo *info) {
  TokenInfo moduleToken;
  lex(info, report, &moduleToken);
  if (moduleToken.type != TT_MODULE) {
    if (!tokenInfoIsLexerError(&moduleToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s",
                  info->filename, moduleToken.line, moduleToken.character,
                  tokenTypeToString(moduleToken.type));
    }
    tokenInfoUninit(&moduleToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(info, report, &semiToken);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate the "
                  "module declaration, but found %s",
                  info->filename, semiToken.line, semiToken.character,
                  tokenTypeToString(semiToken.type));
    }
    nodeDestroy(idNode);
    tokenInfoUninit(&semiToken);
    return NULL;
  }

  return moduleNodeCreate(moduleToken.line, moduleToken.character, idNode);
}

// imports
static Node *parseDeclFile(Report *report, Options *options,
                           ModuleTypeTableMap *typeTable,
                           Stack *dependencyStack, ModuleLexerInfoMap *miMap,
                           ModuleNodeMap *mnMap, ModuleAstMap *decls,
                           LexerInfo *lexerInfo, Node *module);
static Node *parseDeclImport(Report *report, Options *options,
                             ModuleTypeTableMap *typeTable,
                             TypeEnvironment *env, Stack *dependencyStack,
                             ModuleLexerInfoMap *miMap, ModuleNodeMap *mnMap,
                             ModuleAstMap *decls, LexerInfo *info) {
  TokenInfo importToken;
  lex(info, report, &importToken);
  if (importToken.type != TT_IMPORT) {
    if (!tokenInfoIsLexerError(&importToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s",
                  info->filename, importToken.line, importToken.character,
                  tokenTypeToString(importToken.type));
    }
    tokenInfoUninit(&importToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(info, report, &semiToken);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate "
                  "the module declaration, but found %s",
                  info->filename, semiToken.line, semiToken.character,
                  tokenTypeToString(semiToken.type));
    }
    tokenInfoUninit(&semiToken);
    nodeDestroy(idNode);
    return NULL;
  }

  // find and add the stab in typeTables to env
  TypeTable *importedTypeTable =
      moduleTypeTableMapGet(typeTable, idNode->data.id.id);
  if (importedTypeTable == NULL) {
    for (size_t idx = 0; idx < dependencyStack->size; idx++) {
      if (strcmp(idNode->data.id.id, dependencyStack->elements[idx]) == 0) {
        // circular dep between current and all later elements in the stack
        reportError(report,
                    "%s:%zu:%zu: error: circular dependency on module '%s'",
                    info->filename, importToken.line, importToken.character,
                    (char const *)dependencyStack->elements[idx]);
        reportMessage(report,
                      "\tmodule '%s' imports module '%s', which imports",
                      env->currentModuleName,
                      (char const *)dependencyStack->elements[idx]);
        for (size_t rptIdx = idx + 1; rptIdx < dependencyStack->size;
             rptIdx++) {
          reportMessage(report, "\tmodule '%s', which imports",
                        (char const *)dependencyStack->elements[rptIdx]);
        }
        reportMessage(report, "\tthe current module");
        nodeDestroy(idNode);
        return NULL;
      }
    }
    Node *parsed =
        parseDeclFile(report, options, typeTable, dependencyStack, miMap, mnMap,
                      decls, moduleLexerInfoMapGet(miMap, idNode->data.id.id),
                      moduleNodeMapGet(mnMap, idNode->data.id.id));
    if (parsed != NULL) {
      moduleAstMapPut(decls, idNode->data.id.id, parsed);
      importedTypeTable = moduleTypeTableMapGet(typeTable, idNode->data.id.id);
    } else {
      nodeDestroy(idNode);
      return NULL;
    }
  }
  int retVal = moduleTypeTableMapPut(&env->imports, idNode->data.id.id,
                                     importedTypeTable);
  if (retVal == HM_EEXISTS) {
    switch (optionsGet(options, optionWDuplicateImport)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: module '%s' already imported",
                    info->filename, importToken.line, importToken.character,
                    idNode->data.id.id);
        nodeDestroy(idNode);
        return NULL;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: module '%s' already imported",
                      info->filename, importToken.line, importToken.character,
                      idNode->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  }

  return moduleNodeCreate(importToken.line, importToken.character, idNode);
}
static Node *parseCodeImport(Report *report, Options *options,
                             ModuleTypeTableMap *typeTables,
                             TypeEnvironment *env, LexerInfo *info) {
  TokenInfo importToken;
  lex(info, report, &importToken);
  if (importToken.type != TT_IMPORT) {
    if (!tokenInfoIsLexerError(&importToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s",
                  info->filename, importToken.line, importToken.character,
                  tokenTypeToString(importToken.type));
    }
    tokenInfoUninit(&importToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(info, report, &semiToken);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate "
                  "the module declaration, but found %s",
                  info->filename, semiToken.line, semiToken.character,
                  tokenTypeToString(semiToken.type));
    }
    tokenInfoUninit(&semiToken);
    nodeDestroy(idNode);
    return NULL;
  }

  // find and add the stab in typeTables to env
  int retVal = moduleTypeTableMapPut(
      &env->imports, idNode->data.id.id,
      moduleTypeTableMapGet(typeTables, idNode->data.id.id));
  if (retVal == HM_EEXISTS) {
    switch (optionsGet(options, optionWDuplicateImport)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: module '%s' already imported",
                    info->filename, importToken.line, importToken.character,
                    idNode->data.id.id);
        nodeDestroy(idNode);
        return NULL;
      }
      case O_WT_WARN: {
        reportError(report, "%s:%zu:%zu: warning: module '%s' already imported",
                    info->filename, importToken.line, importToken.character,
                    idNode->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  }

  return moduleNodeCreate(importToken.line, importToken.character, idNode);
}
static NodeList *parseDeclImports(Report *report, Options *options,
                                  ModuleTypeTableMap *typeTables,
                                  TypeEnvironment *env, Stack *dependencyStack,
                                  ModuleLexerInfoMap *miMap,
                                  ModuleNodeMap *mnMap, ModuleAstMap *decls,
                                  LexerInfo *info) {
  NodeList *imports = nodeListCreate();

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_IMPORT) {
    unLex(info, &peek);
    Node *node = parseDeclImport(report, options, typeTables, env,
                                 dependencyStack, miMap, mnMap, decls, info);
    if (node == NULL) {
      nodeListDestroy(imports);
      return NULL;
    }

    nodeListInsert(imports, node);

    lex(info, report, &peek);
  }

  unLex(info, &peek);
  return imports;
}
static NodeList *parseCodeImports(Report *report, Options *options,
                                  ModuleTypeTableMap *typeTables,
                                  TypeEnvironment *env, LexerInfo *info) {
  NodeList *imports = nodeListCreate();

  TokenInfo peek;

  lex(info, report, &peek);
  while (peek.type == TT_IMPORT) {
    unLex(info, &peek);
    Node *node = parseCodeImport(report, options, typeTables, env, info);
    if (node == NULL) {
      nodeListDestroy(imports);
      return NULL;
    }

    nodeListInsert(imports, node);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return imports;
}

// expression
static Node *parseStringLiteral(Report *report, Options *options,
                                TypeEnvironment *env, LexerInfo *info) {
  TokenInfo string;
  lex(info, report, &string);

  if (string.type != TT_LITERALSTRING) {
    if (!tokenInfoIsLexerError(&string)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a string literal, but found %s",
                  info->filename, string.line, string.character,
                  tokenTypeToString(string.type));
    }
    tokenInfoUninit(&string);
    return NULL;
  }

  return constStringExpNodeCreate(string.line, string.character,
                                  string.data.string);
}
static Node *parseIntOrEnumLiteral(Report *report, Options *options,
                                   TypeEnvironment *env, LexerInfo *info) {
  TokenInfo constant;
  lex(info, report, &constant);

  if (!tokenInfoIsIntConst(&constant) || constant.type == TT_SCOPED_ID) {
    if (!tokenInfoIsLexerError(&constant)) {
      reportError(
          report,
          "%s:%zu:%zu: error: expected an integer constant, but found %s",
          info->filename, constant.line, constant.character,
          tokenTypeToString(constant.type));
    }
    tokenInfoUninit(&constant);
    return NULL;
  }

  switch (constant.type) {
    case TT_LITERALINT_0: {
      return constZeroIntExpNodeCreate(constant.line, constant.character,
                                       constant.data.string);
    }
    case TT_LITERALINT_B: {
      return constBinaryIntExpNodeCreate(constant.line, constant.character,
                                         constant.data.string);
    }
    case TT_LITERALINT_O: {
      return constOctalIntExpNodeCreate(constant.line, constant.character,
                                        constant.data.string);
    }
    case TT_LITERALINT_D: {
      return constDecimalIntExpNodeCreate(constant.line, constant.character,
                                          constant.data.string);
    }
    case TT_LITERALINT_H: {
      return constHexadecimalIntExpNodeCreate(constant.line, constant.character,
                                              constant.data.string);
    }
    case TT_SCOPED_ID: {
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &constant, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_TYPE:
        case ST_ID: {
          reportError(
              report,
              "%s:%zu:%zu: error: expected an integer constant, but found %s",
              info->filename, constant.line, constant.character,
              tokenTypeToString(constant.type));
          return NULL;
        }
        case ST_ENUMCONST: {
          return enumConstExpNodeCreate(constant.line, constant.character,
                                        constant.data.string);
        }
        default: {
          assert(false);  // error: not a valid enum
        }
      }
    }
    default: {
      assert(false);  // error: tokenInfoIsIntConst is broken.
    }
  }
}
static Node *parseIntLiteral(Report *report, Options *options,
                             TypeEnvironment *env, LexerInfo *info) {
  TokenInfo constant;
  lex(info, report, &constant);

  if (!tokenInfoIsIntConst(&constant)) {
    if (!tokenInfoIsLexerError(&constant)) {
      reportError(
          report,
          "%s:%zu:%zu: error: expected an integer constant, but found %s",
          info->filename, constant.line, constant.character,
          tokenTypeToString(constant.type));
    }
    tokenInfoUninit(&constant);
    return NULL;
  }

  switch (constant.type) {
    case TT_LITERALINT_0: {
      return constZeroIntExpNodeCreate(constant.line, constant.character,
                                       constant.data.string);
    }
    case TT_LITERALINT_B: {
      return constBinaryIntExpNodeCreate(constant.line, constant.character,
                                         constant.data.string);
    }
    case TT_LITERALINT_O: {
      return constOctalIntExpNodeCreate(constant.line, constant.character,
                                        constant.data.string);
    }
    case TT_LITERALINT_D: {
      return constDecimalIntExpNodeCreate(constant.line, constant.character,
                                          constant.data.string);
    }
    case TT_LITERALINT_H: {
      return constHexadecimalIntExpNodeCreate(constant.line, constant.character,
                                              constant.data.string);
    }
    default: {
      assert(false);  // error: tokenInfoIsIntConst is broken.
    }
  }
}
static Node *parseLiteral(Report *, Options *, TypeEnvironment *, LexerInfo *);
static NodeList *parseLiteralList(Report *report, Options *options,
                                  TypeEnvironment *env, LexerInfo *info) {
  NodeList *literals = nodeListCreate();

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type != TT_RSQUARE) {
    unLex(info, &peek);

    Node *literal = parseLiteral(report, options, env, info);
    if (literal == NULL) {
      nodeListDestroy(literals);
      return NULL;
    }
    nodeListInsert(literals, literal);

    TokenInfo comma;
    lex(info, report, &comma);
    if (comma.type != TT_COMMA) {
      break;
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return literals;
}
static Node *parseLiteral(Report *report, Options *options,
                          TypeEnvironment *env, LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_LITERALINT_0: {
      return constZeroIntExpNodeCreate(peek.line, peek.character,
                                       peek.data.string);
    }
    case TT_LITERALINT_B: {
      return constBinaryIntExpNodeCreate(peek.line, peek.character,
                                         peek.data.string);
    }
    case TT_LITERALINT_O: {
      return constOctalIntExpNodeCreate(peek.line, peek.character,
                                        peek.data.string);
    }
    case TT_LITERALINT_D: {
      return constDecimalIntExpNodeCreate(peek.line, peek.character,
                                          peek.data.string);
    }
    case TT_LITERALINT_H: {
      return constHexadecimalIntExpNodeCreate(peek.line, peek.character,
                                              peek.data.string);
    }
    case TT_LITERALFLOAT: {
      return constFloatExpNodeCreate(peek.line, peek.character,
                                     peek.data.string);
    }
    case TT_LITERALSTRING: {
      return constStringExpNodeCreate(peek.line, peek.character,
                                      peek.data.string);
    }
    case TT_LITERALCHAR: {
      return constCharExpNodeCreate(peek.line, peek.character,
                                    peek.data.string);
    }
    case TT_LITERALWSTRING: {
      return constWStringExpNodeCreate(peek.line, peek.character,
                                       peek.data.string);
    }
    case TT_LITERALWCHAR: {
      return constWCharExpNodeCreate(peek.line, peek.character,
                                     peek.data.string);
    }
    case TT_FALSE:
    case TT_TRUE: {
      return (peek.type == TT_TRUE
                  ? constTrueNodeCreate
                  : constFalseNodeCreate)(peek.line, peek.character);
    }
    case TT_SCOPED_ID: {
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_ENUMCONST: {
          return enumConstExpNodeCreate(peek.line, peek.character,
                                        peek.data.string);
        }
        case ST_ID: {
          reportError(report,
                      "%s:%zu:%zu: error: expected a constant, but found an "
                      "identifier",
                      info->filename, peek.line, peek.character);
          return NULL;
        }
        case ST_TYPE: {
          reportError(report,
                      "%s:%zu:%zu: error: expected a constant, but found a "
                      "type keyword",
                      info->filename, peek.line, peek.character);
          return NULL;
        }
        default: {
          assert(false);  // error: not a valid enum!
        }
      }
      break;
    }
    case TT_LSQUARE: {
      NodeList *literals = parseLiteralList(report, options, env, info);
      if (literals == NULL) {
        return NULL;
      }

      TokenInfo closeSquare;
      lex(info, report, &closeSquare);
      if (closeSquare.type != TT_RSQUARE) {
        if (!tokenInfoIsLexerError(&closeSquare)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a close square paren to "
                      "close the literal list, but found %s",
                      info->filename, closeSquare.line, closeSquare.character,
                      tokenTypeToString(closeSquare.type));
        }
        tokenInfoUninit(&closeSquare);
        nodeListDestroy(literals);
        return NULL;
      }

      return aggregateInitExpNodeCreate(peek.line, peek.character, literals);
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a constant, but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      tokenInfoUninit(&peek);
      return NULL;
    }
  }
}
static Node *parseExpression(Report *, Options *, TypeEnvironment *,
                             LexerInfo *);
static Node *parseCast(Report *report, Options *options, TypeEnvironment *env,
                       LexerInfo *info) {
  TokenInfo castKwd;
  lex(info, report, &castKwd);

  TokenInfo openSquare;
  lex(info, report, &openSquare);
  if (openSquare.type != TT_LSQUARE) {
    if (!tokenInfoIsLexerError(&openSquare)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open square bracket after "
                  "'cast', but found %s",
                  info->filename, openSquare.line, openSquare.character,
                  tokenTypeToString(openSquare.type));
    }
    tokenInfoUninit(&openSquare);
    return NULL;
  }

  Node *type = parseType(report, options, env, info);

  TokenInfo closeSquare;
  lex(info, report, &closeSquare);
  if (closeSquare.type != TT_RSQUARE) {
    if (!tokenInfoIsLexerError(&closeSquare)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close square bracket after "
                  "the target type in a cast, but found %s",
                  info->filename, closeSquare.line, closeSquare.character,
                  tokenTypeToString(closeSquare.type));
    }
    tokenInfoUninit(&closeSquare);
    nodeDestroy(type);
    return NULL;
  }

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after the target "
                  "type in a cast, but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    nodeDestroy(type);
    return NULL;
  }

  Node *from = parseExpression(report, options, env, info);

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an close paren, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(from);
    nodeDestroy(type);
    return NULL;
  }

  return castExpNodeCreate(castKwd.line, castKwd.character, type, from);
}
static Node *parseSizeof(Report *report, Options *options, TypeEnvironment *env,
                         LexerInfo *info) {
  TokenInfo sizeofKwd;
  lex(info, report, &sizeofKwd);

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after 'sizeof', "
                  "but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    return NULL;
  }

  Node *target;
  bool isType;

  TokenInfo peek;
  lex(info, report, &peek);
  switch (peek.type) {
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
    case TT_CHAR:
    case TT_USHORT:
    case TT_SHORT:
    case TT_UINT:
    case TT_INT:
    case TT_WCHAR:
    case TT_ULONG:
    case TT_LONG:
    case TT_FLOAT:
    case TT_DOUBLE:
    case TT_BOOL: {
      unLex(info, &peek);
      target = parseType(report, options, env, info);
      if (target == NULL) {
        return NULL;
      }
      isType = true;
      break;
    }
    case TT_STAR:
    case TT_AMPERSAND:
    case TT_PLUSPLUS:
    case TT_MINUSMINUS:
    case TT_PLUS:
    case TT_MINUS:
    case TT_BANG:
    case TT_TILDE:
    case TT_LITERALINT_0:
    case TT_LITERALINT_B:
    case TT_LITERALINT_O:
    case TT_LITERALINT_D:
    case TT_LITERALINT_H:
    case TT_LITERALSTRING:
    case TT_LITERALCHAR:
    case TT_LITERALWSTRING:
    case TT_LITERALWCHAR:
    case TT_TRUE:
    case TT_FALSE:
    case TT_LSQUARE:
    case TT_CAST:
    case TT_SIZEOF:
    case TT_LPAREN: {
      unLex(info, &peek);
      target = parseExpression(report, options, env, info);
      if (target == NULL) {
        return NULL;
      }
      isType = false;
      break;
    }
    case TT_ID:
    case TT_SCOPED_ID: {
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_ENUMCONST:
        case ST_ID: {
          unLex(info, &peek);
          target = parseExpression(report, options, env, info);
          if (target == NULL) {
            return NULL;
          }
          isType = false;
          break;
        }
        case ST_TYPE: {
          unLex(info, &peek);
          target = parseType(report, options, env, info);
          if (target == NULL) {
            return NULL;
          }
          isType = true;
          break;
        }
        default: {
          assert(false);  // error: not a valid enum!
        }
      }
      break;
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a statement or expression, "
                    "but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      tokenInfoUninit(&peek);
      return NULL;
    }
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(target);
    return NULL;
  }

  return (isType ? sizeofTypeExpNodeCreate : sizeofExpExpNodeCreate)(
      sizeofKwd.line, sizeofKwd.character, target);
}
static Node *parsePrimaryExpression(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_SCOPED_ID:
    case TT_ID: {
      // peek's id must be a valid id to get here
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_TYPE: {
          reportError(
              report,
              "%s:%zu:%zu: error: expected an expression, but found a type",
              info->filename, peek.line, peek.character);
          return NULL;
        }
        case ST_ID: {
          return idExpNodeCreate(peek.line, peek.character, peek.data.string);
        }
        case ST_ENUMCONST: {
          unLex(info, &peek);
          return parseLiteral(report, options, env, info);
        }
        default: {
          assert(false);  // error: not a valid enum
        }
      }
    }
    case TT_LITERALINT_0:
    case TT_LITERALINT_B:
    case TT_LITERALINT_O:
    case TT_LITERALINT_D:
    case TT_LITERALINT_H:
    case TT_LITERALFLOAT:
    case TT_LITERALSTRING:
    case TT_LITERALCHAR:
    case TT_LITERALWSTRING:
    case TT_LITERALWCHAR:
    case TT_TRUE:
    case TT_FALSE:
    case TT_LSQUARE: {
      unLex(info, &peek);
      return parseLiteral(report, options, env, info);
    }
    case TT_CAST: {
      unLex(info, &peek);
      return parseCast(report, options, env, info);
    }
    case TT_SIZEOF: {
      unLex(info, &peek);
      return parseSizeof(report, options, env, info);
    }
    case TT_LPAREN: {
      Node *expression = parseExpression(report, options, env, info);

      TokenInfo closeParen;
      lex(info, report, &closeParen);
      if (closeParen.type != TT_RPAREN) {
        if (!tokenInfoIsLexerError(&closeParen)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a close paren, but found %s",
                      info->filename, closeParen.line, closeParen.character,
                      tokenTypeToString(closeParen.type));
        }
        tokenInfoUninit(&closeParen);
        return NULL;
      }

      return expression;
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected an expression, but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      tokenInfoUninit(&peek);
      return NULL;
    }
  }
}
static Node *parseAssignmentExpression(Report *, Options *, TypeEnvironment *,
                                       LexerInfo *);
static NodeList *parseArgumentList(Report *report, Options *options,
                                   TypeEnvironment *env, LexerInfo *info) {
  NodeList *args = nodeListCreate();

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type != TT_RPAREN) {
    unLex(info, &peek);

    Node *arg = parseAssignmentExpression(report, options, env, info);
    if (arg == NULL) {
      nodeListDestroy(args);
      return NULL;
    }
    nodeListInsert(args, arg);

    TokenInfo comma;
    lex(info, report, &comma);
    if (comma.type != TT_COMMA) {
      break;
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return args;
}
static Node *parsePostfixExpression(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  Node *first = parsePrimaryExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_DOT || peek.type == TT_ARROW ||
         peek.type == TT_LPAREN || peek.type == TT_LSQUARE ||
         peek.type == TT_PLUSPLUS || peek.type == TT_MINUSMINUS) {
    switch (peek.type) {
      case TT_ARROW:
      case TT_DOT: {
        // consume operator
        Node *id = parseUnscopedId(report, info);
        if (id == NULL) {
          nodeDestroy(first);
          return NULL;
        }

        first = (peek.type == TT_ARROW ? structPtrAccessExpNodeCreate
                                       : structAccessExpNodeCreate)(
            first->line, first->character, first, id);
        break;
      }
      case TT_LPAREN: {
        NodeList *args = parseArgumentList(report, options, env, info);
        if (args == NULL) {
          nodeDestroy(first);
          return NULL;
        }

        TokenInfo closeParen;
        lex(info, report, &closeParen);
        if (closeParen.type != TT_RPAREN) {
          if (!tokenInfoIsLexerError(&closeParen)) {
            reportError(report,
                        "%s:%zu:%zu: error: expected a close paren after the "
                        "arguments in a function call, but found %s",
                        info->filename, closeParen.line, closeParen.character,
                        tokenTypeToString(closeParen.type));
          }
          tokenInfoUninit(&closeParen);
          nodeDestroy(first);
          return NULL;
        }

        first = fnCallExpNodeCreate(first->line, first->character, first, args);
        break;
      }
      case TT_LSQUARE: {
        Node *index = parseExpression(report, options, env, info);
        if (index == NULL) {
          nodeDestroy(first);
          return NULL;
        }

        TokenInfo closeSquare;
        lex(info, report, &closeSquare);
        if (closeSquare.type != TT_RSQUARE) {
          if (!tokenInfoIsLexerError(&closeSquare)) {
            reportError(report,
                        "%s:%zu:%zu: error: expected a close square bracket "
                        "after the index in an array access, but found %s",
                        info->filename, closeSquare.line, closeSquare.character,
                        tokenTypeToString(closeSquare.type));
          }
          tokenInfoUninit(&closeSquare);
          nodeDestroy(first);
          return NULL;
        }

        first = binOpExpNodeCreate(first->line, first->character,
                                   BO_ARRAYACCESS, first, index);
        break;
      }
      case TT_PLUSPLUS:
      case TT_MINUSMINUS: {
        first = unOpExpNodeCreate(
            first->line, first->character,
            peek.type == TT_PLUSPLUS ? UO_POSTINC : UO_POSTDEC, first);
        break;
      }
      default: {
        assert(false);  // mutation to something that wasn't mutated!
      }
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parsePrefixExpression(Report *report, Options *options,
                                   TypeEnvironment *env, LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_ASSIGN:
    case TT_MULASSIGN:
    case TT_DIVASSIGN:
    case TT_MODASSIGN:
    case TT_ADDASSIGN:
    case TT_SUBASSIGN:
    case TT_LSHIFTASSIGN:
    case TT_ARSHIFTASSIGN:
    case TT_LRSHIFTASSIGN:
    case TT_BITANDASSIGN:
    case TT_BITORASSIGN: {
      Node *target = parsePrefixExpression(report, options, env, info);
      if (target == NULL) {
        return NULL;
      }

      return unOpExpNodeCreate(peek.line, peek.character,
                               tokenTypeToPrefixUnop(peek.type), target);
    }
    default: {
      unLex(info, &peek);
      return parsePostfixExpression(report, options, env, info);
    }
  }
}
static Node *parseMultiplicationExpression(Report *report, Options *options,
                                           TypeEnvironment *env,
                                           LexerInfo *info) {
  Node *first = parsePrefixExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_PLUS || peek.type == TT_MINUS) {
    // consume the operator

    Node *next = parsePrefixExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = binOpExpNodeCreate(first->line, first->character,
                               tokenTypeToMulBinop(peek.type), first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseAdditionExpression(Report *report, Options *options,
                                     TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseMultiplicationExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_PLUS || peek.type == TT_MINUS) {
    // consume the operator

    Node *next = parseMultiplicationExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first =
        binOpExpNodeCreate(first->line, first->character,
                           peek.type == TT_PLUS ? BO_ADD : BO_SUB, first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseShiftExpression(Report *report, Options *options,
                                  TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseAdditionExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_LSHIFT || peek.type == TT_ARSHIFT ||
         peek.type == TT_LRSHIFT) {
    // consume the operator

    Node *next = parseAdditionExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = binOpExpNodeCreate(first->line, first->character,
                               tokenTypeToShiftBinop(peek.type), first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseSpaceshipExpression(Report *report, Options *options,
                                      TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseShiftExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_SPACESHIP) {
    // consume the operator

    Node *next = parseShiftExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = binOpExpNodeCreate(first->line, first->character, BO_SPACESHIP,
                               first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseComparisonExpression(Report *report, Options *options,
                                       TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseSpaceshipExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_LANGLE || peek.type == TT_RANGLE ||
         peek.type == TT_LTEQ || peek.type == TT_GTEQ) {
    // consume the operator

    Node *next = parseSpaceshipExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = compOpExpNodeCreate(first->line, first->character,
                                tokenTypeToCompop(peek.type), first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseEqualityExpression(Report *report, Options *options,
                                     TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseComparisonExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_EQ || peek.type == TT_NEQ) {
    // consume the operator

    Node *next = parseComparisonExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first =
        compOpExpNodeCreate(first->line, first->character,
                            peek.type == TT_EQ ? CO_EQ : CO_NEQ, first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseBitwiseExpression(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseEqualityExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_AMPERSAND || peek.type == TT_PIPE ||
         peek.type == TT_CARET) {
    // consume the operator

    Node *next = parseEqualityExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = binOpExpNodeCreate(first->line, first->character,
                               tokenTypeToBitwiseBinop(peek.type), first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseLogicalExpression(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseBitwiseExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_LAND || peek.type == TT_LOR) {
    // consume the operator

    Node *next = parseBitwiseExpression(report, options, env, info);
    if (next == NULL) {
      nodeDestroy(first);
      return NULL;
    }

    first = (peek.type == TT_LAND ? landExpNodeCreate : lorExpNodeCreate)(
        first->line, first->character, first, next);

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return first;
}
static Node *parseTernaryExpression(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  Node *test = parseLogicalExpression(report, options, env, info);
  if (test == NULL) {
    return NULL;
  }

  TokenInfo next;
  lex(info, report, &next);
  if (next.type != TT_QUESTION) {
    unLex(info, &next);
    return test;
  }

  Node *consequent = parseExpression(report, options, env, info);
  if (consequent == NULL) {
    nodeDestroy(test);
    return NULL;
  }

  TokenInfo colon;
  lex(info, report, &colon);
  if (colon.type != TT_COLON) {
    if (!tokenInfoIsLexerError(&colon)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a colon as part of a ternary "
                  "expression, but found %s",
                  info->filename, colon.line, colon.character,
                  tokenTypeToString(colon.type));
    }
    tokenInfoUninit(&colon);
    nodeDestroy(consequent);
    nodeDestroy(test);
  }

  Node *alternative = parseTernaryExpression(report, options, env, info);
  if (alternative == NULL) {
    nodeDestroy(consequent);
    nodeDestroy(test);
    return NULL;
  }

  return ternaryExpNodeCreate(test->line, test->character, test, consequent,
                              alternative);
}
static Node *parseAssignmentExpression(Report *report, Options *options,
                                       TypeEnvironment *env, LexerInfo *info) {
  Node *lhs = parseTernaryExpression(report, options, env, info);
  if (lhs == NULL) {
    return NULL;
  }

  TokenInfo next;
  lex(info, report, &next);

  switch (next.type) {
    case TT_ASSIGN:
    case TT_MULASSIGN:
    case TT_DIVASSIGN:
    case TT_MODASSIGN:
    case TT_ADDASSIGN:
    case TT_SUBASSIGN:
    case TT_LSHIFTASSIGN:
    case TT_ARSHIFTASSIGN:
    case TT_LRSHIFTASSIGN:
    case TT_BITANDASSIGN:
    case TT_BITORASSIGN: {
      Node *rhs = parseAssignmentExpression(report, options, env, info);
      if (rhs == NULL) {
        nodeDestroy(rhs);
        return NULL;
      }

      return binOpExpNodeCreate(lhs->line, lhs->character,
                                tokenTypeToAssignmentBinop(next.type), lhs,
                                rhs);
    }
    case TT_LORASSIGN:
    case TT_LANDASSIGN: {
      Node *rhs = parseAssignmentExpression(report, options, env, info);
      if (rhs == NULL) {
        nodeDestroy(rhs);
        return NULL;
      }

      return (next.type == TT_LANDASSIGN ? landAssignExpNodeCreate
                                         : lorAssignExpNodeCreate)(
          lhs->line, lhs->character, lhs, rhs);
    }
    default: {
      unLex(info, &next);
      return lhs;
    }
  }
}
static Node *parseExpression(Report *report, Options *options,
                             TypeEnvironment *env, LexerInfo *info) {
  Node *first = parseAssignmentExpression(report, options, env, info);
  if (first == NULL) {
    return NULL;
  }

  TokenInfo next;
  lex(info, report, &next);
  if (next.type != TT_COMMA) {
    unLex(info, &next);
    return first;
  }

  Node *rest = parseExpression(report, options, env, info);
  if (rest == NULL) {
    nodeDestroy(first);
    return NULL;
  }

  return seqExpNodeCreate(first->line, first->character, first, rest);
}

// statement
static Node *parseStatement(Report *, Options *, TypeEnvironment *,
                            LexerInfo *);
static Node *parseIfStatement(Report *report, Options *options,
                              TypeEnvironment *env, LexerInfo *info) {
  TokenInfo ifKwd;
  lex(info, report, &ifKwd);

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(
          report,
          "%s:%zu:%zu: error: expected an open paren after 'if', but found %s",
          info->filename, openParen.line, openParen.character,
          tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    return NULL;
  }

  Node *test = parseExpression(report, options, env, info);
  if (test == NULL) {
    return NULL;
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren after the test "
                  "expression in an if, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(test);
    return NULL;
  }

  Node *thenExp = parseStatement(report, options, env, info);
  if (thenExp == NULL) {
    nodeDestroy(test);
  }

  TokenInfo next;
  lex(info, report, &next);
  if (next.type != TT_ELSE) {
    unLex(info, &next);
    return ifStmtNodeCreate(ifKwd.line, ifKwd.character, test, thenExp, NULL);
  }

  Node *elseExp = parseStatement(report, options, env, info);
  if (elseExp == NULL) {
    nodeDestroy(thenExp);
    nodeDestroy(test);
  }

  return ifStmtNodeCreate(ifKwd.line, ifKwd.character, test, thenExp, NULL);
}
static Node *parseWhileStatement(Report *report, Options *options,
                                 TypeEnvironment *env, LexerInfo *info) {
  TokenInfo whileKwd;
  lex(info, report, &whileKwd);

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after 'while', "
                  "but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    return NULL;
  }

  Node *test = parseExpression(report, options, env, info);
  if (test == NULL) {
    return NULL;
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren after the test "
                  "expression in a while loop, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(test);
    return NULL;
  }

  Node *body = parseStatement(report, options, env, info);
  if (body == NULL) {
    nodeDestroy(test);
  }

  return whileStmtNodeCreate(whileKwd.line, whileKwd.character, test, body);
}
static Node *parseDoWhileStatement(Report *report, Options *options,
                                   TypeEnvironment *env, LexerInfo *info) {
  TokenInfo doKwd;
  lex(info, report, &doKwd);

  Node *body = parseStatement(report, options, env, info);
  if (body == NULL) {
    return NULL;
  }

  TokenInfo whileKwd;
  lex(info, report, &whileKwd);
  if (whileKwd.type != TT_WHILE) {
    if (!tokenInfoIsLexerError(&whileKwd)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected 'while' after the body of a "
                  "do-while loop, but found %s",
                  info->filename, whileKwd.line, whileKwd.character,
                  tokenTypeToString(whileKwd.type));
    }
    tokenInfoUninit(&whileKwd);
    nodeDestroy(body);
    return NULL;
  }

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after 'while', "
                  "but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    nodeDestroy(body);
    return NULL;
  }

  Node *test = parseExpression(report, options, env, info);
  if (test == NULL) {
    nodeDestroy(body);
    return NULL;
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren after the test "
                  "expression in a while loop, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(test);
    return NULL;
  }

  return doWhileStmtNodeCreate(doKwd.line, doKwd.character, test, body);
}
static Node *parseVarDecl(Report *, Options *, TypeEnvironment *, LexerInfo *);
static Node *parseForStatement(Report *report, Options *options,
                               TypeEnvironment *env, LexerInfo *info) {
  TokenInfo forKwd;
  lex(info, report, &forKwd);

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after 'while', "
                  "but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    return NULL;
  }

  Node *init;

  TokenInfo next;
  lex(info, report, &next);
  switch (next.type) {
    case TT_SEMI: {
      unLex(info, &next);
      init = NULL;
      break;
    }
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
    case TT_CHAR:
    case TT_USHORT:
    case TT_SHORT:
    case TT_UINT:
    case TT_INT:
    case TT_WCHAR:
    case TT_ULONG:
    case TT_LONG:
    case TT_FLOAT:
    case TT_DOUBLE:
    case TT_BOOL: {
      unLex(info, &next);
      init = parseVarDecl(report, options, env, info);
      if (init == NULL) {
        return NULL;
      }
      break;
    }
    case TT_STAR:
    case TT_AMPERSAND:
    case TT_PLUSPLUS:
    case TT_MINUSMINUS:
    case TT_PLUS:
    case TT_MINUS:
    case TT_BANG:
    case TT_TILDE:
    case TT_LITERALINT_0:
    case TT_LITERALINT_B:
    case TT_LITERALINT_O:
    case TT_LITERALINT_D:
    case TT_LITERALINT_H:
    case TT_LITERALSTRING:
    case TT_LITERALCHAR:
    case TT_LITERALWSTRING:
    case TT_LITERALWCHAR:
    case TT_TRUE:
    case TT_FALSE:
    case TT_LSQUARE:
    case TT_CAST:
    case TT_SIZEOF:
    case TT_LPAREN: {
      unLex(info, &next);
      init = parseExpression(report, options, env, info);
      if (init == NULL) {
        return NULL;
      }
      break;
    }
    case TT_ID:
    case TT_SCOPED_ID: {
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &next, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_ENUMCONST:
        case ST_ID: {
          unLex(info, &next);
          init = parseExpression(report, options, env, info);
          if (init == NULL) {
            return NULL;
          }
          break;
        }
        case ST_TYPE: {
          unLex(info, &next);
          init = parseVarDecl(report, options, env, info);
          if (init == NULL) {
            return NULL;
          }
          break;
        }
        default: {
          assert(false);  // error: not a valid enum!
        }
      }
      break;
    }
    default: {
      if (!tokenInfoIsLexerError(&next)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected an expression or variable "
                    "declaration, but found %s",
                    info->filename, next.line, next.character,
                    tokenTypeToString(next.type));
      }
      tokenInfoUninit(&next);
      return NULL;
    }
  }

  TokenInfo semi;
  lex(info, report, &semi);
  if (semi.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semi)) {
      reportError(
          report,
          "%s:%zu:%zu: error: expected a semicolon after the initialization "
          "expression or declaration in a for loop, but found %s",
          info->filename, semi.line, semi.character,
          tokenTypeToString(semi.type));
    }
    tokenInfoUninit(&semi);
    nodeDestroy(init);
    return NULL;
  }

  Node *test = parseExpression(report, options, env, info);
  if (test == NULL) {
    nodeDestroy(init);
    return NULL;
  }

  lex(info, report, &semi);
  if (semi.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semi)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon after the test "
                  "expression in a for loop, but found %s",
                  info->filename, semi.line, semi.character,
                  tokenTypeToString(semi.type));
    }
    tokenInfoUninit(&semi);
    nodeDestroy(test);
    nodeDestroy(init);
    return NULL;
  }

  Node *update;

  lex(info, report, &next);
  if (next.type == TT_RPAREN) {
    unLex(info, &next);
    update = NULL;
  } else {
    unLex(info, &next);
    update = parseExpression(report, options, env, info);
    if (update == NULL) {
      nodeDestroy(test);
      nodeDestroy(init);
      return NULL;
    }
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren after the update "
                  "expression in a for loop, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(update);
    nodeDestroy(test);
    nodeDestroy(init);
    return NULL;
  }

  Node *body = parseStatement(report, options, env, info);
  if (body == NULL) {
    nodeDestroy(update);
    nodeDestroy(test);
    nodeDestroy(init);
    return NULL;
  }

  return forStmtNodeCreate(forKwd.line, forKwd.character, init, test, update,
                           body);
}
static Node *parseCaseCase(Report *report, Options *options,
                           TypeEnvironment *env, LexerInfo *info) {
  NodeList *consts = nodeListCreate();

  TokenInfo firstCase;
  lex(info, report, &firstCase);
  Node *constant = parseIntOrEnumLiteral(report, options, env, info);
  if (constant == NULL) {
    nodeListDestroy(consts);
    return NULL;
  }

  nodeListInsert(consts, constant);

  TokenInfo colon;
  lex(info, report, &colon);
  if (colon.type != TT_COLON) {
    if (!tokenInfoIsLexerError(&colon)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a colon after the case "
                  "constant, but found %s",
                  info->filename, colon.line, colon.character,
                  tokenTypeToString(colon.type));
    }
    tokenInfoUninit(&colon);
    nodeListDestroy(consts);
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_CASE) {
    constant = parseIntOrEnumLiteral(report, options, env, info);
    if (constant == NULL) {
      nodeListDestroy(consts);
      return NULL;
    }

    nodeListInsert(consts, constant);

    lex(info, report, &colon);
    if (colon.type != TT_COLON) {
      if (!tokenInfoIsLexerError(&colon)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a colon after the case "
                    "constant, but found %s",
                    info->filename, colon.line, colon.character,
                    tokenTypeToString(colon.type));
      }
      tokenInfoUninit(&colon);
      nodeListDestroy(consts);
      return NULL;
    }
  }
  unLex(info, &peek);

  Node *statement = parseStatement(report, options, env, info);

  return numCaseNodeCreate(firstCase.line, firstCase.character, consts,
                           statement);
}
static Node *parseDefaultCase(Report *report, Options *options,
                              TypeEnvironment *env, LexerInfo *info) {
  TokenInfo defaultKwd;
  lex(info, report, &defaultKwd);

  TokenInfo colon;
  lex(info, report, &colon);
  if (colon.type != TT_COLON) {
    if (!tokenInfoIsLexerError(&colon)) {
      reportError(
          report,
          "%s:%zu:%zu: error: expected a colon after 'default', but found %s",
          info->filename, colon.line, colon.character,
          tokenTypeToString(colon.type));
    }
    tokenInfoUninit(&colon);
    return NULL;
  }

  Node *body = parseStatement(report, options, env, info);
  if (body == NULL) {
    return NULL;
  }

  return defaultCaseNodeCreate(defaultKwd.line, defaultKwd.character, body);
}
static NodeList *parseSwitchStatementCases(Report *report, Options *options,
                                           TypeEnvironment *env,
                                           LexerInfo *info) {
  NodeList *list = nodeListCreate();

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type == TT_CASE || peek.type == TT_DEFAULT) {
    unLex(info, &peek);

    if (peek.type == TT_CASE) {
      Node *clause = parseCaseCase(report, options, env, info);
      if (clause == NULL) {
        nodeListDestroy(list);
        return NULL;
      }
      nodeListInsert(list, clause);
    } else {
      Node *clause = parseDefaultCase(report, options, env, info);
      if (clause == NULL) {
        nodeListDestroy(list);
        return NULL;
      }
      nodeListInsert(list, clause);
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return list;
}
static Node *parseSwitchStatement(Report *report, Options *options,
                                  TypeEnvironment *env, LexerInfo *info) {
  TokenInfo switchKwd;
  lex(info, report, &switchKwd);

  TokenInfo openParen;
  lex(info, report, &openParen);
  if (openParen.type != TT_LPAREN) {
    if (!tokenInfoIsLexerError(&openParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open paren after 'switch', "
                  "but found %s",
                  info->filename, openParen.line, openParen.character,
                  tokenTypeToString(openParen.type));
    }
    tokenInfoUninit(&openParen);
    return NULL;
  }

  Node *switchedOn = parseExpression(report, options, env, info);
  if (switchedOn == NULL) {
    return NULL;
  }

  TokenInfo closeParen;
  lex(info, report, &closeParen);
  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an close paren after the "
                  "switched on expression, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeDestroy(switchedOn);
    return NULL;
  }

  TokenInfo openBrace;
  lex(info, report, &openBrace);
  if (openBrace.type != TT_LBRACE) {
    if (!tokenInfoIsLexerError(&openBrace)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open brace after the "
                  "switched on expression, but found %s",
                  info->filename, openBrace.line, openBrace.character,
                  tokenTypeToString(openBrace.type));
    }
    tokenInfoUninit(&openBrace);
    nodeDestroy(switchedOn);
    return NULL;
  }

  NodeList *cases = parseSwitchStatementCases(report, options, env, info);
  if (cases == NULL) {
    nodeDestroy(switchedOn);
  }

  TokenInfo closeBrace;
  lex(info, report, &closeBrace);
  if (closeBrace.type != TT_RBRACE) {
    if (!tokenInfoIsLexerError(&closeBrace)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected an open close after the "
                  "cases in the switch, but found %s",
                  info->filename, closeBrace.line, closeBrace.character,
                  tokenTypeToString(closeBrace.type));
    }
    tokenInfoUninit(&closeBrace);
    nodeDestroy(switchedOn);
    nodeListDestroy(cases);
    return NULL;
  }

  return switchStmtNodeCreate(switchKwd.line, switchKwd.character, switchedOn,
                              cases);
}
static Node *parseReturnStatement(Report *report, Options *options,
                                  TypeEnvironment *env, LexerInfo *info) {
  TokenInfo returnKwd;
  lex(info, report, &returnKwd);

  TokenInfo next;
  lex(info, report, &next);

  if (next.type == TT_SEMI) {
    return returnStmtNodeCreate(returnKwd.line, returnKwd.character, NULL);
  } else {
    Node *expression = parseExpression(report, options, env, info);
    if (expression == NULL) {
      return NULL;
    }

    TokenInfo semi;
    lex(info, report, &semi);
    if (semi.type != TT_SEMI) {
      if (!tokenInfoIsLexerError(&semi)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a semicolon to terminate the "
                    "return statement, but found %s",
                    info->filename, semi.line, semi.character,
                    tokenTypeToString(semi.type));
      }
      tokenInfoUninit(&semi);
      nodeDestroy(expression);
      return NULL;
    }

    return returnStmtNodeCreate(returnKwd.line, returnKwd.character,
                                expression);
  }
}
static Node *parseCompoundStatement(Report *, Options *, TypeEnvironment *,
                                    LexerInfo *);
static Node *parseUnionOrStructDeclOrDefn(Report *, Options *,
                                          TypeEnvironment *, LexerInfo *);
static Node *parseEnumDeclOrDefn(Report *, Options *, TypeEnvironment *,
                                 LexerInfo *);
static Node *parseTypedef(Report *, Options *, TypeEnvironment *, LexerInfo *);
static Node *parseStatement(Report *report, Options *options,
                            TypeEnvironment *env, LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_LBRACE: {
      unLex(info, &peek);
      return parseCompoundStatement(report, options, env, info);
    }
    case TT_IF: {
      unLex(info, &peek);
      return parseIfStatement(report, options, env, info);
    }
    case TT_WHILE: {
      unLex(info, &peek);
      return parseWhileStatement(report, options, env, info);
    }
    case TT_DO: {
      unLex(info, &peek);
      return parseDoWhileStatement(report, options, env, info);
    }
    case TT_FOR: {
      unLex(info, &peek);
      return parseForStatement(report, options, env, info);
    }
    case TT_SWITCH: {
      unLex(info, &peek);
      return parseSwitchStatement(report, options, env, info);
    }
    case TT_BREAK: {
      TokenInfo semi;
      lex(info, report, &semi);

      if (semi.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semi)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon to terminate "
                      "the break statement, but found %s",
                      info->filename, semi.line, semi.character,
                      tokenTypeToString(semi.type));
        }
        tokenInfoUninit(&semi);
        return NULL;
      }

      return breakStmtNodeCreate(peek.line, peek.character);
    }
    case TT_CONTINUE: {
      TokenInfo semi;
      lex(info, report, &semi);

      if (semi.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semi)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon to terminate "
                      "the break statement, but found %s",
                      info->filename, semi.line, semi.character,
                      tokenTypeToString(semi.type));
        }
        tokenInfoUninit(&semi);
        return NULL;
      }

      return continueStmtNodeCreate(peek.line, peek.character);
    }
    case TT_RETURN: {
      unLex(info, &peek);
      return parseReturnStatement(report, options, env, info);
    }
    case TT_ASM: {
      Node *string = parseStringLiteral(report, options, env, info);
      if (string == NULL) {
        return NULL;
      }

      TokenInfo semi;
      lex(info, report, &semi);

      if (semi.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semi)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon to terminate "
                      "the break statement, but found %s",
                      info->filename, semi.line, semi.character,
                      tokenTypeToString(semi.type));
        }
        tokenInfoUninit(&semi);
        nodeDestroy(string);
        return NULL;
      }

      return asmStmtNodeCreate(peek.line, peek.character, string);
    }
    case TT_STRUCT:
    case TT_UNION: {
      unLex(info, &peek);
      return parseUnionOrStructDeclOrDefn(report, options, env, info);
    }
    case TT_ENUM: {
      unLex(info, &peek);
      return parseEnumDeclOrDefn(report, options, env, info);
    }
    case TT_TYPEDEF: {
      unLex(info, &peek);
      return parseTypedef(report, options, env, info);
    }
    case TT_SEMI: {
      return nullStmtNodeCreate(peek.line, peek.character);
    }
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
    case TT_CHAR:
    case TT_USHORT:
    case TT_SHORT:
    case TT_UINT:
    case TT_INT:
    case TT_WCHAR:
    case TT_ULONG:
    case TT_LONG:
    case TT_FLOAT:
    case TT_DOUBLE:
    case TT_BOOL: {
      unLex(info, &peek);
      return parseVarDecl(report, options, env, info);
    }
    case TT_STAR:
    case TT_AMPERSAND:
    case TT_PLUSPLUS:
    case TT_MINUSMINUS:
    case TT_PLUS:
    case TT_MINUS:
    case TT_BANG:
    case TT_TILDE:
    case TT_LITERALINT_0:
    case TT_LITERALINT_B:
    case TT_LITERALINT_O:
    case TT_LITERALINT_D:
    case TT_LITERALINT_H:
    case TT_LITERALSTRING:
    case TT_LITERALCHAR:
    case TT_LITERALWSTRING:
    case TT_LITERALWCHAR:
    case TT_TRUE:
    case TT_FALSE:
    case TT_LSQUARE:
    case TT_CAST:
    case TT_SIZEOF:
    case TT_LPAREN: {
      unLex(info, &peek);
      Node *expression = parseExpression(report, options, env, info);

      TokenInfo semi;
      lex(info, report, &semi);

      if (semi.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semi)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon after an "
                      "expression, but found %s",
                      info->filename, semi.line, semi.character,
                      tokenTypeToString(semi.type));
        }
        tokenInfoUninit(&semi);
        nodeDestroy(expression);
        return NULL;
      }

      return expressionStmtNodeCreate(expression->line, expression->character,
                                      expression);
    }
    case TT_ID:
    case TT_SCOPED_ID: {
      SymbolType symbolType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      switch (symbolType) {
        case ST_UNDEFINED: {
          return NULL;
        }
        case ST_ENUMCONST:
        case ST_ID: {
          unLex(info, &peek);
          Node *expression = parseExpression(report, options, env, info);

          TokenInfo semi;
          lex(info, report, &semi);

          if (semi.type != TT_SEMI) {
            if (!tokenInfoIsLexerError(&semi)) {
              reportError(report,
                          "%s:%zu:%zu: error: expected a semicolon after an "
                          "expression, but found %s",
                          info->filename, semi.line, semi.character,
                          tokenTypeToString(semi.type));
            }
            tokenInfoUninit(&semi);
            nodeDestroy(expression);
            return NULL;
          }

          return expressionStmtNodeCreate(expression->line,
                                          expression->character, expression);
        }
        case ST_TYPE: {
          unLex(info, &peek);
          return parseVarDecl(report, options, env, info);
        }
        default: {
          assert(false);  // error: not a valid enum!
        }
      }
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a statement or declaration, "
                    "but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      tokenInfoUninit(&peek);
      return NULL;
    }
  }
}
static Node *parseCompoundStatement(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
  typeEnvironmentPush(env);
  NodeList *stmts = nodeListCreate();
  TokenInfo openBrace;
  lex(info, report, &openBrace);  // must be an open brace to get here

  TokenInfo peek;
  lex(info, report, &peek);
  while (peek.type != TT_RBRACE) {
    unLex(info, &peek);

    Node *stmt = parseStatement(report, options, env, info);
    if (stmt == NULL) {
      nodeListDestroy(stmts);
      typeEnvironmentPop(env);
      return NULL;
    }

    lex(info, report, &peek);
  }

  // unLex(info, &peek);
  // lex(info, report, &peek);  // the rbrace is already munched

  typeEnvironmentPop(env);
  return compoundStmtNodeCreate(openBrace.line, openBrace.character, stmts);
}

// body
static Node *parseFieldDecl(Report *report, Options *options,
                            TypeEnvironment *env, LexerInfo *info) {
  Node *type = parseType(report, options, env, info);
  if (type == NULL) {
    return NULL;
  }

  NodeList *ids = parseUnscopedIdList(report, options, env, info);
  if (ids == NULL) {
    nodeDestroy(type);
    return NULL;
  } else if (ids->size == 0) {
    reportError(report,
                "%s:%zu:%zu: error: expected at least one identifier in a "
                "variable or field declaration",
                info->filename, type->line, type->character);
    nodeDestroy(type);
    nodeListDestroy(ids);
    return NULL;
  }

  TokenInfo semicolon;
  lex(info, report, &semicolon);
  if (semicolon.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semicolon)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon after a variable or "
                  "field declaration, but found %s",
                  info->filename, semicolon.line, semicolon.character,
                  tokenTypeToString(semicolon.type));
    }
    tokenInfoUninit(&semicolon);
    nodeListDestroy(ids);
    nodeDestroy(type);
    return NULL;
  }

  return fieldDeclNodeCreate(type->line, type->character, type, ids);
}
static NodeList *parseFields(Report *report, Options *options,
                             TypeEnvironment *env, LexerInfo *info) {
  NodeList *elements = nodeListCreate();

  TokenInfo peek;

  lex(info, report, &peek);
  while (tokenInfoIsTypeKeyword(&peek) || peek.type == TT_SCOPED_ID ||
         peek.type == TT_ID) {
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      SymbolType isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == ST_UNDEFINED) {
        unLex(info, &peek);
        nodeListDestroy(elements);
        return NULL;
      } else if (isType == ST_ID) {
        break;
      }
    }
    unLex(info, &peek);

    // is the start of a type!
    Node *dec = parseFieldDecl(report, options, env, info);
    if (dec == NULL) {
      nodeListDestroy(elements);
      return NULL;
    }
    nodeListInsert(elements, dec);

    lex(info, report, &peek);
  }

  unLex(info, &peek);

  return elements;
}
static NodeList *parseEnumFields(Report *report, Options *options,
                                 TypeEnvironment *env, LexerInfo *info) {
  NodeList *ids = nodeListCreate();

  TokenInfo next;

  lex(info, report, &next);
  while (next.type == TT_ID) {
    nodeListInsert(ids,
                   idNodeCreate(next.line, next.character, next.data.string));

    lex(info, report, &next);
    if (next.type != TT_COMMA) {
      break;
    }

    lex(info, report, &next);
  }

  if (next.type == TT_ID) {
    nodeListInsert(ids,
                   idNodeCreate(next.line, next.character, next.data.string));
  } else {
    unLex(info, &next);
  }

  return ids;
}
static Node *parseUnionOrStructDeclOrDefn(Report *report, Options *options,
                                          TypeEnvironment *env,
                                          LexerInfo *info) {
  TokenInfo kwd;
  lex(info, report, &kwd);  // must be right to get here

  Node *id = parseUnscopedId(report, info);
  if (id == NULL) return NULL;

  TokenInfo nextToken;
  lex(info, report, &nextToken);

  switch (nextToken.type) {
    case TT_SEMI: {
      // declaration
      TypeTable *table = typeEnvironmentTop(env);
      SymbolType type = typeTableGet(table, id->data.id.id);
      switch (type) {
        case ST_UNDEFINED: {
          typeTableSet(table, id->data.id.id, ST_TYPE);
          break;
        }
        case ST_TYPE: {
          break;
        }
        case ST_ID: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as a variable or function name",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
        case ST_ENUMCONST: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as an enumeration constant",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
      }
      return (kwd.type == TT_STRUCT
                  ? structForwardDeclNodeCreate
                  : unionForwardDeclNodeCreate)(kwd.line, kwd.character, id);
    }
    case TT_LBRACE: {
      // definition: must not be exist or only be declared
      TypeTable *table = typeEnvironmentTop(env);
      SymbolType type = typeTableGet(table, id->data.id.id);

      switch (type) {
        case ST_UNDEFINED: {
          typeTableSet(table, id->data.id.id, ST_TYPE);
          break;
        }
        case ST_TYPE: {
          break;
        }
        case ST_ID: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as a variable or function name",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
        case ST_ENUMCONST: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as an enumeration constant",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
      }
      NodeList *elements = parseFields(report, options, env, info);
      if (elements == NULL) {
        nodeDestroy(id);
        return NULL;
      } else if (elements->size == 0) {
        reportError(report,
                    "%s:%zu:%zu: error: expected at least one field in a "
                    "%s declaration",
                    info->filename, nextToken.line, nextToken.character,
                    kwd.type == TT_STRUCT ? "struct" : "union");
        nodeListDestroy(elements);
        nodeDestroy(id);
        return NULL;
      }

      TokenInfo closeBrace;
      lex(info, report, &closeBrace);
      if (closeBrace.type != TT_RBRACE) {
        if (!tokenInfoIsLexerError(&closeBrace)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a right brace to close the "
                      "%s definition, but found %s",
                      info->filename, closeBrace.line, closeBrace.character,
                      kwd.type == TT_STRUCT ? "struct" : "union",
                      tokenTypeToString(closeBrace.type));
        }
        tokenInfoUninit(&closeBrace);
        nodeListDestroy(elements);
        nodeDestroy(id);
      }

      TokenInfo semicolon;
      lex(info, report, &semicolon);
      if (semicolon.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semicolon)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon to close the "
                      "%s definition, but found %s",
                      info->filename, semicolon.line, semicolon.character,
                      kwd.type == TT_STRUCT ? "struct" : "union",
                      tokenTypeToString(semicolon.type));
        }
        tokenInfoUninit(&semicolon);
        nodeListDestroy(elements);
        nodeDestroy(id);
      }

      return (kwd.type == TT_STRUCT
                  ? structDeclNodeCreate
                  : unionDeclNodeCreate)(kwd.line, kwd.character, id, elements);
    }
    default: {
      if (!tokenInfoIsLexerError(&nextToken)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a semicolon or a left brace, "
                    "but found %s",
                    info->filename, nextToken.line, nextToken.character,
                    tokenTypeToString(nextToken.type));
      }
      nodeDestroy(id);
      return NULL;
    }
  }
}
static Node *parseEnumDeclOrDefn(Report *report, Options *options,
                                 TypeEnvironment *env, LexerInfo *info) {
  TokenInfo kwd;
  lex(info, report, &kwd);  // must be an enum to get here

  Node *id = parseUnscopedId(report, info);
  if (id == NULL) return NULL;

  TokenInfo nextToken;
  lex(info, report, &nextToken);

  switch (nextToken.type) {
    case TT_SEMI: {
      // declaration
      TypeTable *table = typeEnvironmentTop(env);
      SymbolType type = typeTableGet(table, id->data.id.id);
      switch (type) {
        case ST_UNDEFINED: {
          typeTableSet(table, id->data.id.id, ST_TYPE);
          break;
        }
        case ST_TYPE: {
          break;
        }
        case ST_ID: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as a variable or function name",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
        case ST_ENUMCONST: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as an enumeration constant",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
      }
      return enumForwardDeclNodeCreate(kwd.line, kwd.character, id);
    }
    case TT_LBRACE: {
      // definition: must not be exist or only be declared
      TypeTable *table = typeEnvironmentTop(env);
      SymbolType type = typeTableGet(table, id->data.id.id);

      switch (type) {
        case ST_UNDEFINED: {
          typeTableSet(table, id->data.id.id, ST_TYPE);
          break;
        }
        case ST_TYPE: {
          break;
        }
        case ST_ID: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as a variable or function name",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
        case ST_ENUMCONST: {
          reportError(report,
                      "%s:%zu:%zu: error: identifier '%s' has already been "
                      "declared as an enumeration constant",
                      info->filename, id->line, id->character, id->data.id.id);
          nodeDestroy(id);
          return NULL;
        }
      }
      NodeList *elements = parseEnumFields(report, options, env, info);
      if (elements == NULL) {
        nodeDestroy(id);
        return NULL;
      } else if (elements->size == 0) {
        reportError(report,
                    "%s:%zu:%zu: error: expected at least one field in a "
                    "enum declaration",
                    info->filename, nextToken.line, nextToken.character);
        nodeListDestroy(elements);
        nodeDestroy(id);
        return NULL;
      }

      TokenInfo closeBrace;
      lex(info, report, &closeBrace);
      if (closeBrace.type != TT_RBRACE) {
        if (!tokenInfoIsLexerError(&closeBrace)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a right brace to close the "
                      "%s definition, but found %s",
                      info->filename, closeBrace.line, closeBrace.character,
                      kwd.type == TT_STRUCT ? "struct" : "union",
                      tokenTypeToString(closeBrace.type));
        }
        tokenInfoUninit(&closeBrace);
        nodeListDestroy(elements);
        nodeDestroy(id);
      }

      TokenInfo semicolon;
      lex(info, report, &semicolon);
      if (semicolon.type != TT_SEMI) {
        if (!tokenInfoIsLexerError(&semicolon)) {
          reportError(report,
                      "%s:%zu:%zu: error: expected a semicolon to close the "
                      "%s definition, but found %s",
                      info->filename, semicolon.line, semicolon.character,
                      kwd.type == TT_STRUCT ? "struct" : "union",
                      tokenTypeToString(semicolon.type));
        }
        tokenInfoUninit(&semicolon);
        nodeListDestroy(elements);
        nodeDestroy(id);
      }

      return enumDeclNodeCreate(kwd.line, kwd.character, id, elements);
    }
    default: {
      if (!tokenInfoIsLexerError(&nextToken)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a semicolon or a left brace, "
                    "but found %s",
                    info->filename, nextToken.line, nextToken.character,
                    tokenTypeToString(nextToken.type));
      }
      nodeDestroy(id);
      return NULL;
    }
  }
}
static Node *parseTypedef(Report *report, Options *options,
                          TypeEnvironment *env, LexerInfo *info) {
  TokenInfo kwd;
  lex(info, report, &kwd);  // must be typedef to get here

  Node *type = parseType(report, options, env, info);
  if (type == NULL) {
    return NULL;
  }

  Node *id = parseUnscopedId(report, info);
  if (id == NULL) {
    nodeDestroy(type);
    return NULL;
  }

  TokenInfo semicolon;
  lex(info, report, &semicolon);

  if (semicolon.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semicolon)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon after a 'typedef' "
                  "definition, but found %s",
                  info->filename, semicolon.line, semicolon.character,
                  tokenTypeToString(semicolon.type));
    }
    tokenInfoUninit(&semicolon);
    nodeDestroy(id);
    nodeDestroy(type);
    return NULL;
  }

  TypeTable *table = typeEnvironmentTop(env);
  SymbolType symbolType = typeTableGet(table, id->data.id.id);
  switch (symbolType) {
    case ST_UNDEFINED: {
      typeTableSet(table, id->data.id.id, ST_TYPE);
      break;
    }
    case ST_TYPE: {
      break;
    }
    case ST_ID: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as a variable or function name",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      nodeDestroy(type);
      return NULL;
    }
    case ST_ENUMCONST: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as an enumeration constant",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      return NULL;
    }
  }

  return typedefNodeCreate(kwd.line, kwd.character, type, id);
}
// produce false on an error
static bool parseFunctionParam(Report *report, Options *options,
                               TypeEnvironment *env, NodeTripleList *list,
                               LexerInfo *info) {
  Node *type = parseType(report, options, env, info);
  if (type == NULL) {
    return false;
  }

  TokenInfo peek;
  lex(info, report, &peek);
  if (peek.type != TT_ID && peek.type != TT_EQ) {
    // end of the param
    unLex(info, &peek);
    nodeTripleListInsert(list, type, NULL, NULL);
    return true;
  }
  unLex(info, &peek);

  Node *id;
  if (peek.type == TT_ID) {
    id = parseUnscopedId(report, info);
    if (id == NULL) {
      nodeDestroy(type);
      return false;
    }
    TypeTable *table = typeEnvironmentTop(env);
    SymbolType symbolType = typeTableGet(table, id->data.id.id);
    switch (symbolType) {
      case ST_UNDEFINED: {
        typeTableSet(table, id->data.id.id, ST_ID);
        break;
      }
      case ST_TYPE: {
        reportError(report,
                    "%s:%zu:%zu: error: identifier '%s' has already been "
                    "declared as a type",
                    info->filename, id->line, id->character, id->data.id.id);
        nodeDestroy(id);
        nodeDestroy(type);
        return false;
      }
      case ST_ID: {
        break;
      }
      case ST_ENUMCONST: {
        reportError(report,
                    "%s:%zu:%zu: error: identifier '%s' has already been "
                    "declared as an enumeration constant",
                    info->filename, id->line, id->character, id->data.id.id);
        nodeDestroy(id);
        return NULL;
      }
    }
  } else {
    id = NULL;
  }

  TokenInfo eq;
  lex(info, report, &eq);
  if (eq.type != TT_EQ) {
    // end of the param
    unLex(info, &eq);
    nodeTripleListInsert(list, type, id, NULL);
    return true;
  }

  Node *literal = parseLiteral(report, options, env, info);
  if (literal == NULL) {
    nodeDestroy(id);
    nodeDestroy(type);
    return false;
  }

  // absolutely the end of the param
  nodeTripleListInsert(list, type, id, literal);
  return true;
}
static NodeTripleList *parseFunctionParams(Report *report, Options *options,
                                           TypeEnvironment *env,
                                           LexerInfo *info) {
  NodeTripleList *list = nodeTripleListCreate();

  TokenInfo peek;

  lex(info, report, &peek);
  while (tokenInfoIsTypeKeyword(&peek) || peek.type == TT_SCOPED_ID ||
         peek.type == TT_ID) {
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      SymbolType isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == ST_UNDEFINED) {
        unLex(info, &peek);
        nodeTripleListDestroy(list);
        return NULL;
      } else if (isType == ST_ID) {
        break;
      }
    }
    unLex(info, &peek);

    // is the start of a type!
    if (!parseFunctionParam(report, options, env, list, info)) {
      nodeTripleListDestroy(list);
      return NULL;
    }

    // deal with comma (if it isn't a comma, return early)
    lex(info, report, &peek);
    if (peek.type != TT_COMMA) {
      break;
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return list;
}
static Node *parseFunctionDeclOrDefn(Report *report, Options *options,
                                     TypeEnvironment *env, Node *type, Node *id,
                                     LexerInfo *info) {
  typeEnvironmentPush(env);
  NodeTripleList *params = parseFunctionParams(report, options, env, info);

  TokenInfo closeParen;
  lex(info, report, &closeParen);

  if (closeParen.type != TT_RPAREN) {
    if (!tokenInfoIsLexerError(&closeParen)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a close paren after the "
                  "parameter list, but found %s",
                  info->filename, closeParen.line, closeParen.character,
                  tokenTypeToString(closeParen.type));
    }
    tokenInfoUninit(&closeParen);
    nodeTripleListDestroy(params);
    nodeDestroy(id);
    nodeDestroy(type);
    typeEnvironmentPop(env);
    return NULL;
  }

  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_LBRACE: {
      unLex(info, &peek);
      Node *compoundStatement =
          parseCompoundStatement(report, options, env, info);
      if (compoundStatement == NULL) {
        nodeTripleListDestroy(params);
        nodeDestroy(id);
        nodeDestroy(type);
        typeEnvironmentPop(env);
        return NULL;
      }
      typeEnvironmentPop(env);
      return functionNodeCreate(type->line, type->character, type, id, params,
                                compoundStatement);
    }
    case TT_SEMI: {
      NodePairList *types = nodePairListCreate();
      for (size_t idx = 0; idx < params->size; idx++) {
        nodePairListInsert(types, params->firstElements[idx],
                           params->thirdElements[idx]);
        if (params->secondElements[idx] != NULL) {
          nodeDestroy(params->secondElements[idx]);
        }
      }
      free(params->firstElements);
      free(params->secondElements);
      free(params->thirdElements);
      free(params);

      typeEnvironmentPop(env);
      return funDeclNodeCreate(type->line, type->character, type, id, types);
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a function body or a "
                    "semicolon, but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      tokenInfoUninit(&peek);
      nodeTripleListDestroy(params);
      nodeDestroy(id);
      nodeDestroy(type);
      typeEnvironmentPop(env);
      return NULL;
    }
  }
}
// produce false on an error
static bool parseVarId(Report *report, Options *options, TypeEnvironment *env,
                       NodePairList *list, LexerInfo *info) {
  Node *id = parseUnscopedId(report, info);
  if (id == NULL) {
    return false;
  }
  TypeTable *table = typeEnvironmentTop(env);
  SymbolType type = typeTableGet(table, id->data.id.id);
  switch (type) {
    case ST_UNDEFINED: {
      typeTableSet(table, id->data.id.id, ST_ID);
      break;
    }
    case ST_TYPE: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as a type",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      return false;
    }
    case ST_ID: {
      break;
    }
    case ST_ENUMCONST: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as an enumeration constant",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      return NULL;
    }
  }

  TokenInfo peek;
  lex(info, report, &peek);
  if (peek.type != TT_EQ) {
    // end of the decl
    unLex(info, &peek);
    nodePairListInsert(list, id, NULL);
    return true;
  }

  Node *literal = parseLiteral(report, options, env, info);
  if (literal == NULL) {
    nodeDestroy(id);
    return false;
  }

  // absolutely the end of the decl
  nodePairListInsert(list, id, literal);
  return true;
}
static NodePairList *parseVarIds(Report *report, Options *options,
                                 TypeEnvironment *env, Node *firstId,
                                 LexerInfo *info) {
  NodePairList *list = nodePairListCreate();

  TokenInfo next;
  lex(info, report, &next);

  switch (next.type) {
    case TT_EQ: {
      Node *value = parseLiteral(report, options, env, info);
      if (value == NULL) {
        nodePairListDestroy(list);
        nodeDestroy(firstId);
        return NULL;
      }
      nodePairListInsert(list, firstId, value);
      break;
    }
    case TT_COMMA: {
      nodePairListInsert(list, firstId, NULL);
      unLex(info, &next);
      break;
    }
    default: {
      if (!tokenInfoIsLexerError(&next)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected an initializer or a comma, "
                    "but found %s",
                    info->filename, next.line, next.character,
                    tokenTypeToString(next.type));
      }
      nodePairListDestroy(list);
      nodeDestroy(firstId);
      tokenInfoUninit(&next);
      return NULL;
    }
  }

  TokenInfo peek;
  lex(info, report, &peek);
  while (tokenInfoIsTypeKeyword(&peek) || peek.type == TT_ID ||
         peek.type == TT_SCOPED_ID) {
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      SymbolType isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == ST_UNDEFINED) {
        unLex(info, &peek);
        nodePairListDestroy(list);
        return NULL;
      } else if (isType == ST_ID) {
        break;
      }
    }
    unLex(info, &peek);

    // is the start of a type!
    if (!parseVarId(report, options, env, list, info)) {
      nodePairListDestroy(list);
      return NULL;
    }

    // deal with comma (if it isn't a comma, return early)
    lex(info, report, &peek);
    if (peek.type != TT_COMMA) {
      break;
    }

    lex(info, report, &peek);
  }
  unLex(info, &peek);

  return list;
}
static Node *parseVarDeclPrefixed(Report *report, Options *options,
                                  TypeEnvironment *env, Node *type,
                                  Node *firstId, LexerInfo *info) {
  // currently at int foo .
  NodePairList *decls = parseVarIds(report, options, env, firstId, info);
  if (decls == NULL) {
    return NULL;
  }

  TokenInfo semicolon;
  lex(info, report, &semicolon);
  if (semicolon.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semicolon)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon after a variable "
                  "declaration, but found %s",
                  info->filename, semicolon.line, semicolon.character,
                  tokenTypeToString(semicolon.type));
    }
    tokenInfoUninit(&semicolon);
    nodePairListDestroy(decls);
    return NULL;
  }

  return varDeclNodeCreate(type->line, type->character, type, decls);
}
static Node *parseVarDecl(Report *report, Options *options,
                          TypeEnvironment *env, LexerInfo *info) {
  Node *type = parseType(report, options, env, info);
  if (type == NULL) {
    return NULL;
  }

  Node *id = parseUnscopedId(report, info);
  if (id == NULL) {
    nodeDestroy(type);
    return NULL;
  }
  TypeTable *table = typeEnvironmentTop(env);
  SymbolType symbolType = typeTableGet(table, id->data.id.id);
  switch (symbolType) {
    case ST_UNDEFINED: {
      typeTableSet(table, id->data.id.id, ST_ID);
      break;
    }
    case ST_TYPE: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as a type",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      nodeDestroy(type);
      return false;
    }
    case ST_ID: {
      break;
    }
    case ST_ENUMCONST: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as an enumeration constant",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      return NULL;
    }
  }

  return parseVarDeclPrefixed(report, options, env, type, id, info);
}
static Node *parseVarOrFunDeclOrDefn(Report *report, Options *options,
                                     TypeEnvironment *env, LexerInfo *info) {
  Node *type = parseType(report, options, env, info);

  if (type == NULL) {
    return NULL;
  }

  Node *id = parseUnscopedId(report, info);
  if (id == NULL) {
    nodeDestroy(type);
    return NULL;
  }
  TypeTable *table = typeEnvironmentTop(env);
  SymbolType symbolType = typeTableGet(table, id->data.id.id);
  switch (symbolType) {
    case ST_UNDEFINED: {
      typeTableSet(table, id->data.id.id, ST_ID);
      break;
    }
    case ST_TYPE: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as a type",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      nodeDestroy(type);
      return false;
    }
    case ST_ID: {
      break;
    }
    case ST_ENUMCONST: {
      reportError(report,
                  "%s:%zu:%zu: error: identifier '%s' has already been "
                  "declared as an enumeration constant",
                  info->filename, id->line, id->character, id->data.id.id);
      nodeDestroy(id);
      return NULL;
    }
  }

  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_LPAREN: {
      return parseFunctionDeclOrDefn(report, options, env, type, id, info);
    }
    case TT_SEMI: {
      NodePairList *elms = nodePairListCreate();
      nodePairListInsert(elms, id, NULL);
      return varDeclNodeCreate(type->line, type->character, type, elms);
    }
    case TT_EQ:
    case TT_COMMA: {
      unLex(info, &peek);
      return parseVarDeclPrefixed(report, options, env, type, id, info);
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a variable or function "
                    "declaration or definition, but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      nodeDestroy(id);
      nodeDestroy(type);
      return NULL;
    }
  }
}
static Node *parseBody(Report *report, Options *options, TypeEnvironment *env,
                       LexerInfo *info) {
  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_UNION:
    case TT_STRUCT: {
      // struct or struct forward decl
      unLex(info, &peek);
      return parseUnionOrStructDeclOrDefn(report, options, env, info);
    }
    case TT_ENUM: {
      // enum or enum forward decl
      unLex(info, &peek);
      return parseEnumDeclOrDefn(report, options, env, info);
    }
    case TT_TYPEDEF: {
      // typedef or typedef forward decl
      unLex(info, &peek);
      return parseTypedef(report, options, env, info);
    }

    case TT_ID:
    case TT_SCOPED_ID: {
      SymbolType type =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (type == ST_ID) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a declaration, but found an "
                    "identifier",
                    info->filename, peek.line, peek.character);
      }
      if (type != ST_TYPE) {
        return NULL;
      }
      // is a type
      __attribute__((fallthrough));
    }
    case TT_VOID:
    case TT_UBYTE:
    case TT_BYTE:
    case TT_CHAR:
    case TT_UINT:
    case TT_INT:
    case TT_WCHAR:
    case TT_ULONG:
    case TT_LONG:
    case TT_FLOAT:
    case TT_DOUBLE:
    case TT_BOOL: {
      // type, introducing a function decl/defn or variable decl/defn
      unLex(info, &peek);
      return parseVarOrFunDeclOrDefn(report, options, env, info);
    }
    default: {
      if (!tokenInfoIsLexerError(&peek)) {
        reportError(report,
                    "%s:%zu:%zu: error: expected a declaration or definition, "
                    "but found %s",
                    info->filename, peek.line, peek.character,
                    tokenTypeToString(peek.type));
      }
      return NULL;
    }
  }
}
static NodeList *parseBodies(Report *report, Options *options,
                             TypeEnvironment *env, LexerInfo *info) {
  NodeList *bodies = nodeListCreate();

  TokenInfo peek;
  lex(info, report, &peek);

  while (peek.type != TT_EOF && peek.type != TT_ERR) {
    unLex(info, &peek);
    Node *node = parseBody(report, options, env, info);
    if (node == NULL) {
      nodeListDestroy(bodies);
      return NULL;
    } else {
      nodeListInsert(bodies, node);
    }
    lex(info, report, &peek);
  }

  tokenInfoUninit(&peek);  // actually not needed
  return bodies;
}

// whole file
static Node *parseDeclFile(Report *report, Options *options,
                           ModuleTypeTableMap *typeTables,
                           Stack *dependencyStack, ModuleLexerInfoMap *miMap,
                           ModuleNodeMap *mnMap, ModuleAstMap *decls,
                           LexerInfo *info, Node *module) {
  TypeTable *currTypes = typeTableCreate();  // env setup
  TypeEnvironment env;
  typeEnvironmentInit(&env, currTypes, module->data.module.id->data.id.id);

  NodeList *imports =
      parseDeclImports(report, options, typeTables, &env, dependencyStack,
                       miMap, mnMap, decls, info);
  if (imports == NULL) {
    typeEnvironmentUninit(&env);
    typeTableDestroy(currTypes);
    nodeDestroy(module);
    return NULL;
  }
  NodeList *bodies = parseBodies(report, options, &env, info);
  if (bodies == NULL) {
    nodeListDestroy(imports);
    typeEnvironmentUninit(&env);
    typeTableDestroy(currTypes);
    nodeDestroy(module);
    return NULL;
  }

  // env cleanup
  moduleTypeTableMapPut(typeTables, module->data.module.id->data.id.id,
                        currTypes);
  typeEnvironmentUninit(&env);

  return fileNodeCreate(module->line, module->character, module, imports,
                        bodies, info->filename);
}
static Node *parseCodeFile(Report *report, Options *options,
                           ModuleTypeTableMap *typeTables, LexerInfo *info) {
  Node *module = parseModule(report, info);
  if (module == NULL) {
    return NULL;
  }

  TypeTable *currTypes = typeTableCreate();  // env setup
  TypeEnvironment env;
  typeEnvironmentInit(&env, currTypes, module->data.module.id->data.id.id);

  NodeList *imports = parseCodeImports(report, options, typeTables, &env, info);
  if (imports == NULL) {
    typeEnvironmentUninit(&env);
    typeTableDestroy(currTypes);
    nodeDestroy(module);
    return NULL;
  }
  NodeList *bodies = parseBodies(report, options, &env, info);
  if (bodies == NULL) {
    nodeListDestroy(imports);
    typeEnvironmentUninit(&env);
    typeTableDestroy(currTypes);
    nodeDestroy(module);
    return NULL;
  }

  // env cleanup
  moduleTypeTableMapPut(typeTables, module->data.module.id->data.id.id,
                        currTypes);
  typeEnvironmentUninit(&env);

  return fileNodeCreate(module->line, module->character, module,
                        nodeListCreate(), nodeListCreate(), info->filename);
}

void parse(ModuleAstMapPair *asts, Report *report, Options *options,
           FileList *files) {
  moduleAstMapPairInit(asts);

  KeywordMap kwMap;
  keywordMapInit(&kwMap);

  ModuleLexerInfoMap miMap;
  moduleLexerInfoMapInit(&miMap);
  ModuleNodeMap mnMap;
  moduleNodeMapInit(&mnMap);

  // for each decl file, read the module line, and add an entry to a
  // module-lexerinfo hashMap
  for (size_t idx = 0; idx < files->decls.size; idx++) {
    LexerInfo *li = lexerInfoCreate(files->decls.elements[idx], &kwMap);
    if (li == NULL) {  // bad file!
      reportError(report, "%s: error: no such file",
                  (char *)files->decls.elements[idx]);
    }
    Node *module = parseModule(report, li);
    if (module == NULL) {  // didn't find it, file can't be parsed.
      reportError(report, "%s: error: no module declaration found",
                  (char const *)files->decls.elements[idx]);
    } else if (moduleNodeMapPut(&mnMap, module->data.module.id->data.id.id,
                                module) == HM_EEXISTS) {
      reportError(
          report,
          "%s: error: module '%s' has already been declared (in file %s)",
          (char const *)files->decls.elements[idx],
          module->data.module.id->data.id.id,
          moduleLexerInfoMapGet(&miMap, module->data.module.id->data.id.id)
              ->filename);
      nodeDestroy(module);
    } else {
      moduleLexerInfoMapPut(&miMap, module->data.module.id->data.id.id, li);
    }
  }

  // if we aren't good, we can't build the stab properly
  // cleanup and return
  if (reportState(report) == RPT_ERR) {
    keywordMapUninit(&kwMap);
    moduleLexerInfoMapUninit(&miMap);
    moduleNodeMapUninit(&mnMap);
    return;
  }

  ModuleTypeTableMap typeTables;
  moduleTypeTableMapInit(&typeTables);

  // parse all decl files in order
  while (asts->decls.size < miMap.size) {
    Stack dependencyStack;
    stackInit(&dependencyStack);

    // select one to parse
    for (size_t idx = 0; idx < miMap.capacity; idx++) {
      // if this is a module, and it hasn't been parsed
      if (miMap.keys[idx] != NULL &&
          moduleAstMapGet(&asts->decls, miMap.keys[idx]) == NULL) {
        Node *parsed =
            parseDeclFile(report, options, &typeTables, &dependencyStack,
                          &miMap, &mnMap, &asts->decls, miMap.values[idx],
                          moduleNodeMapGet(&mnMap, miMap.keys[idx]));
        if (parsed != NULL) {
          moduleAstMapPut(&asts->decls, miMap.keys[idx], parsed);
        } else {
          // something's broke - get out
          stackUninit(&dependencyStack, nullDtor);
          moduleLexerInfoMapUninit(&miMap);
          moduleNodeMapUninit(&mnMap);
          moduleTypeTableMapUninit(&typeTables);
          keywordMapUninit(&kwMap);
          return;
        }
      }
    }

    stackUninit(&dependencyStack, nullDtor);
  }
  moduleLexerInfoMapUninit(&miMap);
  moduleNodeMapUninit(&mnMap);

  for (size_t idx = 0; idx < files->codes.size; idx++) {
    Node *parsed =
        parseCodeFile(report, options, &typeTables,
                      lexerInfoCreate(files->codes.elements[idx],
                                      &kwMap));  // may be null, but don't case
    if (parsed != NULL) {
      Node const *duplicateCheck = moduleAstMapGet(
          &asts->codes, parsed->data.file.module->data.module.id->data.id.id);
      if (duplicateCheck != NULL) {
        // exists, error!
        reportError(
            report,
            "%s: error: module '%s' has already been declared (in file %s)",
            (char const *)files->codes.elements[idx],
            parsed->data.file.module->data.module.id->data.id.id,
            duplicateCheck->data.file.filename);
      } else {
        moduleAstMapPut(&asts->codes,
                        parsed->data.file.module->data.module.id->data.id.id,
                        parsed);
      }
    }
  }

  moduleTypeTableMapUninit(&typeTables);
  keywordMapUninit(&kwMap);
}