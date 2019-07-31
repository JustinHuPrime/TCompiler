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

// A "polymorphic" IR node, and lists of nodes

#ifndef TLC_IR_IR_H_
#define TLC_IR_IR_H_

#include "util/container/vector.h"

#include <stdint.h>

struct IRExp;
struct IRStm;

typedef Vector IRStmVector;
IRStmVector *irStmVectorCreate(void);
void irStmVectorInit(IRStmVector *);
void irStmVectorInsert(IRStmVector *, struct IRStm *);
void irStmVectorUninit(IRStmVector *);
void irStmVectorDestroy(IRStmVector *);

typedef enum {
  IS_MOVE,
} IRStmKind;
typedef struct IRStm {
  IRStmKind kind;
  union {
    struct {
      struct IRExp *to;
      struct IRExp *from;
    } move;
  } data;
} IRStm;
IRStm *moveIRStmCreate(struct IRExp *to, struct IRExp *from);
void irStmDestroy(IRStm *);

typedef Vector IRExpVector;
IRExpVector *irExpVectorCreate(void);
void irExpVectorInit(IRExpVector *);
void irExpVectorInsert(IRExpVector *, struct IRExp *);
void irExpVectorUninit(IRExpVector *);
void irExpVectorDestroy(IRExpVector *);

typedef enum {
  IE_BYTE_CONST,
} IRExpKind;
typedef struct IRExp {
  IRExpKind kind;
  union {
    struct {
      uint8_t value;
    } byteConst;
    struct {
      uint16_t value;
    } shortConst;
    struct {
      uint32_t value;
    } intConst;
    struct {
      uint64_t value;
    } longConst;
  } data;
} IRExp;
IRExp *byteConstIRExpCreate(uint8_t value);
void irExpDestroy(IRExp *);

typedef enum {
  FK_DATA,
  FK_RO_DATA,
  FK_BSS_DATA,
  FK_FUNCTION,
} FragmentKind;
typedef struct {
  FragmentKind kind;
  union {
    struct {
      char *label;
      IRExpVector data;
    } data;
    struct {
      char *label;
      IRExpVector data;
    } roData;
    struct {
      char *label;
      size_t nBytes;
    } bssData;
    struct {
      char *label;
      IRStmVector body;
    } function;
  } data;
} Fragment;
Fragment *dataFragmentCreate(char *label);
Fragment *roDataFragmentCreate(char *label);
Fragment *bssDataFragmentCreate(char *label, size_t nBytes);
Fragment *functionFragmentCreate(char *label);
void fragmentDestroy(Fragment *);

typedef Vector FragmentVector;
FragmentVector *fragmentVectorCreate(void);
void fragmentVectorInsert(FragmentVector *, Fragment *);
void fragmentVectorDestroy(FragmentVector *);

#endif  // TLC_IR_IR_H_