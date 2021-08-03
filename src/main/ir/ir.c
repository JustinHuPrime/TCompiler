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

#include "fileList.h"
#include "util/internalError.h"
#include "util/numericSizing.h"
#include "util/string.h"

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
IRDatum *irDatumCopy(IRDatum const *d) {
  switch (d->type) {
    case DT_BYTE: {
      return byteDatumCreate(d->data.byteVal);
    }
    case DT_SHORT: {
      return shortDatumCreate(d->data.shortVal);
    }
    case DT_INT: {
      return intDatumCreate(d->data.intVal);
    }
    case DT_LONG: {
      return longDatumCreate(d->data.longVal);
    }
    case DT_PADDING: {
      return paddingDatumCreate(d->data.paddingLength);
    }
    case DT_STRING: {
      return stringDatumCreate(tstrdup(d->data.string));
    }
    case DT_WSTRING: {
      return wstringDatumCreate(twstrdup(d->data.wstring));
    }
    case DT_LABEL: {
      return labelDatumCreate(d->data.label);
    }
    default: {
      error(__FILE__, __LINE__, "invalid DatumType");
    }
  }
}
static size_t irDatumSizeof(IRDatum const *d) {
  switch (d->type) {
    case DT_BYTE: {
      return BYTE_WIDTH;
    }
    case DT_SHORT: {
      return SHORT_WIDTH;
    }
    case DT_INT: {
      return INT_WIDTH;
    }
    case DT_LONG: {
      return LONG_WIDTH;
    }
    case DT_PADDING: {
      return d->data.paddingLength;
    }
    case DT_STRING: {
      return (tstrlen(d->data.string) + 1) * WCHAR_WIDTH;
    }
    case DT_WSTRING: {
      return (twstrlen(d->data.wstring) + 1) * WCHAR_WIDTH;
    }
    case DT_LABEL: {
      return POINTER_WIDTH;
    }
    default: {
      error(__FILE__, __LINE__, "invalid DatumType");
    }
  }
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
IROperand *regOperandCreate(size_t name, size_t size) {
  IROperand *o = irOperandCreate(OK_REG);
  o->data.reg.name = name;
  o->data.reg.size = size;
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
IROperand *irOperandCopy(IROperand const *o) {
  switch (o->kind) {
    case OK_TEMP: {
      return tempOperandCreate(o->data.temp.name, o->data.temp.alignment,
                               o->data.temp.size, o->data.temp.kind);
    }
    case OK_REG: {
      return regOperandCreate(o->data.reg.name, o->data.reg.size);
    }
    case OK_CONSTANT: {
      IROperand *retval = constantOperandCreate(o->data.constant.alignment);
      for (size_t idx = 0; idx < o->data.constant.data.size; ++idx)
        vectorInsert(&retval->data.constant.data,
                     irDatumCopy(o->data.constant.data.elements[idx]));
      return retval;
    }
    case OK_LABEL: {
      return labelOperandCreate(strdup(o->data.label.name));
    }
    case OK_OFFSET: {
      return offsetOperandCreate(o->data.offset.offset);
    }
    case OK_ASM: {
      return assemblyOperandCreate(strdup(o->data.assembly.assembly));
    }
    default: {
      error(__FILE__, __LINE__, "invalid IROperandKind");
    }
  }
}
size_t irOperandSizeof(IROperand const *o) {
  switch (o->kind) {
    case OK_TEMP: {
      return o->data.temp.size;
    }
    case OK_REG: {
      return o->data.reg.size;
    }
    case OK_CONSTANT: {
      size_t retval = 0;
      for (size_t idx = 0; idx < o->data.constant.data.size; ++idx)
        retval += irDatumSizeof(o->data.constant.data.elements[idx]);
      return retval;
    }
    case OK_LABEL: {
      return POINTER_WIDTH;
    }
    case OK_OFFSET: {
      return LONG_WIDTH;
    }
    case OK_ASM: {
      return 0;
    }
    default: {
      error(__FILE__, __LINE__, "invalid IROperandKind");
    }
  }
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

size_t irOperatorArity(IROperator op) {
  switch (op) {
    case IO_NOP:
    case IO_RETURN: {
      return 0;
    }
    case IO_ASM:
    case IO_LABEL:
    case IO_VOLATILE:
    case IO_JUMP:
    case IO_CALL: {
      return 1;
    }
    case IO_MOVE:
    case IO_ADDROF:
    case IO_STK_STORE:
    case IO_STK_LOAD:
    case IO_NEG:
    case IO_FNEG:
    case IO_NOT:
    case IO_Z:
    case IO_NZ:
    case IO_LNOT:
    case IO_SX:
    case IO_ZX:
    case IO_TRUNC:
    case IO_UNSIGNED2FLOATING:
    case IO_SIGNED2FLOATING:
    case IO_RESIZEFLOATING:
    case IO_FLOATING2INTEGRAL: {
      return 2;
    }
    case IO_MEM_STORE:
    case IO_MEM_LOAD:
    case IO_OFFSET_STORE:
    case IO_OFFSET_LOAD:
    case IO_ADD:
    case IO_FADD:
    case IO_SUB:
    case IO_FSUB:
    case IO_SMUL:
    case IO_UMUL:
    case IO_FMUL:
    case IO_SDIV:
    case IO_UDIV:
    case IO_FDIV:
    case IO_SMOD:
    case IO_UMOD:
    case IO_FMOD:
    case IO_SLL:
    case IO_SLR:
    case IO_SAR:
    case IO_AND:
    case IO_XOR:
    case IO_OR:
    case IO_L:
    case IO_LE:
    case IO_E:
    case IO_NE:
    case IO_G:
    case IO_GE:
    case IO_A:
    case IO_AE:
    case IO_B:
    case IO_BE:
    case IO_FL:
    case IO_FLE:
    case IO_FE:
    case IO_FNE:
    case IO_FG:
    case IO_FGE:
    case IO_JZ:
    case IO_JNZ: {
      return 3;
    }
    case IO_JL:
    case IO_JLE:
    case IO_JE:
    case IO_JNE:
    case IO_JG:
    case IO_JGE:
    case IO_JA:
    case IO_JAE:
    case IO_JB:
    case IO_JBE:
    case IO_JFL:
    case IO_JFLE:
    case IO_JFE:
    case IO_JFNE:
    case IO_JFG:
    case IO_JFGE: {
      return 4;
    }
    default: {
      error(__FILE__, __LINE__, "invalid IROperator enum");
    }
  }
}

IRInstruction *irInstructionCreate(IROperator op) {
  IRInstruction *i = malloc(sizeof(IRInstruction));
  i->op = op;
  i->args = malloc(irOperatorArity(op) * sizeof(IRInstruction *));
  return i;
}
static void irOperandArrayFree(IROperand **arry, size_t size) {
  for (size_t idx = 0; idx < size; ++idx) irOperandFree(arry[idx]);
  free(arry);
}
void irInstructionFree(IRInstruction *i) {
  irOperandArrayFree(i->args, irOperatorArity(i->op));
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

void validateIr(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      if (frag->type == FT_TEXT) {
        Vector *blocks = &frag->data.text.blocks;
        IROperand *temps = calloc(file->nextId, sizeof(IROperand *));
        for (size_t blockIdx = 0; blockIdx < blocks->size; ++blockIdx) {
          IRBlock *block = blocks->elements[blockIdx];
          // TODO
          // block->instructions;
          // TODO: can we assume that later-added blocks are always
          // later-executed?
          // TODO: maybe do trace scheduling before this/dead block elimination
          // before this?
        }
      }
    }
  }
}