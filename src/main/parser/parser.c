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
#include "util/functional.h"

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

ModuleSymbolTableMap *moduleSymbolTableMapCreate(void) {
  return hashMapCreate();
}
void moduleSymbolTableMapInit(ModuleSymbolTableMap *map) { hashMapInit(map); }
SymbolTable *moduleSymbolTableMapGet(ModuleSymbolTableMap const *map,
                                     char const *key) {
  return hashMapGet(map, key);
}
int moduleSymbolTableMapPut(ModuleSymbolTableMap *map, char const *key,
                            SymbolTable *value) {
  return hashMapPut(map, key, value, (void (*)(void *))symbolTableDestroy);
}
void moduleSymbolTableMapUninit(ModuleSymbolTableMap *map) {
  hashMapUninit(map, (void (*)(void *))symbolTableDestroy);
}
void moduleSymbolTableMapDestroy(ModuleSymbolTableMap *map) {
  hashMapDestroy(map, (void (*)(void *))symbolTableDestroy);
}

ModuleSymbolTableMapPair *moduleSymbolTableMapPairCreate(void) {
  ModuleSymbolTableMapPair *pair = malloc(sizeof(ModuleSymbolTableMapPair));
  moduleSymbolTableMapPairInit(pair);
  return pair;
}
void moduleSymbolTableMapPairInit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapInit(&pair->codes);
  moduleSymbolTableMapInit(&pair->decls);
}
void moduleSymbolTableMapPairUninit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapUninit(&pair->codes);
  moduleSymbolTableMapUninit(&pair->decls);
}
void moduleSymbolTableMapPairDestroy(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapPairUninit(pair);
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
  lex(&id, report, info);
  if (id.type != TT_ID && id.type != TT_SCOPED_ID) {
    reportError(report,
                "%s:%zu:%zu: error: expected an identifier, but found %s\n",
                info->filename, id.line, id.character, tokenToName(id.type));
    tokenInfoUninit(&id);
    return NULL;
  }

  return idNodeCreate(id.line, id.character, id.data.string);
}
static Node *parseUnscopedId(Report *report, LexerInfo *info) {
  TokenInfo id;
  lex(&id, report, info);
  if (id.type != TT_ID) {
    reportError(
        report,
        "%s:%zu:%zu: error: expected an unqualified identifier, but found %s\n",
        info->filename, id.line, id.character, tokenToName(id.type));
    tokenInfoUninit(&id);
    return NULL;
  }

  return idNodeCreate(id.line, id.character, id.data.string);
}

static Node *parseModule(Report *report, LexerInfo *info) {
  TokenInfo moduleToken;
  lex(&moduleToken, report, info);
  if (moduleToken.type != TT_MODULE) {
    if (!tokenInfoIsLexerError(&moduleToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s\n",
                  info->filename, moduleToken.line, moduleToken.character,
                  tokenToName(moduleToken.type));
    }
    tokenInfoUninit(&moduleToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(&semiToken, report, info);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate "
                  "the module declaration, but found %s\n",
                  info->filename, semiToken.line, semiToken.character,
                  tokenToName(semiToken.type));
    }
    nodeDestroy(idNode);
    tokenInfoUninit(&semiToken);
    return NULL;
  }

  return moduleNodeCreate(moduleToken.line, moduleToken.character, idNode);
}

static Node *parseDeclFile(Report *report, Options *options,
                           ModuleSymbolTableMapPair *stabs,
                           Stack *dependencyStack, ModuleLexerInfoMap *miMap,
                           ModuleNodeMap *mnMap, ModuleAstMap *decls,
                           LexerInfo *lexerInfo, Node *module);
static Node *parseDeclImport(Report *report, Options *options,
                             ModuleSymbolTableMapPair *stabs, Environment *env,
                             Stack *dependencyStack, ModuleLexerInfoMap *miMap,
                             ModuleNodeMap *mnMap, ModuleAstMap *decls,
                             LexerInfo *info) {
  TokenInfo importToken;
  lex(&importToken, report, info);
  if (importToken.type != TT_IMPORT) {
    if (!tokenInfoIsLexerError(&importToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s\n",
                  info->filename, importToken.line, importToken.character,
                  tokenToName(importToken.type));
    }
    tokenInfoUninit(&importToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(&semiToken, report, info);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate "
                  "the module declaration, but found %s\n",
                  info->filename, semiToken.line, semiToken.character,
                  tokenToName(semiToken.type));
    }
    tokenInfoUninit(&semiToken);
    nodeDestroy(idNode);
    return NULL;
  }

  // find and add the stab in stabs to env
  SymbolTable *importedStab =
      moduleSymbolTableMapGet(&stabs->decls, idNode->data.id.id);
  if (importedStab == NULL) {
    for (size_t idx = 0; idx < dependencyStack->size; idx++) {
      if (strcmp(idNode->data.id.id, dependencyStack->elements[idx]) == 0) {
        // circular dep between current and all later elements in the stack
        reportError(report,
                    "%s:%zu:%zu: error: circular dependency on module '%s'\n",
                    info->filename, importToken.line, importToken.character,
                    (char const *)dependencyStack->elements[idx]);
        reportMessage(report,
                      "\tmodule '%s' imports module '%s', which imports\n",
                      env->currentModuleName,
                      (char const *)dependencyStack->elements[idx]);
        for (size_t rptIdx = idx + 1; rptIdx < dependencyStack->size;
             rptIdx++) {
          reportMessage(report, "\tmodule '%s', which imports\n",
                        (char const *)dependencyStack->elements[rptIdx]);
        }
        reportMessage(report, "\tthe current module\n");
        nodeDestroy(idNode);
        return NULL;
      }
    }
    Node *parsed =
        parseDeclFile(report, options, stabs, dependencyStack, miMap, mnMap,
                      decls, moduleLexerInfoMapGet(miMap, idNode->data.id.id),
                      moduleNodeMapGet(mnMap, idNode->data.id.id));
    if (parsed != NULL) {
      moduleAstMapPut(decls, idNode->data.id.id, parsed);
      importedStab = moduleSymbolTableMapGet(&stabs->decls, idNode->data.id.id);
    } else {
      nodeDestroy(idNode);
      return NULL;
    }
  }
  int retVal =
      moduleTableMapPut(&env->imports, idNode->data.id.id, importedStab);
  if (retVal == HM_EEXISTS) {
    switch (optionsGet(options, optionWDuplicateImport)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: module '%s' already imported\n",
                    info->filename, importToken.line, importToken.character,
                    idNode->data.id.id);
        nodeDestroy(idNode);
        return NULL;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: module '%s' already imported\n",
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
                             ModuleSymbolTableMapPair *stabs, Environment *env,
                             LexerInfo *info) {
  TokenInfo importToken;
  lex(&importToken, report, info);
  if (importToken.type != TT_IMPORT) {
    if (!tokenInfoIsLexerError(&importToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected first thing in file to be a "
                  "module declaration, but found %s\n",
                  info->filename, importToken.line, importToken.character,
                  tokenToName(importToken.type));
    }
    tokenInfoUninit(&importToken);
    return NULL;
  }

  Node *idNode = parseAnyId(report, info);
  if (idNode == NULL) return NULL;

  TokenInfo semiToken;
  lex(&semiToken, report, info);
  if (semiToken.type != TT_SEMI) {
    if (!tokenInfoIsLexerError(&semiToken)) {
      reportError(report,
                  "%s:%zu:%zu: error: expected a semicolon to terminate "
                  "the module declaration, but found %s\n",
                  info->filename, semiToken.line, semiToken.character,
                  tokenToName(semiToken.type));
    }
    tokenInfoUninit(&semiToken);
    nodeDestroy(idNode);
    return NULL;
  }

  // find and add the stab in stabs to env
  int retVal = moduleTableMapPut(
      &env->imports, idNode->data.id.id,
      moduleSymbolTableMapGet(&stabs->decls, idNode->data.id.id));
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
                                  ModuleSymbolTableMapPair *stabs,
                                  Environment *env, Stack *dependencyStack,
                                  ModuleLexerInfoMap *miMap,
                                  ModuleNodeMap *mnMap, ModuleAstMap *decls,
                                  LexerInfo *info) {
  NodeList *imports = nodeListCreate();

  TokenInfo peek;
  lex(&peek, report, info);

  while (peek.type == TT_IMPORT) {
    unLex(info, &peek);
    Node *node = parseDeclImport(report, options, stabs, env, dependencyStack,
                                 miMap, mnMap, decls, info);
    if (node == NULL) {
      nodeListDestroy(imports);
      return NULL;
    }

    nodeListInsert(imports, node);

    lex(&peek, report, info);
  }

  unLex(info, &peek);
  return imports;
}
static NodeList *parseCodeImports(Report *report, Options *options,
                                  ModuleSymbolTableMapPair *stabs,
                                  Environment *env, LexerInfo *info) {
  NodeList *imports = nodeListCreate();

  TokenInfo peek;
  lex(&peek, report, info);

  while (peek.type == TT_IMPORT) {
    unLex(info, &peek);
    Node *node = parseCodeImport(report, options, stabs, env, info);
    if (node == NULL) {
      nodeListDestroy(imports);
      return NULL;
    }

    nodeListInsert(imports, node);

    lex(&peek, report, info);
  }

  unLex(info, &peek);
  return imports;
}

static Node *parseBody(Report *report, Options *options, Environment *env,
                       LexerInfo *info) {
  return NULL;  // TODO: write this
}

static NodeList *parseBodies(Report *report, Options *options, Environment *env,
                             LexerInfo *info) {
  NodeList *bodies = nodeListCreate();

  TokenInfo peek;
  lex(&peek, report, info);

  while (peek.type != TT_EOF && peek.type != TT_ERR) {
    unLex(info, &peek);
    Node *node = parseBody(report, options, env, info);
    if (node == NULL) {
      nodeListDestroy(bodies);
      return NULL;
    } else {
      nodeListInsert(bodies, node);
    }
    lex(&peek, report, info);
  }

  tokenInfoUninit(&peek);  // actually not needed
  return bodies;
}
static Node *parseDeclFile(Report *report, Options *options,
                           ModuleSymbolTableMapPair *stabs,
                           Stack *dependencyStack, ModuleLexerInfoMap *miMap,
                           ModuleNodeMap *mnMap, ModuleAstMap *decls,
                           LexerInfo *lexerInfo, Node *module) {
  SymbolTable *currStab = symbolTableCreate();  // env setup
  Environment env;
  environmentInit(&env, currStab, module->data.module.id->data.id.id);

  NodeList *imports =
      parseDeclImports(report, options, stabs, &env, dependencyStack, miMap,
                       mnMap, decls, lexerInfo);
  if (imports == NULL) {
    environmentUninit(&env);
    symbolTableDestroy(currStab);
    nodeDestroy(module);
    return NULL;
  }
  NodeList *bodies = parseBodies(report, options, &env, lexerInfo);
  if (bodies == NULL) {
    nodeListDestroy(imports);
    environmentUninit(&env);
    symbolTableDestroy(currStab);
    nodeDestroy(module);
    return NULL;
  }

  // env cleanup
  moduleSymbolTableMapPut(&stabs->decls, module->data.module.id->data.id.id,
                          currStab);
  environmentUninit(&env);

  return programNodeCreate(module->line, module->character, module, imports,
                           bodies, lexerInfo->filename);
}
static Node *parseCodeFile(Report *report, Options *options,
                           ModuleSymbolTableMapPair *stabs,
                           LexerInfo *lexerInfo) {
  Node *module = parseModule(report, lexerInfo);
  if (module == NULL) {
    return NULL;
  }

  SymbolTable *currStab = symbolTableCreate();  // env setup
  Environment env;
  environmentInit(&env, currStab, module->data.module.id->data.id.id);

  NodeList *imports = parseCodeImports(report, options, stabs, &env, lexerInfo);
  if (imports == NULL) {
    environmentUninit(&env);
    symbolTableDestroy(currStab);
    nodeDestroy(module);
    return NULL;
  }
  NodeList *bodies = parseBodies(report, options, &env, lexerInfo);
  if (bodies == NULL) {
    nodeListDestroy(imports);
    environmentUninit(&env);
    symbolTableDestroy(currStab);
    nodeDestroy(module);
    return NULL;
  }

  // env cleanup
  moduleSymbolTableMapPut(&stabs->codes, module->data.module.id->data.id.id,
                          currStab);
  environmentUninit(&env);

  return programNodeCreate(module->line, module->character, module,
                           nodeListCreate(), nodeListCreate(),
                           lexerInfo->filename);
}

void parse(ModuleAstMapPair *asts, ModuleSymbolTableMapPair *stabs,
           Report *report, Options *options, FileList *files) {
  moduleAstMapPairInit(asts);
  moduleSymbolTableMapPairInit(stabs);

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
      reportError(report, "%s: error: no such file\n",
                  (char *)files->decls.elements[idx]);
    }
    Node *module = parseModule(report, li);
    if (module == NULL) {  // didn't find it, file can't be parsed.
      reportError(report, "%s: error: no module declaration found\n",
                  (char const *)files->decls.elements[idx]);
    } else if (moduleNodeMapPut(&mnMap, module->data.module.id->data.id.id,
                                module) == HM_EEXISTS) {
      reportError(
          report,
          "%s: error: module '%s' has already been declared (in file %s)\n",
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
            parseDeclFile(report, options, stabs, &dependencyStack, &miMap,
                          &mnMap, &asts->decls, miMap.values[idx],
                          moduleNodeMapGet(&mnMap, miMap.keys[idx]));
        if (parsed != NULL) {
          moduleAstMapPut(&asts->decls, miMap.keys[idx], parsed);
        }
      }
    }

    stackUninit(&dependencyStack, nullDtor);
  }
  moduleLexerInfoMapUninit(&miMap);
  moduleNodeMapUninit(&mnMap);

  for (size_t idx = 0; idx < files->codes.size; idx++) {
    Node *parsed =
        parseCodeFile(report, options, stabs,
                      lexerInfoCreate(files->codes.elements[idx],
                                      &kwMap));  // may be null, but don't case
    if (parsed != NULL) {
      Node const *duplicateCheck = moduleAstMapGet(
          &asts->codes,
          parsed->data.program.module->data.module.id->data.id.id);
      if (duplicateCheck != NULL) {
        // exists, error!
        reportError(
            report,
            "%s: error: module '%s' has already been declared (in file %s)\n",
            (char const *)files->codes.elements[idx],
            parsed->data.program.module->data.module.id->data.id.id,
            duplicateCheck->data.program.filename);
      } else {
        moduleAstMapPut(&asts->codes,
                        parsed->data.program.module->data.module.id->data.id.id,
                        parsed);
      }
    }
  }

  keywordMapUninit(&kwMap);
}