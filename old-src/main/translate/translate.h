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

typedef Vector IREntryVector;
struct ModuleAstMapPair;
struct Frame;
struct Access;
struct LabelGenerator;

typedef enum {
  FK_BSS,
  FK_RODATA,
  FK_DATA,
  FK_TEXT,
} FragmentKind;
// a fragment of assembly data or code
typedef struct Fragment {
  FragmentKind kind;
  char *label;
  union {
    struct {
      size_t size;
      size_t alignment;
    } bss;
    struct {
      IREntryVector *ir;
      size_t size;
      size_t alignment;
    } rodata;
    struct {
      IREntryVector *ir;
      size_t size;
      size_t alignment;
    } data;
    struct {
      struct Frame *frame;
      TempAllocator *tempAllocator;
      IREntryVector *ir;
    } text;
  } data;
} Fragment;
// ctors
Fragment *bssFragmentCreate(char *label, size_t size, size_t alignment);
Fragment *rodataFragmentCreate(char *label, size_t size, size_t alignment);
Fragment *dataFragmentCreate(char *label, size_t size, size_t alignment);
Fragment *textFragmentCreate(char *label, struct Frame *frame,
                             TempAllocator *tempAllocator);
// dtor
void fragmentDestroy(Fragment *);

// vector of fragments
typedef Vector FragmentVector;
// ctor
FragmentVector *fragmentVectorCreate(void);
// in-place ctor
void fragmentVectorInit(FragmentVector *);
// insert
void fragmentVectorInsert(FragmentVector *, Fragment *);
// in-place dtor
void fragmentVectorUninit(FragmentVector *);
// dtor
void fragmentVectorDestroy(FragmentVector *);

// data used by a file
typedef struct {
  FragmentVector fragments;
  char *filename;
  char *sourceFilename;
  struct LabelGenerator *labelGenerator;
} IRFile;
// ctor
IRFile *irFileCreate(char *sourceFilename, char *filename,
                     struct LabelGenerator *labelGenerator);
void irFileDestroy(IRFile *);

// associates the fragments in a file with the file
typedef HashMap FileIRFileMap;
// in-place ctor
void fileIRFileMapInit(FileIRFileMap *);
// get
IRFile *fileIRFileMapGet(FileIRFileMap *, char const *key);
// put
int fileIRFileMapPut(FileIRFileMap *, char const *key, IRFile *file);
// in-place dtor
void fileIRFileMapUninit(FileIRFileMap *);

typedef struct LabelGenerator *(*LabelGeneratorCtor)(void);
typedef struct Frame *(*FrameCtor)(char *name);
typedef struct Access *(*GlobalAccessCtor)(size_t size, size_t alignment,
                                           AllocHint kind, char *name);
typedef struct Access *(*FunctionAccessCtor)(char *name);

// translates an abstract syntax tree into a series of fragments
void translate(FileIRFileMap *fileMap, struct ModuleAstMapPair *asts,
               LabelGeneratorCtor labelGenerator, FrameCtor frameCtor,
               GlobalAccessCtor globalAccessCtor,
               FunctionAccessCtor functionAccessCtor);

#endif  // TLC_TRANSLATE_TRANSLATE_H_