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

static IRFrag *fragCreate(FragmentType type, FragmentNameType nameType) {
  IRFrag *df = malloc(sizeof(IRFrag));
  df->type = type;
  df->nameType = nameType;
  return df;
}
IRFrag *globalDataFragCreate(FragmentType type, char *name, size_t alignment) {
  IRFrag *df = fragCreate(type, FNT_GLOBAL);
  df->name.global = name;
  df->data.data.alignment = alignment;
  vectorInit(&df->data.data.data);
  return df;
}
IRFrag *localDataFragCreate(FragmentType type, size_t name, size_t alignment) {
  IRFrag *df = fragCreate(type, FNT_LOCAL);
  df->name.local = name;
  df->data.data.alignment = alignment;
  vectorInit(&df->data.data.data);
  return df;
}
IRFrag *textFragCreate(char *name) {
  IRFrag *df = fragCreate(FT_TEXT, FNT_GLOBAL);
  df->name.global = name;
  linkedListInit(&df->data.text.blocks);
  return df;
}
IRFrag *findFrag(Vector *frags, size_t label) {
  for (size_t idx = 0; idx < frags->size; ++idx) {
    IRFrag *f = frags->elements[idx];
    if (f->nameType == FNT_LOCAL && f->name.local == label) return f;
  }
  return NULL;
}
void irFragFree(IRFrag *f) {
  switch (f->nameType) {
    case FNT_GLOBAL: {
      free(f->name.global);
      break;
    }
    default: {
      break;
    }
  }
  switch (f->type) {
    case FT_BSS:
    case FT_RODATA:
    case FT_DATA: {
      vectorUninit(&f->data.data.data, (void (*)(void *))irDatumFree);
      break;
    }
    case FT_TEXT: {
      linkedListUninit(&f->data.text.blocks, (void (*)(void *))irBlockFree);
      break;
    }
    default: {
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
IROperand *globalOperandCreate(char *name) {
  IROperand *o = irOperandCreate(OK_GLOBAL);
  o->data.global.name = name;
  return o;
}
IROperand *localOperandCreate(size_t name) {
  IROperand *o = irOperandCreate(OK_LOCAL);
  o->data.local.name = name;
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
    case OK_GLOBAL: {
      return globalOperandCreate(strdup(o->data.global.name));
    }
    case OK_LOCAL: {
      return localOperandCreate(o->data.local.name);
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
    case OK_GLOBAL:
    case OK_LOCAL: {
      return POINTER_WIDTH;
    }
    default: {
      error(__FILE__, __LINE__, "invalid IROperandKind");
    }
  }
}
size_t irOperandAlignof(IROperand const *o) {
  switch (o->kind) {
    case OK_TEMP: {
      return o->data.temp.alignment;
    }
    case OK_REG: {
      return o->data.reg.size;
    }
    case OK_CONSTANT: {
      return o->data.constant.alignment;
    }
    case OK_GLOBAL:
    case OK_LOCAL: {
      return POINTER_WIDTH;
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
    case OK_GLOBAL: {
      free(o->data.global.name);
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
    case IO_LABEL:
    case IO_VOLATILE:
    case IO_UNINITIALIZED:
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
    case IO_U2F:
    case IO_S2F:
    case IO_FRESIZE:
    case IO_F2I:
    case IO_JUMPTABLE:
    case IO_J1Z:
    case IO_J1NZ: {
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
    case IO_J2Z:
    case IO_J2NZ:
    case IO_J1L:
    case IO_J1LE:
    case IO_J1E:
    case IO_J1NE:
    case IO_J1G:
    case IO_J1GE:
    case IO_J1A:
    case IO_J1AE:
    case IO_J1B:
    case IO_J1BE:
    case IO_J1FL:
    case IO_J1FLE:
    case IO_J1FE:
    case IO_J1FNE:
    case IO_J1FG:
    case IO_J1FGE: {
      return 3;
    }
    case IO_J2L:
    case IO_J2LE:
    case IO_J2E:
    case IO_J2NE:
    case IO_J2G:
    case IO_J2GE:
    case IO_J2A:
    case IO_J2AE:
    case IO_J2B:
    case IO_J2BE:
    case IO_J2FL:
    case IO_J2FLE:
    case IO_J2FE:
    case IO_J2FNE:
    case IO_J2FG:
    case IO_J2FGE: {
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
IRInstruction *irInstructionCopy(IRInstruction const *i) {
  IRInstruction *copy = irInstructionCreate(i->op);
  for (size_t idx = 0; idx < irOperatorArity(i->op); ++idx)
    copy->args[idx] = irOperandCopy(i->args[idx]);
  return copy;
}
static void irOperandArrayFree(IROperand **arry, size_t size) {
  for (size_t idx = 0; idx < size; ++idx) irOperandFree(arry[idx]);
  free(arry);
}
void irInstructionFree(IRInstruction *i) {
  irOperandArrayFree(i->args, irOperatorArity(i->op));
  free(i);
}
void irInstructionMakeNop(IRInstruction *i) {
  irOperandArrayFree(i->args, irOperatorArity(i->op));
  i->args = malloc(0);
  i->op = IO_NOP;
}

IRBlock *irBlockCreate(size_t label) {
  IRBlock *b = malloc(sizeof(IRBlock));
  b->label = label;
  linkedListInit(&b->instructions);
  return b;
}
size_t indexOfBlock(LinkedList *blocks, size_t label) {
  size_t idx = 0;
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;
       curr = curr->next) {
    IRBlock *b = curr->data;
    if (b->label == label) return idx;
    ++idx;
  }
  error(__FILE__, __LINE__, "given block label doesn't exist");
}
IRBlock *findBlock(LinkedList *blocks, size_t label) {
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;
       curr = curr->next) {
    IRBlock *b = curr->data;
    if (b->label == label) return b;
  }
  return NULL;
}
void irBlockFree(IRBlock *b) {
  linkedListUninit(&b->instructions, (void (*)(void *))irInstructionFree);
  free(b);
}

char const *const IROPERATOR_NAMES[] = {
    "LABEL",
    "VOLATILE",
    "UNINITIALIZED",
    "ADDROF",
    "NOP",
    "MOVE",
    "MEM_STORE",
    "MEM_LOAD",
    "STK_STORE",
    "STK_LOAD",
    "OFFSET_STORE",
    "OFFSET_LOAD",
    "ADD",
    "SUB",
    "SMUL",
    "UMUL",
    "SDIV",
    "UDIV",
    "SMOD",
    "UMOD",
    "FADD",
    "FSUB",
    "FMUL",
    "FDIV",
    "FMOD",
    "NEG",
    "FNEG",
    "SLL",
    "SLR",
    "SAR",
    "AND",
    "XOR",
    "OR",
    "NOT",
    "L",
    "LE",
    "E",
    "NE",
    "G",
    "GE",
    "A",
    "AE",
    "B",
    "BE",
    "FL",
    "FLE",
    "FE",
    "FNE",
    "FG",
    "FGE",
    "Z",
    "NZ",
    "LNOT",
    "SX",
    "ZX",
    "TRUNC",
    "U2F",
    "S2F",
    "FRESIZE",
    "F2I",
    "JUMP",
    "JUMPTABLE",
    "J2L",
    "J2LE",
    "J2E",
    "J2NE",
    "J2G",
    "J2GE",
    "J2A",
    "J2AE",
    "J2B",
    "J2BE",
    "J2FL",
    "J2FLE",
    "J2FE",
    "J2FNE",
    "J2FG",
    "J2FGE",
    "J2Z",
    "J2NZ",
    "J1L",
    "J1LE",
    "J1E",
    "J1NE",
    "J1G",
    "J1GE",
    "J1A",
    "J1AE",
    "J1B",
    "J1BE",
    "J1FL",
    "J1FLE",
    "J1FE",
    "J1FNE",
    "J1FG",
    "J1FGE",
    "J1Z",
    "J1NZ",
    "CALL",
    "RETURN",
};
char const *const IROPERAND_NAMES[] = {
    "TEMP", "REG", "CONSTANT", "GLOBAL", "LOCAL",
};
char const *const ALLOCHINT_NAMES[] = {
    "NONE",
    "GP",
    "MEM",
    "FP",
};

static void validateTempConsistency(IROperand *definition, IROperand *temp,
                                    char const *phase, FileListEntry *file) {
  if (definition->data.temp.size != temp->data.temp.size) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - temp "
            "%zu's size is inconsistent\n",
            file->inputFilename, phase, temp->data.temp.name);
    file->errored = true;
  }
  if (definition->data.temp.alignment != temp->data.temp.alignment) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - temp "
            "%zu's alignment is inconsistent\n",
            file->inputFilename, phase, temp->data.temp.name);
    file->errored = true;
  }
  if (definition->data.temp.kind != temp->data.temp.kind) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - temp "
            "%zu's allocation kind is inconsistent\n",
            file->inputFilename, phase, temp->data.temp.name);
    file->errored = true;
  }
}
static void validateTempRead(IROperand **temps, IROperand *temp,
                             char const *phase, FileListEntry *file) {
  IROperand *definition = temps[temp->data.temp.name];
  if (definition == NULL) {  // temp must exist
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - temp "
            "%zu is used before it is initialized\n",
            file->inputFilename, phase, temp->data.temp.name);
    file->errored = true;
  } else {
    validateTempConsistency(definition, temp, phase, file);
  }
}
static void validateTempWrite(IROperand **temps, IROperand *temp,
                              char const *phase, FileListEntry *file) {
  IROperand *definition = temps[temp->data.temp.name];
  if (definition == NULL)
    temps[temp->data.temp.name] = temp;
  else
    validateTempConsistency(definition, temp, phase, file);
}
static bool validateArgKind(IRInstruction *i, size_t idx, OperandKind kind,
                            char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != kind) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have %s operand at position %zu, instead it "
            "has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op],
            IROPERAND_NAMES[kind], idx, IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    return true;
  }
}
static bool validateArgOffsettable(IRInstruction *i, size_t idx,
                                   char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have TEMP or REG operand at position %zu, "
            "instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    return true;
  }
}
static bool validateArgWritable(IRInstruction *i, size_t idx, IROperand **temps,
                                char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have TEMP or REG operand at position %zu, "
            "instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    if (i->args[0]->kind == OK_TEMP)
      validateTempWrite(temps, i->args[0], phase, file);
    return true;
  }
}
static void validateArgSize(IRInstruction *i, size_t idx, size_t size,
                            char const *phase, FileListEntry *file) {
  if (irOperandSizeof(i->args[idx]) != size) {
    fprintf(stderr,
            "%s: interal compiler error: IR validation after %s failed - %s "
            "instruction's %s operand at position %zu must have size %zu, but "
            "instead has size %zu\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op],
            IROPERAND_NAMES[i->args[idx]->kind], idx, size,
            irOperandSizeof(i->args[idx]));
    file->errored = true;
  }
}
static void validateTempGP(IRInstruction *i, size_t idx, char const *phase,
                           FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_GP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction's TEMP operand at position %zu does not have GP or "
            "MEM allocation, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateTempFP(IRInstruction *i, size_t idx, char const *phase,
                           FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_FP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction's TEMP operand at position %zu does not have FP or "
            "MEM allocation, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateTempMEM(IRInstruction *i, size_t idx, char const *phase,
                            FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_FP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction's TEMP operand at position %zu does not have MEM "
            "allocation, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateLocalJumpTarget(IRInstruction *i, size_t idx,
                                    bool *localLabels, char const *phase,
                                    FileListEntry *file) {
  if (i->args[idx]->kind == OK_LOCAL &&
      !localLabels[i->args[idx]->data.local.name]) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction's argument %zu (LOCAL %zu) is not a valid label\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            i->args[idx]->data.local.name);
    file->errored = true;
  }
}
static void validateArgPointerRead(IRInstruction *i, size_t idx,
                                   IROperand **temps, bool *localLabels,
                                   char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_GLOBAL && i->args[idx]->kind != OK_LOCAL &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have REG, TEMP, LABEL, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, POINTER_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, phase, file);
      validateTempRead(temps, i->args[idx], phase, file);
    } else if (i->args[idx]->kind == OK_LOCAL) {
      validateLocalJumpTarget(i, idx, localLabels, phase, file);
    }
  }
}
static void validateArgByteRead(IRInstruction *i, size_t idx, IROperand **temps,
                                char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have REG, TEMP, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, BYTE_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, phase, file);
      validateTempRead(temps, i->args[idx], phase, file);
    }
  }
}
static void validateArgPointerWritten(IRInstruction *i, size_t idx,
                                      IROperand **temps, char const *phase,
                                      FileListEntry *file) {
  if (validateArgWritable(i, idx, temps, phase, file)) {
    validateArgSize(i, idx, POINTER_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) validateTempGP(i, idx, phase, file);
  }
}
static void validateArgByteWritten(IRInstruction *i, size_t idx,
                                   IROperand **temps, char const *phase,
                                   FileListEntry *file) {
  if (validateArgWritable(i, idx, temps, phase, file)) {
    validateArgSize(i, idx, BYTE_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) validateTempGP(i, idx, phase, file);
  }
}
static void validateArgOffset(IRInstruction *i, size_t idx, IROperand **temps,
                              char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have REG, TEMP, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, POINTER_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, phase, file);
      validateTempRead(temps, i->args[idx], phase, file);
    }
  }
}
static void validateArgRead(IRInstruction *i, size_t idx, IROperand **temps,
                            bool *localLabels, char const *phase,
                            FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_GLOBAL && i->args[idx]->kind != OK_LOCAL &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have REG, TEMP, LABEL, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    if (i->args[idx]->kind == OK_TEMP)
      validateTempRead(temps, i->args[idx], phase, file);
    else if (i->args[idx]->kind == OK_LOCAL)
      validateLocalJumpTarget(i, idx, localLabels, phase, file);
  }
}
static void validateArgReadNoPtr(IRInstruction *i, size_t idx,
                                 IROperand **temps, char const *phase,
                                 FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - %s "
            "instruction does not have REG, TEMP or CONSTANT operand at "
            "position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    if (i->args[idx]->kind == OK_TEMP)
      validateTempRead(temps, i->args[idx], phase, file);
  }
}
static void validateArgsSameSize(IRInstruction *i, size_t a, size_t b,
                                 char const *phase, FileListEntry *file) {
  if (irOperandSizeof(i->args[a]) != irOperandSizeof(i->args[b])) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - "
            "%s instruction's argument %zu and %zu differ in size\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], a, b);
    file->errored = true;
  }
}
static void validateArgJumpTarget(IRInstruction *i, size_t idx,
                                  IROperand **temps, bool *localLabels,
                                  char const *phase, FileListEntry *file) {
  if (i->args[idx]->kind != OK_GLOBAL && i->args[idx]->kind != OK_LOCAL &&
      i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation after %s failed - "
            "%s instruction does not have LABEL, REG, or TEMP operand at "
            "position %zu, instead it has %s\n",
            file->inputFilename, phase, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, POINTER_WIDTH, phase, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, phase, file);
      validateTempRead(temps, i->args[idx], phase, file);
    } else if (i->args[idx]->kind == OK_LOCAL) {
      validateLocalJumpTarget(i, idx, localLabels, phase, file);
    }
  }
}
static int validateIr(char const *phase, bool blocked) {
  bool errored = false;
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      if (frag->type == FT_TEXT) {
        LinkedList *blocks = &frag->data.text.blocks;
        IROperand **temps = calloc(file->nextId, sizeof(IROperand *));

        // localLabels is the list of existing local labels
        // TODO: better differentiate between code labels (must be from the
        // current function) and data labels (may be from anywhere)
        bool *localLabels = calloc(file->nextId, sizeof(bool));
        for (ListNode *currBlock = blocks->head->next;
             currBlock != blocks->tail; currBlock = currBlock->next) {
          IRBlock *block = currBlock->data;
          if (blocked) localLabels[block->label] = true;
          for (ListNode *currInst = block->instructions.head->next;
               currInst != block->instructions.tail;
               currInst = currInst->next) {
            IRInstruction *i = currInst->data;
            if (!blocked && i->op == IO_LABEL && i->args[0]->kind == OK_LOCAL)
              localLabels[i->args[0]->data.local.name] = true;
          }
        }
        for (size_t dataFragIdx = 0; dataFragIdx < file->irFrags.size;
             ++dataFragIdx) {
          IRFrag *dataFrag = file->irFrags.elements[dataFragIdx];
          if (dataFrag->nameType == FNT_LOCAL)
            localLabels[dataFrag->name.local] = true;
        }

        if (blocked) {
          for (ListNode *currBlock = blocks->head->next;
               currBlock != blocks->tail; currBlock = currBlock->next) {
            IRBlock *block = currBlock->data;
            IRInstruction *last = block->instructions.tail->prev->data;
            switch (last->op) {
              case IO_JUMP:
              case IO_JUMPTABLE:
              case IO_J2L:
              case IO_J2LE:
              case IO_J2E:
              case IO_J2NE:
              case IO_J2G:
              case IO_J2GE:
              case IO_J2A:
              case IO_J2AE:
              case IO_J2B:
              case IO_J2BE:
              case IO_J2FL:
              case IO_J2FLE:
              case IO_J2FE:
              case IO_J2FNE:
              case IO_J2FG:
              case IO_J2FGE:
              case IO_J2Z:
              case IO_J2NZ:
              case IO_J1L:
              case IO_J1LE:
              case IO_J1E:
              case IO_J1NE:
              case IO_J1G:
              case IO_J1GE:
              case IO_J1A:
              case IO_J1AE:
              case IO_J1B:
              case IO_J1BE:
              case IO_J1FL:
              case IO_J1FLE:
              case IO_J1FE:
              case IO_J1FNE:
              case IO_J1FG:
              case IO_J1FGE:
              case IO_J1Z:
              case IO_J1NZ:
              case IO_RETURN: {
                break;
              }
              default: {
                fprintf(
                    stderr,
                    "%s: internal compiler error: IR validation after %s "
                    "failed "
                    "- %s instruction encountered at the end of a basic block "
                    "instead of a jump\n",
                    file->inputFilename, phase, IROPERATOR_NAMES[last->op]);
                file->errored = true;
                break;
              }
            }
          }
        }
        for (ListNode *currBlock = blocks->head->next;
             currBlock != blocks->tail; currBlock = currBlock->next) {
          IRBlock *block = currBlock->data;
          for (ListNode *currInst = block->instructions.head->next;
               currInst != block->instructions.tail;
               currInst = currInst->next) {
            IRInstruction *i = currInst->data;
            if (i->op < IO_LABEL || i->op > IO_RETURN) {
              fprintf(
                  stderr,
                  "%s: internal compiler error: IR validation after %s failed "
                  "- invalid operator (numerically %d) encountered\n",
                  file->inputFilename, phase, i->op);
              file->errored = true;
            }
            switch (i->op) {
              case IO_LABEL: {
                validateArgKind(i, 0, OK_LOCAL, phase, file);

                if (blocked) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - label encountered in basic block IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_VOLATILE: {
                if (validateArgKind(i, 0, OK_TEMP, phase, file))
                  validateTempRead(temps, i->args[0], phase, file);
                break;
              }
              case IO_UNINITIALIZED: {
                if (validateArgKind(i, 0, OK_TEMP, phase, file))
                  validateTempWrite(temps, i->args[0], phase, file);
                break;
              }
              case IO_ADDROF: {
                validateArgPointerWritten(i, 0, temps, phase, file);

                if (validateArgKind(i, 1, OK_TEMP, phase, file)) {
                  validateTempMEM(i, 1, phase, file);
                  validateTempRead(temps, i->args[1], phase, file);
                }
                break;
              }
              case IO_NOP: {
                if (!blocked) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - nop encountered in scheduled IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_MOVE: {
                validateArgWritable(i, 0, temps, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                break;
              }
              case IO_MEM_STORE: {
                validateArgPointerRead(i, 0, temps, localLabels, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);

                validateArgOffset(i, 2, temps, phase, file);
                break;
              }
              case IO_MEM_LOAD: {
                validateArgWritable(i, 0, temps, phase, file);

                validateArgPointerRead(i, 1, temps, localLabels, phase, file);

                validateArgOffset(i, 2, temps, phase, file);
                break;
              }
              case IO_STK_STORE: {
                validateArgOffset(i, 0, temps, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                break;
              }
              case IO_STK_LOAD: {
                validateArgWritable(i, 0, temps, phase, file);

                validateArgOffset(i, 1, temps, phase, file);
                break;
              }
              case IO_OFFSET_STORE: {
                validateArgOffsettable(i, 0, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempWrite(temps, i->args[0], phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);

                validateArgOffset(i, 2, temps, phase, file);
                break;
              }
              case IO_OFFSET_LOAD: {
                validateArgWritable(i, 0, temps, phase, file);

                validateArgOffsettable(i, 1, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempRead(temps, i->args[1], phase, file);

                validateArgOffset(i, 2, temps, phase, file);
                break;
              }
              case IO_ADD:
              case IO_SUB:
              case IO_SMUL:
              case IO_UMUL:
              case IO_SDIV:
              case IO_UDIV:
              case IO_SMOD:
              case IO_UMOD:
              case IO_AND:
              case IO_XOR:
              case IO_OR: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, phase, file);
                  validateTempWrite(temps, i->args[0], phase, file);
                }

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                validateArgRead(i, 2, temps, localLabels, phase, file);
                if (i->args[2]->kind == OK_TEMP)
                  validateTempGP(i, 2, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                validateArgsSameSize(i, 0, 2, phase, file);
                validateArgsSameSize(i, 1, 2, phase, file);
                break;
              }
              case IO_FADD:
              case IO_FSUB:
              case IO_FMUL:
              case IO_FDIV:
              case IO_FMOD: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempFP(i, 0, phase, file);
                  validateTempWrite(temps, i->args[0], phase, file);
                }

                validateArgReadNoPtr(i, 1, temps, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempFP(i, 1, phase, file);

                validateArgReadNoPtr(i, 2, temps, phase, file);
                if (i->args[2]->kind == OK_TEMP)
                  validateTempFP(i, 2, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                validateArgsSameSize(i, 0, 2, phase, file);
                validateArgsSameSize(i, 1, 2, phase, file);
                break;
              }
              case IO_NEG:
              case IO_NOT: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, phase, file);
                  validateTempWrite(temps, i->args[0], phase, file);
                }

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                break;
              }
              case IO_FNEG: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempFP(i, 0, phase, file);
                  validateTempWrite(temps, i->args[0], phase, file);
                }

                validateArgReadNoPtr(i, 1, temps, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempFP(i, 1, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                break;
              }
              case IO_SLL:
              case IO_SLR:
              case IO_SAR: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, phase, file);
                  validateTempWrite(temps, i->args[0], phase, file);
                }

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                validateArgByteRead(i, 2, temps, phase, file);

                validateArgsSameSize(i, 0, 1, phase, file);
                break;
              }
              case IO_L:
              case IO_LE:
              case IO_E:
              case IO_NE:
              case IO_G:
              case IO_GE:
              case IO_A:
              case IO_AE:
              case IO_B:
              case IO_BE: {
                validateArgByteWritten(i, 0, temps, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                validateArgRead(i, 2, temps, localLabels, phase, file);
                if (i->args[2]->kind == OK_TEMP)
                  validateTempGP(i, 2, phase, file);

                validateArgsSameSize(i, 1, 2, phase, file);
                break;
              }
              case IO_FL:
              case IO_FLE:
              case IO_FE:
              case IO_FNE:
              case IO_FG:
              case IO_FGE: {
                validateArgByteWritten(i, 0, temps, phase, file);

                validateArgReadNoPtr(i, 1, temps, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempFP(i, 1, phase, file);

                validateArgReadNoPtr(i, 2, temps, phase, file);
                if (i->args[2]->kind == OK_TEMP)
                  validateTempFP(i, 2, phase, file);

                validateArgsSameSize(i, 1, 2, phase, file);
                break;
              }
              case IO_Z:
              case IO_NZ: {
                validateArgByteWritten(i, 0, temps, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                break;
              }
              case IO_LNOT: {
                validateArgByteWritten(i, 0, temps, phase, file);

                validateArgByteRead(i, 1, temps, phase, file);
                break;
              }
              case IO_SX:
              case IO_ZX: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempGP(i, 0, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                if (irOperandSizeof(i->args[0]) <=
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - %s instruction's argument 0 is not larger "
                          "than "
                          "argument 1\n",
                          file->inputFilename, phase, IROPERATOR_NAMES[i->op]);
                  file->errored = true;
                }
                break;
              }
              case IO_TRUNC: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempGP(i, 0, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);

                if (irOperandSizeof(i->args[0]) >=
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - TRUNC instruction's argument 0 is not "
                          "smaller than argument 1\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_U2F:
              case IO_S2F: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempFP(i, 0, phase, file);

                validateArgRead(i, 1, temps, localLabels, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempGP(i, 1, phase, file);
                break;
              }
              case IO_FRESIZE: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempFP(i, 0, phase, file);

                validateArgReadNoPtr(i, 1, temps, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempFP(i, 1, phase, file);

                if (irOperandSizeof(i->args[0]) ==
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - TRUNC instruction's argument 0 and 1 are "
                          "the same size\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_F2I: {
                validateArgWritable(i, 0, temps, phase, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempGP(i, 0, phase, file);

                validateArgReadNoPtr(i, 1, temps, phase, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempFP(i, 1, phase, file);
                break;
              }
              case IO_JUMP: {
                validateArgKind(i, 0, OK_LOCAL, phase, file);
                validateLocalJumpTarget(i, 0, localLabels, phase, file);

                if (blocked && currInst->next != block->instructions.tail) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - non-terminal jump encountered in basic "
                          "block IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_JUMPTABLE: {
                validateArgKind(i, 0, OK_TEMP, phase, file);

                // TODO: check that this is a RODATA frag
                validateArgKind(i, 1, OK_LOCAL, phase, file);

                if (blocked && currInst->next != block->instructions.tail) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - non-terminal jump encountered in basic "
                          "block IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J2L:
              case IO_J2LE:
              case IO_J2E:
              case IO_J2NE:
              case IO_J2G:
              case IO_J2GE:
              case IO_J2A:
              case IO_J2AE:
              case IO_J2B:
              case IO_J2BE: {
                if (blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgKind(i, 1, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 1, localLabels, phase, file);

                  validateArgRead(i, 2, temps, localLabels, phase, file);
                  if (i->args[2]->kind == OK_TEMP)
                    validateTempGP(i, 2, phase, file);

                  validateArgRead(i, 3, temps, localLabels, phase, file);
                  if (i->args[3]->kind == OK_TEMP)
                    validateTempGP(i, 3, phase, file);

                  validateArgsSameSize(i, 2, 3, phase, file);

                  if (currInst->next != block->instructions.tail) {
                    fprintf(
                        stderr,
                        "%s: internal compiler error: IR validation after %s "
                        "failed - non-terminal jump encountered in basic "
                        "block IR\n",
                        file->inputFilename, phase);
                    file->errored = true;
                  }
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - two-target jump encountered in scheduled "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J2FL:
              case IO_J2FLE:
              case IO_J2FE:
              case IO_J2FNE:
              case IO_J2FG:
              case IO_J2FGE: {
                if (blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgKind(i, 1, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 1, localLabels, phase, file);

                  validateArgReadNoPtr(i, 2, temps, phase, file);
                  if (i->args[2]->kind == OK_TEMP)
                    validateTempFP(i, 2, phase, file);

                  validateArgReadNoPtr(i, 3, temps, phase, file);
                  if (i->args[3]->kind == OK_TEMP)
                    validateTempFP(i, 3, phase, file);

                  validateArgsSameSize(i, 2, 3, phase, file);

                  if (currInst->next != block->instructions.tail) {
                    fprintf(
                        stderr,
                        "%s: internal compiler error: IR validation after %s "
                        "failed - non-terminal jump encountered in basic "
                        "block "
                        "IR\n",
                        file->inputFilename, phase);
                    file->errored = true;
                  }
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - two-target jump encountered in scheduled "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J2Z:
              case IO_J2NZ: {
                if (blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgKind(i, 1, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 1, localLabels, phase, file);

                  validateArgRead(i, 2, temps, localLabels, phase, file);

                  if (currInst->next != block->instructions.tail) {
                    fprintf(
                        stderr,
                        "%s: internal compiler error: IR validation after %s "
                        "failed - non-terminal jump encountered in basic "
                        "block "
                        "IR\n",
                        file->inputFilename, phase);
                    file->errored = true;
                  }
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - two-target jump encountered in scheduled "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J1L:
              case IO_J1LE:
              case IO_J1E:
              case IO_J1NE:
              case IO_J1G:
              case IO_J1GE:
              case IO_J1A:
              case IO_J1AE:
              case IO_J1B:
              case IO_J1BE: {
                if (!blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgRead(i, 1, temps, localLabels, phase, file);
                  if (i->args[1]->kind == OK_TEMP)
                    validateTempGP(i, 1, phase, file);

                  validateArgRead(i, 2, temps, localLabels, phase, file);
                  if (i->args[2]->kind == OK_TEMP)
                    validateTempGP(i, 2, phase, file);

                  validateArgsSameSize(i, 1, 2, phase, file);
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - one-target jump encountered in basic block "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J1FL:
              case IO_J1FLE:
              case IO_J1FE:
              case IO_J1FNE:
              case IO_J1FG:
              case IO_J1FGE: {
                if (!blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgReadNoPtr(i, 1, temps, phase, file);
                  if (i->args[1]->kind == OK_TEMP)
                    validateTempFP(i, 1, phase, file);

                  validateArgReadNoPtr(i, 2, temps, phase, file);
                  if (i->args[2]->kind == OK_TEMP)
                    validateTempFP(i, 2, phase, file);

                  validateArgsSameSize(i, 1, 2, phase, file);
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - one-target jump encountered in basic block "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_J1Z:
              case IO_J1NZ: {
                if (!blocked) {
                  validateArgKind(i, 0, OK_LOCAL, phase, file);
                  validateLocalJumpTarget(i, 0, localLabels, phase, file);

                  validateArgRead(i, 1, temps, localLabels, phase, file);
                } else {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - one-target jump encountered in basic block "
                          "IR\n",
                          file->inputFilename, phase);
                  file->errored = true;
                }
                break;
              }
              case IO_CALL: {
                validateArgJumpTarget(i, 0, temps, localLabels, phase, file);
                break;
              }
              case IO_RETURN: {
                if (blocked && currInst->next != block->instructions.tail) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation after %s "
                          "failed - non-terminal return encountered in %s IR\n",
                          file->inputFilename, phase,
                          blocked ? "basic block" : "scheduled");
                  file->errored = true;
                }
                break;
              }
              default: {
                error(__FILE__, __LINE__, "invalid IROperator enum");
              }
            }
          }
        }
        free(temps);
        free(localLabels);
      }
    }
    errored = errored || file->errored;
  }

  if (errored) return -1;

  return 0;
}

int validateBlockedIr(char const *phase) { return validateIr(phase, true); }

int validateScheduledIr(char const *phase) { return validateIr(phase, false); }