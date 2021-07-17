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
      fprintf(where, "REG(%s)", prettyPrintRegister(o->data.reg.name));
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
    case OK_LABEL: {
      fprintf(where, "LABEL(%s)", o->data.label.name);
      break;
    }
    case OK_OFFSET: {
      fprintf(where, "OFFSET(%ld)", o->data.offset.offset);
      break;
    }
    case OK_ASM: {
      fprintf(where, "ASM(%s)", o->data.assembly.assembly);
      break;
    }
  }
}

static void zeroOperandInstructionDump(FILE *where, char const *name,
                                       IRInstruction *i) {
  fprintf(where, "%s(%zu)", name, i->size);
}
static void oneOperandInstructionDump(FILE *where, char const *name,
                                      IRInstruction *i) {
  fprintf(where, "%s(%zu, ", name, i->size);
  operandDump(where, i->arg1);
  fprintf(where, ")");
}
static void twoOperandInstructionDump(FILE *where, char const *name,
                                      IRInstruction *i) {
  fprintf(where, "%s(%zu, ", name, i->size);
  operandDump(where, i->dest);
  fprintf(where, ", ");
  operandDump(where, i->arg1);
  fprintf(where, ")");
}
static void threeOperandInstructionDump(FILE *where, char const *name,
                                        IRInstruction *i) {
  fprintf(where, "%s(%zu, ", name, i->size);
  operandDump(where, i->dest);
  fprintf(where, ", ");
  operandDump(where, i->arg1);
  fprintf(where, ", ");
  operandDump(where, i->arg2);
  fprintf(where, ")");
}
static void instructionDump(FILE *where, IRInstruction *i) {
  switch (i->op) {
    case IO_ASM: {
      oneOperandInstructionDump(where, "ASM", i);
      break;
    }
    case IO_LABEL: {
      oneOperandInstructionDump(where, "LABEL", i);
      break;
    }
    case IO_VOLATILE: {
      oneOperandInstructionDump(where, "VOLATILE", i);
      break;
    }
    case IO_ADDROF: {
      twoOperandInstructionDump(where, "ADDROF", i);
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
      threeOperandInstructionDump(where, "NEG", i);
      break;
    }
    case IO_FNEG: {
      threeOperandInstructionDump(where, "FNEG", i);
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
      threeOperandInstructionDump(where, "NOT", i);
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
    case IO_LNOT: {
      threeOperandInstructionDump(where, "LNOT", i);
      break;
    }
    case IO_SX_SHORT: {
      twoOperandInstructionDump(where, "SX_SHORT", i);
      break;
    }
    case IO_SX_INT: {
      twoOperandInstructionDump(where, "SX_INT", i);
      break;
    }
    case IO_SX_LONG: {
      twoOperandInstructionDump(where, "SX_LONG", i);
      break;
    }
    case IO_ZX_SHORT: {
      twoOperandInstructionDump(where, "ZX_SHORT", i);
      break;
    }
    case IO_ZX_INT: {
      twoOperandInstructionDump(where, "ZX_INT", i);
      break;
    }
    case IO_ZX_LONG: {
      twoOperandInstructionDump(where, "ZX_LONG", i);
      break;
    }
    case IO_TRUNC_BYTE: {
      twoOperandInstructionDump(where, "TRUNC_BYTE", i);
      break;
    }
    case IO_TRUNC_SHORT: {
      twoOperandInstructionDump(where, "TRUNC_SHORT", i);
      break;
    }
    case IO_TRUNC_INT: {
      twoOperandInstructionDump(where, "TRUNC_INT", i);
      break;
    }
    case IO_U2FLOAT: {
      twoOperandInstructionDump(where, "U2FLOAT", i);
      break;
    }
    case IO_U2DOUBLE: {
      twoOperandInstructionDump(where, "U2DOUBLE", i);
      break;
    }
    case IO_S2FLOAT: {
      twoOperandInstructionDump(where, "S2FLOAT", i);
      break;
    }
    case IO_S2DOUBLE: {
      twoOperandInstructionDump(where, "S2DOUBLE", i);
      break;
    }
    case IO_F2FLOAT: {
      twoOperandInstructionDump(where, "F2FLOAT", i);
      break;
    }
    case IO_F2DOUBLE: {
      twoOperandInstructionDump(where, "F2DOUBLE", i);
      break;
    }
    case IO_F2BYTE: {
      twoOperandInstructionDump(where, "F2BYTE", i);
      break;
    }
    case IO_F2SHORT: {
      twoOperandInstructionDump(where, "F2SHORT", i);
      break;
    }
    case IO_F2INT: {
      twoOperandInstructionDump(where, "F2INT", i);
      break;
    }
    case IO_F2LONG: {
      twoOperandInstructionDump(where, "F2LONG", i);
      break;
    }
    case IO_JUMP: {
      // note - jump is one operand, but the dest is the one operand
      fprintf(where, "JUMP(%zu, ", i->size);
      operandDump(where, i->dest);
      fprintf(where, ")");
      break;
    }
    case IO_JL: {
      threeOperandInstructionDump(where, "JL", i);
      break;
    }
    case IO_JLE: {
      threeOperandInstructionDump(where, "JLE", i);
      break;
    }
    case IO_JE: {
      threeOperandInstructionDump(where, "JE", i);
      break;
    }
    case IO_JNE: {
      threeOperandInstructionDump(where, "JNE", i);
      break;
    }
    case IO_JG: {
      threeOperandInstructionDump(where, "JG", i);
      break;
    }
    case IO_JGE: {
      threeOperandInstructionDump(where, "JGE", i);
      break;
    }
    case IO_JA: {
      threeOperandInstructionDump(where, "JA", i);
      break;
    }
    case IO_JAE: {
      threeOperandInstructionDump(where, "JAE", i);
      break;
    }
    case IO_JB: {
      threeOperandInstructionDump(where, "JB", i);
      break;
    }
    case IO_JBE: {
      threeOperandInstructionDump(where, "JBE", i);
      break;
    }
    case IO_JFL: {
      threeOperandInstructionDump(where, "JFL", i);
      break;
    }
    case IO_JFLE: {
      threeOperandInstructionDump(where, "JFLE", i);
      break;
    }
    case IO_JFE: {
      threeOperandInstructionDump(where, "JFE", i);
      break;
    }
    case IO_JFNE: {
      threeOperandInstructionDump(where, "JFNE", i);
      break;
    }
    case IO_JFG: {
      threeOperandInstructionDump(where, "JFG", i);
      break;
    }
    case IO_JFGE: {
      threeOperandInstructionDump(where, "JFGE", i);
      break;
    }
    case IO_JZ: {
      twoOperandInstructionDump(where, "JZ", i);
      break;
    }
    case IO_JNZ: {
      twoOperandInstructionDump(where, "JNZ", i);
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

static void fragDump(FILE *where, IRFrag *frag) {
  switch (frag->type) {
    case FT_BSS: {
      fprintf(where, "BSS(%s, %zu)\n", frag->name, frag->data.data.alignment);
      break;
    }
    case FT_RODATA: {
      fprintf(where, "RODATA(%s, %zu,\n", frag->name,
              frag->data.data.alignment);
      for (size_t idx = 0; idx < frag->data.data.data.size; ++idx) {
        fprintf(where, "  ");
        datumDump(where, frag->data.data.data.elements[idx]);
        fprintf(where, ",\n");
      }
      fprintf(where, ")\n");
      break;
    }
    case FT_DATA: {
      fprintf(where, "DATA(%s, %zu,\n", frag->name, frag->data.data.alignment);
      for (size_t idx = 0; idx < frag->data.data.data.size; ++idx) {
        fprintf(where, "  ");
        datumDump(where, frag->data.data.data.elements[idx]);
        fprintf(where, ",\n");
      }
      fprintf(where, ")\n");
      break;
    }
    case FT_TEXT: {
      fprintf(where, "TEXT(%s,\n", frag->name);
      for (size_t idx = 0; idx < frag->data.text.blocks.size; ++idx)
        blockDump(where, frag->data.text.blocks.elements[idx]);
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