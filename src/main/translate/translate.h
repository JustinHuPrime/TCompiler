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

// Translation of AST into IR

#ifndef TLC_TRANSLATE_TRANSLATE_H_
#define TLC_TRANSLATE_TRANSLATE_H_

#include "ir/ir.h"

#include "util/container/hashMap.h"
#include "util/container/vector.h"

typedef HashMap ModuleAstMap;
typedef Vector IRVector;
struct Frame;
struct Access;

typedef enum {
  FK_BSS,
  FK_RODATA,
  FK_DATA,
  FK_TEXT,
} FragmentKind;
// a fragment of assembly data or code
typedef struct {
  FragmentKind kind;
  char *label;
  union {
    struct {
      size_t size;
    } bss;
    struct {
      IRVector *ir;
    } rodata;
    struct {
      IRVector *ir;
    } data;
    struct {
      IRVector *ir;
    } text;
  } data;
} Fragment;
// ctors
Fragment *bssFragmentCreate(void);
Fragment *rodataFragmentCreate(void);
Fragment *dataFragmentCreate(void);
Fragment *textFragmentCreate(void);
// dtor
void fragmentDestroy(Fragment *);

// vector of fragments
typedef Vector FragmentVector;
// ctor
FragmentVector *fragmentVectorCreate(void);
// insert
void fragmentVectorInsert(FragmentVector *, Fragment *);
// dtor
void fragmentVectorDestroy(FragmentVector *);

// associates the fragments in a file with the file
typedef HashMap FileFragmentVectorMap;
// in-place ctor
void fileFragmentVectorMapInit(FileFragmentVectorMap *);
// get
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *,
                                         char const *key);
// put
int fileFragmentVectorMapPut(FileFragmentVectorMap *, char const *key,
                             FragmentVector *vector);
// in-place dtor
void fileFragmentVectorMapUninit(FileFragmentVectorMap *);

typedef struct Frame *(*FrameCtor)(void);
typedef struct Access *(*GlobalAccessCtor)(size_t size, AllocHint kind,
                                           char *name);

// translates an abstract syntax tree into a series of fragments
void translate(FileFragmentVectorMap *fragmentMap, ModuleAstMap *codes,
               FrameCtor frameCtor, GlobalAccessCtor globalAccessCtor);

#endif  // TLC_TRANSLATE_TRANSLATE_H_