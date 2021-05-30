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

#include "translation/translation.h"

#include "fileList.h"
#include "ir/ir.h"
#include "util/internalError.h"
#include "util/numericSizing.h"
#include "util/string.h"

/**
 * next id returned by fresh
 */
static size_t nextId = 1;
/**
 * generate a fresh numeric identifier
 */
static size_t fresh(void) { return nextId++; }

/**
 * generate the name prefix for the file
 *
 * @param id module id
 */
static char *generatePrefix(Node *id) {
  char *suffix;

  if (id->type == NT_ID) {
    suffix = format("%zu%s", strlen(id->data.id.id), id->data.id.id);
  } else {
    suffix = strdup("");
    Vector *components = id->data.scopedId.components;
    for (size_t idx = 0; idx < components->size; ++idx) {
      Node *component = components->elements[idx];
      char *old = suffix;
      suffix = format("%s%zu%s", old, strlen(component->data.id.id),
                      component->data.id.id);
      free(old);
    }
  }

  char *retval = format("_T%s", suffix);
  free(suffix);
  return retval;
}

/**
 * produce true if given initializer results in all-zeroes
 */
static bool initializerAllZero(Node const *initializer) {
  return false;  // TODO
}

/**
 * translate an initializer into IRDatums, given a type
 *
 * @param out vector to insert IRDatums into
 * @param type type of initialized thing
 * @param initializer initializer to translate
 */
static void translateInitializer(Vector *out, Type const *type,
                                 Node const *initializer) {
  switch (type->kind) {
    case TK_KEYWORD: {
      // TODO
      break;
    }
    case TK_QUALIFIED: {
      translateInitializer(data, irFrags, type->data.qualified.base,
                           initializer);
      break;
    }
    case TK_POINTER: {
      if (initializer->data.literal.literalType == LT_NULL) {
        vectorInsert(data, longDatumCreate(0));
      } else if (initializer->data.literal.literalType == LT_STRING) {
        size_t label = fresh();
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df =
            dataFragCreate(FT_RODATA, format(".LC%zu", label), CHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     stringDatumCreate(
                         tstrdup(initializer->data.literal.data.stringVal)));
      } else {
        size_t label = fresh();
        vectorInsert(data, labelDatumCreate(label));
        IRFrag *df =
            dataFragCreate(FT_RODATA, format(".LC%zu", label), WCHAR_WIDTH);
        vectorInsert(&df->data.data.data,
                     wstringDatumCreate(
                         twstrdup(initializer->data.literal.data.wstringVal)));
      }
      break;
    }
    case TK_ARRAY: {
      // TODO
      break;
    }
    case TK_REFERENCE: {
      // TODO
      break;
    }
    default: {
      error(__FILE__, __LINE__, "type with no literals being initialized");
    }
  }
}

/**
 * translate a constant into a fragment
 */
static void translateLiteral(Node const *name, Node const *initializer,
                             char const *namePrefix, Vector *irFrags) {
  Type const *type = name->data.id.entry->data.variable.type;
  IRFrag *df;
  if (initializer == NULL || initializerAllZero(initializer)) {
    df = dataFragCreate(FT_BSS,
                        format("%s%zu%s", namePrefix, strlen(name->data.id.id),
                               name->data.id.id),
                        typeAlignof(type));
    vectorInsert(&df->data.data.data, paddingDatumCreate(typeSizeof(type)));
    vectorInsert(irFrags, df);
  } else {
    df = dataFragCreate(
        type->kind == TK_QUALIFIED && type->data.qualified.constQual ? FT_RODATA
                                                                     : FT_DATA,
        format("%s%zu%s", namePrefix, strlen(name->data.id.id),
               name->data.id.id),
        typeAlignof(type));
    translateInitializer(&df->data.data.data, irFrags, type, initializer);
  }
  vectorInsert(irFrags, df);
}

/**
 * translate the given file
 */
static void translateFile(FileListEntry *entry) {
  char *namePrefix =
      generatePrefix(entry->ast->data.file.module->data.module.id);
  Vector *bodies = entry->ast->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; ++idx) {
    Node *body = bodies->elements[idx];
    switch (body->type) {
      case NT_FUNDEFN: {
        // TODO
        break;
      }
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        Vector *initializers = body->data.varDefn.initializers;
        for (size_t idx = 0; idx < names->size; ++idx)
          translateLiteral(names->elements[idx], initializers->elements[idx],
                           namePrefix, &entry->irFrags);
        break;
      }
      default: {
        // no translation stuff otherwise
        break;
      }
    }
  }
  free(namePrefix);
}

void translate(void) {
  // for each code file, translate it
  for (size_t idx = 0; idx < fileList.size; ++idx)
    if (fileList.entries[idx].isCode) translateFile(&fileList.entries[idx]);
}