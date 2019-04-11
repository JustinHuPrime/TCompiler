// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implemetation of parser utility tables

#include "parser/parseTables.h"
#include "util/functional.h"

#include <stdlib.h>
#include <string.h>

ModuleFileTable *moduleFileTableCreate(void) { return hashMapCreate(); }

char const *moduleFileTableGet(ModuleFileTable *table, char const *key) {
  return hashMapGet(table, key);
}

int moduleFileTablePut(ModuleFileTable *table, char const *key,
                       char const *data) {
  // Note that even though data is void *, because we pass it to a nullDtor, it
  // never gets modified
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
  return hashMapPut(table, key, (void *)data, nullDtor);
#pragma GCC diagnostic pop
}

void moduleFileTableDestroy(ModuleFileTable *table) {
  hashMapDestroy(table, nullDtor);
}