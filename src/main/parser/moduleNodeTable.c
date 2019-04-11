// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of module-node tables

#include "parser/moduleNodeTable.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

// // a hash table entry
// typedef struct {
//   char const *key;
//   Node *value;
// } ModuleNodeTableEntry;

// // A hash table between a module name and that module's ast
// typedef struct {
//   size_t size;
//   ModuleNodeTableEntry *entries;
// } ModuleNodeTable;

ModuleNodeTable *moduleNodeTableCreate(void) {
  ModuleNodeTable *table = malloc(sizeof(ModuleNodeTable));
  table->size = 1;
  table->entries = calloc(1, sizeof(ModuleNodeTableEntry));
  return table;
}

Node *moduleNodeTableGet(ModuleNodeTable *table, char const *key) {
  uint64_t hash = djb2(key);
  hash %= table->size;

  if (table->entries[hash].key == NULL) {
    return NULL;                                            // not found
  } else if (strcmp(table->entries[hash].key, key) != 0) {  // collision
    uint64_t hash2 = djb2add(key);
    for (size_t idx = (hash + hash2) % table->size; idx != hash;
         idx = (idx + hash2) % table->size) {
      if (table->entries[idx].key == NULL) {
        return NULL;
      } else if (strcmp(table->entries[idx].key, key) == 0) {  // found it!
        return table->entries[idx].value;
      }
    }
    return NULL;  // searched everywhere, couldn't find it
  } else {        // found it
    return table->entries[hash].value;
  }
}

int const MNT_OK = 0;
int const MNT_EEXISTS = 1;
int moduleNodeTablePut(ModuleNodeTable *table, char const *key, Node *data) {
  uint64_t hash = djb2(key);
  hash %= table->size;

  if (table->entries[hash].key == NULL) {
    table->entries[hash].key = key;
    table->entries[hash].value = data;
    return MNT_OK;                                          // empty spot
  } else if (strcmp(table->entries[hash].key, key) != 0) {  // collision
    uint64_t hash2 = djb2add(key);
    for (size_t idx = (hash + hash2) % table->size; idx != hash;
         idx = (idx + hash2) % table->size) {
      if (table->entries[idx].key == NULL) {  // empty spot
        table->entries[hash].key = key;
        table->entries[hash].value = data;
        return MNT_OK;
      } else if (strcmp(table->entries[idx].key, key) ==
                 0) {  // already in there
        nodeDestroy(data);
        return MNT_EEXISTS;
      }
    }
    size_t oldSize = table->size;
    ModuleNodeTableEntry *oldEntries = table->entries;
    table->size *= 2;
    table->entries =
        calloc(table->size, sizeof(ModuleNodeTableEntry));  // resize the table
    for (size_t idx = 0; idx < oldSize; idx++) {
      if (oldEntries[idx].key != NULL) {
        moduleNodeTablePut(table, oldEntries[idx].key, oldEntries[idx].value);
      }
    }
    free(oldEntries);
    return moduleNodeTablePut(table, key, data);  // recurse
  } else {                                        // already in there
    nodeDestroy(data);
    return MNT_EEXISTS;
  }
}

void moduleNodeTableDestroy(ModuleNodeTable *table) {
  for (size_t idx = 0; idx < table->size; idx++) {
    if (table->entries[idx].key != NULL) {
      nodeDestroy(table->entries[idx].value);
    }
  }
  free(table->entries);
  free(table);
}

ModuleNodeTablePair *moduleNodeTablePairCreate(void) {
  ModuleNodeTablePair *pair = malloc(sizeof(ModuleNodeTablePair));
  pair->decls = moduleNodeTableCreate();
  pair->codes = moduleNodeTableCreate();
  return pair;
}
void moduleNodeTablePairDestroy(ModuleNodeTablePair *pair) {
  moduleNodeTableDestroy(pair->decls);
  moduleNodeTableDestroy(pair->codes);
  free(pair);
}
