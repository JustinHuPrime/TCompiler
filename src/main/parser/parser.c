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

typedef HashMap FileLexerInfoMap;
static FileLexerInfoMap *fileLexerInfoMapCreate(void) {
  return hashMapCreate();
}
static LexerInfo *fileLexerInfoMapGet(FileLexerInfoMap const *map,
                                      char const *key) {
  return hashMapGet(map, key);
}
static int fileLexerInfoMapPut(FileLexerInfoMap *map, char const *key,
                               LexerInfo *value) {
  return hashMapPut(map, key, value, (void (*)(void *))lexerInfoDestroy);
}
static void fileLexerInfoMapDestroy(FileLexerInfoMap *map) {
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

static char *readModule(LexerInfo *li) {}

SOME_TYPE parse(Report *report, Options *options, FileList *files) {
  KeywordMap *kwMap = keywordMapCreate();
  FileLexerInfoMap *fliMap = fileLexerInfoMapCreate();

  // for each decl file, read the module line, and add an entry to a module-file
  // hashMap
  ModuleFileMap *mfMap = moduleFileMapCreate();
  for (size_t idx = 0; idx < files->decls->size; idx++) {
    LexerInfo *li = lexerInfoCreate(files->decls->elements[idx], kwMap);
    fileLexerInfoMapPut(fliMap, files->decls->elements[idx], li);
    char *moduleName = readModule(li);
    if (moduleName == NULL) {
      // something...
      
    } else if (moduleFileMapPut(mfMap, moduleName,
                                files->decls->elements[idx]) == HM_EEXISTS) {
      free(moduleName);
    }
  }

  moduleFileMapDestroy(mfMap);
  fileLexerInfoMapDestroy(fliMap);
  keywordMapDestroy(kwMap);
}