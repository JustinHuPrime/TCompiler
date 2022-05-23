// Copyright 2020-2022 Justin Hu
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

#include "arch/x86_64-linux/asm.h"

#include "fileList.h"
#include "ir/ir.h"
#include "translation/translation.h"
#include "util/container/stringBuilder.h"
#include "util/internalError.h"
#include "util/numericSizing.h"

size_t const X86_64_LINUX_REGISTER_WIDTH = 8;
size_t const X86_64_LINUX_STACK_ALIGNMENT = 16;

static char const *const REGISTER_NAMES[] = {
    "rax",  "rbx",  "rcx",   "rdx",   "rsi",   "rdi",   "rsp",   "rbp",
    "r8",   "r9",   "r10",   "r11",   "r12",   "r13",   "r14",   "r15",
    "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
};

char const *x86_64LinuxPrettyPrintRegister(size_t reg) {
  return REGISTER_NAMES[reg];
}

static X86_64LinuxOperand *x86_64LinuxOperandCreateBase(
    X86_64LinuxOperandKind kind) {
  X86_64LinuxOperand *retval = malloc(sizeof(X86_64LinuxOperand));
  retval->kind = kind;
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxRegOperandCreate(X86_64LinuxRegister reg,
                                                       size_t size) {
  X86_64LinuxOperand *retval =
      x86_64LinuxOperandCreateBase(X86_64_LINUX_OK_REG);
  retval->data.reg.reg = reg;
  if (size <= 1) {
    retval->data.reg.size = 1;
  } else if (size <= 2) {
    retval->data.reg.size = 2;
  } else if (size <= 4) {
    retval->data.reg.size = 4;
  } else if (size <= 8) {
    retval->data.reg.size = 8;
  } else if (size <= 16 && reg >= X86_64_LINUX_XMM0) {
    retval->data.reg.size = 16;
  } else {
    error(__FILE__, __LINE__, "Invalid register size");
  }
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxTempOperandCreate(IROperand const *temp,
                                                        bool escapes) {
  X86_64LinuxOperand *retval =
      x86_64LinuxOperandCreateBase(X86_64_LINUX_OK_TEMP);
  retval->data.temp.name = temp->data.temp.name;
  retval->data.temp.alignment = temp->data.temp.alignment;
  retval->data.temp.size = temp->data.temp.size;
  retval->data.temp.kind = temp->data.temp.kind;
  retval->data.temp.escapes = escapes;
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxTempOperandCreateEscaping(
    IROperand const *temp) {
  return x86_64LinuxTempOperandCreate(temp, true);
}
static X86_64LinuxOperand *x86_64LinuxTempOperandCreatePatch(
    IROperand const *temp, size_t name, AllocHint kind) {
  X86_64LinuxOperand *retval =
      x86_64LinuxOperandCreateBase(X86_64_LINUX_OK_TEMP);
  retval->data.temp.name = name;
  retval->data.temp.alignment = temp->data.temp.alignment;
  retval->data.temp.size = temp->data.temp.size;
  retval->data.temp.kind = kind;
  retval->data.temp.escapes = false;
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxTempOperandCreateConstOffset(
    IROperand const *temp, char *offset) {
  X86_64LinuxOperand *retval =
      x86_64LinuxOperandCreateBase(X86_64_LINUX_OK_OFFSET_TEMP);
  retval->data.offsetTemp.base = x86_64LinuxTempOperandCreate(temp, false);
  retval->data.offsetTemp.kind = X86_64_LINUX_TOK_CONSTANT;
  retval->data.offsetTemp.offset.constant = offset;
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxTempOperandCreateOperandOffset(
    IROperand const *temp, X86_64LinuxOperand *operand) {
  X86_64LinuxOperand *retval =
      x86_64LinuxOperandCreateBase(X86_64_LINUX_OK_OFFSET_TEMP);
  retval->data.offsetTemp.base = x86_64LinuxTempOperandCreate(temp, false);
  retval->data.offsetTemp.kind = X86_64_LINUX_TOK_OPERAND;
  retval->data.offsetTemp.offset.operand = operand;
  return retval;
}
static X86_64LinuxOperand *x86_64LinuxOperandCreate(IROperand const *op) {
  switch (op->kind) {
    case OK_REG: {
      return x86_64LinuxRegOperandCreate(op->data.reg.name, op->data.reg.size);
    }
    case OK_TEMP: {
      return x86_64LinuxTempOperandCreate(op, false);
    }
    case OK_CONSTANT:
    default: {
      error(__FILE__, __LINE__, "unexpected operand kind");
    }
  }
}
static void x86_64LinuxOperandFree(X86_64LinuxOperand *o) {
  switch (o->kind) {
    case X86_64_LINUX_OK_REG:
    case X86_64_LINUX_OK_TEMP: {
      break;  // nothing to free
    }
    case X86_64_LINUX_OK_OFFSET_TEMP: {
      x86_64LinuxOperandFree(o->data.offsetTemp.base);
      switch (o->data.offsetTemp.kind) {
        case X86_64_LINUX_TOK_CONSTANT: {
          free(o->data.offsetTemp.offset.constant);
          break;
        }
        case X86_64_LINUX_TOK_OPERAND: {
          x86_64LinuxOperandFree(o->data.offsetTemp.offset.operand);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "unexpected offset temp kind");
        }
      }
      break;
    }
    default: {
      error(__FILE__, __LINE__, "unexpected operand kind");
    }
  }
  free(o);
}

static void x86_64LinuxInstructionFree(X86_64LinuxInstruction *i) {
  free(i->skeleton);
  vectorUninit(&i->defines, (void (*)(void *))x86_64LinuxOperandFree);
  vectorUninit(&i->uses, (void (*)(void *))x86_64LinuxOperandFree);
  switch (i->kind) {
    case X86_64_LINUX_IK_JUMP:
    case X86_64_LINUX_IK_JUMPTABLE:
    case X86_64_LINUX_IK_CJUMP: {
      sizeVectorUninit(&i->data.jumpTargets);
      break;
    }
    default: {
      break;
    }
  }
  free(i);
}

static X86_64LinuxFrag *x86_64LinuxFragCreate(X86_64LinuxFragKind kind) {
  X86_64LinuxFrag *retval = malloc(sizeof(X86_64LinuxFrag));
  retval->kind = kind;
  return retval;
}
static X86_64LinuxFrag *x86_64LinuxDataFragCreate(char *data) {
  X86_64LinuxFrag *retval = x86_64LinuxFragCreate(X86_64_LINUX_FK_DATA);
  retval->data.data.data = data;
  return retval;
}
static X86_64LinuxFrag *x86_64LinuxTextFragCreate(char *header, char *footer) {
  X86_64LinuxFrag *retval = x86_64LinuxFragCreate(X86_64_LINUX_FK_TEXT);
  retval->data.text.header = header;
  retval->data.text.footer = footer;
  linkedListInit(&retval->data.text.instructions);
  return retval;
}
static void x86_64LinuxFragFree(X86_64LinuxFrag *frag) {
  switch (frag->kind) {
    case X86_64_LINUX_FK_TEXT: {
      free(frag->data.text.header);
      free(frag->data.text.footer);
      linkedListUninit(&frag->data.text.instructions,
                       (void (*)(void *))x86_64LinuxInstructionFree);
      break;
    }
    case X86_64_LINUX_FK_DATA: {
      free(frag->data.data.data);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid X86_64LinuxFrag");
    }
  }
  free(frag);
}

static X86_64LinuxFile *x86_64LinuxFileCreate(char *header, char *footer) {
  X86_64LinuxFile *retval = malloc(sizeof(X86_64LinuxFile));
  retval->header = header;
  retval->footer = footer;
  vectorInit(&retval->frags);
  return retval;
}
void x86_64LinuxFileFree(X86_64LinuxFile *file) {
  free(file->header);
  free(file->footer);
  vectorUninit(&file->frags, (void (*)(void *))x86_64LinuxFragFree);
  free(file);
}

static char *x86_64LinuxDataToString(Vector *v) {
  char *data = strdup("");
  for (size_t idx = 0; idx < v->size; ++idx) {
    IRDatum *d = v->elements[idx];
    switch (d->type) {
      case DT_BYTE: {
        char *old = data;
        data = format("%s\tdb %hhu\n", old, d->data.byteVal);
        free(old);
        break;
      }
      case DT_SHORT: {
        char *old = data;
        data = format("%s\tdw %hu\n", old, d->data.shortVal);
        free(old);
        break;
      }
      case DT_INT: {
        char *old = data;
        data = format("%s\tdd %u\n", old, d->data.intVal);
        free(old);
        break;
      }
      case DT_LONG: {
        char *old = data;
        data = format("%s\tdq %lu\n", old, d->data.longVal);
        free(old);
        break;
      }
      case DT_PADDING: {
        char *old = data;
        data = format("%s\tresb %zu\n", old, d->data.paddingLength);
        free(old);
        break;
      }
      case DT_STRING: {
        for (uint8_t *in = d->data.string; *in != 0; ++in) {
          char *old = data;
          data = format("%s\tdb %hhu\n", old, *in);
          free(old);
        }
        char *old = data;
        data = format("%s\tdb 0\n", old);
        free(old);
        break;
      }
      case DT_WSTRING: {
        for (uint32_t *in = d->data.wstring; *in != 0; ++in) {
          char *old = data;
          data = format("%s\tdd %u\n", old, *in);
          free(old);
        }
        char *old = data;
        data = format("%s\tdd 0\n", old);
        free(old);
        break;
      }
      case DT_LOCAL: {
        char *old = data;
        data = format("%s\tdq L%zu\n", old, d->data.localLabel);
        free(old);
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid datum type");
      }
    }
  }
  return data;
}

static X86_64LinuxFrag *x86_64LinuxGenerateDataAsm(IRFrag *frag) {
  char *section;
  switch (frag->type) {
    case FT_BSS: {
      section = format("section .bss align=%zu\n", frag->data.data.alignment);
      break;
    }
    case FT_RODATA: {
      section =
          format("section .rodata align=%zu\n", frag->data.data.alignment);
      break;
    }
    case FT_DATA: {
      section = format("section .data align=%zu\n", frag->data.data.alignment);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid data fragment type");
    }
  }
  char *name;
  switch (frag->nameType) {
    case FNT_LOCAL: {
      name = format("L%zu:\n", frag->name.local);
      break;
    }
    case FNT_GLOBAL: {
      name = format("global %s:data (%s.end - %s)\n%s:\n", frag->name.global,
                    frag->name.global, frag->name.global, frag->name.global);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid fragment name type");
    }
  }
  char *data = x86_64LinuxDataToString(&frag->data.data.data);

  X86_64LinuxFrag *retval =
      x86_64LinuxDataFragCreate(format("%s%s%s.end\n", section, name, data));
  free(section);
  free(name);
  free(data);
  return retval;
}

static X86_64LinuxInstruction *INST(X86_64LinuxInstructionKind kind,
                                    char *skeleton) {
  X86_64LinuxInstruction *retval = malloc(sizeof(X86_64LinuxInstruction));
  retval->kind = kind;
  retval->skeleton = skeleton;
  vectorInit(&retval->defines);
  vectorInit(&retval->uses);
  switch (retval->kind) {
    case X86_64_LINUX_IK_JUMP:
    case X86_64_LINUX_IK_JUMPTABLE:
    case X86_64_LINUX_IK_CJUMP: {
      sizeVectorInit(&retval->data.jumpTargets);
      break;
    }
    case X86_64_LINUX_IK_REGULAR: {
      retval->data.move.from = NULL;
      retval->data.move.to = NULL;
      break;
    }
    default: {
      break;
    }
  }
  return retval;
}
static void DEFINES(X86_64LinuxInstruction *i, X86_64LinuxOperand *arg) {
  vectorInsert(&i->defines, arg);
}
static void USES(X86_64LinuxInstruction *i, X86_64LinuxOperand *arg) {
  vectorInsert(&i->uses, arg);
}
static void MOVES(X86_64LinuxInstruction *i, size_t dest, size_t src) {
  i->data.move.from = i->uses.elements[src];
  i->data.move.to = i->defines.elements[dest];
}
static void DONE(X86_64LinuxFrag *assembly, X86_64LinuxInstruction *i) {
  insertNodeEnd(&assembly->data.text.instructions, i);
}

static bool isGpReg(IROperand const *o) {
  return o->kind == OK_REG && /* X86_64_LINUX_RAX <= o->data.reg.name && */
         o->data.reg.name <= X86_64_LINUX_R15;
}
static bool isFpReg(IROperand const *o) {
  return o->kind == OK_REG && X86_64_LINUX_XMM0 <= o->data.reg.name &&
         o->data.reg.name <= X86_64_LINUX_XMM15;
}

/**
 * encode a non-label constant as a 64 bit unsigned integer
 *
 * @param constant constant to encode - must be 8 bytes or smaller
 * @returns encoded value
 */
static uint64_t x86_64LinuxConstantToNumber(IROperand const *constant) {
  uint8_t bytes[8];
  memset(&bytes, 0, 8);

  // encode into bytes
  size_t next = 0;
  for (size_t idx = 0; idx < constant->data.constant.data.size; ++idx) {
    IRDatum const *datum = constant->data.constant.data.elements[idx];
    switch (datum->type) {
      case DT_BYTE: {
        bytes[next] = datum->data.byteVal;
        next += 1;
        break;
      }
      case DT_SHORT: {
        bytes[next] = (uint8_t)(datum->data.shortVal >> 0) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.shortVal >> 8) & 0xff;
        next += 1;
        break;
      }
      case DT_INT: {
        bytes[next] = (uint8_t)(datum->data.intVal >> 0) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.intVal >> 8) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.intVal >> 16) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.intVal >> 24) & 0xff;
        next += 1;
        break;
      }
      case DT_LONG: {
        bytes[next] = (uint8_t)(datum->data.longVal >> 0) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 8) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 16) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 24) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 32) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 40) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 48) & 0xff;
        next += 1;
        bytes[next] = (uint8_t)(datum->data.longVal >> 56) & 0xff;
        next += 1;
        break;
      }
      case DT_PADDING: {
        next += datum->data.paddingLength;
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid datum type");
      }
    }
  }

  // encode into little-endian
  uint64_t out = 0;
  for (size_t idx = 0; idx < 8; ++idx)
    out |= ((uint64_t)bytes[idx]) << (idx * 8);
  return out;
}

static char *x86_64LinuxSmallConstantToString(IROperand const *constant) {
  if (irOperandIsLocal(constant))
    return format("L%zu", localOperandName(constant));
  else if (irOperandIsGlobal(constant))
    return strdup(globalOperandName(constant));
  else
    return format("%lu", x86_64LinuxConstantToNumber(constant));
}

/**
 * generates the rest of a memcpy, returning the actual movs instruction
 *
 * assumes that RSI and RDI are already set
 */
static X86_64LinuxInstruction *x86_64LinuxFinishMemcpy(
    X86_64LinuxFrag *assembly, size_t size) {
  X86_64LinuxInstruction *i;

  size_t factor;
  char const *postfix;
  if (size % 8 == 0) {
    factor = 8;
    postfix = "q";
  } else if (size % 4 == 0) {
    factor = 4;
    postfix = "d";
  } else if (size % 2 == 0) {
    factor = 2;
    postfix = "w";
  } else {
    factor = 1;
    postfix = "b";
  }

  i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, %lu\n", size / factor));
  DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX, 8));
  DONE(assembly, i);

  i = INST(X86_64_LINUX_IK_REGULAR, format("\trep mov%s\n", postfix));
  USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
  USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
  USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX, 8));
  DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
  DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
  DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX, 8));
  return i;
}

/**
 * generate a fragment holding a constant
 */
static X86_64LinuxFrag *x86_64LinuxConstantToFrag(IROperand *constant,
                                                  size_t name) {
  char *data = x86_64LinuxDataToString(&constant->data.constant.data);
  X86_64LinuxFrag *retval = x86_64LinuxDataFragCreate(
      format("section .rodata align=%zu\nL%zu:\n%s.end\n",
             constant->data.constant.alignment, name, data));
  free(data);
  return retval;
}

static char const *x86_64LinuxSizeToPrefix(size_t size) {
  switch (size) {
    case 1: {
      return "BYTE";
    }
    case 2: {
      return "WORD";
    }
    case 4: {
      return "DWORD";
    }
    case 8: {
      return "QWORD";
    }
    default: {
      error(__FILE__, __LINE__, "invalid size");
    }
  }
}

static X86_64LinuxFrag *x86_64LinuxGenerateTextAsm(IRFrag *frag,
                                                   FileListEntry *file) {
  X86_64LinuxFile *asmFile = file->asmFile;
  X86_64LinuxFrag *assembly = x86_64LinuxTextFragCreate(
      format("section .text\nglobal %s:function\n%s:\n", frag->name.global,
             frag->name.global),
      format(".end\n"));
  IRBlock *b = frag->data.text.blocks.head->next->data;
  for (ListNode *currInst = b->instructions.head->next;
       currInst != b->instructions.tail; currInst = currInst->next) {
    IRInstruction *ir = currInst->data;
    X86_64LinuxInstruction *i;
    switch (ir->op) {
      case IO_LABEL: {
        // arg 0: local
        i = INST(X86_64_LINUX_IK_LABEL,
                 format("L%zu:\n", localOperandName(ir->args[0])));
        i->data.labelName = localOperandName(ir->args[0]);
        DONE(assembly, i);
        break;
      }
      case IO_VOLATILE: {
        // arg 0: temp
        i = INST(X86_64_LINUX_IK_REGULAR, strdup(""));  // empty instruction
        USES(i, x86_64LinuxOperandCreate(ir->args[0]));
        DONE(assembly, i);
        break;
      }
      case IO_UNINITIALIZED:
      case IO_NOP: {
        break;  // not translated
      }
      case IO_ADDROF: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: mem temp

        if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea `d, `u\n"));
          USES(i, x86_64LinuxTempOperandCreateEscaping(ir->args[1]));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1])) {
          size_t patchTemp = fresh(file);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea `d, `u\n"));
          USES(i, x86_64LinuxTempOperandCreateEscaping(ir->args[1]));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                       AH_GP));
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                    AH_GP));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to addrof");
        }
        break;
      }
      case IO_MOVE: {
        // arg 0: gp reg, fp reg, gp temp, fp temp, mem temp
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1])) ||
            (isGpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1])) ||
            (isMemTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1])) ||
                   (isFpReg(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov%c `d, `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'q' : 'd'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, %s\n", constant));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(constant);
        } else if ((isFpReg(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isFpReg(ir->args[0]) && isFpTemp(ir->args[1])) ||
                   (isFpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isFpTemp(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1])) ||
                   (isMemTemp(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isConst(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]))) {
          size_t constFrag = fresh(file);
          vectorInsert(&asmFile->frags,
                       x86_64LinuxConstantToFrag(ir->args[1], constFrag));
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c `d, [L%zu]\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', constFrag));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1])) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            // TODO: do we want to split this across two instructions? The move
            // can't be elided as is
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            // TODO: ditto for 16-byte moves
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 3, 3);
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1])) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov `d, %s\n", constant));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to move");
        }
        break;
      }
      case IO_MEM_STORE: {
        // arg 0: gp reg, gp temp, mem temp, const
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpTemp(ir->args[2]))) {
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov %s [`u + `u], `u\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov %s [`u + `u], `u\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov %s [`u + %s], `u\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                          constant));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);
        } else if ((isGpReg(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [`u + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [`u + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c [`u + %s], `u\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', constant));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + `u], `u\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t moveTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t moveTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + `u], `u\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + %s], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
            free(constant);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + %s], `u\n", constant));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
            free(constant);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [`u + %s]\n", constant));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
            free(constant);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *data = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + %s], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       offset, data));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
            free(data);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [`u + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(offset);
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov %s [`u + `u], `u\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov %s [`u + `u], `u\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                    AH_GP));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov %s [`u + %s], `u\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                          constant));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);
        } else if ((isMemTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                       AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [`u + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                    AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [`u + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                    AH_FP));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                       AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c [`u + %s], `u\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', constant));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                    AH_FP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + `u], `u\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1]))));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + `u], `u\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isConst(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + %s], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [`u + %s], `u\n", constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [`u + %s]\n", constant));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
          free(constant);
        } else if ((isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isConst(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *data = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [`u + %s], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       offset, data));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
            free(data);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [`u + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(offset);
        } else if ((isConst(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isConst(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov %s [%s + `u], `u\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                          constant));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);
        } else if ((isConst(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isConst(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *constant = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov %s [%s + `u], `u\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                          constant));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);
        } else if ((isConst(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isConst(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov %s [%s + %s], `u\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                          base, offset));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(base);
          free(offset);
        } else if ((isConst(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isConst(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [%s + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isConst(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isConst(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t patchTemp = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [%s + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], patchTemp,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isConst(ir->args[0]) && isFpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isConst(ir->args[0]) && isFpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [%s + %s], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base,
                          offset));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isConst(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [%s + `u], `u\n", base));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isConst(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t moveTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + `u], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t moveTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [%s + `u], `u\n", base));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], moveTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isConst(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isConst(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + %s], `u\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base, offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu [%s + %s], `u\n", base, offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + %s]\n", base, offset));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
          free(offset);
        } else if ((isConst(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isConst(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *data = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base, data));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);
            free(data);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isConst(ir->args[0]) && isConst(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base, constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isConst(ir->args[0]) && isConst(ir->args[1]) &&
                   isConst(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[0]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [%s + %s], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       base, offset, constant));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [%s + %s]\n", base, offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(base);
          free(offset);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to memory store");
        }
        break;
      }
      case IO_MEM_LOAD: {
        // arg 0: gp reg, fp reg, gp temp, fp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
             isGpTemp(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpReg(ir->args[2])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
             isGpTemp(ir->args[2]))) {
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov `d, %s [`u + `u]\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov `d, %s [`u + `u]\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, %s [`u + %s]\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                          offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(offset);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov `d, %s [`u + `u]\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmov `d, %s [`u + `u]\n",
                     x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, %s [`u + %s]\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                          offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          DONE(assembly, i);
          free(offset);
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, %s [%s + `u]\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                          base));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
          free(base);
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, %s [%s + `u]\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                          base));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
          free(base);
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, %s [%s + %s]\n",
                          x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                          base, offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(base);
          free(offset);
        } else if ((isFpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [`u + `u]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [`u + `u]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c `d, [`u + %s]\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(offset);
        } else if ((isFpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [`u + `u]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [`u + `u]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);

          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c `d, [`u + %s]\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                    AH_GP));
          DONE(assembly, i);
          free(offset);
        } else if ((isFpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [`u + %s]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
          free(base);
        } else if ((isFpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [%s + `u]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
          free(base);
        } else if ((isFpReg(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [%s + %s]\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', base,
                          offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(base);
          free(offset);
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isGpTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + `u]\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isMemTemp(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + `u]\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1]) &&
                    isConst(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]) &&
                    isConst(ir->args[2]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + %s]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                       offset));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + %s]\n", offset));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [`u + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(offset);
        } else if ((isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + `u]\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0]))));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + `u]\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, [`u + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isConst(ir->args[2])) {
          size_t basePatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [`u + %s]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                       offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [`u + %s]\n", offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [`u + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], basePatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(offset);
        } else if ((isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                    isGpTemp(ir->args[2]))) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [%s + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                       base));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [%s + `u]\n", base));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[2]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);

          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [%s + `u]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                       base));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [%s + `u]\n", base));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [%s + `u]\n", base));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(base);
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isConst(ir->args[2])) {
          char *base = x86_64LinuxSmallConstantToString(ir->args[1]);
          char *offset = x86_64LinuxSmallConstantToString(ir->args[2]);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov `d, %s [%s + %s]\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[0])),
                       base, offset));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [%s + %s]\n", base, offset));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            MOVES(i, 0, 0);
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [%s + %s]\n", base, offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(offset);
          free(base);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to memory load");
        }
        break;
      }
      case IO_STK_STORE: {
        // arg 0: gp reg, gp temp, mem temp, const
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isGpReg(ir->args[0]) && isFpTemp(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isFpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [rsp + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [rsp + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isMemTemp(ir->args[0]) && isFpTemp(ir->args[1]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c [rsp + `u], `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's'));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                    AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1])) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov [rsp + `u], `u\n"));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                      AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1])) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [rsp + `u], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
        } else if ((isConst(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isConst(ir->args[0]) && isGpTemp(ir->args[1]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov [rsp + %s], `u\n", offset));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(offset);
        } else if ((isConst(ir->args[0]) && isFpReg(ir->args[1])) ||
                   (isConst(ir->args[0]) && isFpTemp(ir->args[1]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c [rsp + %s], `u\n",
                     irOperandSizeof(ir->args[1]) == 8 ? 'd' : 's', offset));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          free(offset);
        } else if (isConst(ir->args[0]) && isMemTemp(ir->args[1])) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[0]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov [rsp + %s], `u\n", offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov [rsp + %s], `u\n", offset));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);
          }
          free(offset);
        } else if (isConst(ir->args[0]) && isConst(ir->args[1])) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[0]);
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s [rsp + %s], %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       offset, constant));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rdi, [rsp + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DONE(assembly, i);
          }
          free(offset);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to stack store");
        }
        break;
      }
      case IO_STK_LOAD: {
        // arg 0: gp reg, fp reg, gp temp, fp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        if ((isGpReg(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpReg(ir->args[0]) && isGpTemp(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
            (isGpTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, [rsp + `u]\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isMemTemp(ir->args[1]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, [rsp + `u]\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[0]) && isConst(ir->args[1])) ||
                   (isGpTemp(ir->args[0]) && isConst(ir->args[1]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, [rsp + %s]\n", offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(offset);
        } else if ((isFpReg(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isFpReg(ir->args[0]) && isGpTemp(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [rsp + `u]\n",
                          irOperandSizeof(ir->args[0]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isMemTemp(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isMemTemp(ir->args[1]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmovs%c `d, [rsp + `u]\n",
                          irOperandSizeof(ir->args[0]) == 8 ? 'd' : 's'));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                    AH_GP));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[0]) && isConst(ir->args[1])) ||
                   (isFpTemp(ir->args[0]) && isConst(ir->args[1]))) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(
              X86_64_LINUX_IK_REGULAR,
              format("\tmovs%c `d, [rsp + %s]\n",
                     irOperandSizeof(ir->args[0]) == 8 ? 'd' : 's', offset));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(offset);
        } else if ((isMemTemp(ir->args[0]) && isGpReg(ir->args[1])) ||
                   (isMemTemp(ir->args[0]) && isGpTemp(ir->args[1]))) {
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[0]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1])) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[0]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [rsp + `u]\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], offsetPatch,
                                                      AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1])) {
          char *offset = x86_64LinuxSmallConstantToString(ir->args[1]);
          if (irOperandSizeof(ir->args[0]) == 1 ||
              irOperandSizeof(ir->args[0]) == 2 ||
              irOperandSizeof(ir->args[0]) == 4 ||
              irOperandSizeof(ir->args[0]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov `d, [rsp + %s]\n", offset));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_GP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[0]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmovdqu `d, [rsp + %s]\n", offset));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                         AH_FP));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u"));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [rsp + %s]\n", offset));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[0]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
          free(offset);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to stack load");
        }
        break;
      }
      case IO_OFFSET_STORE: {
        // arg 0: mem temp
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        if ((isGpReg(ir->args[1]) && isGpReg(ir->args[2])) ||
            (isGpReg(ir->args[1]) && isGpTemp(ir->args[2])) ||
            (isGpTemp(ir->args[1]) && isGpReg(ir->args[2])) ||
            (isGpTemp(ir->args[1]) && isGpTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                         ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[1]) && isMemTemp(ir->args[2])) ||
                   (isGpTemp(ir->args[1]) && isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                         ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                          ir->args[2], offsetPatch, AH_GP)));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isGpReg(ir->args[1]) && isConst(ir->args[2])) ||
                   (isGpTemp(ir->args[1]) && isConst(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreateConstOffset(
                         ir->args[0],
                         x86_64LinuxSmallConstantToString(ir->args[2])));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[1]) && isGpReg(ir->args[2])) ||
                   (isFpReg(ir->args[1]) && isGpTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[1]) && isGpReg(ir->args[2])) ||
                   (isFpTemp(ir->args[1]) && isGpTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov%c `d, `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'q' : 'd'));
          DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                         ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[1]) && isMemTemp(ir->args[2])) ||
                   (isFpTemp(ir->args[1]) && isMemTemp(ir->args[2]))) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov%c `d, `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'q' : 'd'));
          DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                         ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                          ir->args[2], offsetPatch, AH_GP)));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isFpReg(ir->args[1]) && isConst(ir->args[2])) ||
                   (isFpTemp(ir->args[1]) && isConst(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov%c `d, `u\n",
                          irOperandSizeof(ir->args[1]) == 8 ? 'q' : 'd'));
          DEFINES(i, x86_64LinuxTempOperandCreateConstOffset(
                         ir->args[0],
                         x86_64LinuxSmallConstantToString(ir->args[2])));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
        } else if ((isMemTemp(ir->args[1]) && isGpReg(ir->args[2])) ||
                   (isMemTemp(ir->args[1]) && isGpTemp(ir->args[2]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateOperandOffset(
                        ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 3, 3);
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[1]) && isMemTemp(ir->args[2])) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t dataPatch = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                            ir->args[2], offsetPatch, AH_GP)));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], dataPatch,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                            ir->args[2], offsetPatch, AH_GP)));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateOperandOffset(
                        ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                         ir->args[2], offsetPatch, AH_GP)));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 3, 3);
            DONE(assembly, i);
          }
        } else if (isMemTemp(ir->args[1]) && isConst(ir->args[2])) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_GP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateConstOffset(
                           ir->args[0],
                           x86_64LinuxSmallConstantToString(ir->args[2])));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (irOperandSizeof(ir->args[1]) == 16) {
            size_t patchTemp = fresh(file);
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                         AH_FP));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 0, 0);
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateConstOffset(
                           ir->args[0],
                           x86_64LinuxSmallConstantToString(ir->args[2])));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[1], patchTemp,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rsi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateConstOffset(
                        ir->args[0],
                        x86_64LinuxSmallConstantToString(ir->args[2])));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            USES(i, x86_64LinuxOperandCreate(ir->args[1]));
            MOVES(i, 3, 3);
            DONE(assembly, i);
          }
        } else if ((isConst(ir->args[1]) && isGpReg(ir->args[2])) ||
                   (isConst(ir->args[1]) && isGpTemp(ir->args[2]))) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s `d, %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateOperandOffset(
                        ir->args[0], x86_64LinuxOperandCreate(ir->args[2])));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isConst(ir->args[1]) && isMemTemp(ir->args[2])) {
          size_t offsetPatch = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], offsetPatch,
                                                       AH_GP));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s `d, %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            DEFINES(i, x86_64LinuxTempOperandCreateOperandOffset(
                           ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                            ir->args[2], offsetPatch, AH_GP)));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateOperandOffset(
                        ir->args[0], x86_64LinuxTempOperandCreatePatch(
                                         ir->args[2], offsetPatch, AH_GP)));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else if (isConst(ir->args[1]) && isConst(ir->args[2])) {
          if (irOperandSizeof(ir->args[1]) == 1 ||
              irOperandSizeof(ir->args[1]) == 2 ||
              irOperandSizeof(ir->args[1]) == 4 ||
              irOperandSizeof(ir->args[1]) == 8) {
            char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
            i = INST(
                X86_64_LINUX_IK_REGULAR,
                format("\tmov %s `d, %s\n",
                       x86_64LinuxSizeToPrefix(irOperandSizeof(ir->args[1])),
                       constant));
            DEFINES(i, x86_64LinuxTempOperandCreateConstOffset(
                           ir->args[0],
                           x86_64LinuxSmallConstantToString(ir->args[2])));
            DONE(assembly, i);
            free(constant);
          } else {
            size_t constFrag = fresh(file);
            vectorInsert(&asmFile->frags,
                         x86_64LinuxConstantToFrag(ir->args[1], constFrag));
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tlea rsi, [L%zu]\n", constFrag));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, format("\tlea rdi, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
            USES(i, x86_64LinuxTempOperandCreateConstOffset(
                        ir->args[0],
                        x86_64LinuxSmallConstantToString(ir->args[2])));
            DONE(assembly, i);

            i = x86_64LinuxFinishMemcpy(assembly, irOperandSizeof(ir->args[1]));
            DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
            DONE(assembly, i);
          }
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to offset store");
        }
        break;
      }
      case IO_OFFSET_LOAD: {
        // arg 0: gp reg, fp reg, gp temp, fp temp, mem temp
        // arg 1: mem temp
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_ADD: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SUB: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SMUL: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_UMUL: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SDIV: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_UDIV: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SMOD: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_UMOD: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FADD: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FSUB: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FMUL: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FDIV: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FMOD: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_NEG: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FNEG: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SLL: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SLR: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SAR: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_AND: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_XOR: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const

        /* (R|T) (R|T|M) (R|T|M) */
        if ((isGpReg(ir->args[0]) || isGpTemp(ir->args[0])) &&
            (isGpReg(ir->args[1]) || isGpTemp(ir->args[1]) ||
             isMemTemp(ir->args[1])) &&
            (isGpReg(ir->args[2]) || isGpTemp(ir->args[2]) ||
             isMemTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);

          /* (R|T) C (R|T|M) */
        } else if ((isGpReg(ir->args[0]) || isGpTemp(ir->args[0])) &&
                   isConst(ir->args[1]) &&
                   (isGpReg(ir->args[2]) || isGpTemp(ir->args[2]) ||
                    isMemTemp(ir->args[2]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, %s\n", constant));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);

          /* (R|T) (R|T|M) C */
        } else if ((isGpReg(ir->args[0]) || isGpTemp(ir->args[0])) &&
                   (isGpReg(ir->args[1]) || isGpTemp(ir->args[1]) ||
                    isMemTemp(ir->args[1])) &&
                   isConst(ir->args[2])) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          MOVES(i, 0, 0);
          DONE(assembly, i);

          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, %s\n", constant));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(constant);

          /* (R|T) C C */
        } else if ((isGpReg(ir->args[0]) || isGpTemp(ir->args[0])) &&
                   isConst(ir->args[1]) && isConst(ir->args[2])) {
          char *constant1 = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\tmov `d, `%s\n", constant1));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(constant1);

          char *constant2 = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR,
                   format("\txor `d, %s\n", constant2));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          DONE(assembly, i);
          free(constant2);

          /* M (R|T) (R|T|M) */
          // XOR into the first argument, move into memory
        } else if (isMemTemp(ir->args[0]) &&
                   (isGpReg(ir->args[1]) || isGpTemp(ir->args[1])) &&
                   (isGpReg(ir->args[2]) || isGpTemp(ir->args[2]) ||
                    isMemTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[1]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);

          /* M (R|T) C */
        } else if (isMemTemp(ir->args[0]) &&
                   (isGpReg(ir->args[1]) || isGpTemp(ir->args[1])) &&
                   isConst(ir->args[2])) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[2]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, %s\n", constant));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);
          free(constant);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);

          /* M M (R|T) */
          // XOR into the second argument, move into memory
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   (isGpReg(ir->args[2]) || isGpTemp(ir->args[2]))) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[2]));
          USES(i, x86_64LinuxOperandCreate(ir->args[1]));
          DONE(assembly, i);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);

          /* M C (R|T) */
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   (isGpReg(ir->args[2]) || isGpTemp(ir->args[2]))) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[1]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\txor `d, %s\n", constant));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);
          free(constant);

          i = INST(X86_64_LINUX_IK_REGULAR, format("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxOperandCreate(ir->args[0]));
          USES(i, x86_64LinuxOperandCreate(ir->args[2]));
          DONE(assembly, i);

          /* M C M */
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          // TODO

          /* M M C */
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isConst(ir->args[2])) {
          // TODO

          /* M C C */
        } else if (isMemTemp(ir->args[0]) && isConst(ir->args[1]) &&
                   isConst(ir->args[2])) {
          // TODO

          /* M M M */
        } else if (isMemTemp(ir->args[0]) && isMemTemp(ir->args[1]) &&
                   isMemTemp(ir->args[2])) {
          // TODO

        } else {
          error(__FILE__, __LINE__, "unhandled arguments to xor");
        }
        break;
      }
      case IO_OR: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_NOT: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_L: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_LE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_E: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_NE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_G: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_GE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_A: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_AE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_B: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_BE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FL: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FLE: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FE: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FNE: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FG: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FGE: {
        // arg 0: fp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_Z: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // TODO
        // TODO: split this into IO_Z and IO_FZ at the IR level
        break;
      }
      case IO_NZ: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // TODO
        // TODO: ditto
        break;
      }
      case IO_LNOT: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_SX: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_ZX: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_TRUNC: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_U2F: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_S2F: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_FRESIZE: {
        // arg 0: fp reg, fp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_F2I: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_JUMP: {
        // arg 0: local
        // TODO
        break;
      }
      case IO_JUMPTABLE: {
        // arg 0: gp reg, gp temp, mem temp
        // arg 1: local
        // TODO
        break;
      }
      case IO_J1L: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1LE: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1E: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1NE: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1G: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1GE: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1A: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1AE: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1B: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1BE: {
        // arg 0: local
        // arg 1: gp reg, gp temp, mem temp, const
        // arg 2: gp reg, gp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FL: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FLE: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FE: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FNE: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FG: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1FGE: {
        // arg 0: local
        // arg 1: fp reg, fp temp, mem temp, const
        // arg 2: fp reg, fp temp, mem temp, const
        // TODO
        break;
      }
      case IO_J1Z: {
        // arg 0: local
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // TODO
        // TODO: also split this into two operations at IR level
        break;
      }
      case IO_J1NZ: {
        // arg 0: local
        // arg 1: gp reg, fp reg, gp temp, fp temp, mem temp, const
        // TODO
        // TODO: ditto
        break;
      }
      case IO_CALL: {
        // arg 0: reg, gp temp, mem temp, global, local
        if (isGpReg(ir->args[0]) || isGpTemp(ir->args[0]) ||
            isMemTemp(ir->args[0])) {
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tcall `u\n"));
          USES(i, x86_64LinuxOperandCreate(ir->args[0]));
        } else if (isConst(ir->args[0])) {
          char *constant = x86_64LinuxSmallConstantToString(ir->args[0]);
          i = INST(X86_64_LINUX_IK_REGULAR, format("\tcall %s\n", constant));
          free(constant);
        } else {
          error(__FILE__, __LINE__, "unhandled arguments to call");
        }
        // yes - I know this breaks the pattern, but I don't want to copy-paste
        // code
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RAX, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDX, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_R8, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_R9, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_R10, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_R11, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM0, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM1, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM2, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM3, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM4, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM5, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM6, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM7, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM8, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM9, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM10, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM11, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM12, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM13, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM14, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_XMM15, 8));
        DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RFLAGS, 8));
        DONE(assembly, i);
        break;
      }
      case IO_RETURN: {
        // no args
        i = INST(X86_64_LINUX_IK_LEAVE, format("\tret\n"));
        DONE(assembly, i);
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid IR opcode");
      }
    }
  }
  return assembly;
}
void x86_64LinuxGenerateAsm(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    X86_64LinuxFile *asmFile = file->asmFile =
        x86_64LinuxFileCreate(format("lprefix .\n"), strdup(""));

    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      switch (frag->type) {
        case FT_BSS:
        case FT_RODATA:
        case FT_DATA: {
          vectorInsert(&asmFile->frags, x86_64LinuxGenerateDataAsm(frag));
          break;
        }
        case FT_TEXT: {
          vectorInsert(&asmFile->frags, x86_64LinuxGenerateTextAsm(frag, file));
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid fragment type");
        }
      }
    }
  }
}