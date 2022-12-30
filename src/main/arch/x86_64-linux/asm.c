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
#include "util/functional.h"
#include "util/internalError.h"
#include "util/numericSizing.h"

///////////////////////////////////////////////////////////////////////////////
// misc stuff                                                                //
///////////////////////////////////////////////////////////////////////////////

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

// assembly data structures

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
    case X86_64_LINUX_IK_JUMPTABLE:
    case X86_64_LINUX_IK_LABEL: {
      sizeVectorUninit(&i->jumpData);
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

///////////////////////////////////////////////////////////////////////////////
// data frags                                                                //
///////////////////////////////////////////////////////////////////////////////

static char *dataToString(Vector const *v) {
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

static void generateDataAsm(IRFrag const *frag, FileListEntry *file) {
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
  char *data = dataToString(&frag->data.data.data);

  X86_64LinuxFile *asmFile = file->asmFile;
  vectorInsert(&asmFile->frags, x86_64LinuxDataFragCreate(format(
                                    "%s%s%s.end:\n", section, name, data)));
  free(section);
  free(name);
  free(data);
}

static size_t generateLargeConstantDataAsm(IROperand const *constant,
                                           FileListEntry *file) {
  size_t label = fresh(file);
  char *data = dataToString(&constant->data.constant.data);

  X86_64LinuxFile *asmFile = file->asmFile;
  vectorInsert(&asmFile->frags,
               x86_64LinuxDataFragCreate(
                   format("section .rodata align=%zu\nL%zu:\n%s.end:\n",
                          constant->data.constant.alignment, label, data)));
  free(data);

  return label;
}

///////////////////////////////////////////////////////////////////////////////
// text frags                                                                //
///////////////////////////////////////////////////////////////////////////////

static X86_64LinuxOperand *operandCreate(IROperand const *ir,
                                         FileListEntry *file) {
  X86_64LinuxOperand *operand = malloc(sizeof(X86_64LinuxOperand));
  operand->mode = 0;

  switch (ir->kind) {
    case OK_TEMP: {
      operand->kind = X86_64_LINUX_OK_TEMP;
      operand->data.temp.name = ir->data.temp.name;
      operand->data.temp.alignment = ir->data.temp.alignment;
      operand->data.temp.size = ir->data.temp.size;
      operand->data.temp.kind = ir->data.temp.kind;
      break;
    }
    case OK_REG: {
      operand->kind = X86_64_LINUX_OK_REG;
      operand->data.reg.reg = ir->data.reg.name;
      operand->data.reg.size = ir->data.reg.size;
      break;
    }
    case OK_CONSTANT: {
      if (ir->data.constant.data.size == 1) {
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
      } else {
        operand->kind = X86_64_LINUX_OK_LOCAL_LABEL;
        operand->data.localLabel.label = generateLargeConstantDataAsm(ir, file);
      }
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }

  return operand;
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
       * if true, constant must be exactly 1 datum
       *
       * if false, constant must be more than 1 datum
       */
      bool small;
      /**
       * allowed alignments; empty = allow any
       */
      SizeVector alignments;
      /**
       * allowed datum type (must be single datum); empty = allow any
       */
      SizeVector types;
    } constant;
  } data;
} IRRequirementClause;

/**
 * cast type required for this an operand
 */
typedef enum {
  /**
   * cast must copy from the actual into the casted value
   */
  CT_READ,
  /**
   * cast must copy from the casted into the actual value
   */
  CT_WRITE,
  /**
   * cast must do both
   *
   * probably unused
   */
  CT_READ_WRITE,
} CastType;
typedef struct {
  /**
   * cast type for operand
   */
  CastType castType;
  /**
   * list of clauses; require at least one met; empty = allow any
   */
  Vector clauses;
} IRRequirement;

static IRRequirementClause *requirementClauseCreate(OperandKind kind) {
  IRRequirementClause *clause = malloc(sizeof(IRRequirementClause));
  clause->kind = kind;
  switch (kind) {
    case OK_TEMP: {
      sizeVectorInit(&clause->data.temp.alignments);
      sizeVectorInit(&clause->data.temp.sizes);
      sizeVectorInit(&clause->data.temp.kinds);
      break;
    }
    case OK_REG: {
      sizeVectorInit(&clause->data.reg.sizes);
      break;
    }
    case OK_CONSTANT: {
      clause->data.constant.small = true;
      sizeVectorInit(&clause->data.constant.alignments);
      sizeVectorInit(&clause->data.constant.types);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }
  return clause;
}
static bool operandSatisfiesClause(IRRequirementClause const *clause,
                                   IROperand const *operand) {
  if (clause->kind != operand->kind) {
    return false;
  }
  switch (operand->kind) {
    case OK_TEMP: {
      return (clause->data.temp.alignments.size == 0 ||
              sizeVectorContains(&clause->data.temp.alignments,
                                 operand->data.temp.alignment)) &&
             (clause->data.temp.sizes.size == 0 ||
              sizeVectorContains(&clause->data.temp.sizes,
                                 operand->data.temp.size)) &&
             (clause->data.temp.kinds.size == 0 ||
              sizeVectorContains(&clause->data.temp.kinds,
                                 operand->data.temp.kind));
    }
    case OK_REG: {
      return clause->data.reg.sizes.size == 0 ||
             sizeVectorContains(&clause->data.reg.sizes,
                                operand->data.reg.size);
    }
    case OK_CONSTANT: {
      if (clause->data.constant.small) {
        if (operand->data.constant.data.size != 1) return false;
        IRDatum const *constant = operand->data.constant.data.elements[0];
        return (clause->data.constant.alignments.size == 0 ||
                sizeVectorContains(&clause->data.constant.alignments,
                                   operand->data.constant.alignment)) &&
               (clause->data.constant.types.size == 0 ||
                sizeVectorContains(&clause->data.constant.types,
                                   constant->type));
      } else {
        if (operand->data.constant.data.size <= 1) return false;
        return (clause->data.constant.alignments.size == 0 ||
                sizeVectorContains(&clause->data.constant.alignments,
                                   operand->data.constant.alignment));
      }
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }
}
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
      sizeVectorUninit(&clause->data.constant.alignments);
      sizeVectorUninit(&clause->data.constant.types);
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }

  free(clause);
}

static IRRequirement *requirementCreate(CastType castType) {
  IRRequirement *requirement = malloc(sizeof(IRRequirement));
  requirement->castType = castType;
  vectorInit(&requirement->clauses);
  return requirement;
}
static bool operandSatisfiesRequirement(IRRequirement const *requirement,
                                        IROperand const *operand) {
  if (requirement->clauses.size == 0) {
    return true;  // requirement allows any
  }

  for (size_t idx = 0; idx < requirement->clauses.size; ++idx) {
    IRRequirementClause *clause = requirement->clauses.elements[idx];
    if (operandSatisfiesClause(clause, operand)) {
      return true;
    }
  }

  return false;
}
static void irRequirementFree(IRRequirement *requirement) {
  vectorUninit(&requirement->clauses,
               (void (*)(void *))irRequirementClauseFree);
  free(requirement);
}

typedef enum {
  TOK_ARG,
  TOK_FIXED,
  TOK_SPECIAL_OPSIZE,
} AsmTemplateOperandKind;
typedef struct {
  AsmTemplateOperandKind kind;
  union {
    struct {
      size_t index;
      X86_64LinuxOperandMode mode;
    } arg;
    struct {
      IROperand *fixed;
      X86_64LinuxOperandMode mode;
    } fixed;
  } data;
} AsmTemplateOperand;

static AsmTemplateOperand *argTemplateOperandCreate(
    size_t arg, X86_64LinuxOperandMode mode) {
  AsmTemplateOperand *operand = malloc(sizeof(AsmTemplateOperand));
  operand->kind = TOK_ARG;
  operand->data.arg.index = arg;
  operand->data.arg.mode = mode;
  return operand;
}
static AsmTemplateOperand *fixedTemplateOperandCreate(
    IROperand *fixed, X86_64LinuxOperandMode mode) {
  AsmTemplateOperand *operand = malloc(sizeof(AsmTemplateOperand));
  operand->kind = TOK_FIXED;
  operand->data.fixed.fixed = fixed;
  operand->data.fixed.mode = mode;
  return operand;
}
static AsmTemplateOperand *specialTemplateOperandCreate(
    AsmTemplateOperandKind kind) {
  AsmTemplateOperand *operand = malloc(sizeof(AsmTemplateOperand));
  operand->kind = kind;
  return operand;
}
static void asmTemplateOperandFree(AsmTemplateOperand *operand) {
  switch (operand->kind) {
    case TOK_FIXED: {
      irOperandFree(operand->data.fixed.fixed);
      break;
    }
    default: {
      break;
    }
  }
  free(operand);
}

typedef struct {
  X86_64LinuxInstructionKind kind;
  char *skeleton;
  /**
   * vector of AsmTemplateOperand
   */
  Vector operands;
  /**
   * one of:
   *
   * operand index of jump table; expects operand to be a local label pointing
   * to a rodata frag of local labels
   *
   * operand index of label; expects operand to be a local label
   */
  size_t label;
} AsmTemplate;

static AsmTemplate *asmTemplateCreate(X86_64LinuxInstructionKind kind,
                                      char *skeleton) {
  AsmTemplate *template = malloc(sizeof(AsmTemplate));
  template->kind = kind;
  template->skeleton = skeleton;
  vectorInit(&template->operands);
  template->label = 0;
  return template;
}
static X86_64LinuxOperand *asmOperandCopy(X86_64LinuxOperand const *operand) {
  X86_64LinuxOperand *copy = malloc(sizeof(X86_64LinuxOperand));
  copy->mode = operand->mode;
  copy->kind = operand->kind;
  switch (operand->kind) {
    case X86_64_LINUX_OK_REG: {
      copy->data.reg.reg = operand->data.reg.reg;
      copy->data.reg.size = operand->data.reg.size;
      break;
    }
    case X86_64_LINUX_OK_TEMP: {
      copy->data.temp.name = operand->data.temp.name;
      copy->data.temp.alignment = operand->data.temp.alignment;
      copy->data.temp.size = operand->data.temp.size;
      copy->data.temp.kind = operand->data.temp.kind;
      break;
    }
    case X86_64_LINUX_OK_OFFSET_TEMP: {
      copy->data.offsetTemp.base =
          asmOperandCopy(operand->data.offsetTemp.base);
      copy->data.offsetTemp.offset =
          asmOperandCopy(operand->data.offsetTemp.offset);
      break;
    }
    case X86_64_LINUX_OK_LOCAL_LABEL: {
      copy->data.globalLabel.label = strdup(operand->data.globalLabel.label);
      break;
    }
    case X86_64_LINUX_OK_GLOBAL_LABEL: {
      copy->data.localLabel.label = operand->data.localLabel.label;
      break;
    }
    case X86_64_LINUX_OK_NUMBER: {
      copy->data.number.value = operand->data.number.value;
      copy->data.number.size = operand->data.number.size;
      break;
    }
    default: {
      error(__FILE__, __LINE__, "invalid operand kind");
    }
  }
  return copy;
}
static X86_64LinuxInstruction *instantiateTemplate(
    AsmTemplate const *template, IRInstruction const *irInstruction,
    X86_64LinuxOperand const *const *operands, FileListEntry const *file) {
  X86_64LinuxInstruction *instruction = malloc(sizeof(X86_64LinuxInstruction));
  instruction->kind = template->kind;
  instruction->skeleton = strdup(template->skeleton);
  vectorInit(&instruction->operands);
  for (size_t idx = 0; idx < template->operands.size; ++idx) {
    AsmTemplateOperand *templateOperand = template->operands.elements[idx];
    switch (templateOperand->kind) {
      case TOK_ARG: {
        X86_64LinuxOperand *operand =
            asmOperandCopy(operands[templateOperand->data.arg.index]);
        operand->mode = templateOperand->data.arg.mode;
        vectorInsert(&instruction->operands, operand);
        break;
      }
      case TOK_FIXED: {
        X86_64LinuxOperand *operand =
            operandCreate(templateOperand->data.fixed.fixed, file);
        operand->mode = templateOperand->data.fixed.mode;
        vectorInsert(&instruction->operands, operand);
        break;
      }
      case TOK_SPECIAL_OPSIZE: {
        X86_64LinuxOperand *operand = malloc(sizeof(X86_64LinuxOperand));
        operand->kind = X86_64_LINUX_OK_NUMBER;
        operand->mode = X86_64_LINUX_OM_READ;
        operand->data.number.value = irOperandSizeof(irInstruction->args[0]);
        operand->data.number.size = 8;
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid template operand kind");
      }
    }
  }

  switch (template->kind) {
    case X86_64_LINUX_IK_JUMP:
    case X86_64_LINUX_IK_CJUMP:
    case X86_64_LINUX_IK_LABEL: {
      sizeVectorInit(&instruction->jumpData);
      sizeVectorInsert(&instruction->jumpData,
                       operands[template->label]->data.localLabel.label);
      break;
    }
    case X86_64_LINUX_IK_JUMPTABLE: {
      sizeVectorInit(&instruction->jumpData);
      for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
        IRFrag *frag = file->irFrags.elements[fragIdx];
        if (frag->type == FT_RODATA && frag->nameType == FNT_LOCAL &&
            frag->name.local ==
                operands[template->label]->data.localLabel.label) {
          // found jumptable - fill it in
          for (size_t labelIdx = 0; labelIdx < frag->data.data.data.size;
               ++labelIdx) {
            IRDatum *label = frag->data.data.data.elements[labelIdx];
            sizeVectorInsert(&instruction->jumpData, label->data.localLabel);
          }
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  return instruction;
}
static void asmTemplateFree(AsmTemplate *template) {
  free(template->skeleton);
  vectorUninit(&template->operands, (void (*)(void *))asmTemplateOperandFree);
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
  /**
   * do we avoid using this instruction
   *
   * (for expensive ops, like memcpy; prefer a cast instead of directly doing a
   * memcpy)
   */
  bool avoid;
} AsmInstruction;

static AsmInstruction *asmInstructionCreate(IROperator op) {
  AsmInstruction *instruction = malloc(sizeof(AsmInstruction));
  instruction->op = op;
  vectorInit(&instruction->requirements);
  vectorInit(&instruction->templates);
  instruction->avoid = false;
  return instruction;
}

static void instantiateInstruction(AsmInstruction const *instruction,
                                   IRInstruction const *irInstruction,
                                   X86_64LinuxOperand const *const *operands,
                                   X86_64LinuxFrag *assembly,
                                   FileListEntry const *file) {
  // instantiate each template
  for (size_t templateIdx = 0; templateIdx < instruction->templates.size;
       ++templateIdx) {
    insertNodeEnd(
        &assembly->data.text.instructions,
        instantiateTemplate(instruction->templates.elements[templateIdx],
                            irInstruction, operands, file));
  }
}
static void asmInstructionFree(AsmInstruction *instruction) {
  vectorUninit(&instruction->requirements, (void (*)(void *))irRequirementFree);
  vectorUninit(&instruction->templates, (void (*)(void *))asmTemplateFree);
  free(instruction);
}

typedef struct {
  /**
   * source IROperand type
   */
  IRRequirement *source;
  /**
   * destination IROperand template
   *
   * if kind = OK_TEMP, then name is unspecified and will be filled in with a
   * fresh name otherwise, all elements should be specified
   */
  IROperand *destination;
  /**
   * what type of cast is this
   *
   * read = copies source to destination before op
   * write = copies destination to source after op
   * both = both
   *
   * read only has prefix templates
   * write has prefix and postfix templates
   */
  CastType castType;
  /**
   * generated instruction templates
   *
   * assumes source operand is index 1 and destination operand is index 0
   */
  Vector prefixTemplates;
  Vector postfixTemplates;
} AsmCast;

static X86_64LinuxOperand *instantiateCastPrefix(
    AsmCast const *cast, IRInstruction const *irInstruction,
    IROperand const *source, X86_64LinuxFrag *assembly, FileListEntry *file) {
  X86_64LinuxOperand *operands[2];

  operands[0] = operandCreate(cast->destination, file);
  if (cast->destination->kind == OK_TEMP)
    operands[0]->data.temp.name = fresh(file);

  operands[1] = operandCreate(source, file);

  // instantiate each template
  for (size_t templateIdx = 0; templateIdx < cast->prefixTemplates.size;
       ++templateIdx) {
    insertNodeEnd(
        &assembly->data.text.instructions,
        instantiateTemplate(cast->prefixTemplates.elements[templateIdx],
                            irInstruction,
                            (X86_64LinuxOperand const *const *)operands, file));
  }

  x86_64LinuxOperandFree(operands[1]);

  return operands[0];
}

static void instantiateCastPostfix(AsmCast const *cast,
                                   IRInstruction const *irInstruction,
                                   X86_64LinuxOperand *destination,
                                   IROperand const *source,
                                   X86_64LinuxFrag *assembly,
                                   FileListEntry *file) {
  X86_64LinuxOperand *operands[2];

  operands[0] = destination;
  operands[1] = operandCreate(source, file);

  // instantiate each template
  for (size_t templateIdx = 0; templateIdx < cast->postfixTemplates.size;
       ++templateIdx) {
    insertNodeEnd(
        &assembly->data.text.instructions,
        instantiateTemplate(cast->postfixTemplates.elements[templateIdx],
                            irInstruction,
                            (X86_64LinuxOperand const *const *)operands, file));
  }

  x86_64LinuxOperandFree(operands[1]);
}
static AsmCast const *findCast(Vector const *casts,
                               IRRequirement const *destination,
                               IROperand const *source) {
  for (size_t idx = 0; idx < casts->size; ++idx) {
    AsmCast const *cast = casts->elements[idx];
    if (operandSatisfiesRequirement(destination, cast->destination) &&
        operandSatisfiesRequirement(cast->source, source) &&
        destination->castType == cast->castType) {
      return cast;
    }
  }

  return NULL;
}

static void asmCastFree(AsmCast *cast) {
  irRequirementFree(cast->source);
  irOperandFree(cast->destination);
  vectorUninit(&cast->prefixTemplates, (void (*)(void *))asmTemplateFree);
  vectorUninit(&cast->postfixTemplates, (void (*)(void *))asmTemplateFree);
  free(cast);
}

static void generateAsmInstructions(Vector *instructions, Vector *casts) {
  vectorInit(instructions);
  vectorInit(casts);

  AsmInstruction *i;
  IRRequirement *r;
  IRRequirementClause *c;
  AsmTemplate *t;

  // IO_LABEL
  {
    i = asmInstructionCreate(IO_LABEL);

    // arg 0
    {
      r = requirementCreate(CT_READ);

      // LOCAL
      {
        c = requirementClauseCreate(OK_CONSTANT);
        sizeVectorInsert(&c->data.constant.types, DT_LOCAL);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_LABEL, format("{}:\n"));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_READ));
      t->label = 0;
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_UNINITIALIZED
  {
    i = asmInstructionCreate(IO_UNINITIALIZED);
    // arg 0
    {
      r = requirementCreate(CT_READ);
      // TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        vectorInsert(&r->clauses, c);
      }
      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, strdup(""));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(
                       0, X86_64_LINUX_OM_READ | X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_ADDROF
  {
    i = asmInstructionCreate(IO_ADDROF);

    // arg 0
    {
      r = requirementCreate(CT_WRITE);
      // REG
      {
        c = requirementClauseCreate(OK_REG);
        sizeVectorInsert(&c->data.reg.sizes, 8);
        vectorInsert(&r->clauses, c);
      }

      // GP TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_GP);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    // arg 1
    {
      r = requirementCreate(X86_64_LINUX_OM_READ | X86_64_LINUX_OM_ESCAPES);

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, format("\tlea {}, {}\n"));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(1, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_NOP
  {
    i = asmInstructionCreate(IO_NOP);
    // no args

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, strdup(""));
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_MOVE (mov r/m*, r*; mov r/m*, imm*)
  {
    i = asmInstructionCreate(IO_MOVE);

    // arg 0
    {
      r = requirementCreate(CT_WRITE);

      // REG
      {
        c = requirementClauseCreate(OK_REG);
        vectorInsert(&r->clauses, c);
      }

      // GP TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_GP);
        vectorInsert(&r->clauses, c);
      }

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    // arg 1
    {
      r = requirementCreate(CT_READ);

      // REG
      {
        c = requirementClauseCreate(OK_REG);
        vectorInsert(&r->clauses, c);
      }

      // GP TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_GP);
        vectorInsert(&r->clauses, c);
      }

      // CONST (1, 2, 4 bytes)
      {
        c = requirementClauseCreate(OK_CONSTANT);
        sizeVectorInsert(&c->data.constant.types, DT_BYTE);
        sizeVectorInsert(&c->data.constant.types, DT_SHORT);
        sizeVectorInsert(&c->data.constant.types, DT_INT);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\tmov {}, {}\n"));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(1, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_MOVE (mov r*, m*; mov r64, imm64)
  {
    i = asmInstructionCreate(IO_MOVE);

    // arg 0
    {
      r = requirementCreate(CT_WRITE);

      // REG
      {
        c = requirementClauseCreate(OK_REG);
        vectorInsert(&r->clauses, c);
      }

      // GP TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_GP);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    // arg 1
    {
      r = requirementCreate(CT_READ);

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      // CONST (8 bytes)
      {
        c = requirementClauseCreate(OK_CONSTANT);
        sizeVectorInsert(&c->data.constant.types, DT_LONG);
        sizeVectorInsert(&c->data.constant.types, DT_LOCAL);
        sizeVectorInsert(&c->data.constant.types, DT_GLOBAL);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\tmov {}, {}\n"));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(1, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);
    }

    vectorInsert(instructions, i);
  }

  // IO_MOVE (memcpy temp, temp)
  {
    i = asmInstructionCreate(IO_MOVE);
    i->avoid = true;

    // arg 0
    {
      r = requirementCreate(CT_WRITE);

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    // arg 1
    {
      r = requirementCreate(CT_READ);

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\tlea {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RDI, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, format("\tlea {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RSI, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(1, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, format("\tmov {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RCX, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   specialTemplateOperandCreate(TOK_SPECIAL_OPSIZE));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\trep movsb\n"));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RDI, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RSI, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RCX, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&i->templates, t);
    }
  }

  // IO_MOVE (memcpy temp, constant)
  {
    i = asmInstructionCreate(IO_MOVE);
    i->avoid = true;

    // arg 0
    {
      r = requirementCreate(CT_WRITE);

      // MEM TEMP
      {
        c = requirementClauseCreate(OK_TEMP);
        sizeVectorInsert(&c->data.temp.kinds, AH_MEM);
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    // arg 1
    {
      r = requirementCreate(CT_READ);

      // LARGE CONSTANT
      {
        c = requirementClauseCreate(OK_CONSTANT);
        c->data.constant.small = false;
        vectorInsert(&r->clauses, c);
      }

      vectorInsert(&i->requirements, r);
    }

    {
      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\tlea {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RDI, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(0, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, format("\tmov {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RSI, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   argTemplateOperandCreate(1, X86_64_LINUX_OM_READ));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_REGULAR, format("\tmov {}, {}\n"));
      vectorInsert(&t->operands, fixedTemplateOperandCreate(
                                     regOperandCreate(X86_64_LINUX_RCX, 8),
                                     X86_64_LINUX_OM_WRITE));
      vectorInsert(&t->operands,
                   specialTemplateOperandCreate(TOK_SPECIAL_OPSIZE));
      vectorInsert(&i->templates, t);

      t = asmTemplateCreate(X86_64_LINUX_IK_MOVE, format("\trep movsb\n"));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RDI, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RSI, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&t->operands,
                   fixedTemplateOperandCreate(
                       regOperandCreate(X86_64_LINUX_RCX, 8),
                       X86_64_LINUX_OM_READ | X86_64_LINUX_OM_WRITE |
                           X86_64_LINUX_OM_IMPLICIT));
      vectorInsert(&i->templates, t);
    }
  }

  // TODO
}

static void generateTextAsm(IRFrag const *frag, FileListEntry *file,
                            Vector const *asmInstructions,
                            Vector const *asmCasts) {
  // TODO: this is probably generic enough to be non-arch-specific
  // just need to make the operand->text function arch-specific

  X86_64LinuxFile *asmFile = file->asmFile;
  X86_64LinuxFrag *assembly = x86_64LinuxTextFragCreate(
      format("section .text\nglobal %s:function\n%s:\n", frag->name.global,
             frag->name.global),
      strdup(""));
  IRBlock *b = frag->data.text.blocks.head->next->data;
  for (ListNode *currInst = b->instructions.head->next;
       currInst != b->instructions.tail; currInst = currInst->next) {
    IRInstruction *ir = currInst->data;

    // find appropriate instruction template
    AsmInstruction const *selectedInstruction = NULL;
    AsmCast const **selectedCasts = NULL;
    size_t selectedRequiredCasts = 0;
    for (size_t instructionIdx = 0; instructionIdx < asmInstructions->size;
         ++instructionIdx) {
      AsmInstruction const *candidateInstruction =
          asmInstructions->elements[instructionIdx];

      // must implement the right op
      if (candidateInstruction->op != ir->op) {
        continue;
      }

      AsmCast const **candidateCasts =
          malloc(sizeof(AsmCast *) * candidateInstruction->requirements.size);
      size_t candidateRequiredCasts = 0;

      // for each operand, must be satisfied or able to convert it
      bool requirementsSatisfiable = true;
      for (size_t argIdx = 0; argIdx < candidateInstruction->requirements.size;
           ++argIdx) {
        IRRequirement *requirement =
            candidateInstruction->requirements.elements[argIdx];
        IROperand *operand = ir->args[argIdx];

        if (operandSatisfiesRequirement(requirement, operand)) {
          // if the requirement is directly satisfied, move on
          candidateCasts[argIdx] = NULL;
          continue;
        }

        // find required cast
        AsmCast const *found = findCast(asmCasts, requirement, operand);
        if (found == NULL) {
          // can't satisfy requirements; stop
          requirementsSatisfiable = false;
          break;
        }

        // save required cast
        candidateCasts[argIdx] = found;
        ++candidateRequiredCasts;
      }

      if (!requirementsSatisfiable) {
        // this isn't a candidate; move on
        free(candidateCasts);
        continue;
      }

      // compare against current selected instruction
      if (selectedInstruction == NULL) {
        // first candidate; select it
        selectedInstruction = candidateInstruction;
        selectedCasts = candidateCasts;
        selectedRequiredCasts = candidateRequiredCasts;
      } else if (selectedRequiredCasts > candidateRequiredCasts ||
                 (selectedInstruction->avoid && !candidateInstruction->avoid)) {
        // better than current candidate; select it
        // either because this requires less casts
        // or because this isn't avoided and the current candidate is
        selectedInstruction = candidateInstruction;
        free(selectedCasts);
        selectedCasts = candidateCasts;
        selectedRequiredCasts = candidateRequiredCasts;
      } else {
        // this wasn't better; clean up and move on
        free(candidateCasts);
        continue;
      }
    }

    if (selectedInstruction == NULL) {
      // note: yes, this leaks, but it aborts immiediately after, so it's GC'd
      // by the OS
      error(__FILE__, __LINE__,
            format("no instruction template found for %s",
                   IROPERATOR_NAMES[ir->op]));
    }

    // get operands (with casts)
    X86_64LinuxOperand **operands =
        malloc(sizeof(IROperand *) * selectedInstruction->requirements.size);
    for (size_t argIdx = 0; argIdx < selectedInstruction->requirements.size;
         ++argIdx) {
      if (selectedCasts[argIdx] == NULL) {
        // no cast required
        operands[argIdx] = operandCreate(ir->args[argIdx], file);
      } else {
        // do cast
        operands[argIdx] = instantiateCastPrefix(
            selectedCasts[argIdx], ir, ir->args[argIdx], assembly, file);
      }
    }

    // generate instruction
    instantiateInstruction(selectedInstruction, ir,
                           (X86_64LinuxOperand const *const *)operands,
                           assembly, file);

    // generate cast finalization
    for (size_t argIdx = 0; argIdx < selectedInstruction->requirements.size;
         ++argIdx) {
      if (selectedCasts[argIdx] != NULL) {
        // finalize cast
        instantiateCastPostfix(selectedCasts[argIdx], ir, operands[argIdx],
                               ir->args[argIdx], assembly, file);
      }
    }

    // clean up
    for (size_t argIdx = 0; argIdx < selectedInstruction->requirements.size;
         ++argIdx)
      x86_64LinuxOperandFree(operands[argIdx]);
    free(operands);
    free(selectedCasts);
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
          generateDataAsm(frag, file);
          break;
        }
        case FT_TEXT: {
          generateTextAsm(frag, file, &asmInstructions, &asmCasts);
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