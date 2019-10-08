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

// Implementation of IR

#include "ir/ir.h"

#include "util/tstring.h"

#include <stdio.h>  // FIXME: debug only
#include <stdlib.h>
#include <string.h>

static uint8_t punByteToUByte(int8_t value) {
  union {
    int8_t s;
    uint8_t u;
  } u;
  u.s = value;
  return u.u;
}
static uint16_t punShortToUShort(int16_t value) {
  union {
    int16_t s;
    uint16_t u;
  } u;
  u.s = value;
  return u.u;
}
static uint32_t punIntToUInt(int32_t value) {
  union {
    int32_t s;
    uint32_t u;
  } u;
  u.s = value;
  return u.u;
}
static uint64_t punLongToULong(int64_t value) {
  union {
    int64_t s;
    uint64_t u;
  } u;
  u.s = value;
  return u.u;
}
static IROperand *irOperandCreate(OperandKind kind) {
  IROperand *o = malloc(sizeof(IROperand));
  o->kind = kind;
  return o;
}
IROperand *tempIROperandCreate(size_t n, size_t size, size_t alignment,
                               AllocHint kind) {
  IROperand *o = irOperandCreate(OK_TEMP);
  o->data.temp.n = n;
  o->data.temp.size = size;
  o->data.temp.alignment = alignment;
  o->data.temp.kind = kind;
  return o;
}
IROperand *regIROperandCreate(size_t n) {
  IROperand *o = irOperandCreate(OK_REG);
  o->data.reg.n = n;
  return o;
}
IROperand *ubyteIROperandCreate(uint8_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = value;
  return o;
}
IROperand *byteIROperandCreate(int8_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = punByteToUByte(value);
  return o;
}
IROperand *ushortIROperandCreate(uint16_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = value;
  return o;
}
IROperand *shortIROperandCreate(int16_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = punShortToUShort(value);
  return o;
}
IROperand *uintIROperandCreate(uint32_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = value;
  return o;
}
IROperand *intIROperandCreate(int32_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = punIntToUInt(value);
  return o;
}
IROperand *ulongIROperandCreate(uint64_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = value;
  return o;
}
IROperand *longIROperandCreate(int64_t value) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = punLongToULong(value);
  return o;
}
IROperand *floatIROperandCreate(uint32_t bits) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = bits;
  return o;
}
IROperand *doubleIROperandCreate(uint64_t bits) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.bits = bits;
  return o;
}
IROperand *nameIROperandCreate(char *name) {
  IROperand *o = irOperandCreate(OK_NAME);
  o->data.name.name = name;
  return o;
}
IROperand *asmIROperandCreate(char *assembly) {
  IROperand *o = irOperandCreate(OK_ASM);
  o->data.assembly.assembly = assembly;
  return o;
}
IROperand *stringIROperandCreate(uint8_t *data) {
  IROperand *o = irOperandCreate(OK_STRING);
  o->data.string.data = data;
  return o;
}
IROperand *wstringIROperandCreate(uint32_t *data) {
  IROperand *o = irOperandCreate(OK_WSTRING);
  o->data.wstring.data = data;
  return o;
}
IROperand *stackOffsetIROperandCreate(int64_t baseOffset) {
  IROperand *o = irOperandCreate(OK_STACKOFFSET);
  o->data.stackOffset.stackOffset = baseOffset;
  return o;
}
IROperand *irOperandCopy(IROperand const *o2) {
  printf("FIXME: DEBUG ONLY: copying ir operand of kind %d\n", o2->kind);
  IROperand *o1 = irOperandCreate(o2->kind);
  switch (o2->kind) {
    case OK_TEMP: {
      o1->data.temp = o2->data.temp;
      break;
    }
    case OK_REG: {
      o1->data.reg = o2->data.reg;
      break;
    }
    case OK_CONSTANT: {
      o1->data.constant = o2->data.constant;
      break;
    }
    case OK_STACKOFFSET: {
      o1->data.stackOffset = o2->data.stackOffset;
      break;
    }
    case OK_NAME: {
      o1->data.name.name = strdup(o2->data.name.name);
      break;
    }
    case OK_ASM: {
      o1->data.assembly.assembly = strdup(o2->data.assembly.assembly);
      break;
    }
    case OK_STRING: {
      o1->data.string.data = tstrdup(o2->data.string.data);
      break;
    }
    case OK_WSTRING: {
      o1->data.wstring.data = twstrdup(o2->data.wstring.data);
      break;
    }
  }

  return o1;
}
void irOperandDestroy(IROperand *o) {
  switch (o->kind) {
    case OK_TEMP:
    case OK_REG:
    case OK_CONSTANT:
    case OK_STACKOFFSET: {
      break;
    }
    case OK_NAME: {
      free(o->data.name.name);
      break;
    }
    case OK_ASM: {
      free(o->data.assembly.assembly);
      break;
    }
    case OK_STRING: {
      free(o->data.string.data);
      break;
    }
    case OK_WSTRING: {
      free(o->data.wstring.data);
      break;
    }
  }
  free(o);
}

static IREntry *irEntryCreate(size_t size, IROperator op) {
  IREntry *e = malloc(sizeof(IREntry));
  e->opSize = size;
  e->op = op;
  e->arg1 = e->arg2 = e->dest = NULL;
  return e;
}
IREntry *constantIREntryCreate(size_t size, IROperand *constant) {
  IREntry *e = irEntryCreate(size, IO_CONST);
  e->arg1 = constant;
  return e;
}
IREntry *asmIREntryCreate(IROperand *assembly) {
  IREntry *e = irEntryCreate(0, IO_ASM);
  e->arg1 = assembly;
  return e;
}
IREntry *labelIREntryCreate(IROperand *label) {
  IREntry *e = irEntryCreate(0, IO_LABEL);
  e->arg1 = label;
  return e;
}
IREntry *moveIREntryCreate(size_t size, IROperand *dest, IROperand *source) {
  IREntry *e = irEntryCreate(size, IO_MOVE);
  e->dest = dest;
  e->arg1 = source;
  return e;
}
IREntry *memStoreIREntryCreate(size_t size, IROperand *destAddr,
                               IROperand *source) {
  IREntry *e = irEntryCreate(size, IO_MEM_STORE);
  e->dest = destAddr;
  e->arg1 = source;
  return e;
}
IREntry *memLoadIREntryCreate(size_t size, IROperand *dest,
                              IROperand *sourceAddr) {
  IREntry *e = irEntryCreate(size, IO_MEM_LOAD);
  e->dest = dest;
  e->arg1 = sourceAddr;
  return e;
}
IREntry *stackStoreIREntryCreate(size_t size, IROperand *destOffset,
                                 IROperand *source) {
  IREntry *e = irEntryCreate(size, IO_STK_STORE);
  e->dest = destOffset;
  e->arg1 = source;
  return e;
}
IREntry *stackLoadIREntryCreate(size_t size, IROperand *dest,
                                IROperand *sourceOffset) {
  IREntry *e = irEntryCreate(size, IO_STK_LOAD);
  e->dest = dest;
  e->arg1 = sourceOffset;
  return e;
}
IREntry *offsetStoreIREntryCreate(size_t size, IROperand *destMemTemp,
                                  IROperand *source, IROperand *offset) {
  IREntry *e = irEntryCreate(size, IO_OFFSET_STORE);
  e->dest = destMemTemp;
  e->arg1 = source;
  e->arg2 = offset;
  return e;
}
IREntry *offsetLoadIREntryCreate(size_t size, IROperand *dest,
                                 IROperand *sourceMemTemp, IROperand *offset) {
  IREntry *e = irEntryCreate(size, IO_OFFSET_LOAD);
  e->dest = dest;
  e->arg1 = sourceMemTemp;
  e->arg2 = offset;
  return e;
}
IREntry *binopIREntryCreate(size_t size, IROperator op, IROperand *dest,
                            IROperand *arg1, IROperand *arg2) {
  IREntry *e = irEntryCreate(size, op);
  e->dest = dest;
  e->arg1 = arg1;
  e->arg2 = arg2;
  return e;
}
IREntry *unopIREntryCreate(size_t size, IROperator op, IROperand *dest,
                           IROperand *arg) {
  IREntry *e = irEntryCreate(size, op);
  e->dest = dest;
  e->arg1 = arg;
  return e;
}
IREntry *jumpIREntryCreate(IROperand *dest) {
  IREntry *e = irEntryCreate(0, IO_JUMP);
  e->dest = dest;
  return e;
}
IREntry *cjumpIREntryCreate(size_t size, IROperator op, IROperand *dest,
                            IROperand *lhs, IROperand *rhs) {
  IREntry *e = irEntryCreate(size, op);
  e->dest = dest;
  e->arg1 = lhs;
  e->arg2 = rhs;
  return e;
}
IREntry *callIREntryCreate(IROperand *who) {
  IREntry *e = irEntryCreate(0, IO_CALL);
  e->arg1 = who;
  return e;
}
IREntry *returnIREntryCreate(void) { return irEntryCreate(0, IO_RETURN); }
void irEntryDestroy(IREntry *e) {
  if (e->dest != NULL) {
    irOperandDestroy(e->dest);
  }
  if (e->arg1 != NULL) {
    irOperandDestroy(e->arg1);
  }
  if (e->arg2 != NULL) {
    irOperandDestroy(e->arg2);
  }
  free(e);
}

IROperandVector *irOperandVectorCreate(void) { return vectorCreate(); }
void irOperandVectorInit(IROperandVector *v) { vectorInit(v); }
void irOperandVectorInsert(IROperandVector *v, IROperand *o) {
  vectorInsert(v, o);
}
void irOperandVectorUninit(IROperandVector *v) {
  vectorUninit(v, (void (*)(void *))irOperandDestroy);
}
void irOperandVectorDestroy(IROperandVector *v) {
  vectorDestroy(v, (void (*)(void *))irOperandDestroy);
}

IREntryVector *irEntryVectorCreate(void) { return vectorCreate(); }
void irEntryVectorInit(IREntryVector *v) { vectorInit(v); }
void irEntryVectorInsert(IREntryVector *v, IREntry *e) { vectorInsert(v, e); }
IREntryVector *irEntryVectorMerge(IREntryVector *v1, IREntryVector *v2) {
  return vectorMerge(v1, v2);
}
void irEntryVectorUninit(IREntryVector *v) {
  vectorUninit(v, (void (*)(void *))irEntryDestroy);
}
void irEntryVectorDestroy(IREntryVector *v) {
  vectorDestroy(v, (void (*)(void *))irEntryDestroy);
}

TempAllocator *tempAllocatorCreate(void) {
  TempAllocator *a = malloc(sizeof(TempAllocator));
  a->next = 1;
  return a;
}
size_t tempAllocatorAllocate(TempAllocator *allocator) {
  return allocator->next++;
}
void tempAllocatorDestroy(TempAllocator *allocator) { free(allocator); }
