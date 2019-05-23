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

// Implementation of the stack

#include "util/container/stack.h"

#include <stdlib.h>

Stack *stackCreate(void) { return vectorCreate(); }
void stackInit(Stack *s) { vectorInit(s); }
void stackPush(Stack *s, void *element) { vectorInsert(s, element); }
void *stackPeek(Stack const *s) { return s->elements[s->size - 1]; }
void *stackPop(Stack *s) { return s->elements[--s->size]; }
void stackUninit(Stack *s, void (*dtor)(void *)) { vectorUninit(s, dtor); }
void stackDestroy(Stack *s, void (*dtor)(void *)) { vectorDestroy(s, dtor); }