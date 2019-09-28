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

#include <stdlib.h>

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
IROperand *labelIROperandCreate(char *name) {
  IROperand *o = irOperandCreate(OK_LABEL);
  o->data.label.name = name;
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
void irOperandDestroy(IROperand *o) {
  switch (o->kind) {
    case OK_TEMP: {
      break;
    }
    case OK_REG: {
      break;
    }
    case OK_CONSTANT: {
      break;
    }
    case OK_LABEL: {
      free(o->data.label.name);
      break;
    }
    case OK_ASM: {
      free(o->data.assembly.assembly);
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

IRVector *irVectorCreate(void) { return vectorCreate(); }
void irVectorInit(IRVector *v) { vectorInit(v); }
void irVectorInsert(IRVector *v, IREntry *e) { vectorInsert(v, e); }
IRVector *irVectorMerge(IRVector *v1, IRVector *v2) {
  return vectorMerge(v1, v2);
}
void irVectorUninit(IRVector *v) {
  vectorUninit(v, (void (*)(void *))irEntryDestroy);
}
void irVectorDestroy(IRVector *v) {
  vectorDestroy(v, (void (*)(void *))irEntryDestroy);
}

void tempAllocatorInit(TempAllocator *allocator) {
  allocator->next = 1;  // skipping zero
}
size_t tempAllocatorAllocate(TempAllocator *allocator) {
  return allocator->next++;
}
void tempAllocatorUninit(TempAllocator *allocator) {
  (void)allocator;  // do nothing
}
