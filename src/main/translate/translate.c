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

// Implementation of translation

#include "translate/translate.h"
#include "ir/ir.h"
#include "parser/parser.h"

#include <stdlib.h>

void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_BSS: {
      break;
    }
    case FK_RODATA: {
      fragmentVectorDestroy(f->data.rodata.ir);
      break;
    }
    case FK_DATA: {
      fragmentVectorDestroy(f->data.data.ir);
      break;
    }
    case FK_TEXT: {
      fragmentVectorDestroy(f->data.text.ir);
      break;
    }
  }
  free(f->label);
  free(f);
}

FragmentVector *fragmentVectorCreate(void) { return vectorCreate(); }
void fragmentVectorInsert(FragmentVector *v, Fragment *f) {
  vectorInsert(v, f);
}
void fragmentVectorDestroy(FragmentVector *v) {
  vectorDestroy(v, (void (*)(void *))fragmentDestroy);
}

void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *key) {
  return hashMapGet(map, key);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char const *key,
                             FragmentVector *vector) {
  return hashMapPut(map, key, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  hashMapUninit(map, (void (*)(void *))fragmentVectorDestroy);
}

void translate(FileFragmentVectorMap *fragments, ModuleAstMap *codes) {
  fileFragmentVectorMapInit(fragments);
}