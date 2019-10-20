// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// implementation of x86_64 assembly representation

#include "architecture/x86_64/assembly.h"

#include "constants.h"
#include "ir/ir.h"
#include "translate/translate.h"
#include "util/format.h"
#include "util/internalError.h"

#include <stdio.h>  // TODO: DEBUG ONLY
#include <stdlib.h>
#include <string.h>

static X86_64Fragment *x86_64FragmentCreate(X86_64FragmentKind kind) {
  X86_64Fragment *f = malloc(sizeof(X86_64Fragment));
  f->kind = kind;
  return f;
}
X86_64Fragment *x86_64DataFragmentCreate(char *data) {
  X86_64Fragment *f = x86_64FragmentCreate(X86_64_FK_DATA);
  f->data.data.data = data;
  return f;
}
X86_64Fragment *x86_64TextFragmentCreate(void) {
  X86_64Fragment *f = x86_64FragmentCreate(X86_64_FK_TEXT);
  return f;
}
void x86_64FragmentDestroy(X86_64Fragment *f) {
  switch (f->kind) {
    case X86_64_FK_DATA: {
      free(f->data.data.data);
      break;
    }
    case X86_64_FK_TEXT: {
      break;
    }
  }

  free(f);
}

void x86_64FragmentVectorInit(X86_64FragmentVector *v) { vectorInit(v); }
void x86_64FragmentVectorInsert(X86_64FragmentVector *v, X86_64Fragment *f) {
  vectorInsert(v, f);
}
void x86_64FragmentVectorUninit(X86_64FragmentVector *v) {
  vectorUninit(v, (void (*)(void *))x86_64FragmentDestroy);
}

X86_64File *x86_64FileCreate(char *header, char *footer) {
  X86_64File *f = malloc(sizeof(X86_64File));
  f->header = header;
  f->footer = footer;
  x86_64FragmentVectorInit(&f->fragments);
  return f;
}
void x86_64FileDestroy(X86_64File *f) {
  free(f->header);
  free(f->footer);
  x86_64FragmentVectorUninit(&f->fragments);
  free(f);
}

void fileX86_64FileMapInit(FileX86_64FileMap *m) { hashMapInit(m); }
X86_64File *fileX86_64FileMapGet(FileX86_64FileMap *m, char const *key) {
  return hashMapGet(m, key);
}
int fileX86_64FileMapPut(FileX86_64FileMap *m, char const *key,
                         X86_64File *file) {
  return hashMapPut(m, key, file, (void (*)(void *))x86_64FileDestroy);
}
void fileX86_64FileMapUninit(FileX86_64FileMap *m) {
  hashMapUninit(m, (void (*)(void *))x86_64FileDestroy);
}

static bool isLocalLabel(char const *str) { return strncmp(str, ".L", 2) == 0; }
static char *tstrToX86_64Str(uint8_t *str) {
  char *buffer = strdup("");
  for (; *str != 0; str++) {
    buffer = format(
        "%s"
        "\t.byte\t%hhu",
        buffer, *str);
  }
  buffer = format(
      "%s"
      "\t.byte\t0\n",
      buffer);
  return buffer;
}
static char *twstrToX86_64WStr(uint32_t *wstr) {
  char *buffer = strdup("");
  for (; *wstr != 0; wstr++) {
    buffer = format(
        "%s"
        "\t.long\t%u",
        buffer, *wstr);
  }
  buffer = format(
      "%s"
      "\t.long\t0\n",
      buffer);
  return buffer;
}
static char *dataToString(IREntryVector *data) {
  char *acc = strdup("");
  for (size_t idx = 0; idx < data->size; idx++) {
    IREntry *datum = data->elements[idx];
    // datum->op == IO_CONST
    IROperand *value = datum->arg1;
    switch (value->kind) {
      case OK_CONSTANT: {
        switch (datum->opSize) {
          case 1: {  // BYTE_WIDTH, CHAR_WIDTH
            acc = format(
                "%s"
                "\t.byte\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 2: {  // SHORT_WIDTH
            acc = format(
                "%s"
                "\t.int\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 4: {  // INT_WIDTH, WCHAR_WIDTH
            acc = format(
                "%s"
                "\t.long\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
          case 8: {  // LONG_WIDTH, POINTER_WIDTH
            acc = format(
                "%s"
                "\t.quad\t%lu\n",
                acc, value->data.constant.bits);
            break;
          }
        }
        break;
      }
      case OK_NAME: {
        acc = format(
            "%s"
            "\t.quad\t%s\n",
            acc, value->data.name.name);
        break;
      }
      case OK_STRING: {
        char *str = tstrToX86_64Str(value->data.string.data);
        acc = format(
            "%s"
            "%s",
            acc, str);
        free(str);
        break;
      }
      case OK_WSTRING: {
        char *str = twstrToX86_64WStr(value->data.wstring.data);
        acc = format(
            "%s"
            "%s",
            acc, str);
        free(str);
        break;
      }
      default: {
        error(__FILE__, __LINE__, "invalid constant operand kind encountered");
      }
    }
  }
  return acc;
}
static X86_64File *fileInstructionSelect(IRFile *ir) {
  X86_64File *file =
      x86_64FileCreate(format("\t.file\t\"%s\"\n", ir->sourceFilename),
                       format("\t.ident\t\"%s\"\n"
                              "\t.section\t.note.GNU-stack,\"\",@progbits\n",
                              VERSION_STRING));

  for (size_t idx = 0; idx < ir->fragments.size; idx++) {
    Fragment *irFrag = ir->fragments.elements[idx];
    switch (irFrag->kind) {
      case FK_BSS: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.bss.size);
        }
        x86_64FragmentVectorInsert(&file->fragments,
                                   x86_64DataFragmentCreate(format(
                                       "%s"
                                       "\t.bss\n"
                                       "\t.align\t%zu\t"
                                       "%s:\n"
                                       "\t.zero\t%zu\n",
                                       prefix, irFrag->data.bss.alignment,
                                       irFrag->label, irFrag->data.bss.size)));
        free(prefix);
        break;
      }
      case FK_RODATA: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu\n",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.rodata.size);
        }
        char *data = dataToString(irFrag->data.rodata.ir);
        x86_64FragmentVectorInsert(
            &file->fragments,
            x86_64DataFragmentCreate(format(
                "%s"
                "\t.section\t.rodata\n"
                "\t.align\t%zu\n"
                "%s:\n"
                "%s",
                prefix, irFrag->data.rodata.alignment, irFrag->label, data)));
        free(prefix);
        break;
      }
      case FK_DATA: {
        char *prefix;
        if (isLocalLabel(irFrag->label)) {
          prefix = strdup("");
        } else {
          // isn't a local - add .globl and symbol data
          prefix = format(
              "\t.globl\t%s\n"
              "\t.type\t%s, @object\n"
              "\t.size\t%s, %zu\n",
              irFrag->label, irFrag->label, irFrag->label,
              irFrag->data.rodata.size);
        }
        char *data = dataToString(irFrag->data.rodata.ir);
        x86_64FragmentVectorInsert(
            &file->fragments,
            x86_64DataFragmentCreate(format(
                "%s"
                "\t.data\n"
                "\t.align\t%zu\n"
                "%s:\n"
                "%s",
                prefix, irFrag->data.rodata.alignment, irFrag->label, data)));
        free(prefix);
        break;
      }
      case FK_TEXT: {
        break;
      }
    }
  }

  return file;
}

void x86_64InstructionSelect(FileX86_64FileMap *asmFileMap,
                             FileIRFileMap *irFileMap) {
  fileX86_64FileMapInit(asmFileMap);

  for (size_t idx = 0; idx < irFileMap->capacity; idx++) {
    char const *key = irFileMap->keys[idx];
    if (key != NULL) {
      IRFile *file = irFileMap->values[idx];
      fileX86_64FileMapPut(asmFileMap, irFileMap->keys[idx],
                           fileInstructionSelect(file));
    }
  }
}