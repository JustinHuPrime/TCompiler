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
      fprintf(where, "REG(%zu)", o->data.reg.name);
      break;
    }
    case OK_CONSTANT: {
      fprintf(where, "CONSTANT(");
      datumDump(where, o->data.constant.value);
      fprintf(where, ")");
      break;
    }
    case OK_ASM: {
      fprintf(where, "ASM(%s)", o->data.assembly.assembly);
      break;
    }
  }
}

static void instructionDump(FILE *where, IRInstruction *i) {
  switch (i->op) {
    case IO_ASM: {
      fprintf(where, "ASM(");
      operandDump(where, i->arg1);
      fprintf(where, ")");
      break;
    }
    case IO_LABEL: {
      fprintf(where, "LABEL(");
      operandDump(where, i->arg1);
      fprintf(where, ")");
      break;
    }
    case IO_MOVE: {
      fprintf(where, "MOVE(");
      operandDump(where, i->dest);
      fprintf(where, ", ");
      operandDump(where, i->arg1);
      fprintf(where, ")");
      break;
    }
  }
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
      for (size_t idx = 0; idx < frag->data.text.instructions.size; ++idx) {
        fprintf(where, "  ");
        instructionDump(where, frag->data.text.instructions.elements[idx]);
        fprintf(where, ",\n");
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