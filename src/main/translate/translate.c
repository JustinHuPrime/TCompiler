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

// implemetation of the translation phase

#include "translate/translate.h"

#include "util/container/stringBuilder.h"
#include "util/format.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

// data structures
void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *file) {
  return hashMapGet(map, file);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char *file,
                             FragmentVector *vector) {
  return hashMapPut(map, file, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
      fragmentVectorDestroy(map->values[idx]);
    }
  }
  free(map->values);
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
      free((char *)(map->keys[idx]));
#pragma GCC diagnostic pop
    }
  }
  free(map->keys);
}

// helpers
static char *mangleModuleName(char const *moduleName) {
  char *buffer = strcpy(malloc(4), "__Z");
  StringVector *exploded = explodeName(moduleName);
  for (size_t idx = 0; idx < exploded->size; idx++) {
    buffer = format("%s%zu%s", buffer, strlen(exploded->elements[idx]),
                    (char const *)exploded->elements[idx]);
  }
  stringVectorDestroy(exploded, true);
  return buffer;
}
static char *mangleTypeString(TypeVector const *args) {
  return NULL;  // TODO: write this
}
static char *mangleVarName(char const *moduleName, Node const *id) {
  return format("%s%zu%s", mangleModuleName(moduleName), strlen(id->data.id.id),
                id->data.id.id);
}
static char *mangleFunctionName(char const *moduleName, Node const *id) {
  return format("%s%zu%s%s", mangleModuleName(moduleName),
                strlen(id->data.id.id), id->data.id.id,
                mangleTypeString(&id->data.id.overload->argumentTypes));
}

// top level stuff
static void translateGlobalVar(Node *varDecl, FragmentVector *fragments,
                               char const *moduleName) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *id = idValuePairs->firstElements[idx];
    Node *initializer = idValuePairs->secondElements[idx];
    char *mangledName = mangleVarName(moduleName, id);
    Fragment *f = dataFragmentCreate(mangledName);
  }

  return;
  // TODO: write this
}
static void translateFunction(Node *function, FragmentVector *fragments,
                              char const *moduleName) {
  return;
  // TODO: write this
}
static void translateBody(Node *body, FragmentVector *fragments,
                          char const *moduleName) {
  switch (body->type) {
    case NT_VARDECL: {
      translateGlobalVar(body, fragments, moduleName);
      return;
    }
    case NT_FUNCTION: {
      translateFunction(body, fragments, moduleName);
      return;
    }
    default: { return; }
  }
}

// file level stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static FragmentVector *translateFile(Node *file) {
  FragmentVector *fragments = fragmentVectorCreate();

  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    translateBody(bodies->elements[idx], fragments,
                  file->data.module.id->data.id.id);
  }

  return fragments;
}

void translate(FileFragmentVectorMap *fragments, ModuleAstMapPair *asts) {
  fileFragmentVectorMapInit(fragments);
  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      fileFragmentVectorMapPut(
          fragments, codeFilenameToAssembyFilename(file->data.file.filename),
          translateFile(file));
    }
  }
}