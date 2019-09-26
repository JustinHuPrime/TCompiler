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
#include "ir/frame.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
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

static void translateFunction(Node *function, FragmentVector *out) {}
static void translateGlobal(Node *global, FragmentVector *out) {}
static FragmentVector *translateModule(Node *module,
                                       LabelGenerator *labelGenerator,
                                       FrameCtor frameCtor,
                                       GlobalAccessCtor globalAccessCtor) {
  FragmentVector *fragments = fragmentVectorCreate();
  NodeList *bodies = module->data.file.bodies;

  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    switch (body->type) {
      case NT_FUNCTION: {
        translateFunction(body, fragments);
        break;
      }
      case NT_VARDECL: {
        translateGlobal(body, fragments);
        break;
      }
      default: {
        break;  // no code generated from this
      }
    }
  }

  labelGenerator->vtable->dtor(labelGenerator);

  return fragments;
}

void translate(FileFragmentVectorMap *fragmentMap, ModuleAstMap *codes,
               LabelGeneratorCtor labelGeneratorCtor, FrameCtor frameCtor,
               GlobalAccessCtor globalAccessCtor) {
  fileFragmentVectorMapInit(fragmentMap);

  for (size_t idx = 0; idx < codes->capacity; idx++) {
    if (codes->keys[idx] != NULL) {
      Node *module = codes->values[idx];
      FragmentVector *fragments = translateModule(module, labelGeneratorCtor(),
                                                  frameCtor, globalAccessCtor);
      fileFragmentVectorMapPut(fragmentMap, module->data.file.filename,
                               fragments);
    }
  }
}