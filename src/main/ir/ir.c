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

// Implementation of the IR

#include "ir/ir.h"

#include "util/tstring.h"

#include <stdlib.h>

AccessVector *accessVectorCreate(void) { return vectorCreate(); }
void accessVectorInsert(AccessVector *vector, struct Access *access) {
  vectorInsert(vector, access);
}
static void accessDtorCaller(Access *access) { access->vtable->dtor(access); }
void accessVectorDestroy(AccessVector *vector) {
  vectorDestroy(vector, (void (*)(void *))accessDtorCaller);
}

void tempGeneratorInit(TempGenerator *generator) { generator->nextTemp = 0; }
size_t tempGeneratorGenerate(TempGenerator *generator) {
  return generator->nextTemp++;
}
void tempGeneratorUninit(TempGenerator *generator) {}

IRStmVector *irStmVectorCreate(void) { return vectorCreate(); }
void irStmVectorInit(IRStmVector *v) { vectorInit(v); }
void irStmVectorInsert(IRStmVector *v, struct IRStm *s) { vectorInsert(v, s); }
void irStmVectorUninit(IRStmVector *v) {
  vectorUninit(v, (void (*)(void *))irStmDestroy);
}
void irStmVectorDestroy(IRStmVector *v) {
  vectorDestroy(v, (void (*)(void *))irStmDestroy);
}

static IRStm *irStmCreate(IRStmKind kind) {
  IRStm *s = malloc(sizeof(IRStm));
  s->kind = kind;
  return s;
}
IRStm *moveIRStmCreate(struct IRExp *to, struct IRExp *from, size_t size) {
  IRStm *s = irStmCreate(IS_MOVE);
  s->data.move.to = to;
  s->data.move.from = from;
  s->data.move.size = size;
  return s;
}
IRStm *labelIRStmCreate(char *name) {
  IRStm *s = irStmCreate(IS_LABEL);
  s->data.label.name = name;
  return s;
}
IRStm *jumpIRStmCreate(char const *target) {
  IRStm *s = irStmCreate(IS_JUMP);
  s->data.jump.target = target;
  return s;
}
IRStm *expIRStmCreate(struct IRExp *exp) {
  IRStm *s = irStmCreate(IS_EXP);
  s->data.exp.exp = exp;
  return s;
}
IRStm *asmIRStmCreate(char *assembly) {
  IRStm *s = irStmCreate(IS_ASM);
  s->data.assembly.assembly = assembly;
  return s;
}
void irStmDestroy(IRStm *s) {
  switch (s->kind) {
    case IS_MOVE: {
      irExpDestroy(s->data.move.to);
      irExpDestroy(s->data.move.from);
      break;
    }
    case IS_LABEL: {
      free(s->data.label.name);
      break;
    }
    case IS_JUMP: {
      break;
    }
    case IS_EXP: {
      irExpDestroy(s->data.exp.exp);
      break;
    }
    case IS_ASM: {
      free(s->data.assembly.assembly);
      break;
    }
    case IS_CJUMP: {
      irExpDestroy(s->data.cjump.lhs);
      irExpDestroy(s->data.cjump.rhs);
      break;
    }
  }
  free(s);
}

IRExpVector *irExpVectorCreate(void) { return vectorCreate(); }
void irExpVectorInit(IRExpVector *v) { vectorInit(v); }
void irExpVectorInsert(IRExpVector *v, struct IRExp *e) { vectorInsert(v, e); }
void irExpVectorUninit(IRExpVector *v) {
  vectorUninit(v, (void (*)(void *))irExpDestroy);
}
void irExpVectorDestroy(IRExpVector *v) {
  vectorDestroy(v, (void (*)(void *))irExpDestroy);
}

static uint8_t punSignedToUnsigned8(int8_t value) {
  union {
    uint8_t u;
    int8_t s;
  } u;
  u.s = value;
  return u.u;
}
static uint16_t punSignedToUnsigned16(int16_t value) {
  union {
    uint16_t u;
    int16_t s;
  } u;
  u.s = value;
  return u.u;
}
static uint32_t punSignedToUnsigned32(int32_t value) {
  union {
    uint32_t u;
    int32_t s;
  } u;
  u.s = value;
  return u.u;
}
static uint64_t punSignedToUnsigned64(int64_t value) {
  union {
    uint64_t u;
    int64_t s;
  } u;
  u.s = value;
  return u.u;
}
static IRExp *irExpCreate(IRExpKind kind) {
  IRExp *e = malloc(sizeof(IRExp));
  e->kind = kind;
  return e;
}
IRExp *byteConstIRExpCreate(int8_t value) {
  IRExp *e = irExpCreate(IE_BYTE_CONST);
  e->data.byteConst.value = punSignedToUnsigned8(value);
  return e;
}
IRExp *ubyteConstIRExpCreate(uint8_t value) {
  IRExp *e = irExpCreate(IE_BYTE_CONST);
  e->data.byteConst.value = value;
  return e;
}
IRExp *shortConstIRExpCreate(int16_t value) {
  IRExp *e = irExpCreate(IE_SHORT_CONST);
  e->data.shortConst.value = punSignedToUnsigned16(value);
  return e;
}
IRExp *ushortConstIRExpCreate(uint16_t value) {
  IRExp *e = irExpCreate(IE_SHORT_CONST);
  e->data.shortConst.value = value;
  return e;
}
IRExp *intConstIRExpCreate(int32_t value) {
  IRExp *e = irExpCreate(IE_INT_CONST);
  e->data.intConst.value = punSignedToUnsigned32(value);
  return e;
}
IRExp *uintConstIRExpCreate(uint32_t value) {
  IRExp *e = irExpCreate(IE_INT_CONST);
  e->data.intConst.value = value;
  return e;
}
IRExp *longConstIRExpCreate(int64_t value) {
  IRExp *e = irExpCreate(IE_LONG_CONST);
  e->data.longConst.value = punSignedToUnsigned64(value);
  return e;
}
IRExp *ulongConstIRExpCreate(uint64_t value) {
  IRExp *e = irExpCreate(IE_LONG_CONST);
  e->data.longConst.value = value;
  return e;
}
IRExp *floatConstIRExpCreate(uint32_t bits) {
  return uintConstIRExpCreate(bits);
}
IRExp *doubleConstIRExpCreate(uint64_t bits) {
  return ulongConstIRExpCreate(bits);
}
IRExp *stringConstIRExpCreate(uint8_t *string) {
  IRExp *e = irExpCreate(IE_STRING_CONST);
  e->data.stringConst.value = string;
  e->data.stringConst.nbytes = (tstrlen(string) + 1) * sizeof(uint8_t);
  return e;
}
IRExp *wstringConstIRExpCreate(uint32_t *string) {
  IRExp *e = irExpCreate(IE_WSTRING_CONST);
  e->data.wstringConst.value = string;
  e->data.wstringConst.nbytes = (twstrlen(string) + 1) * sizeof(uint32_t);
  return e;
}
IRExp *nameIRExpCreate(char const *label) {
  IRExp *e = irExpCreate(IE_NAME);
  e->data.name.label = label;
  return e;
}
IRExp *regIRExpCreate(size_t n, size_t size) {
  IRExp *e = irExpCreate(IE_REG);
  e->data.reg.n = n;
  e->data.reg.size = size;
  return e;
}
IRExp *tempIRExpCreate(size_t n, size_t size) {
  IRExp *e = irExpCreate(IE_TEMP);
  e->data.temp.n = n;
  e->data.temp.size = size;
  return e;
}
IRExp *memTempIRExpCreate(size_t n, size_t size) {
  IRExp *e = irExpCreate(IE_MEM_TEMP);
  e->data.memTemp.n = n;
  e->data.memTemp.size = size;
  return e;
}
IRExp *memTempOffsetIRExpCreate(IRExp *target, size_t offset) {
  IRExp *e = irExpCreate(IE_MEM_TEMP_OFFSET);
  e->data.memTempOffset.target = target;
  e->data.memTempOffset.offset = offset;
  return e;
}
IRExp *eseqStmCreate(IRExp *value) {
  IRExp *e = irExpCreate(IE_ESEQ);
  irStmVectorInit(&e->data.eseq.stms);
  e->data.eseq.value = value;
  return e;
}
void irExpDestroy(IRExp *e) {
  switch (e->kind) {
    case IE_BYTE_CONST:
    case IE_SHORT_CONST:
    case IE_INT_CONST:
    case IE_LONG_CONST:
    case IE_NAME:
    case IE_REG:
    case IE_TEMP:
    case IE_MEM_TEMP: {
      break;
    }
    case IE_MEM_TEMP_OFFSET: {
      irExpDestroy(e->data.memTempOffset.target);
      break;
    }
    case IE_STRING_CONST: {
      free(e->data.stringConst.value);
      break;
    }
    case IE_WSTRING_CONST: {
      free(e->data.wstringConst.value);
      break;
    }
    case IE_CALL: {
      irExpDestroy(e->data.call.who);
      irExpVectorUninit(&e->data.call.withWhat);
      break;
    }
    case IE_UNOP: {
      irExpDestroy(e->data.unOp.target);
      break;
    }
    case IE_BINOP: {
      irExpDestroy(e->data.binOp.lhs);
      irExpDestroy(e->data.binOp.rhs);
      break;
    }
    case IE_ESEQ: {
      irStmVectorUninit(&e->data.eseq.stms);
      irExpDestroy(e->data.eseq.value);
      break;
    }
  }
  free(e);
}

static Fragment *fragmentCreate(FragmentKind kind) {
  Fragment *f = malloc(sizeof(Fragment));
  f->kind = kind;
  return f;
}
Fragment *dataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_DATA);
  f->data.data.label = label;
  irExpVectorInit(&f->data.data.data);
  return f;
}
Fragment *roDataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_RO_DATA);
  f->data.roData.label = label;
  irExpVectorInit(&f->data.roData.data);
  return f;
}
Fragment *bssDataFragmentCreate(char *label, size_t nBytes) {
  Fragment *f = fragmentCreate(FK_BSS_DATA);
  f->data.bssData.label = label;
  f->data.bssData.nBytes = nBytes;
  return f;
}
Fragment *functionFragmentCreate(char *label, Frame *frame) {
  Fragment *f = fragmentCreate(FK_FUNCTION);
  f->data.function.label = label;
  f->data.function.frame = frame;
  irStmVectorInit(&f->data.function.body);
  return f;
}
void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_DATA: {
      free(f->data.data.label);
      irExpVectorUninit(&f->data.data.data);
      break;
    }
    case FK_RO_DATA: {
      free(f->data.roData.label);
      irExpVectorUninit(&f->data.roData.data);
      break;
    }
    case FK_BSS_DATA: {
      free(f->data.bssData.label);
      break;
    }
    case FK_FUNCTION: {
      free(f->data.function.label);
      f->data.function.frame->vtable->dtor(f->data.function.frame);
      irStmVectorUninit(&f->data.function.body);
      break;
    }
  }
  free(f);
}

FragmentVector *fragmentVectorCreate(void) { return vectorCreate(); }
void fragmentVectorInsert(FragmentVector *v, Fragment *f) {
  vectorInsert(v, f);
}
void fragmentVectorDestroy(FragmentVector *v) {
  vectorDestroy(v, (void (*)(void *))fragmentDestroy);
}