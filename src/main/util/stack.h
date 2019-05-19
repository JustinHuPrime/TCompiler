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

// A stack of pointers, java generic style

#ifndef TLC_UTIL_STACK_H_
#define TLC_UTIL_STACK_H_

#include "util/vector.h"

// specialization of a vector, with additional functions
typedef Vector Stack;
// ctor
Stack *stackCreate(void);
// adds an element onto the top of the stack
void stackPush(Stack *, void *);
// returns a non-owning pointer to the top of the stack, does not change stack
void *stackPeek(Stack const *);
// returns an owning pointer to the top of the stack, removes element from stack
void *stackPop(Stack *);
// dtor
void stackDestroy(Stack *, void (*)(void *));

#endif  // TLC_UTIL_STACK_H_