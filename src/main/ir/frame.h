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

// abstract function call frame object

#ifndef TLC_IR_FRAME_H_
#define TLC_IR_FRAME_H_

#include "util/container/vector.h"

struct IROperand;
struct IREntry;
typedef Vector IRVector;
typedef Vector TypeVector;

struct Frame;
struct Access;

typedef struct AccessVTable {
  void (*dtor)(struct Access *);
  struct IROperand *(*load)(struct Access *this, IRVector *code);
  void (*store)(struct Access *this, IRVector *code, struct IROperand *input);
  struct IROperand *(*addrof)(struct Access *this, IRVector *code);
  // addrof is nullable - will be invalid in non-escaping/non-global accesses
} AccessVTable;
typedef struct Access {
  AccessVTable *vtable;
} Access;

typedef Vector AccessVector;
AccessVector *accessVectorCreate(void);
void accessVectorInsert(AccessVector *, Access *);
void accessVectorDestroy(AccessVector *);

typedef struct FrameVTable {
  void (*dtor)(struct Frame *);
  AccessVector *(*allocArgs)(struct Frame *this, TypeVector *types);
} FrameVTable;
typedef struct Frame {
  FrameVTable *vtable;
} Frame;

#endif  // TLC_IR_FRAME_H_