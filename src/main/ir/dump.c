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

#include "ir/dump.h"

#include <stdio.h>

#include "arch/interface.h"
#include "ir/ir.h"
#include "util/internalError.h"

static void datumDump(FILE *where, IRDatum *datum) {
  switch (datum->type) {
    case DT_BYTE: {
      fprintf(where, "BYTE(%hhu)", datum->data.byteVal);
      break;
    }
    case DT_SHORT: {
      fprintf(where, "SHORT(%hu)", datum->data.shortVal);
      break;
    }
    case DT_INT: {
      fprintf(where, "INT(%u)", datum->data.intVal);
      break;
    }
    case DT_LONG: {
      fprintf(where, "LONG(%lu)", datum->data.longVal);
      break;
    }
    case DT_PADDING: {
      fprintf(where, "PADDING(%zu)", datum->data.paddingLength);
      break;
    }
    case DT_STRING: {
      fprintf(where, "STRING(");
      for (uint8_t *c = datum->data.string; *c != 0; ++c) {
        fprintf(where, "%02hhX", *c);
      }
      fprintf(where, ")");
      break;
    }
    case DT_WSTRING: {
      fprintf(where, "WSTRING(");
      for (uint32_t *c = datum->data.wstring; *c != 0; ++c) {
        fprintf(where, "%08X", *c);
      }
      fprintf(where, ")");
      break;
    }
    case DT_LOCAL: {
      fprintf(where, "LABEL(LOCAL(%zu))", datum->data.localLabel);
      break;
    }
    case DT_GLOBAL: {
      fprintf(where, "LABEL(GLOBAL(%s))", datum->data.globalLabel);
      break;
    }
  }
}

static void operandDump(FILE *where, IROperand *o) {
  switch (o->kind) {
    case OK_TEMP: {
      fprintf(where, "TEMP(temp%zu, %zu, %zu, %s)", o->data.temp.name,
              o->data.temp.alignment, o->data.temp.size,
              ALLOCHINT_NAMES[o->data.temp.kind]);
      break;
    }
    case OK_REG: {
      fprintf(where, "REG(%s, %zu)", prettyPrintRegister(o->data.reg.name),
              o->data.reg.size);
      break;
    }
    case OK_CONSTANT: {
      fprintf(where, "CONSTANT(%zu", o->data.constant.alignment);
      for (size_t idx = 0; idx < o->data.constant.data.size; ++idx) {
        fprintf(where, ", ");
        datumDump(where, o->data.constant.data.elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case OK_GLOBAL: {
      fprintf(where, "GLOBAL(%s)", o->data.global.name);
      break;
    }
    case OK_LOCAL: {
      fprintf(where, "LOCAL(%zu)", o->data.local.name);
      break;
    }
  }
}

static void zeroOperandInstructionDump(FILE *where, char const *name,
                                       IRInstruction *i) {
  fprintf(where, "%s()", name);
}
static void oneOperandInstructionDump(FILE *where, char const *name,
                                      IRInstruction *i) {
  fprintf(where, "%s(", name);
  operandDump(where, i->args[0]);
  fprintf(where, ")");
}
static void twoOperandInstructionDump(FILE *where, char const *name,
                                      IRInstruction *i) {
  fprintf(where, "%s(", name);
  operandDump(where, i->args[0]);
  fprintf(where, ", ");
  operandDump(where, i->args[1]);
  fprintf(where, ")");
}
static void threeOperandInstructionDump(FILE *where, char const *name,
                                        IRInstruction *i) {
  fprintf(where, "%s(", name);
  operandDump(where, i->args[0]);
  fprintf(where, ", ");
  operandDump(where, i->args[1]);
  fprintf(where, ", ");
  operandDump(where, i->args[2]);
  fprintf(where, ")");
}
static void fourOperandInstructionDump(FILE *where, char const *name,
                                       IRInstruction *i) {
  fprintf(where, "%s(", name);
  operandDump(where, i->args[0]);
  fprintf(where, ", ");
  operandDump(where, i->args[1]);
  fprintf(where, ", ");
  operandDump(where, i->args[2]);
  fprintf(where, ", ");
  operandDump(where, i->args[3]);
  fprintf(where, ")");
}
static void instructionDump(FILE *where, IRInstruction *i) {
  switch (irOperatorArity(i->op)) {
    case 0: {
      zeroOperandInstructionDump(where, IROPERATOR_NAMES[i->op], i);
      break;
    }
    case 1: {
      oneOperandInstructionDump(where, IROPERATOR_NAMES[i->op], i);
      break;
    }
    case 2: {
      twoOperandInstructionDump(where, IROPERATOR_NAMES[i->op], i);
      break;
    }
    case 3: {
      threeOperandInstructionDump(where, IROPERATOR_NAMES[i->op], i);
      break;
    }
    case 4: {
      fourOperandInstructionDump(where, IROPERATOR_NAMES[i->op], i);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid IROperand enum");
    }
  }
}

static void blockDump(FILE *where, IRBlock *b) {
  fprintf(where, "  BLOCK(%zu,\n", b->label);
  for (ListNode *curr = b->instructions.head->next;
       curr != b->instructions.tail; curr = curr->next) {
    fprintf(where, "    ");
    instructionDump(where, curr->data);
    fprintf(where, ",\n");
  }
  fprintf(where, "  ),\n");
}

static void fragNameDump(FILE *where, IRFrag *frag) {
  switch (frag->nameType) {
    case FNT_GLOBAL: {
      fprintf(where, "GLOBAL(%s)", frag->name.global);
      break;
    }
    case FNT_LOCAL: {
      fprintf(where, "LOCAL(%zu)", frag->name.local);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid FragnementNameType enum encountered");
    }
  }
}
static void fragDump(FILE *where, IRFrag *frag) {
  switch (frag->type) {
    case FT_BSS: {
      fprintf(where, "BSS(");
      fragNameDump(where, frag);
      fprintf(where, ", %zu)\n", frag->data.data.alignment);
      break;
    }
    case FT_RODATA: {
      fprintf(where, "RODATA(");
      fragNameDump(where, frag);
      fprintf(where, ", %zu,\n", frag->data.data.alignment);
      for (size_t idx = 0; idx < frag->data.data.data.size; ++idx) {
        fprintf(where, "  ");
        datumDump(where, frag->data.data.data.elements[idx]);
        fprintf(where, ",\n");
      }
      fprintf(where, ")\n");
      break;
    }
    case FT_DATA: {
      fprintf(where, "DATA(");
      fragNameDump(where, frag);
      fprintf(where, ", %zu,\n", frag->data.data.alignment);
      for (size_t idx = 0; idx < frag->data.data.data.size; ++idx) {
        fprintf(where, "  ");
        datumDump(where, frag->data.data.data.elements[idx]);
        fprintf(where, ",\n");
      }
      fprintf(where, ")\n");
      break;
    }
    case FT_TEXT: {
      fprintf(where, "TEXT(");
      fragNameDump(where, frag);
      fprintf(where, ",\n");
      for (ListNode *curr = frag->data.text.blocks.head->next;
           curr != frag->data.text.blocks.tail; curr = curr->next) {
        blockDump(where, curr->data);
      }
      fprintf(where, ")\n");
      break;
    }
  }
}

void irDump(FILE *where, FileListEntry *entry) {
  fprintf(where, "%s:\n", entry->inputFilename);
  for (size_t idx = 0; idx < entry->irFrags.size; idx++)
    fragDump(where, entry->irFrags.elements[idx]);
}