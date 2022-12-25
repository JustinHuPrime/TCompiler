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

static void x86_64LinuxOperandFree(X86_64LinuxOperand *o) {
  switch (o->kind) {
    case X86_64_LINUX_OK_OFFSET_TEMP: {
      x86_64LinuxOperandFree(o->data.offsetTemp.base);
      x86_64LinuxOperandFree(o->data.offsetTemp.offset);
      break;
    }
    case X86_64_LINUX_OK_GLOBAL_LABEL: {
      free(o->data.globalLabel.label);
      break;
    }
    default: {
      break;
    }
  }
  free(o);
}

static void x86_64LinuxInstructionFree(X86_64LinuxInstruction *i) {
  switch (i->kind) {
    case X86_64_LINUX_IK_JUMP:
    case X86_64_LINUX_IK_CJUMP:
    case X86_64_LINUX_IK_JUMPTABLE: {
      sizeVectorUninit(&i->data.jumpTargets);
      break;
    }
    default: {
      break;
    }
  }
  free(i->skeleton);
  vectorUninit(&i->operands, (void (*)(void *))x86_64LinuxOperandFree);
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
      error(__FILE__, __LINE__, "invalid fragment kind enum encountered");
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

static void x86_64LinuxGenerateDataAsm(IRFrag *frag, FileListEntry *file) {
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

  vectorInsert(
      &((X86_64LinuxFile *)&file->asmFile)->frags,
      x86_64LinuxDataFragCreate(format("%s%s%s.end:\n", section, name, data)));
  free(section);
  free(name);
  free(data);
}

typedef struct {
  /**
   * what kind is allowed
   */
  OperandKind kind;
  /**
   * detail for each kind
   */
  union {
    struct {
      /**
       * allowed alignments; empty = allow any
       */
      SizeVector alignments;
      /**
       * allowed sizes; empty = allow any
       */
      SizeVector sizes;
      /**
       * allowed alloc kinds; empty = allow any
       */
      SizeVector kinds;
    } temp;
    struct {
      /**
       * allowed sizes; empty = allow any
       */
      SizeVector sizes;
    } reg;
    struct {
      /**
       * allowed sizes; empty = allow any
       */
      SizeVector sizes;
      /**
       * allowed datum type (must be single datum); empty = allow any
       */
      SizeVector types;
    } constant;
  } data;
} IRRequirementClause;

static void irRequirementClauseFree(IRRequirementClause *clause) {
  switch (clause->kind) {
    case OK_TEMP: {
      sizeVectorUninit(&clause->data.temp.alignments);
      sizeVectorUninit(&clause->data.temp.sizes);
      sizeVectorUninit(&clause->data.temp.kinds);
      break;
    }
    case OK_REG: {
      sizeVectorUninit(&clause->data.reg.sizes);
      break;
    }
    case OK_CONSTANT: {
      sizeVectorUninit(&clause->data.constant.sizes);
      sizeVectorUninit(&clause->data.constant.types);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }

  free(clause);
}

typedef struct {
  /**
   * list of clauses; require at least one met; empty = allow any
   */
  Vector clauses;
} IRRequirement;

static void irRequirementFree(IRRequirement *requirement) {
  vectorUninit(&requirement->clauses,
               (void (*)(void *))irRequirementClauseFree);
  free(requirement);
}

typedef struct {
  char *skeleton;
  SizeVector operands;
} AsmTemplate;

static void asmTemplateFree(AsmTemplate *template) {
  free(template->skeleton);
  sizeVectorUninit(&template->operands);
  free(template);
}

typedef struct {
  /**
   * which op this implements
   */
  IROperator op;
  /**
   * requirements for IR Operands
   *
   * vector of IRRequirement; one element per operand
   */
  Vector requirements;
  /**
   * generated instruction templates
   */
  Vector templates;
} AsmInstruction;

static void asmInstructionFree(AsmInstruction *instruction) {
  vectorUninit(&instruction->requirements, (void (*)(void *))irRequirementFree);
  vectorUninit(&instruction->templates, (void (*)(void *))asmTemplateFree);
  free(instruction);
}

typedef struct {
  /**
   * source IROperand type
   */
  IRRequirement source;
  /**
   * destination IROperand type
   */
  IRRequirement destination;
  /**
   * generated instruction templates
   *
   * assumes source operand is index 1 and destination operand is index 2
   */
  Vector templates;
} AsmCast;

static void asmCastFree(AsmCast *cast) {
  irRequirementFree(&cast->source);
  irRequirementFree(&cast->destination);
  vectorUninit(&cast->templates, (void (*)(void *))asmTemplateFree);
  free(cast);
}

static void generateAsmInstructions(Vector *instructions, Vector *casts) {
  vectorInit(instructions);
  vectorInit(casts);
}

static X86_64LinuxOperand *operandCreate(IROperand const *ir) {
  X86_64LinuxOperand *operand = malloc(sizeof(X86_64LinuxOperand));

  switch (ir->kind) {
    case OK_TEMP: {
      operand->kind = X86_64_LINUX_OK_TEMP;
      operand->data.temp.name = ir->data.temp.name;
      operand->data.temp.alignment = ir->data.temp.alignment;
      operand->data.temp.size = ir->data.temp.size;
      operand->data.temp.kind = ir->data.temp.kind;
      operand->data.temp.escapes = false;
      break;
    }
    case OK_REG: {
      operand->kind = X86_64_LINUX_OK_REG;
      operand->data.reg.reg = ir->data.reg.name;
      operand->data.reg.size = ir->data.reg.size;
      break;
    }
    case OK_CONSTANT: {
      if (ir->data.constant.data.size != 1) {
        error(__FILE__, __LINE__,
              "constant too large to convert to a single operand");
      }

      IRDatum *d = ir->data.constant.data.elements[0];
      switch (d->type) {
        case DT_BYTE:
        case DT_SHORT:
        case DT_INT:
        case DT_LONG: {
          operand->kind = X86_64_LINUX_OK_NUMBER;
          switch (d->type) {
            case DT_BYTE: {
              operand->data.number.value = d->data.byteVal;
              operand->data.number.size = 1;
              break;
            }
            case DT_SHORT: {
              operand->data.number.value = d->data.shortVal;
              operand->data.number.size = 2;
              break;
            }
            case DT_INT: {
              operand->data.number.value = d->data.intVal;
              operand->data.number.size = 4;
              break;
            }
            case DT_LONG: {
              operand->data.number.value = d->data.longVal;
              operand->data.number.size = 8;
              break;
            }
            default: {
              error(__FILE__, __LINE__,
                    "impossible invalid datum type (CPU error?)");
            }
          }
          break;
        }
        case DT_LOCAL: {
          operand->kind = X86_64_LINUX_OK_LOCAL_LABEL;
          operand->data.localLabel.label = d->data.localLabel;
          break;
        }
        case DT_GLOBAL: {
          operand->kind = X86_64_LINUX_OK_GLOBAL_LABEL;
          operand->data.globalLabel.label = strdup(d->data.globalLabel);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid constant type");
        }
      }
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }

  return operand;
}

static void x86_64LinuxGenerateTextAsm(IRFrag *frag, FileListEntry *file) {
  X86_64LinuxFile *asmFile = file->asmFile;
  X86_64LinuxFrag *assembly = x86_64LinuxTextFragCreate(
      format("section .text\nglobal %s:function\n%s:\n", frag->name.global,
             frag->name.global),
      strdup(""));
  IRBlock *b = frag->data.text.blocks.head->next->data;
  for (ListNode *currInst = b->instructions.head->next;
       currInst != b->instructions.tail; currInst = currInst->next) {
    IRInstruction *ir = currInst->data;
    // TODO
  }
  vectorInsert(&asmFile->frags, assembly);
}

void x86_64LinuxGenerateAsm(void) {
  Vector asmInstructions;
  Vector asmCasts;
  generateAsmInstructions(&asmInstructions, &asmCasts);

  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    file->asmFile = x86_64LinuxFileCreate(strdup(""), strdup(""));

    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      switch (frag->type) {
        case FT_BSS:
        case FT_RODATA:
        case FT_DATA: {
          x86_64LinuxGenerateDataAsm(frag, file);
          break;
        }
        case FT_TEXT: {
          x86_64LinuxGenerateTextAsm(frag, file);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "invalid fragment type");
        }
      }
    }
  }

  vectorUninit(&asmInstructions, (void (*)(void *))asmInstructionFree);
  vectorUninit(&asmCasts, (void (*)(void *))asmCastFree);
}