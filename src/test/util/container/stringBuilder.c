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

// Tests for format

#include "util/container/stringBuilder.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void stringBuilderTest(TestStatus *status) {
  StringBuilder *sb = stringBuilderCreate();

  test(status,
       "[util] [stringBuilder] [ctor] ctor produces stringbuilder of size 0",
       sb->size == 0);
  test(
      status,
      "[util] [stringBuilder] [ctor] ctor produces stringbuilder of capacity 1",
      sb->capacity == 1);
  test(status,
       "[util] [stringBuilder] [ctor] ctor produces stringBuilder with "
       "non-null buffer",
       sb->string != NULL);

  stringBuilderPush(sb, 'a');
  test(status, "[util] [stringBuilder] [stringBuilderPush] push changes size",
       sb->size == 1);
  test(status,
       "[util] [stringBuilder] [stringBuilderPush] push doesn't change "
       "capacity when not full",
       sb->capacity == 1);
  test(status,
       "[util] [stringBuilder] [stringBuilderPush] push writes the char",
       sb->string[0] == 'a');

  stringBuilderPush(sb, 'b');
  test(status, "[util] [stringBuilder] [stringBuilderPush] push changes size",
       sb->size == 2);
  test(status,
       "[util] [stringBuilder] [stringBuilderPush] push changes capacity when "
       "full",
       sb->capacity == 2);
  test(status,
       "[util] [stringBuilder] [stringBuilderPush] push writes the char",
       sb->string[1] == 'b');
  test(status,
       "[util] [stringBuilder] [stringBuilderPush] push doesn't change "
       "previous chars",
       sb->string[0] == 'a');

  stringBuilderPop(sb);
  test(status, "[util] [stringBuilder] [stringBuilderPop] pop changes size",
       sb->size == 1);
  test(status,
       "[util] [stringBuilder] [stringBuilderPop] pop doesn't change capacity",
       sb->capacity == 2);
  test(status,
       "[util] [stringBuilder] [stringBuilderPop] pop doesn't change unpopped "
       "chars",
       sb->string[0] == 'a');

  char *data = stringBuilderData(sb);
  test(status,
       "[util] [stringBuilder] [stringBuilderData] data doesn't change size",
       sb->size == 1);
  test(status,
       "[util] [stringBuilder] [stringBuilderData] data doesn't change "
       "capacity",
       sb->capacity == 2);
  test(status,
       "[util] [stringBuilder] [stringBuilderData] data doesn't change "
       "existing data",
       sb->string[0] == 'a');
  test(status,
       "[util] [stringBuilder] [stringBuilderData] data produces copy, with "
       "added null",
       strcmp(data, "a") == 0);
  free(data);

  stringBuilderClear(sb);
  test(status,
       "[util] [stringBuilder] [stringBuilderClear] clear sets size to zero",
       sb->size == 0);
  test(status,
       "[util] [stringBuilder] [stringBuilderClear] clear doesn't change "
       "capacity",
       sb->capacity == 2);

  stringBuilderDestroy(sb);
}