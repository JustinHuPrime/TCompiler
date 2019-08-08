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
  IS_LABEL,
  IS_JUMP,
  IS_EXP,
  IS_ASM,
} IRStmKind;
typedef struct IRStm {
  IRStmKind kind;
  union {
    struct {
      struct IRExp *to;
      struct IRExp *from;
      size_t size;
    } move;
    struct {
      char *name;
    } label;
    struct {
      char const *target;
    } jump;
    struct {
      struct IRExp *exp;
    } exp;
    struct {
      char *assembly;
    } assembly;
  } data;
} IRStm;
IRStm *moveIRStmCreate(struct IRExp *to, struct IRExp *from, size_t size);
IRStm *labelIRStmCreate(char *name);
IRStm *jumpIRStmCreate(char const *target);
IRStm *expIRStmCreate(struct IRExp *exp);
IRStm *asmIRStmCreate(char *assembly);
void irStmDestroy(IRStm *);

typedef Vector IRExpVector;
IRExpVector *irExpVectorCreate(void);
void irExpVectorInit(IRExpVector *);
void irExpVectorInsert(IRExpVector *, struct IRExp *);
void irExpVectorUninit(IRExpVector *);
void irExpVectorDestroy(IRExpVector *);

typedef enum {
  IE_BYTE_CONST,
  IE_SHORT_CONST,
  IE_INT_CONST,
  IE_LONG_CONST,
  IE_STRING_CONST,
  IE_WSTRING_CONST,
  IE_NAME,
  IE_REG,
  IE_TEMP,
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
    struct {
      uint8_t *value;
      size_t nbytes;
    } stringConst;
    struct {
      uint32_t *value;
      size_t nbytes;
    } wstringConst;
    struct {
      char const *label;
    } name;
    struct {
      size_t n;
      size_t size;
    } reg;
    struct {
      size_t n;
      size_t size;
    } temp;
    struct {
      struct IRExp *who;
      IRExpVector withWhat;
    } call;
    struct {
      struct IRExp *target;
    } unOp;
    struct {
      struct IRExp *lhs;
      struct IRExp *rhs;
    } binOp;
  } data;
} IRExp;
IRExp *byteConstIRExpCreate(int8_t value);
IRExp *ubyteConstIRExpCreate(uint8_t value);
IRExp *shortConstIRExpCreate(int16_t value);
IRExp *ushortConstIRExpCreate(uint16_t value);
IRExp *intConstIRExpCreate(int32_t value);
IRExp *uintConstIRExpCreate(uint32_t value);
IRExp *longConstIRExpCreate(int64_t value);
IRExp *ulongConstIRExpCreate(uint64_t value);
IRExp *floatConstIRExpCreate(uint32_t bits);
IRExp *doubleConstIRExpCreate(uint64_t bits);
IRExp *stringConstIRExpCreate(uint8_t *string);
IRExp *wstringConstIRExpCreate(uint32_t *string);
IRExp *nameIRExpCreate(char const *label);
IRExp *regIRExpCreate(size_t n, size_t size);
IRExp *tempIRExpCreate(size_t n, size_t size);
IRExp *callIRExpCreate(IRExp *who);
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