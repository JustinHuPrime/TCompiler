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
struct Frame;
struct Access;
struct LabelGenerator;

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
  IS_CJUMP,
} IRStmKind;
typedef enum {
  CJ_EQ,
  CJ_NEQ,
  CJ_LT,
  CJ_GT,
  CJ_LTEQ,
  CJ_GTEQ,
} CJumpType;
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
    struct {
      CJumpType compOp;
      struct IRExp *lhs;
      struct IRExp *rhs;
      char const *trueTarget;
      char const *falseTarget;
    } cjump;
  } data;
} IRStm;
IRStm *moveIRStmCreate(struct IRExp *to, struct IRExp *from, size_t size);
IRStm *labelIRStmCreate(char *name);
IRStm *jumpIRStmCreate(char const *target);
IRStm *expIRStmCreate(struct IRExp *exp);
IRStm *asmIRStmCreate(char *assembly);
IRStm *cjumpIRStmCreate(CJumpType op, struct IRExp *lhs, struct IRExp *rhs,
                        char const *trueTarget, char const *falseTarget);
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
  IE_MEM_TEMP,
  IE_MEM_TEMP_OFFSET,
  IE_CALL,
  IE_UNOP,
  IE_BINOP,
  IE_ESEQ,
} IRExpKind;
typedef enum {
  IU_ZX_BYTETOSHORT,
  IU_ZX_BYTETOINT,
  IU_ZX_BYTETOLONG,
  IU_SX_BYTETOSHORT,
  IU_SX_BYTETOINT,
  IU_SX_BYTETOLONG,
  IU_ZX_SHORTTOINT,
  IU_ZX_SHORTTOLONG,
  IU_SX_SHORTTOINT,
  IU_SX_SHORTTOLONG,
  IU_ZX_INTTOLONG,
  IU_SX_INTTOLONG,
  IU_UBYTETOFLOAT,
  IU_USHORTTOFLOAT,
  IU_UINTTOFLOAT,
  IU_ULONGTOFLOAT,
  IU_BYTETOFLOAT,
  IU_SHORTTOFLOAT,
  IU_INTTOFLOAT,
  IU_LONGTOFLOAT,
  IU_UBYTETODOUBLE,
  IU_USHORTTODOUBLE,
  IU_UINTTODOUBLE,
  IU_ULONGTODOUBLE,
  IU_BYTETODOUBLE,
  IU_SHORTTODOUBLE,
  IU_INTTODOUBLE,
  IU_LONGTODOUBLE,
} IRUnOpType;
typedef enum {
  IB_BYTEADD,
  IB_SHORTADD,
  IB_INTADD,
  IB_LONGADD,
} IRBinOpType;
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
      size_t n;
      size_t size;
    } memTemp;
    struct {
      struct IRExp *target;
      size_t offset;
    } memTempOffset;
    struct {
      struct IRExp *who;
      IRExpVector withWhat;
    } call;
    struct {
      IRUnOpType op;
      struct IRExp *target;
    } unOp;
    struct {
      IRBinOpType op;
      struct IRExp *lhs;
      struct IRExp *rhs;
    } binOp;
    struct {
      IRStmVector stms;
      struct IRExp *value;
    } eseq;
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
IRExp *memTempIRExpCreate(size_t n, size_t size);
IRExp *memTempOffsetIRExpCreate(IRExp *target, size_t offset);
IRExp *callIRExpCreate(IRExp *who);
IRExp *unopIRExpCreate(IRUnOpType op, IRExp *target);
IRExp *binopIRExpCreate(IRBinOpType op, IRExp *lhs, IRExp *rhs);
IRExp *eseqStmCreate(IRExp *value);
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
      struct Frame *frame;
    } function;
  } data;
} Fragment;
Fragment *dataFragmentCreate(char *label);
Fragment *roDataFragmentCreate(char *label);
Fragment *bssDataFragmentCreate(char *label, size_t nBytes);
Fragment *functionFragmentCreate(char *label, struct Frame *frame);
void fragmentDestroy(Fragment *);

typedef Vector FragmentVector;
FragmentVector *fragmentVectorCreate(void);
void fragmentVectorInsert(FragmentVector *, Fragment *);
void fragmentVectorDestroy(FragmentVector *);

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

#endif  // TLC_IR_IR_H_