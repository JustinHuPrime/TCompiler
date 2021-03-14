// Copyright 2020-2021 Justin Hu
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

#include <string.h>

#include "util/container/stringBuilder.h"
#include "util/conversions.h"
#include "util/format.h"

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

  for (char const *curr = input; *curr != '\0'; ++curr) {
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

char *escapeTChar(uint8_t c) {
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
      if (c >= ' ' && c <= '~') {
        return format("'%c'", u8ToChar(c));
      } else {
        return format("'\\x%hhu%hhu'", (c >> 4) & 0xf, (c >> 0) & 0xf);
      }
    }
  }
}

char *escapeTString(uint8_t const *input) {
  StringBuilder sb;
  stringBuilderInit(&sb);

  for (uint8_t const *curr = input; *curr != '\0'; ++curr) {
    uint8_t c = *curr;
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
        if (c >= ' ' && c <= '~') {
          // plain character
          stringBuilderPush(&sb, u8ToChar(c));
        } else {
          // hex escape
          stringBuilderPush(&sb, '\\');
          stringBuilderPush(&sb, 'x');
          stringBuilderPush(&sb, u8ToNybble((c >> 4) & 0xf));
          stringBuilderPush(&sb, u8ToNybble((c >> 0) & 0xf));
        }
        break;
      }
    }
  }

  char *retval = stringBuilderData(&sb);
  stringBuilderUninit(&sb);
  return retval;
}

char *escapeTWChar(uint32_t c) {
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
      if (c >= ' ' && c <= '~') {
        return format("'%c'", u8ToChar((uint8_t)c));
      } else if (c <= 0xff) {
        return format("'\\x%hhu%hhu'", (c >> 4) & 0xf, (c >> 0) & 0xf);
      } else {
        return format("'\\u%hhu%hhu%hhu%hhu%hhu%hhu%hhu%hhu'", (c >> 28) & 0xf,
                      (c >> 24) & 0xf, (c >> 20) & 0xf, (c >> 16) & 0xf,
                      (c >> 12) & 0xf, (c >> 8) & 0xf, (c >> 4) & 0xf,
                      (c >> 0) & 0xf);
      }
    }
  }
}

char *escapeTWString(uint32_t const *input) {
  StringBuilder sb;
  stringBuilderInit(&sb);

  for (uint32_t const *curr = input; *curr != '\0'; ++curr) {
    uint32_t c = *curr;
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
        if (c >= ' ' && c <= '~') {
          // plain character
          stringBuilderPush(&sb, u8ToChar((uint8_t)c));
        } else if (c <= 0xff) {
          // hex escape
          stringBuilderPush(&sb, '\\');
          stringBuilderPush(&sb, 'x');
          stringBuilderPush(&sb, u8ToNybble((c >> 4) & 0xf));
          stringBuilderPush(&sb, u8ToNybble((c >> 0) & 0xf));
        } else {
          // extended hex escape
          stringBuilderPush(&sb, '\\');
          stringBuilderPush(&sb, 'u');
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 28) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 24) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 20) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 16) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 12) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 8) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 4) & 0xf)));
          stringBuilderPush(&sb, u8ToNybble((uint8_t)((c >> 0) & 0xf)));
        }
        break;
      }
    }
  }

  char *retval = stringBuilderData(&sb);
  stringBuilderUninit(&sb);
  return retval;
}