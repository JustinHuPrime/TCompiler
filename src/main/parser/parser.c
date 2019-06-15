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
static Node *parseIntConstant(Report *report, Options *options,
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
static NodeList *parseUnscopedIdList(Report *report, Options *options,
                                     TypeEnvironment *env, LexerInfo *info) {
  NodeList *ids = nodeListCreate();
  TokenInfo next;

  lex(info, report, &next);
  while (next.type == TT_ID) {
    nodeListInsert(ids,
                   idNodeCreate(next.line, next.character, next.data.string));

    lex(info, report, &next);
    if (next.type != TT_COMMA) break;

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
    unLex(info, &peek);
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      TernaryValue isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == INDETERMINATE) {
        nodeListDestroy(types);
        return NULL;
      } else if (isType == NO) {
        break;
      }
    }

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
      Node *size = parseIntConstant(report, options, env, info);

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

// statement

// body
static Node *parseVariableDecl(Report *report, Options *options,
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
    unLex(info, &peek);
    if (peek.type == TT_ID || peek.type == TT_SCOPED_ID) {
      SymbolType isType =
          typeEnvironmentLookup(env, report, &peek, info->filename);
      if (isType == ST_UNDEFINED) {
        nodeListDestroy(elements);
        return NULL;
      } else if (isType == ST_ID) {
        break;
      }
    }

    // is the start of a type!
    Node *dec = parseVariableDecl(report, options, env, info);
    if (dec == NULL) {
      nodeListDestroy(elements);
      return NULL;
    }
    nodeListInsert(elements, dec);

    lex(info, report, &peek);
  }

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
    if (next.type != TT_COMMA) break;

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
static Node *parseUnionOrStructDecl(Report *report, Options *options,
                                    TypeEnvironment *env, LexerInfo *info) {
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
static Node *parseEnumDecl(Report *report, Options *options,
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
  }

  return typedefNodeCreate(kwd.line, kwd.character, type, id);
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

  TokenInfo peek;
  lex(info, report, &peek);

  switch (peek.type) {
    case TT_LPAREN: {
      // function defn or decl
      break;
    }
    case TT_SEMI: {
      // is a completed var decl
      break;
    }
    case TT_EQ: {
      // is a var decl
      break;
    }
    case TT_COMMA: {
      // is a var decl
      break;
    }
    default: {
      // incomplete!
      break;
    }
  }

  return NULL;
  // TODO: write this
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
      return parseUnionOrStructDecl(report, options, env, info);
    }
    case TT_ENUM: {
      // enum or enum forward decl
      unLex(info, &peek);
      return parseEnumDecl(report, options, env, info);
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