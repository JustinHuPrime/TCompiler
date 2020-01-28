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
struct LabelGenerator;
struct OverloadSetElement;

typedef struct AccessVTable {
  void (*dtor)(struct Access *);
  // inserts instructions into code to load the var, produces the operand
  // where the result can be found
  IROperand *(*load)(struct Access const *this, IREntryVector *code,
                     struct TempAllocator *tempAllocator);
  // inserts instructions into code to store to the var, takes an operand to
  // store
  void (*store)(struct Access const *this, IREntryVector *code,
                IROperand *input, struct TempAllocator *tempAllocator);
  // addrof is nullable - will be invalid in non-escaping/non-global accesses
  // gets the address of the var
  IROperand *(*addrof)(struct Access const *this, IREntryVector *code,
                       struct TempAllocator *tempAllocator);
  // getLabel is nullable - will be invalid in non-global accesses
  // gets the label of the var
  char *(*getLabel)(struct Access const *this);
} AccessVTable;
// an abstract access to some value, of some size and some kind
typedef struct Access {
  AccessVTable *vtable;
  size_t size;
  size_t alignment;
  AllocHint kind;
} Access;
void accessDtor(Access *);
IROperand *accessLoad(Access const *, IREntryVector *code,
                      struct TempAllocator *tempAllocator);
void accessStore(Access const *, IREntryVector *code, IROperand *input,
                 struct TempAllocator *tempAllocator);
IROperand *accessAddrof(struct Access const *, IREntryVector *code,
                        struct TempAllocator *tempAllocator);
char *accessGetLabel(Access const *);
// typedef Vector AccessVector;
// AccessVector *accessVectorCreate(void);
// void accessVectorInsert(AccessVector *, Access *);
// void accessVectorDestroy(AccessVector *);

typedef struct FrameVTable {
  void (*dtor)(struct Frame *);
  // adds an argument of the given type - may not be called except in outermost
  // scope
  Access *(*allocArg)(struct Frame *this, struct Type const *type, bool escapes,
                      struct TempAllocator *tempAllocator);
  // adds a local variable of the given type
  Access *(*allocLocal)(struct Frame *this, struct Type const *type,
                        bool escapes, struct TempAllocator *tempAllocator);
  // allocates a place to put the return value (addrof invalid) - may not be
  // called except in outermost scope
  Access *(*allocRetVal)(struct Frame *this, struct Type const *type,
                         struct TempAllocator *tempAllocator);
  // starts a scope
  void (*scopeStart)(struct Frame *this);
  // ends a scope, and generates code for it
  // also called to end the whole function's scope
  IREntryVector *(*scopeEnd)(struct Frame *this, IREntryVector *out,
                             struct TempAllocator *tempAllocator);
  // generates code to call, clean up after, and produce the return value for a
  // function
  // returns NULL if function's return type is void
  IROperand *(*indirectCall)(struct Frame *this, IROperand *who,
                             IROperandVector *actualArgs,
                             struct Type const *functionType,
                             IREntryVector *out, TempAllocator *tempAllocator);
  IROperand *(*directCall)(struct Frame *this, char *who,
                           IROperandVector *actualArgs,
                           struct OverloadSetElement const *function,
                           IREntryVector *out, TempAllocator *tempAllocator);
} FrameVTable;
// an abstract function frame
typedef struct Frame {
  FrameVTable *vtable;
  char *name;
} Frame;
void frameDtor(Frame *);
Access *frameAllocArg(Frame *, struct Type const *type, bool escapes,
                      struct TempAllocator *tempAllocator);
Access *frameAllocLocal(Frame *, struct Type const *type, bool escapes,
                        struct TempAllocator *tempAllocator);
Access *frameAllocRetVal(Frame *, struct Type const *type,
                         struct TempAllocator *tempAllocator);
void frameScopeStart(Frame *);
IREntryVector *frameScopeEnd(Frame *, IREntryVector *out,
                             struct TempAllocator *tempAllocator);
IROperand *frameIndirectCall(Frame *, IROperand *who,
                             IROperandVector *actualArgs,
                             struct Type const *functionType,
                             IREntryVector *out, TempAllocator *tempAllocator);
IROperand *frameDirectCall(Frame *, char *who, IROperandVector *actualArgs,
                           struct OverloadSetElement const *function,
                           IREntryVector *out, TempAllocator *tempAllocator);

typedef struct LabelGeneratorVTable {
  void (*dtor)(struct LabelGenerator *);
  char *(*generateCodeLabel)(struct LabelGenerator *);
  char *(*generateDataLabel)(struct LabelGenerator *);
} LabelGeneratorVTable;
// an abstract label generator
typedef struct LabelGenerator {
  LabelGeneratorVTable *vtable;
} LabelGenerator;
void labelGeneratorDtor(LabelGenerator *);
char *labelGeneratorGenerateCodeLabel(LabelGenerator *);
char *labelGeneratorGenerateDataLabel(LabelGenerator *);

#endif  // TLC_IR_FRAME_H_