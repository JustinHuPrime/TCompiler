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

// x86_64 frame implementation

#include "x86_64/frame.h"

#include "util/format.h"
#include "util/internalError.h"

#include <stdlib.h>

static void x86_64FrameDtor(X86_64Frame *frame) {
  free(frame->base.vtable);
  free(frame);
}
static IRStmVector *x86_64GenerateEntryExit(X86_64Frame *frame,
                                            IRStmVector *body,
                                            char *exitLabel) {
  notYetImplemented(__FILE__, __LINE__);
}
static IRExp *x86_64FPExp(void) { notYetImplemented(__FILE__, __LINE__); }
static Access *x86_64AllocLocal(X86_64Frame *frame, size_t size, bool escapes) {
  notYetImplemented(__FILE__, __LINE__);
}
static Access *x86_64AllocOutArg(X86_64Frame *frame, size_t size) {
  notYetImplemented(__FILE__, __LINE__);
}
static Access *x86_64AllocInArg(X86_64Frame *frame, size_t size, bool escapes) {
  notYetImplemented(__FILE__, __LINE__);
}
Frame *x86_64FrameCtor(void) {
  X86_64Frame *frame = malloc(sizeof(X86_64Frame));
  frame->base.vtable = malloc(sizeof(FrameVTable));
  frame->base.vtable->dtor = (void (*)(Frame *))x86_64FrameDtor;
  frame->base.vtable->generateEntryExit =
      (IRStmVector * (*)(Frame *, IRStmVector *, char *))
          x86_64GenerateEntryExit;
  frame->base.vtable->fpExp = x86_64FPExp;
  frame->base.vtable->allocLocal =
      (Access * (*)(Frame *, size_t, bool)) x86_64AllocLocal;
  frame->base.vtable->allocOutArg =
      (Access * (*)(Frame *, size_t)) x86_64AllocOutArg;
  frame->base.vtable->allocInArg =
      (Access * (*)(Frame *, size_t, bool)) x86_64AllocInArg;
  return (Frame *)frame;
}

static void x86_64GlobalAccessDtor(X86_64GlobalAccess *access) {
  free(access->base.vtable);
  if (access->labelOwned) {
    free(access->labelName);
  }
  free(access);
}
static IRExp *x86_64ValueExp(X86_64GlobalAccess *access, IRExp *fp) {
  irExpDestroy(fp);
  return memIRExpCreate(nameIRExpCreate(access->labelName));
}
static char *x86_64GetLabel(X86_64GlobalAccess *access) {
  access->labelOwned = false;
  return access->labelName;
}
Access *x86_64GlobalAccessCtor(char *label) {
  X86_64GlobalAccess *access = malloc(sizeof(X86_64GlobalAccess));
  access->base.vtable = malloc(sizeof(AccessVTable));
  access->base.vtable->dtor = (void (*)(Access *))x86_64GlobalAccessDtor;
  access->base.vtable->valueExp =
      (IRExp * (*)(Access *, IRExp *)) x86_64ValueExp;
  access->base.vtable->getLabel = (char *(*)(Access *))x86_64GetLabel;
  access->labelName = label;
  access->labelOwned = true;
  return (Access *)access;
}

static void x86_64LabelGeneratorDtor(X86_64LabelGenerator *generator) {
  free(generator->base.vtable);
  free(generator);
}
static char *x86_64LabelGeneratorGenerateDataLabel(
    X86_64LabelGenerator *generator) {
  return format(".LC%zu", generator->nextLabel++);
}
static char *x86_64LabelGeneratorGenerateCodeLabel(
    X86_64LabelGenerator *generator) {
  return format(".L%zu", generator->nextLabel++);
}
LabelGenerator *x86_64LabelGeneratorCtor(void) {
  X86_64LabelGenerator *generator = malloc(sizeof(X86_64LabelGenerator));
  generator->base.vtable = malloc(sizeof(LabelGeneratorVTable));
  generator->base.vtable->dtor =
      (void (*)(LabelGenerator *))x86_64LabelGeneratorDtor;
  generator->base.vtable->generateDataLabel =
      (char *(*)(LabelGenerator *))x86_64LabelGeneratorGenerateDataLabel;
  generator->base.vtable->generateCodeLabel =
      (char *(*)(LabelGenerator *))x86_64LabelGeneratorGenerateCodeLabel;
  generator->nextLabel = 1;
  return (LabelGenerator *)generator;
}