// Copyright 2020 Justin Hu
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

  char *retval = stringBuilderData(&sb);
  stringBuilderUninit(&sb);
  return retval;
}