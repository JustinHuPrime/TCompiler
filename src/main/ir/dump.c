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

static char const *const ALLOCHINT_NAMES[] = {
    "GP",
    "MEM",
    "FP",
};

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
    case DT_LABEL: {
      fprintf(where, "LABEL(");
      fprintf(where, localLabelFormat(), datum->data.label);
      fprintf(where, ")");
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
  switch (i->op) {
    case IO_LABEL: {
      oneOperandInstructionDump(where, "LABEL", i);
      break;
    }
    case IO_VOLATILE: {
      oneOperandInstructionDump(where, "VOLATILE", i);
      break;
    }
    case IO_UNINITIALIZED: {
      oneOperandInstructionDump(where, "UNINITIALIZED", i);
      break;
    }
    case IO_ADDROF: {
      twoOperandInstructionDump(where, "ADDROF", i);
      break;
    }
    case IO_NOP: {
      zeroOperandInstructionDump(where, "NOP", i);
      break;
    }
    case IO_MOVE: {
      twoOperandInstructionDump(where, "MOVE", i);
      break;
    }
    case IO_MEM_STORE: {
      threeOperandInstructionDump(where, "MEM_STORE", i);
      break;
    }
    case IO_MEM_LOAD: {
      threeOperandInstructionDump(where, "MEM_LOAD", i);
      break;
    }
    case IO_STK_STORE: {
      twoOperandInstructionDump(where, "STK_STORE", i);
      break;
    }
    case IO_STK_LOAD: {
      twoOperandInstructionDump(where, "STK_LOAD", i);
      break;
    }
    case IO_OFFSET_STORE: {
      threeOperandInstructionDump(where, "OFFSET_STORE", i);
      break;
    }
    case IO_OFFSET_LOAD: {
      threeOperandInstructionDump(where, "OFFSET_LOAD", i);
      break;
    }
    case IO_ADD: {
      threeOperandInstructionDump(where, "ADD", i);
      break;
    }
    case IO_FADD: {
      threeOperandInstructionDump(where, "FADD", i);
      break;
    }
    case IO_SUB: {
      threeOperandInstructionDump(where, "SUB", i);
      break;
    }
    case IO_FSUB: {
      threeOperandInstructionDump(where, "FSUB", i);
      break;
    }
    case IO_SMUL: {
      threeOperandInstructionDump(where, "SMUL", i);
      break;
    }
    case IO_UMUL: {
      threeOperandInstructionDump(where, "UMUL", i);
      break;
    }
    case IO_FMUL: {
      threeOperandInstructionDump(where, "FMUL", i);
      break;
    }
    case IO_SDIV: {
      threeOperandInstructionDump(where, "SDIV", i);
      break;
    }
    case IO_UDIV: {
      threeOperandInstructionDump(where, "UDIV", i);
      break;
    }
    case IO_FDIV: {
      threeOperandInstructionDump(where, "FDIV", i);
      break;
    }
    case IO_SMOD: {
      threeOperandInstructionDump(where, "SMOD", i);
      break;
    }
    case IO_UMOD: {
      threeOperandInstructionDump(where, "UMOD", i);
      break;
    }
    case IO_FMOD: {
      threeOperandInstructionDump(where, "FMOD", i);
      break;
    }
    case IO_NEG: {
      twoOperandInstructionDump(where, "NEG", i);
      break;
    }
    case IO_FNEG: {
      twoOperandInstructionDump(where, "FNEG", i);
      break;
    }
    case IO_SLL: {
      threeOperandInstructionDump(where, "SLL", i);
      break;
    }
    case IO_SLR: {
      threeOperandInstructionDump(where, "SLR", i);
      break;
    }
    case IO_SAR: {
      threeOperandInstructionDump(where, "SAR", i);
      break;
    }
    case IO_AND: {
      threeOperandInstructionDump(where, "AND", i);
      break;
    }
    case IO_XOR: {
      threeOperandInstructionDump(where, "XOR", i);
      break;
    }
    case IO_OR: {
      threeOperandInstructionDump(where, "OR", i);
      break;
    }
    case IO_NOT: {
      twoOperandInstructionDump(where, "NOT", i);
      break;
    }
    case IO_L: {
      threeOperandInstructionDump(where, "L", i);
      break;
    }
    case IO_LE: {
      threeOperandInstructionDump(where, "LE", i);
      break;
    }
    case IO_E: {
      threeOperandInstructionDump(where, "E", i);
      break;
    }
    case IO_NE: {
      threeOperandInstructionDump(where, "NE", i);
      break;
    }
    case IO_G: {
      threeOperandInstructionDump(where, "G", i);
      break;
    }
    case IO_GE: {
      threeOperandInstructionDump(where, "GE", i);
      break;
    }
    case IO_A: {
      threeOperandInstructionDump(where, "A", i);
      break;
    }
    case IO_AE: {
      threeOperandInstructionDump(where, "AE", i);
      break;
    }
    case IO_B: {
      threeOperandInstructionDump(where, "B", i);
      break;
    }
    case IO_BE: {
      threeOperandInstructionDump(where, "BE", i);
      break;
    }
    case IO_FL: {
      threeOperandInstructionDump(where, "FL", i);
      break;
    }
    case IO_FLE: {
      threeOperandInstructionDump(where, "FLE", i);
      break;
    }
    case IO_FE: {
      threeOperandInstructionDump(where, "FE", i);
      break;
    }
    case IO_FNE: {
      threeOperandInstructionDump(where, "FNE", i);
      break;
    }
    case IO_FG: {
      threeOperandInstructionDump(where, "FG", i);
      break;
    }
    case IO_FGE: {
      threeOperandInstructionDump(where, "FGE", i);
      break;
    }
    case IO_Z: {
      twoOperandInstructionDump(where, "Z", i);
      break;
    }
    case IO_NZ: {
      twoOperandInstructionDump(where, "NZ", i);
      break;
    }
    case IO_LNOT: {
      twoOperandInstructionDump(where, "LNOT", i);
      break;
    }
    case IO_SX: {
      twoOperandInstructionDump(where, "SX", i);
      break;
    }
    case IO_ZX: {
      twoOperandInstructionDump(where, "ZX", i);
      break;
    }
    case IO_TRUNC: {
      twoOperandInstructionDump(where, "TRUNC", i);
      break;
    }
    case IO_U2F: {
      twoOperandInstructionDump(where, "UNSIGNED2FLOATING", i);
      break;
    }
    case IO_S2F: {
      twoOperandInstructionDump(where, "SIGNED2FLOATING", i);
      break;
    }
    case IO_FRESIZE: {
      twoOperandInstructionDump(where, "RESIZEFLOATING", i);
      break;
    }
    case IO_F2I: {
      twoOperandInstructionDump(where, "FLOATING2INTEGRAL", i);
      break;
    }
    case IO_JUMP: {
      oneOperandInstructionDump(where, "JUMP", i);
      break;
    }
    case IO_J2L: {
      fourOperandInstructionDump(where, "J2L", i);
      break;
    }
    case IO_J2LE: {
      fourOperandInstructionDump(where, "J2LE", i);
      break;
    }
    case IO_J2E: {
      fourOperandInstructionDump(where, "J2E", i);
      break;
    }
    case IO_J2NE: {
      fourOperandInstructionDump(where, "J2NE", i);
      break;
    }
    case IO_J2G: {
      fourOperandInstructionDump(where, "J2G", i);
      break;
    }
    case IO_J2GE: {
      fourOperandInstructionDump(where, "J2GE", i);
      break;
    }
    case IO_J2A: {
      fourOperandInstructionDump(where, "J2A", i);
      break;
    }
    case IO_J2AE: {
      fourOperandInstructionDump(where, "J2AE", i);
      break;
    }
    case IO_J2B: {
      fourOperandInstructionDump(where, "J2B", i);
      break;
    }
    case IO_J2BE: {
      fourOperandInstructionDump(where, "J2BE", i);
      break;
    }
    case IO_J2FL: {
      fourOperandInstructionDump(where, "J2FL", i);
      break;
    }
    case IO_J2FLE: {
      fourOperandInstructionDump(where, "J2FLE", i);
      break;
    }
    case IO_J2FE: {
      fourOperandInstructionDump(where, "J2FE", i);
      break;
    }
    case IO_J2FNE: {
      fourOperandInstructionDump(where, "J2FNE", i);
      break;
    }
    case IO_J2FG: {
      fourOperandInstructionDump(where, "J2FG", i);
      break;
    }
    case IO_J2FGE: {
      fourOperandInstructionDump(where, "J2FGE", i);
      break;
    }
    case IO_J2Z: {
      threeOperandInstructionDump(where, "J2Z", i);
      break;
    }
    case IO_J2NZ: {
      threeOperandInstructionDump(where, "J2NZ", i);
      break;
    }
    case IO_J1L: {
      threeOperandInstructionDump(where, "J1L", i);
      break;
    }
    case IO_J1LE: {
      threeOperandInstructionDump(where, "J1LE", i);
      break;
    }
    case IO_J1E: {
      threeOperandInstructionDump(where, "J1E", i);
      break;
    }
    case IO_J1NE: {
      threeOperandInstructionDump(where, "J1NE", i);
      break;
    }
    case IO_J1G: {
      threeOperandInstructionDump(where, "J1G", i);
      break;
    }
    case IO_J1GE: {
      threeOperandInstructionDump(where, "J1GE", i);
      break;
    }
    case IO_J1A: {
      threeOperandInstructionDump(where, "J1A", i);
      break;
    }
    case IO_J1AE: {
      threeOperandInstructionDump(where, "J1AE", i);
      break;
    }
    case IO_J1B: {
      threeOperandInstructionDump(where, "J1B", i);
      break;
    }
    case IO_J1BE: {
      threeOperandInstructionDump(where, "J1BE", i);
      break;
    }
    case IO_J1FL: {
      threeOperandInstructionDump(where, "J1FL", i);
      break;
    }
    case IO_J1FLE: {
      threeOperandInstructionDump(where, "J1FLE", i);
      break;
    }
    case IO_J1FE: {
      threeOperandInstructionDump(where, "J1FE", i);
      break;
    }
    case IO_J1FNE: {
      threeOperandInstructionDump(where, "J1FNE", i);
      break;
    }
    case IO_J1FG: {
      threeOperandInstructionDump(where, "J1FG", i);
      break;
    }
    case IO_J1FGE: {
      threeOperandInstructionDump(where, "J1FGE", i);
      break;
    }
    case IO_J1Z: {
      twoOperandInstructionDump(where, "J1Z", i);
      break;
    }
    case IO_J1NZ: {
      twoOperandInstructionDump(where, "J1NZ", i);
      break;
    }
    case IO_CALL: {
      oneOperandInstructionDump(where, "CALL", i);
      break;
    }
    case IO_RETURN: {
      zeroOperandInstructionDump(where, "RETURN", i);
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