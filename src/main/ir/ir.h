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

// A "polymorphic" IR node, and lists of nodes

#ifndef TLC_IR_IR_H_
#define TLC_IR_IR_H_

#include "util/container/vector.h"

#include <stdint.h>

struct IRExp;

typedef enum {
  IS_MOVE,
} IRStmKind;
typedef struct {
  IRStmKind kind;
  union {
    struct {
      struct IRExp *to;
      struct IRExp *from;
    } move;
  } data;
} IRStm;
typedef enum {
  IE_BYTE_CONST,
} IRExpKind;
typedef struct {
  IRExpKind kind;
  union {
    struct {
      uint8_t value;
    } byteConst;
  } data;
} IRExp;

typedef enum {
  FK_DATA,
  FK_BSS_DATA,
  FK_FUNCTION,
} FragmentKind;
typedef struct {
  FragmentKind kind;
  union {
    struct {
      IRStm *label;
      Vector data;
    } data;
    struct {
      IRStm *label;
      size_t nBytes;
    } bssData;
    struct {
      IRStm *label;
      Vector body;
    } function;
  } data;
} Fragment;

typedef Vector FragmentVector;

#endif  // TLC_IR_IR_H_