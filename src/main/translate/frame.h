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

// abstract frames used in the translate phase

#ifndef TLC_TRANSLATE_FRAME_H_
#define TLC_TRANSLATE_FRAME_H_

#include "ir/ir.h"

#include <stddef.h>

struct Frame;
struct Access;
struct LabelGenerator;

typedef Vector AccessVector;
AccessVector *accessVectorCreate(void);
void accessVectorInsert(AccessVector *, struct Access *);
void accessVectorDestroy(AccessVector *);

typedef struct {
  void (*dtor)(struct Frame *);
  IRStmVector *(*generateEntryExit)(struct Frame *this, IRStmVector *body,
                                    char *exitLabel);
  IRExp *(*fpExp)(void);
  struct Access *(*allocLocal)(struct Frame *this, size_t size, bool escapes);
  struct Access *(*allocOutArg)(struct Frame *this, size_t size);
  struct Access *(*allocInArg)(struct Frame *this, size_t size, bool escapes);
} FrameVTable;

typedef struct Frame {
  FrameVTable *vtable;
  // other stuff, depending on implementation
} Frame;
typedef Frame *(*FrameCtor)(void);

typedef struct {
  void (*dtor)(struct Access *);
  IRExp *(*valueExp)(struct Access *this, IRExp *fp);
  IRExp *(*addressExp)(struct Access *this, IRExp *fp);
} AccessVTable;

typedef struct Access {
  AccessVTable *vtable;
  // other stuff, depending on implementation
} Access;
typedef Access *(*GlobalAccessCtor)(char *label);

typedef struct {
  void (*dtor)(struct LabelGenerator *);
  char *(*generateDataLabel)(struct LabelGenerator *this);
  char *(*generateCodeLabel)(struct LabelGenerator *this);
} LabelGeneratorVTable;

typedef struct LabelGenerator {
  LabelGeneratorVTable *vtable;
  // other stuff, depending on implementation
} LabelGenerator;
typedef LabelGenerator *(*LabelGeneratorCtor)(void);

typedef struct {
  size_t nextTemp;
} TempGenerator;
void tempGeneratorInit(TempGenerator *);
size_t tempGeneratorGenerate(TempGenerator *);
void tempGeneratorUninit(TempGenerator *);

#endif  // TLC_TRANSLATE_FRAME_H_