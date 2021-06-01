// Copyright 2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ir/ir.h"

#include <stdlib.h>

static IRFrag *fragCreate(FragmentType type, char *name) {
  IRFrag *df = malloc(sizeof(IRFrag));
  df->type = type;
  df->name = name;
  return df;
}
IRFrag *dataFragCreate(FragmentType type, char *name, size_t alignment) {
  IRFrag *df = fragCreate(type, name);
  df->data.data.alignment = alignment;
  vectorInit(&df->data.data.data);
  return df;
}
IRFrag *textFragCreate(char *name) {
  IRFrag *df = fragCreate(FT_TEXT, name);
  vectorInit(&df->data.text.blocks);
  return df;
}
void irFragFree(IRFrag *f) {
  free(f->name);
  switch (f->type) {
    case FT_BSS:
    case FT_RODATA:
    case FT_DATA: {
      vectorUninit(&f->data.data.data, (void (*)(void *))irDatumFree);
      break;
    }
    case FT_TEXT: {
      vectorUninit(&f->data.text.blocks, (void (*)(void *))irBlockFree);
      break;
    }
  }
  free(f);
}
void irFragVectorUninit(Vector *v) {
  vectorUninit(v, (void (*)(void *))irFragFree);
}

static IRDatum *datumCreate(DatumType type) {
  IRDatum *d = malloc(sizeof(IRDatum));
  d->type = type;
  return d;
}
IRDatum *byteDatumCreate(uint8_t val) {
  IRDatum *d = datumCreate(DT_BYTE);
  d->data.byteVal = val;
  return d;
}
IRDatum *shortDatumCreate(uint16_t val) {
  IRDatum *d = datumCreate(DT_SHORT);
  d->data.shortVal = val;
  return d;
}
IRDatum *intDatumCreate(uint32_t val) {
  IRDatum *d = datumCreate(DT_INT);
  d->data.intVal = val;
  return d;
}
IRDatum *longDatumCreate(uint64_t val) {
  IRDatum *d = datumCreate(DT_LONG);
  d->data.longVal = val;
  return d;
}
IRDatum *paddingDatumCreate(size_t len) {
  IRDatum *d = datumCreate(DT_PADDING);
  d->data.paddingLength = len;
  return d;
}
IRDatum *stringDatumCreate(uint8_t *string) {
  IRDatum *d = datumCreate(DT_STRING);
  d->data.string = string;
  return d;
}
IRDatum *wstringDatumCreate(uint32_t *wstring) {
  IRDatum *d = datumCreate(DT_WSTRING);
  d->data.wstring = wstring;
  return d;
}
IRDatum *labelDatumCreate(size_t label) {
  IRDatum *d = datumCreate(DT_LABEL);
  d->data.label = label;
  return d;
}
void irDatumFree(IRDatum *d) {
  switch (d->type) {
    case DT_STRING: {
      free(d->data.string);
      break;
    }
    case DT_WSTRING: {
      free(d->data.wstring);
      break;
    }
    default: {
      break;
    }
  }
  free(d);
}

static IROperand *irOperandCreate(OperandKind kind) {
  IROperand *o = malloc(sizeof(IROperand));
  o->kind = kind;
  return o;
}
IROperand *tempOperandCreate(size_t name, size_t alignment, size_t size,
                             AllocHint kind) {
  IROperand *o = irOperandCreate(OK_TEMP);
  o->data.temp.name = name;
  o->data.temp.alignment = alignment;
  o->data.temp.size = size;
  o->data.temp.kind = kind;
  return o;
}
IROperand *regOperandCreate(size_t name) {
  IROperand *o = irOperandCreate(OK_REG);
  o->data.reg.name = name;
  return o;
}
IROperand *constantOperandCreate(size_t alignment) {
  IROperand *o = irOperandCreate(OK_CONSTANT);
  o->data.constant.alignment = alignment;
  vectorInit(&o->data.constant.data);
  return o;
}
IROperand *labelOperandCreate(char *name) {
  IROperand *o = irOperandCreate(OK_LABEL);
  o->data.label.name = name;
  return o;
}
IROperand *offsetOperandCreate(int64_t offset) {
  IROperand *o = irOperandCreate(OK_OFFSET);
  o->data.offset.offset = offset;
  return o;
}
IROperand *assemblyOperandCreate(char *assembly) {
  IROperand *o = irOperandCreate(OK_ASM);
  o->data.assembly.assembly = assembly;
  return o;
}
IROperand *stackFrameSizeOperandCreate(void) {
  return irOperandCreate(OK_STACK_FRAME_SIZE);
}
void irOperandFree(IROperand *o) {
  if (o == NULL) return;

  switch (o->kind) {
    case OK_CONSTANT: {
      vectorUninit(&o->data.constant.data, (void (*)(void *))irDatumFree);
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
    default: {
      break;
    }
  }
  free(o);
}

IRInstruction *irInstructionCreate(IROperator op, size_t size, IROperand *dest,
                                   IROperand *arg1, IROperand *arg2) {
  IRInstruction *i = malloc(sizeof(IRInstruction));
  i->op = op;
  i->size = size;
  i->dest = dest;
  i->arg1 = arg1;
  i->arg2 = arg2;
  return i;
}
void irInstructionFree(IRInstruction *i) {
  irOperandFree(i->dest);
  irOperandFree(i->arg1);
  irOperandFree(i->arg2);
  free(i);
}

IRBlock *irBlockCreate(size_t label) {
  IRBlock *b = malloc(sizeof(IRBlock));
  b->label = label;
  linkedListInit(&b->instructions);
  return b;
}
void irBlockFree(IRBlock *b) {
  linkedListUninit(&b->instructions, (void (*)(void *))irInstructionFree);
  free(b);
}