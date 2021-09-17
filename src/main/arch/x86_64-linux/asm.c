// Copyright 2020-2021 Justin Hu
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

static X86_64LinuxOperand *x86_64LinuxOperandCreate(
    X86_64LinuxOperandKind kind) {
  X86_64LinuxOperand *retval = malloc(sizeof(X86_64LinuxOperand));
  retval->kind = kind;
  return retval;
}

X86_64LinuxOperand *x86_64LinuxRegOperandCreate(X86_64LinuxRegister reg) {
  X86_64LinuxOperand *retval = x86_64LinuxOperandCreate(X86_64_LINUX_OK_REG);
  retval->data.reg.reg = reg;
  return retval;
}
X86_64LinuxOperand *x86_64LinuxTempOperandCreateCopy(IROperand const *temp) {
  X86_64LinuxOperand *retval = x86_64LinuxOperandCreate(X86_64_LINUX_OK_TEMP);
  retval->data.temp.name = temp->data.temp.name;
  retval->data.temp.alignment = temp->data.temp.alignment;
  retval->data.temp.size = temp->data.temp.size;
  retval->data.temp.kind = temp->data.temp.kind;
  return retval;
}
X86_64LinuxOperand *x86_64LinuxTempOperandCreatePatch(IROperand const *temp,
                                                      size_t name,
                                                      AllocHint kind) {
  X86_64LinuxOperand *retval = x86_64LinuxOperandCreate(X86_64_LINUX_OK_TEMP);
  retval->data.temp.name = name;
  retval->data.temp.alignment = temp->data.temp.alignment;
  retval->data.temp.size = temp->data.temp.size;
  retval->data.temp.kind = kind;
  return retval;
}
X86_64LinuxOperand *x86_64LinuxOffsetOperandCreate(int64_t offset) {
  X86_64LinuxOperand *retval = x86_64LinuxOperandCreate(X86_64_LINUX_OK_OFFSET);
  retval->data.offset.offset = offset;
  return retval;
}
X86_64LinuxOperand *x86_64LinuxAddrofOperandCreate(IROperand const *who,
                                                   int64_t offset) {
  X86_64LinuxOperand *retval = x86_64LinuxOperandCreate(X86_64_LINUX_OK_ADDROF);
  retval->data.addrof.who = who->data.temp.name;
  retval->data.addrof.offset = offset;
  return retval;
}
void x86_64LinuxOperandFree(X86_64LinuxOperand *o) { free(o); }

X86_64LinuxInstruction *x86_64LinuxInstructionCreate(
    X86_64LinuxInstructionKind kind, char *skeleton) {
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
    default: {
      break;
    }
  }
  return retval;
}
void x86_64LinuxInstructionFree(X86_64LinuxInstruction *i) {
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
X86_64LinuxFrag *x86_64LinuxDataFragCreate(char *data) {
  X86_64LinuxFrag *retval = x86_64LinuxFragCreate(X86_64_LINUX_FK_DATA);
  retval->data.data.data = data;
  return retval;
}
X86_64LinuxFrag *x86_64LinuxTextFragCreate(char *header, char *footer) {
  X86_64LinuxFrag *retval = x86_64LinuxFragCreate(X86_64_LINUX_FK_TEXT);
  retval->data.text.header = header;
  retval->data.text.footer = footer;
  linkedListInit(&retval->data.text.instructions);
  return retval;
}
void x86_64LinuxFragFree(X86_64LinuxFrag *frag) {
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

X86_64LinuxFile *x86_64LinuxFileCreate(char *header, char *footer) {
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
      case DT_LABEL: {
        char *old = data;
        data = format("%s\tdq L%zu\n", old, d->data.label);
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
  return x86_64LinuxInstructionCreate(kind, skeleton);
}
static void DEFINES(X86_64LinuxInstruction *i, X86_64LinuxOperand *arg) {
  vectorInsert(&i->defines, arg);
}
static void USES(X86_64LinuxInstruction *i, X86_64LinuxOperand *arg) {
  vectorInsert(&i->uses, arg);
}
static void OTHER(X86_64LinuxInstruction *i, X86_64LinuxOperand *arg) {
  vectorInsert(&i->other, arg);
}
static void DONE(X86_64LinuxFrag *assembly, X86_64LinuxInstruction *i) {
  insertNodeEnd(&assembly->data.text.instructions, i);
}
/**
 * encode a non-label constant as a 64 bit unsigned integer
 *
 * @param constant constant to encode - must be 8 bytes or smaller
 * @returns encoded value
 */
static uint64_t datumToNumber(IROperand *constant) {
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
  for (size_t idx = 0; idx < 8; ++idx) {
    out <<= 8;
    out |= bytes[idx];
  }
  return out;
}
// TODO: factor encoding an arg's string into its own function
static X86_64LinuxFrag *x86_64LinuxGenerateTextAsm(IRFrag *frag,
                                                   FileListEntry *file) {
  X86_64LinuxFrag *assembly = x86_64LinuxTextFragCreate(
      format("section .text\nglobal %s:function\n%s:\n", frag->name.global,
             frag->name.global),
      strdup(".end\n"));
  IRBlock *b = frag->data.text.blocks.head->next->data;
  for (ListNode *currInst = b->instructions.head->next;
       currInst != b->instructions.tail; currInst = currInst->next) {
    IRInstruction *ir = currInst->data;
    X86_64LinuxInstruction *i;
    switch (ir->op) {
      case IO_LABEL: {
        i = INST(X86_64_LINUX_IK_LABEL,
                 format("L%zu:\n", ir->args[0]->data.local.name));
        i->data.labelName = ir->args[0]->data.local.name;
        DONE(assembly, i);
        break;
      }
      case IO_VOLATILE: {
        i = INST(X86_64_LINUX_IK_REGULAR, strdup(""));
        USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
        DONE(assembly, i);
        break;
      }
      case IO_UNINITIALIZED: {
        break;  // not translated
      }
      case IO_ADDROF: {
        // arg 0: reg, gp temp, mem temp
        // arg 1: mem temp

        if (ir->args[0]->kind == OK_REG) {
          // case: arg 0 == reg
          // lea <reg 0>, <temp 1>
          i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
          DEFINES(i, x86_64LinuxRegOperandCreate(ir->args[0]->data.reg.name));
          USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
          DONE(assembly, i);
          // TODO: this temp must be considered "always live"
        } else if (ir->args[0]->kind == OK_TEMP &&
                   ir->args[0]->data.temp.kind == AH_GP) {
          // case: arg 0 == gp temp
          // lea <gp temp 0>, <temp 1>
          i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
          DONE(assembly, i);
          // TODO: this temp must be considered "always live"
        } else {
          // case: arg 0 == mem temp
          // lea <patch temp>, <temp 1>
          // mov <mem temp 0>, <patch temp>
          size_t patchName = fresh(file);
          i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                       AH_GP));
          USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
          insertNodeEnd(&assembly->data.text.instructions, i);

          i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
          USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                    AH_GP));
          DONE(assembly, i);
          // TODO: this temp must be considered "always live"
        }
        break;
      }
      case IO_NOP: {
        break;  // not translated
      }
      case IO_MOVE: {
        // arg 0: reg, non-mem temp, mem temp
        // arg 1: reg, non-mem temp, mem temp, constant, global label, local
        // label

        if ((ir->args[0]->kind == OK_REG ||
             (ir->args[0] == OK_TEMP &&
              (ir->args[0]->data.temp.kind == AH_GP ||
               ir->args[0]->data.temp.kind == AH_FP))) ||
            ((ir->args[0]->kind == OK_TEMP &&
              ir->args[0]->data.temp.kind == AH_MEM) &&
             !(ir->args[1]->kind != OK_TEMP &&
               ir->args[1]->data.temp.kind != AH_MEM))) {
          // case: arg 0 == reg | non-mem temp, arg 1 == *
          //    OR arg 0 == mem-temp, arg 1 != mem-temp
          // mov <arg 0>, <arg 1>
          if (ir->args[1]->kind == OK_CONSTANT) {
            // subcase: arg 1 == const
            // mov <arg 0>, <constant 1>

            IRDatum *first = ir->args[1]->data.constant.data.elements[0];
            if (first->type == DT_LABEL &&
                ir->args[1]->data.constant.data.size == 1) {
              // subsubcase: arg 1 == label constant
              // mov <arg 0>, <label constant 1>
              i = INST(X86_64_LINUX_IK_REGULAR,
                       format("\tmov `d, L%zu\n", first->data.label));
            } else if (ir->args[1]->kind == OK_REG ||
                       ir->args[1]->data.temp.size == 1 ||
                       ir->args[1]->data.temp.size == 2 ||
                       ir->args[1]->data.temp.size == 4 ||
                       ir->args[1]->data.temp.size == 8) {
              // subsubcase: arg 1 == regular sized non-label constant
              // mov <arg 0>, <constant 1>
              i = INST(X86_64_LINUX_IK_REGULAR,
                       format("\tmov `d, %zu\n", datumToNumber(ir->args[1])));
            } else {
              // subsubcase: oddly or oversized constants, must do a 'memcpy'
              // lea rsi, <constant location>
              // lea rdi, <temp 0>
              // mov rcx, <sizeof move / multiple>
              // rep movs<multiple>

              size_t moveSize = irOperandSizeof(ir->args[1]);

              size_t fragName = fresh(file);
              X86_64LinuxFile *asmFile = file->asmFile;
              char *data =
                  x86_64LinuxDataToString(&ir->args[1]->data.constant.data);
              vectorInsert(&asmFile->frags,
                           x86_64LinuxDataFragCreate(format(
                               "section .rodata align=%zu\nL%zu:\n%s.end\n",
                               irOperandAlignof(ir->args[1]), fragName, data)));
              free(data);

              i = INST(X86_64_LINUX_IK_REGULAR,
                       format("\tmov `d, L%zu\n", fragName));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
              DONE(assembly, i);

              i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
              USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
              DONE(assembly, i);

              size_t scale;
              if (moveSize % 8 == 0) {
                scale = 8;
              } else if (moveSize % 4 == 0) {
                scale = 4;
              } else if (moveSize % 2 == 0) {
                scale = 2;
              } else {
                scale = 1;
              }

              i = INST(X86_64_LINUX_IK_REGULAR,
                       format("\tmov `d, %zu\n", moveSize / scale));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
              DONE(assembly, i);

              switch (scale) {
                case 8: {
                  i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsq\n"));
                  break;
                }
                case 4: {
                  i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsd\n"));
                  break;
                }
                case 2: {
                  i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsw\n"));
                  break;
                }
                case 1: {
                  i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsb\n"));
                  break;
                }
              }
              USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
              USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
              USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
              USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
              DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
              DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
              DONE(assembly, i);
            }
          } else if (ir->args[1]->kind == OK_GLOBAL) {
            // subcase: arg 1 == global
            // mov <arg 0>, <global 1>
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov `d, %s\n", ir->args[1]->data.global.name));
          } else if (ir->args[1]->kind == OK_LOCAL) {
            // subcase: arg 1 == local
            // mov <arg 0>, <local 1>
            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov `d, L%zu\n", ir->args[1]->data.local.name));
          } else {
            // subcase: arg 1 == reg | temp
            i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
            USES(i,
                 ir->args[1]->kind == OK_REG
                     ? x86_64LinuxRegOperandCreate(ir->args[1]->data.reg.name)
                     : x86_64LinuxTempOperandCreateCopy(ir->args[1]));
          }
          DEFINES(i,
                  ir->args[0]->kind == OK_REG
                      ? x86_64LinuxRegOperandCreate(ir->args[0]->data.reg.name)
                      : x86_64LinuxTempOperandCreateCopy(ir->args[0]));
          DONE(assembly, i);
        } else {
          // case: arg 0 == mem temp, arg 1 == mem temp
          size_t moveSize = ir->args[0]->data.temp.size;
          if (moveSize == 1 || moveSize == 2 || moveSize == 4 ||
              moveSize == 8) {
            // subcase: can patch through a gp temp
            // mov <patch temp>, <temp 1>
            // mov <temp 0>, <patch temp>
            size_t patchName = fresh(file);
            i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                         AH_GP));
            USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                      AH_GP));
            DONE(assembly, i);
          } else if (moveSize == 16) {
            // subcase: can patch through an SSE temp
            // movdqu <patch temp>, <temp 1>
            // movdqu <temp 0>, <patch temp>
            size_t patchName = fresh(file);
            i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                         AH_FP));
            USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmovdqu `d, `u\n"));
            DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
            USES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], patchName,
                                                      AH_FP));
            DONE(assembly, i);
          } else {
            // subcase: must do a 'memcpy'
            // lea rsi, <temp 1>
            // lea rdi, <temp 0>
            // mov rcx, <sizeof move / multiple>
            // rep movs<multiple>
            i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
            USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
            DONE(assembly, i);

            i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tlea `d, `u\n"));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
            USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
            DONE(assembly, i);

            size_t scale;
            if (moveSize % 8 == 0) {
              scale = 8;
            } else if (moveSize % 4 == 0) {
              scale = 4;
            } else if (moveSize % 2 == 0) {
              scale = 2;
            } else {
              scale = 1;
            }

            i = INST(X86_64_LINUX_IK_REGULAR,
                     format("\tmov `d, %zu\n", moveSize / scale));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
            DONE(assembly, i);

            switch (scale) {
              case 8: {
                i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsq\n"));
                break;
              }
              case 4: {
                i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsd\n"));
                break;
              }
              case 2: {
                i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsw\n"));
                break;
              }
              case 1: {
                i = INST(X86_64_LINUX_IK_REGULAR, strdup("\trep movsb\n"));
                break;
              }
            }
            USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
            USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
            USES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
            USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[1]));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RSI));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RDI));
            DEFINES(i, x86_64LinuxRegOperandCreate(X86_64_LINUX_RCX));
            DEFINES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
            DONE(assembly, i);
          }
        }
        break;
      }
      case IO_MEM_STORE: {
        // arg 0: reg, gp temp, mem temp, const, global, local
        // arg 1: reg, non-mem temp, mem temp, const, global, local
        // arg 2: reg, gp temp, mem temp, const

        // move arg 0, 2 into patch temps if they are mem temps
        size_t arg0Patch = 0;
        if (ir->args[0]->kind == OK_TEMP &&
            ir->args[0]->data.temp.kind == AH_MEM) {
          arg0Patch = fresh(file);

          i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[0], arg0Patch,
                                                       AH_GP));
          USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[0]));
          DONE(assembly, i);
        }

        size_t arg2Patch = 0;
        if (ir->args[2]->kind == OK_TEMP &&
            ir->args[2]->data.temp.kind == AH_MEM) {
          arg2Patch = fresh(file);
          i = INST(X86_64_LINUX_IK_MOVE, strdup("\tmov `d, `u\n"));
          DEFINES(i, x86_64LinuxTempOperandCreatePatch(ir->args[2], arg0Patch,
                                                       AH_GP));
          USES(i, x86_64LinuxTempOperandCreateCopy(ir->args[2]));
          DONE(assembly, i);
        }

        if (ir->args[1]->kind == OK_REG ||
            (ir->args[1]->kind == OK_TEMP &&
             ir->args[1]->data.temp.kind != AH_MEM)) {
          // case: arg 1 == reg | non-mem temp
          // mov [<arg 0> + <arg 2>], <arg 1>
          i = INST(X86_64_LINUX_IK_REGULAR, strdup("\tmov [`u + `u], `u\n"));
          // TODO
          DONE(assembly, i);
        } else if (ir->args[1]->kind == OK_TEMP &&
                   ir->args[1]->data.temp.kind == AH_MEM) {
          // case: arg 1 == mem temp

          if (ir->args[1]->data.temp.size == 1 ||
              ir->args[1]->data.temp.size == 2 ||
              ir->args[1]->data.temp.size == 4 ||
              ir->args[1]->data.temp.size == 8) {
            // subcase: can patch through a gp temp
            // mov <patch temp>, <arg 1>
            // mov [<arg 0> + <arg 2>], <patch temp>
          } else if (ir->args[1]->data.temp.size == 16) {
            // subcase: can patch through an sse temp
            // movdqu <patch temp>, <arg 1>
            // movdqu [<arg 0> + <arg 2>], <patch temp>
          } else {
            // subcase: must do a memcpy
            // lea rsi, <arg 1>
            // lea rdi, [<arg 0> + <arg 2>]
            // mov rcx, <sizeof move / multiple>
            // rep movs<multiple>
          }
        } else if (ir->args[1]->kind == OK_CONSTANT) {
        }

        // case: arg 1 == const

        // subcase: arg 1 == regular-sized non-label const
        // mov [<arg 0> + <arg 2>], <arg 1>

        // subcase: arg 1 == label const
        // mov [<arg 0> + <arg 2>], <arg 1>

        // subcase: arg 1 == irregular-sized const
        // mov [<arg 0> + <arg 2>], <arg 1>

        // case: arg 1 == global
        // mov [<arg 0> + <arg 2>], <arg 1>

        // case: arg 1 == local
        // mov [<arg 0> + <arg 2>], <arg 1>

        // TODO
        break;
      }
      case IO_MEM_LOAD: {
        // arg 0: reg, non-mem temp, mem temp
        // arg 1: reg, gp temp, mem temp, const, global, local
        // arg 2: reg, gp temp, mem temp, const

        // move arg 1, 2, into patch temps if they are mem temps

        // case: arg 0 != mem temp
        // mov <arg 0>, [<arg 1> + <arg 2>]

        // case: arg 0 == mem temp

        // subcase: can patch through a gp temp
        // mov <patch temp>, [<arg 0> + <arg 2>]
        // mov <arg 1>, <patch temp>

        // subcase: can patch through an sse temp
        // movdqu <patch temp>, [<arg 0> + <arg 2>]
        // movdqu <arg 1>, <patch temp>

        // subcase: must do a memcpy
        // lea rsi, [<arg 0> + <arg 2>]
        // lea rdi, <arg 1>
        // mov rcx, <sizeof move / multiple>
        // rep movs<multiple>

        // TODO
        break;
      }
      case IO_STK_STORE: {
        // TODO
        break;
      }
      case IO_STK_LOAD: {
        // TODO
        break;
      }
      case IO_OFFSET_STORE: {
        // TODO
        break;
      }
      case IO_OFFSET_LOAD: {
        // TODO
        break;
      }
      case IO_ADD: {
        // TODO
        break;
      }
      case IO_SUB: {
        // TODO
        break;
      }
      case IO_SMUL: {
        // TODO
        break;
      }
      case IO_UMUL: {
        // TODO
        break;
      }
      case IO_SDIV: {
        // TODO
        break;
      }
      case IO_UDIV: {
        // TODO
        break;
      }
      case IO_SMOD: {
        // TODO
        break;
      }
      case IO_UMOD: {
        // TODO
        break;
      }
      case IO_FADD: {
        // TODO
        break;
      }
      case IO_FSUB: {
        // TODO
        break;
      }
      case IO_FMUL: {
        // TODO
        break;
      }
      case IO_FDIV: {
        // TODO
        break;
      }
      case IO_FMOD: {
        // TODO
        break;
      }
      case IO_NEG: {
        // TODO
        break;
      }
      case IO_FNEG: {
        // TODO
        break;
      }
      case IO_SLL: {
        // TODO
        break;
      }
      case IO_SLR: {
        // TODO
        break;
      }
      case IO_SAR: {
        // TODO
        break;
      }
      case IO_AND: {
        // TODO
        break;
      }
      case IO_XOR: {
        // TODO
        break;
      }
      case IO_OR: {
        // TODO
        break;
      }
      case IO_NOT: {
        // TODO
        break;
      }
      case IO_L: {
        // TODO
        break;
      }
      case IO_LE: {
        // TODO
        break;
      }
      case IO_E: {
        // TODO
        break;
      }
      case IO_NE: {
        // TODO
        break;
      }
      case IO_G: {
        // TODO
        break;
      }
      case IO_GE: {
        // TODO
        break;
      }
      case IO_A: {
        // TODO
        break;
      }
      case IO_AE: {
        // TODO
        break;
      }
      case IO_B: {
        // TODO
        break;
      }
      case IO_BE: {
        // TODO
        break;
      }
      case IO_FL: {
        // TODO
        break;
      }
      case IO_FLE: {
        // TODO
        break;
      }
      case IO_FE: {
        // TODO
        break;
      }
      case IO_FNE: {
        // TODO
        break;
      }
      case IO_FG: {
        // TODO
        break;
      }
      case IO_FGE: {
        // TODO
        break;
      }
      case IO_Z: {
        // TODO
        break;
      }
      case IO_NZ: {
        // TODO
        break;
      }
      case IO_LNOT: {
        // TODO
        break;
      }
      case IO_SX: {
        // TODO
        break;
      }
      case IO_ZX: {
        // TODO
        break;
      }
      case IO_TRUNC: {
        // TODO
        break;
      }
      case IO_U2F: {
        // TODO
        break;
      }
      case IO_S2F: {
        // TODO
        break;
      }
      case IO_FRESIZE: {
        // TODO
        break;
      }
      case IO_F2I: {
        // TODO
        break;
      }
      case IO_JUMP: {
        // TODO
        break;
      }
      case IO_JUMPTABLE: {
        // TODO
        break;
      }
      case IO_J1L: {
        // TODO
        break;
      }
      case IO_J1LE: {
        // TODO
        break;
      }
      case IO_J1E: {
        // TODO
        break;
      }
      case IO_J1NE: {
        // TODO
        break;
      }
      case IO_J1G: {
        // TODO
        break;
      }
      case IO_J1GE: {
        // TODO
        break;
      }
      case IO_J1A: {
        // TODO
        break;
      }
      case IO_J1AE: {
        // TODO
        break;
      }
      case IO_J1B: {
        // TODO
        break;
      }
      case IO_J1BE: {
        // TODO
        break;
      }
      case IO_J1FL: {
        // TODO
        break;
      }
      case IO_J1FLE: {
        // TODO
        break;
      }
      case IO_J1FE: {
        // TODO
        break;
      }
      case IO_J1FNE: {
        // TODO
        break;
      }
      case IO_J1FG: {
        // TODO
        break;
      }
      case IO_J1FGE: {
        // TODO
        break;
      }
      case IO_J1Z: {
        // TODO
        break;
      }
      case IO_J1NZ: {
        // TODO
        break;
      }
      case IO_CALL: {
        // TODO
        break;
      }
      case IO_RETURN: {
        // TODO
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
        x86_64LinuxFileCreate(strdup("lprefix .\n"), strdup(""));

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