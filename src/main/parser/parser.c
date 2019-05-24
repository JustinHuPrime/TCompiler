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
#include "util/container/hashSet.h"
#include "util/format.h"
#include "util/functional.h"

#include <stdlib.h>

typedef HashMap ModuleLexerInfoMap;
static ModuleLexerInfoMap *moduleLexerInfoMapCreate(void) {
  return hashMapCreate();
}
static LexerInfo *moduleLexerInfoMapGet(ModuleLexerInfoMap const *map,
                                        char const *key) {
  return hashMapGet(map, key);
}
static int moduleLexerInfoMapPut(ModuleLexerInfoMap *map, char const *key,
                                 LexerInfo *value) {
  return hashMapPut(map, key, value, (void (*)(void *))lexerInfoDestroy);
}
static void moduleLexerInfoMapDestroy(ModuleLexerInfoMap *map) {
  hashMapDestroy(map, (void (*)(void *))lexerInfoDestroy);
}

typedef HashMap ModuleFileMap;
static ModuleFileMap *moduleFileMapCreate(void) { return hashMapCreate(); }
static char const *moduleFileMapGet(ModuleFileMap const *map, char const *key) {
  return hashMapGet(map, key);
}
static int moduleFileMapPut(ModuleFileMap *map, char const *key,
                            char const *value) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
  return hashMapPut(map, key, (void *)value, nullDtor);
#pragma GCC diagnostic pop
}
static void moduleFileMapDestroy(ModuleFileMap *map) {
  hashMapDestroy(map, nullDtor);
}

typedef HashMap ModuleNodeMap;
static ModuleNodeMap *moduleNodeMapCreate(void) { return hashMapCreate(); }
static Node *moduleNodeMapGet(ModuleNodeMap const *map, char const *key) {
  return hashMapGet(map, key);
}
static int moduleNodeMapPut(ModuleNodeMap *map, char const *key, Node *value) {
  return hashMapPut(map, key, value, (void (*)(void *))nodeDestroy);
}
static void moduleNodeMapDestroy(ModuleNodeMap *map) {
  hashMapDestroy(map, nullDtor);
}

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
  moduleAstMapInit(&pair->decls);
  moduleAstMapInit(&pair->codes);
  return pair;
}
void moduleAstMapPairDestroy(ModuleAstMapPair *pair) {
  moduleAstMapUninit(&pair->decls);
  moduleAstMapUninit(&pair->codes);
  free(pair);
}

static Node *parseAnyId(Report *report, LexerInfo *li) {
  TokenInfo info;
  TokenType type;
  type = lex(report, li, &info);
  if (type != TT_ID && type != TT_SCOPED_ID) {
    tokenInfoUninit(type, &info);
    reportError(
        report,
        format("%s:%zu:%zu: error: expected an identifier, but found %s",
               li->fileName, info.line, info.character, tokenToName(type)));
    return NULL;
  } else {
    return idNodeCreate(info.line, info.character, info.data.string);
  }
}
static Node *parseUnscopedId(Report *report, LexerInfo *li) {
  TokenInfo info;
  TokenType type;
  type = lex(report, li, &info);
  if (type != TT_ID) {
    tokenInfoUninit(type, &info);
    reportError(
        report,
        format(
            "%s:%zu:%zu: error: expected an unscoped identifier, but found %s",
            li->fileName, info.line, info.character, tokenToName(type)));
    return NULL;
  } else {
    return idNodeCreate(info.line, info.character, info.data.string);
  }
}

static Node *parseModule(Report *report, LexerInfo *li) {
  TokenInfo info;
  TokenType type;

  type = lex(report, li, &info);
  if (type != TT_MODULE) {
    tokenInfoUninit(type, &info);
    if (!lexerError(type)) {
      reportError(
          report,
          format("%s:%zu:%zu: error: expected first thing in "
                 "file to be a 'module' declaration, but found %s",
                 li->fileName, info.line, info.character, tokenToName(type)));
    }
    return NULL;
  } else {
    Node *idNode = parseAnyId(report, li);
    if (idNode == NULL) {
      tokenInfoUninit(type, &info);
      return NULL;
    } else {
      return moduleNodeCreate(info.line, info.character, idNode);
    }
  }
}
static Node *parseFile(Report *report, Options *options, HashSet *stalled,
                       LexerInfo *lexerInfo, Node *module) {
  return programNodeCreate(module->line, module->character, module,
                           nodeListCreate(), nodeListCreate());
  // FIXME: this is only a stub.
}

ModuleAstMapPair *parse(Report *report, Options *options, FileList *files) {
  KeywordMap *kwMap = keywordMapCreate();

  ModuleLexerInfoMap *miMap = moduleLexerInfoMapCreate();
  ModuleNodeMap *mnMap = moduleNodeMapCreate();

  // for each decl file, read the module line, and add an entry to a
  // module-lexerinfo hashMap
  for (size_t idx = 0; idx < files->decls.size; idx++) {
    LexerInfo *li = lexerInfoCreate(files->decls.elements[idx], kwMap);
    Node *module = parseModule(report, li);
    if (module == NULL) {  // didn't find it, file can't be parsed.
      reportError(report, format("%s: error: no module declaration found",
                                 (char const *)files->decls.elements[idx]));
    } else if (moduleNodeMapPut(mnMap, module->data.module.id->data.id.id,
                                module) == HM_EEXISTS) {
      reportError(
          report,
          format(
              "%s: error: module '%s' has already been declared (in file %s)",
              (char const *)files->decls.elements[idx],
              module->data.module.id->data.id.id,
              moduleLexerInfoMapGet(miMap, module->data.module.id->data.id.id)
                  ->fileName));
      nodeDestroy(module);
    } else {
      moduleLexerInfoMapPut(miMap, module->data.module.id->data.id.id, li);
    }
  }

  ModuleAstMapPair *parseResult = moduleAstMapPairCreate();

  // parse all decl files in order
  while (parseResult->decls.size < miMap->size) {
    HashSet *stalled = hashSetCreate();

    // select one to parse
    for (size_t idx = 0; idx < miMap->capacity; idx++) {
      // if this is a module, and it hasn't been parsed
      if (miMap->keys[idx] != NULL &&
          moduleAstMapGet(&parseResult->decls, miMap->keys[idx]) == NULL) {
        parseFile(report, options, stalled, miMap->values[idx],
                  moduleNodeMapGet(mnMap, miMap->keys[idx]));
        // TODO: also needs parseResult->decls, and map b/w module and exported
        // types
      }
    }

    hashSetDestroy(stalled, false);
  }
  moduleLexerInfoMapDestroy(miMap);
  moduleNodeMapDestroy(mnMap);

  // TODO: parse the codes files

  keywordMapDestroy(kwMap);

  return parseResult;
}