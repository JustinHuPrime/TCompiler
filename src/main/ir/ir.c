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
    case IO_F2I: {
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

static char const *const IROPERATOR_NAMES[] = {
             "LABEL",     "VOLATILE", "UNINITIALIZED",
    "ADDROF",      "NOP",       "MOVE",     "MEM_STORE",
    "MEM_LOAD",    "STK_STORE", "STK_LOAD", "OFFSET_STORE",
    "OFFSET_LOAD", "ADD",       "SUB",      "SMUL",
    "UMUL",        "SDIV",      "UDIV",     "SMOD",
    "UMOD",        "FADD",      "FSUB",     "FMUL",
    "FDIV",        "FMOD",      "NEG",      "FNEG",
    "SLL",         "SLR",       "SAR",      "AND",
    "XOR",         "OR",        "NOT",      "L",
    "LE",          "E",         "NE",       "G",
    "GE",          "A",         "AE",       "B",
    "BE",          "FL",        "FLE",      "FE",
    "FNE",         "FG",        "FGE",      "Z",
    "NZ",          "LNOT",      "SX",       "ZX",
    "TRUNC",       "U2F",       "S2F",      "FRESIZE",
    "F2I",         "JUMP",      "JL",       "JLE",
    "JE",          "JNE",       "JG",       "JGE",
    "JA",          "JAE",       "JB",       "JBE",
    "JFL",         "JFLE",      "JFE",      "JFNE",
    "JFG",         "JFGE",      "JZ",       "JNZ",
    "CALL",        "RETURN",
};
static char const *const IROPERAND_NAMES[] = {
    "TEMP", "REG", "CONSTANT", "LABEL", 
};
static char const *const ALLOCHINT_NAMES[] = {
    "GP",
    "MEM",
    "FP",
};

static void validateTempConsistency(IROperand *definition, IROperand *temp,
                                    FileListEntry *file) {
  if (definition->data.temp.size != temp->data.temp.size) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - temp %zu's "
            "size is inconsistent\n",
            file->inputFilename, temp->data.temp.name);
    file->errored = true;
  }
  if (definition->data.temp.alignment != temp->data.temp.alignment) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - temp %zu's "
            "alignment is inconsistent\n",
            file->inputFilename, temp->data.temp.name);
    file->errored = true;
  }
  if (definition->data.temp.kind != temp->data.temp.kind) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - temp %zu's "
            "allocation kind is inconsistent\n",
            file->inputFilename, temp->data.temp.name);
    file->errored = true;
  }
}
static void validateTempRead(IROperand **temps, IROperand *temp,
                             FileListEntry *file) {
  IROperand *definition = temps[temp->data.temp.name];
  if (definition == NULL) {  // temp must exist
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - temp %zu is "
            "used before it is initialized\n",
            file->inputFilename, temp->data.temp.name);
    file->errored = true;
  } else {
    validateTempConsistency(definition, temp, file);
  }
}
static void validateTempWrite(IROperand **temps, IROperand *temp,
                              FileListEntry *file) {
  IROperand *definition = temps[temp->data.temp.name];
  if (definition == NULL)
    temps[temp->data.temp.name] = temp;
  else
    validateTempConsistency(definition, temp, file);
}
static bool validateArgKind(IRInstruction *i, size_t idx, OperandKind kind,
                            FileListEntry *file) {
  if (i->args[idx]->kind != kind) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have %s operand at position %zu, instead it "
            "has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], IROPERAND_NAMES[kind],
            idx, IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    return true;
  }
}
static bool validateArgOffsettable(IRInstruction *i, size_t idx,
                                   FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have TEMP or REG operand at position %zu, "
            "instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    return true;
  }
}
static bool validateArgWritable(IRInstruction *i, size_t idx, IROperand **temps,
                                FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have TEMP or REG operand at position %zu, "
            "instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
    return false;
  } else {
    if (i->args[0]->kind == OK_TEMP) validateTempWrite(temps, i->args[0], file);
    return true;
  }
}
static void validateArgSize(IRInstruction *i, size_t idx, size_t size,
                            FileListEntry *file) {
  if (irOperandSizeof(i->args[idx]) != size) {
    fprintf(stderr,
            "%s: interal compiler error: IR validation failed - %s "
            "instruction's %s operand at position %zu must have size %zu, but "
            "instead has size %zu\n",
            file->inputFilename, IROPERATOR_NAMES[i->op],
            IROPERAND_NAMES[i->args[idx]->kind], idx, size,
            irOperandSizeof(i->args[idx]));
    file->errored = true;
  }
}
static void validateTempGP(IRInstruction *i, size_t idx, FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_GP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction's TEMP operand at position %zu does not have GP or "
            "MEM allocation, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateTempFP(IRInstruction *i, size_t idx, FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_FP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction's TEMP operand at position %zu does not have FP or "
            "MEM allocation, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateTempMEM(IRInstruction *i, size_t idx, FileListEntry *file) {
  if (i->args[idx]->data.temp.kind != AH_FP &&
      i->args[idx]->data.temp.kind != AH_MEM) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction's TEMP operand at position %zu does not have MEM "
            "allocation, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            ALLOCHINT_NAMES[i->args[idx]->data.temp.kind]);
    file->errored = true;
  }
}
static void validateArgPointerRead(IRInstruction *i, size_t idx,
                                   IROperand **temps, FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_LABEL && i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have REG, TEMP, LABEL, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, POINTER_WIDTH, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, file);
      validateTempRead(temps, i->args[idx], file);
    }
  }
}
static void validateArgByteRead(IRInstruction *i, size_t idx, IROperand **temps,
                                FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have REG, TEMP, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, BYTE_WIDTH, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, file);
      validateTempRead(temps, i->args[idx], file);
    }
  }
}
static void validateArgPointerWritten(IRInstruction *i, size_t idx,
                                      IROperand **temps, FileListEntry *file) {
  if (validateArgWritable(i, idx, temps, file)) {
    validateArgSize(i, idx, POINTER_WIDTH, file);

    if (i->args[idx]->kind == OK_TEMP) validateTempGP(i, idx, file);
  }
}
static void validateArgByteWritten(IRInstruction *i, size_t idx,
                                   IROperand **temps, FileListEntry *file) {
  if (validateArgWritable(i, idx, temps, file)) {
    validateArgSize(i, idx, BYTE_WIDTH, file);

    if (i->args[idx]->kind == OK_TEMP) validateTempGP(i, idx, file);
  }
}
static void validateArgOffset(IRInstruction *i, size_t idx, IROperand **temps,
                              FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have REG, TEMP, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, idx, POINTER_WIDTH, file);

    if (i->args[idx]->kind == OK_TEMP) {
      validateTempGP(i, idx, file);
      validateTempRead(temps, i->args[idx], file);
    }
  }
}
static void validateArgRead(IRInstruction *i, size_t idx, IROperand **temps,
                            FileListEntry *file) {
  if (i->args[idx]->kind != OK_REG && i->args[idx]->kind != OK_TEMP &&
      i->args[idx]->kind != OK_LABEL && i->args[idx]->kind != OK_CONSTANT) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - %s "
            "instruction does not have REG, TEMP, LABEL, or CONSTANT operand "
            "at position %zu, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[idx]->kind]);
    file->errored = true;
  } else {
    if (i->args[idx]->kind == OK_TEMP)
      validateTempRead(temps, i->args[idx], file);
  }
}
static void validateArgsSameSize(IRInstruction *i, size_t a, size_t b,
                                 FileListEntry *file) {
  if (irOperandSizeof(i->args[a]) != irOperandSizeof(i->args[b])) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - "
            "%s instruction's argument %zu and %zu differ in size\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], a, b);
    file->errored = true;
  }
}
static void validateArgJumpTarget(IRInstruction *i, size_t idx,
                                  IROperand **temps, FileListEntry *file) {
  if (i->args[idx]->kind != OK_LABEL && i->args[idx]->kind != OK_REG &&
      i->args[idx]->kind != OK_TEMP) {
    fprintf(stderr,
            "%s: internal compiler error: IR validation failed - "
            "%s instruction does not have LABEL, REG, or TEMP operand at "
            "position %zu, instead it has %s\n",
            file->inputFilename, IROPERATOR_NAMES[i->op], idx,
            IROPERAND_NAMES[i->args[0]->kind]);
    file->errored = true;
  } else {
    validateArgSize(i, 0, POINTER_WIDTH, file);

    if (i->args[0]->kind == OK_TEMP) {
      validateTempGP(i, 0, file);
      validateTempRead(temps, i->args[0], file);
    }
  }
}
int validateIr(void) {
  bool errored = false;
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      if (frag->type == FT_TEXT) {
        Vector *blocks = &frag->data.text.blocks;
        IROperand **temps = calloc(file->nextId, sizeof(IROperand *));
        for (size_t blockIdx = 0; blockIdx < blocks->size; ++blockIdx) {
          IRBlock *block = blocks->elements[blockIdx];
          for (ListNode *curr = block->instructions.head->next;
               curr != block->instructions.tail; curr = curr->next) {
            IRInstruction *i = curr->data;
            switch (i->op) {
              case IO_LABEL: {
                validateArgKind(i, 0, OK_LABEL, file);
                break;
              }
              case IO_VOLATILE: {
                if (validateArgKind(i, 0, OK_TEMP, file))
                  validateTempRead(temps, i->args[0], file);
                break;
              }
              case IO_UNINITIALIZED: {
                if (validateArgKind(i, 0, OK_TEMP, file))
                  validateTempWrite(temps, i->args[0], file);
                break;
              }
              case IO_ADDROF: {
                validateArgPointerWritten(i, 0, temps, file);

                if (validateArgKind(i, 1, OK_TEMP, file)) {
                  validateTempMEM(i, 1, file);
                  validateTempRead(temps, i->args[1], file);
                }
                break;
              }
              case IO_NOP: {
                // nothing to check
                break;
              }
              case IO_MOVE: {
                validateArgWritable(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);

                validateArgsSameSize(i, 0, 1, file);
                break;
              }
              case IO_MEM_STORE: {
                validateArgPointerRead(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);

                validateArgOffset(i, 2, temps, file);
                break;
              }
              case IO_MEM_LOAD: {
                validateArgWritable(i, 0, temps, file);

                validateArgPointerRead(i, 1, temps, file);

                validateArgOffset(i, 2, temps, file);
                break;
              }
              case IO_STK_STORE: {
                validateArgOffset(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);
                break;
              }
              case IO_STK_LOAD: {
                validateArgWritable(i, 0, temps, file);

                validateArgOffset(i, 1, temps, file);
                break;
              }
              case IO_OFFSET_STORE: {
                validateArgOffsettable(i, 0, file);
                if (i->args[0]->kind == OK_TEMP)
                  validateTempWrite(temps, i->args[0], file);

                validateArgRead(i, 1, temps, file);

                validateArgOffset(i, 2, temps, file);
                break;
              }
              case IO_OFFSET_LOAD: {
                validateArgWritable(i, 0, temps, file);

                validateArgOffsettable(i, 1, file);
                if (i->args[1]->kind == OK_TEMP)
                  validateTempRead(temps, i->args[1], file);

                validateArgOffset(i, 2, temps, file);
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
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, file);
                  validateTempWrite(temps, i->args[0], file);
                }

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempGP(i, 2, file);

                validateArgsSameSize(i, 0, 1, file);
                validateArgsSameSize(i, 0, 2, file);
                validateArgsSameSize(i, 1, 2, file);
                break;
              }
              case IO_FADD:
              case IO_FSUB:
              case IO_FMUL:
              case IO_FDIV:
              case IO_FMOD: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempFP(i, 0, file);
                  validateTempWrite(temps, i->args[0], file);
                }

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempFP(i, 1, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempFP(i, 2, file);

                validateArgsSameSize(i, 0, 1, file);
                validateArgsSameSize(i, 0, 2, file);
                validateArgsSameSize(i, 1, 2, file);
                break;
              }
              case IO_NEG:
              case IO_NOT: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, file);
                  validateTempWrite(temps, i->args[0], file);
                }

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                validateArgsSameSize(i, 0, 1, file);
                break;
              }
              case IO_FNEG: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempFP(i, 0, file);
                  validateTempWrite(temps, i->args[0], file);
                }

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempFP(i, 1, file);

                validateArgsSameSize(i, 0, 1, file);
                break;
              }
              case IO_SLL:
              case IO_SLR:
              case IO_SAR: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) {
                  validateTempGP(i, 0, file);
                  validateTempWrite(temps, i->args[0], file);
                }

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                validateArgByteRead(i, 2, temps, file);

                validateArgsSameSize(i, 0, 1, file);
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
                validateArgByteWritten(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempGP(i, 2, file);

                validateArgsSameSize(i, 1, 2, file);
                break;
              }
              case IO_FL:
              case IO_FLE:
              case IO_FE:
              case IO_FNE:
              case IO_FG:
              case IO_FGE: {
                validateArgByteWritten(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempFP(i, 1, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempFP(i, 2, file);

                validateArgsSameSize(i, 1, 2, file);
                break;
              }
              case IO_Z:
              case IO_NZ: {
                validateArgByteWritten(i, 0, temps, file);

                validateArgRead(i, 1, temps, file);
                break;
              }
              case IO_LNOT: {
                validateArgByteWritten(i, 0, temps, file);

                validateArgByteRead(i, 1, temps, file);
                break;
              }
              case IO_SX:
              case IO_ZX: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) validateTempGP(i, 0, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                if (irOperandSizeof(i->args[0]) <=
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation failed - "
                          "%s instruction's argument 0 is not larger than "
                          "argument 1\n",
                          file->inputFilename, IROPERATOR_NAMES[i->op]);
                  file->errored = true;
                }
                break;
              }
              case IO_TRUNC: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) validateTempGP(i, 0, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);

                if (irOperandSizeof(i->args[0]) >=
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation failed - "
                          "TRUNC instruction's argument 0 is not smaller than "
                          "argument 1\n",
                          file->inputFilename);
                  file->errored = true;
                }
                break;
              }
              case IO_U2F:
              case IO_S2F: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) validateTempFP(i, 0, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempGP(i, 1, file);
                break;
              }
              case IO_FRESIZE: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) validateTempFP(i, 0, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempFP(i, 1, file);

                if (irOperandSizeof(i->args[0]) ==
                    irOperandSizeof(i->args[1])) {
                  fprintf(stderr,
                          "%s: internal compiler error: IR validation failed - "
                          "TRUNC instruction's argument 0 and 1 are the same "
                          "size\n",
                          file->inputFilename);
                  file->errored = true;
                }
                break;
              }
              case IO_F2I: {
                validateArgWritable(i, 0, temps, file);
                if (i->args[0]->kind == OK_TEMP) validateTempGP(i, 0, file);

                validateArgRead(i, 1, temps, file);
                if (i->args[1]->kind == OK_TEMP) validateTempFP(i, 1, file);
                break;
              }
              case IO_JUMP: {
                validateArgJumpTarget(i, 0, temps, file);
                break;
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
              case IO_JBE: {
                validateArgKind(i, 0, OK_LABEL, file);

                validateArgKind(i, 1, OK_LABEL, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempGP(i, 2, file);

                validateArgRead(i, 3, temps, file);
                if (i->args[3]->kind == OK_TEMP) validateTempGP(i, 3, file);

                validateArgsSameSize(i, 2, 3, file);
                break;
              }
              case IO_JFL:
              case IO_JFLE:
              case IO_JFE:
              case IO_JFNE:
              case IO_JFG:
              case IO_JFGE: {
                validateArgKind(i, 0, OK_LABEL, file);

                validateArgKind(i, 1, OK_LABEL, file);

                validateArgRead(i, 2, temps, file);
                if (i->args[2]->kind == OK_TEMP) validateTempFP(i, 2, file);

                validateArgRead(i, 3, temps, file);
                if (i->args[3]->kind == OK_TEMP) validateTempFP(i, 3, file);

                validateArgsSameSize(i, 2, 3, file);
                break;
              }
              case IO_JZ:
              case IO_JNZ: {
                validateArgKind(i, 0, OK_LABEL, file);

                validateArgKind(i, 1, OK_LABEL, file);

                validateArgRead(i, 2, temps, file);
                break;
              }
              case IO_CALL: {
                validateArgJumpTarget(i, 0, temps, file);
                break;
              }
              case IO_RETURN: {
                // nothing to check
                break;
              }
              default: {
                error(__FILE__, __LINE__, "invalid IROperator enum");
              }
            }
          }
        }
        free(temps);
      }
    }
    errored = errored || file->errored;
  }

  if (errored) return -1;

  return 0;
}