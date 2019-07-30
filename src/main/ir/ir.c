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

#include <stdlib.h>

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
IRStm *moveIRStmCreate(struct IRExp *to, struct IRExp *from) {
  IRStm *s = irStmCreate(IS_MOVE);
  s->data.move.to = to;
  s->data.move.from = from;
  return s;
}
void irStmDestroy(IRStm *s) {
  switch (s->kind) {
    case IS_MOVE: {
      irExpDestroy(s->data.move.to);
      irExpDestroy(s->data.move.from);
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

static IRExp *irExpCreate(IRExpKind kind) {
  IRExp *e = malloc(sizeof(IRExp));
  e->kind = kind;
  return e;
}
IRExp *byteConstIRExpCreate(uint8_t value) {
  IRExp *e = irExpCreate(IE_BYTE_CONST);
  e->data.byteConst.value = value;
  return e;
}
void irExpDestroy(IRExp *e) {
  switch (e->kind) {
    case IE_BYTE_CONST: {
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
  byteVectorInit(&f->data.data.data);
  return f;
}
Fragment *bssDataFragmentCreate(char *label, size_t nBytes) {
  Fragment *f = fragmentCreate(FK_BSS_DATA);
  f->data.bssData.label = label;
  f->data.bssData.nBytes = nBytes;
  return f;
}
Fragment *functionFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_FUNCTION);
  f->data.function.label = label;
  irStmVectorInit(&f->data.function.body);
  return f;
}
void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_DATA: {
      free(f->data.data.label);
      byteVectorUninit(&f->data.data.data);
      break;
    }
    case FK_BSS_DATA: {
      free(f->data.bssData.label);
      break;
    }
    case FK_FUNCTION: {
      free(f->data.function.label);
      irStmVectorUninit(&f->data.function.body);
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