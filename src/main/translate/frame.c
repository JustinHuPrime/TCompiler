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

// abstract frames used in the translate phase

#include "translate/frame.h"

AccessVector *accessVectorCreate(void) { return vectorCreate(); }
void accessVectorInsert(AccessVector *vector, struct Access *access) {
  vectorInsert(vector, access);
}
static void accessDtorCaller(Access *access) { access->vtable->dtor(access); }
void accessVectorDestroy(AccessVector *vector) {
  vectorDestroy(vector, (void (*)(void *))accessDtorCaller);
}

void tempGeneratorInit(TempGenerator *generator) { generator->nextTemp = 0; }
size_t tempGeneratorGenerate(TempGenerator *generator) {
  return generator->nextTemp++;
}
void tempGeneratorUninit(TempGenerator *generator) {}