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

// file level stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static FragmentVector *translateFile(Node *file) {
  FragmentVector *v = fragmentVectorCreate();
  // TODO: write this
  return v;
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