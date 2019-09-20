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

#include "ir/ir.h"

typedef Vector TypeVector;
struct Type;

struct Frame;
struct Access;

typedef struct AccessVTable {
  void (*dtor)(struct Access *);
  // inserts instructions into code to load the var, produces the operand
  // where the result can be found
  IROperand *(*load)(struct Access *this, IRVector *code,
                     struct TempAllocator *tempAllocator);
  // inserts instructions into code to store to the var, takes an operand to
  // store
  void (*store)(struct Access *this, IRVector *code, IROperand *input,
                struct TempAllocator *tempAllocator);
  // addrof is nullable - will be invalid in non-escaping/non-global accesses
  // gets the address of the var
  IROperand *(*addrof)(struct Access *this, IRVector *code,
                       struct TempAllocator *tempAllocator);
} AccessVTable;
// an abstract access to some value, of some size and some kind
typedef struct Access {
  AccessVTable *vtable;
  size_t size;
  AllocHint kind;
} Access;

// typedef Vector AccessVector;
// AccessVector *accessVectorCreate(void);
// void accessVectorInsert(AccessVector *, Access *);
// void accessVectorDestroy(AccessVector *);

typedef struct FrameVTable {
  void (*dtor)(struct Frame *);
  // adds an argument of the given type
  Access *(*allocArg)(struct Frame *this, struct Type const *type, bool escapes,
                      struct TempAllocator *tempAllocator);
  // adds a local variable of the given type
  Access *(*allocLocal)(struct Frame *this, struct Type const *type,
                        bool escapes, struct TempAllocator *tempAllocator);
  // allocates a place to put the return value (addrof invalid)
  Access *(*allocRetVal)(struct Frame *this, struct Type const *type,
                         struct TempAllocator *tempAllocator);
  // adds instructions to out to form the whole function body
  void (*wrapBody)(struct Frame *this, IRVector *out);
} FrameVTable;
// an abstract function frame
typedef struct Frame {
  FrameVTable *vtable;
} Frame;

#endif  // TLC_IR_FRAME_H_