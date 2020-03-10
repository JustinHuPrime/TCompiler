// Copyright 2020 Justin Hu
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

#include "util/string.h"

#include "util/container/stringBuilder.h"
#include "util/conversions.h"
#include "util/format.h"

#include <string.h>

char *escapeChar(char c) {
  switch (c) {
    case '\n': {
      return strdup("'\\n'");
    }
    case '\r': {
      return strdup("'\\r'");
    }
    case '\t': {
      return strdup("'\\t'");
    }
    case '\\': {
      return strdup("'\\\\'");
    }
    case '\0': {
      return strdup("'\\0'");
    }
    case '\'': {
      return strdup("'\\''");
    }
    default: {
      if ((c >= ' ' && c <= '~')) {
        return format("'%c'", c);
      } else {
        uint8_t punned = charToU8(c);
        return format("'\\x%hhu%hhu'", (punned >> 4) & 0xf,
                      (punned >> 0) & 0xf);
      }
    }
  }
}

char *escapeString(char const *input) {
  StringBuilder sb;
  stringBuilderInit(&sb);

  for (char const *curr = input; *curr != '\0'; curr++) {
    char c = *curr;
    switch (*curr) {
      case '\n': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 'n');
        break;
      }
      case '\r': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 'r');
        break;
      }
      case '\t': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 't');
        break;
      }
      case '\\': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '\\');
        break;
      }
      case '\0': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '0');
        break;
      }
      case '"': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '"');
        break;
      }
      default: {
        if (c >= ' ' && c <= '~' && c != '"' && c != '\\') {
          // plain character
          stringBuilderPush(&sb, c);
        } else {
          // hex escape
          stringBuilderPush(&sb, '\\');
          stringBuilderPush(&sb, 'x');
          uint8_t value = charToU8(*curr);
          stringBuilderPush(&sb, u8ToNybble((value >> 4) & 0xf));
          stringBuilderPush(&sb, u8ToNybble((value >> 0) & 0xf));
        }
        break;
      }
    }
  }

  char *retVal = stringBuilderData(&sb);
  stringBuilderUninit(&sb);
  return retVal;
}