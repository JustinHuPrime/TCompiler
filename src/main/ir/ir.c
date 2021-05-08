// Copyright 2021 Justin Hu
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

#include "ir/ir.h"

#include <stdlib.h>

IRFrag *fragCreate(FragmentType type, char *name) {
  IRFrag *df = malloc(sizeof(IRFrag));
  df->type = type;
  df->name = name;
  return df;
}
IRFrag *dataFragCreate(FragmentType type, char *name, size_t alignment) {
  IRFrag *df = fragCreate(type, name);
  df->data.data.alignment = alignment;
  vectorInit(&df->data.data.data);
  return df;
}
IRFrag *textFragCreate(char *name) {
  IRFrag *df = fragCreate(FT_TEXT, name);
  vectorInit(&df->data.text.instructions);
  return df;
}

static IRDatum *datumCreate(DatumType type) {
  IRDatum *d = malloc(sizeof(IRDatum));
  d->type = type;
  return d;
}
IRDatum *byteDatumCreate(uint8_t val) {
  IRDatum *d = datumCreate(DT_BYTE);
  d->data.byteVal = val;
  return d;
}
IRDatum *shortDatumCreate(uint16_t val) {
  IRDatum *d = datumCreate(DT_SHORT);
  d->data.shortVal = val;
  return d;
}
IRDatum *intDatumCreate(uint32_t val) {
  IRDatum *d = datumCreate(DT_INT);
  d->data.intVal = val;
  return d;
}
IRDatum *longDatumCreate(uint64_t val) {
  IRDatum *d = datumCreate(DT_LONG);
  d->data.longVal = val;
  return d;
}
IRDatum *paddingDatumCreate(size_t len) {
  IRDatum *d = datumCreate(DT_PADDING);
  d->data.paddingLength = len;
  return d;
}
IRDatum *stringDatumCreate(uint8_t *string) {
  IRDatum *d = datumCreate(DT_STRING);
  d->data.string = string;
  return d;
}
IRDatum *wstringDatumCreate(uint32_t *wstring) {
  IRDatum *d = datumCreate(DT_WSTRING);
  d->data.wstring = wstring;
  return d;
}
IRDatum *labelDatumCreate(size_t label) {
  IRDatum *d = datumCreate(DT_LABEL);
  d->data.label = label;
  return d;
}