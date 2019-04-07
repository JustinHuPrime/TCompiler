// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation for the symbol table

#include "util/symbolTable.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>  // FIXME: debug only

static SymbolTableEntry *symbolTableEntryCreate(char *name) {
  SymbolTableEntry *entry = malloc(sizeof(SymbolTableEntry));
  entry->name = name;
  return entry;
}
SymbolTableEntry *structTypeSymbolTableEntryCreate(char *name,
                                                   NodePairList *elements) {
  SymbolTableEntry *entry = symbolTableEntryCreate(name);
  entry->category = ST_STRUCT;
  entry->data.structType.elements = elements;
  return entry;
}
SymbolTableEntry *unionTypeSymbolTableEntryCreate(char *name,
                                                  NodePairList *options) {
  SymbolTableEntry *entry = symbolTableEntryCreate(name);
  entry->category = ST_UNION;
  entry->data.unionType.options = options;
  return entry;
}
SymbolTableEntry *enumTypeSymbolTableEntryCreate(char *name,
                                                 NodeList *elements) {
  SymbolTableEntry *entry = symbolTableEntryCreate(name);
  entry->category = ST_ENUM;
  entry->data.enumType.elements = elements;
  return entry;
}
SymbolTableEntry *typedefTypeSymbolTableEntryCreate(char *name,
                                                    Node *actualType) {
  SymbolTableEntry *entry = symbolTableEntryCreate(name);
  entry->category = ST_TYPEDEF;
  entry->data.typedefType.actualType = actualType;
  return entry;
}
SymbolTableEntry *varSymbolTableEntryCreate(char *name, Node *type) {
  SymbolTableEntry *entry = symbolTableEntryCreate(name);
  entry->category = ST_VAR;
  entry->data.var.type = type;
  return entry;
}

void symbolTableEntryDestroy(SymbolTableEntry *entry) {
  if (entry == NULL) return;

  free(entry->name);
  switch (entry->category) {
    case ST_STRUCT: {
      nodePairListDestroy(entry->data.structType.elements);
      break;
    }
    case ST_UNION: {
      nodePairListDestroy(entry->data.unionType.options);
      break;
    }
    case ST_ENUM: {
      nodeListDestroy(entry->data.enumType.elements);
      break;
    }
    case ST_TYPEDEF: {
      nodeDestroy(entry->data.typedefType.actualType);
      break;
    }
    case ST_VAR: {
      nodeDestroy(entry->data.var.type);
      break;
    }
  }
  free(entry);
}

SymbolTable *symbolTableCreate(void) {
  SymbolTable *table = malloc(sizeof(SymbolTable));
  table->size = 0;
  table->capacity = 1;
  table->entries = malloc(sizeof(SymbolTableEntry *));
  return table;
}

int const ST_OK = 0;
int const ST_EEXISTS = 1;

static size_t symbolTableLookupExpectedIndex(SymbolTable *table,
                                             char const *name) {
  if (table->size == 0) return 0;  // edge case
  size_t high = table->size - 1;   // hightest possible index that might match
  size_t low = 0;                  // lowest possible index that might match

  while (high != low) {
    size_t half = low + (high - low) / 2;
    int cmpVal = strcmp(table->entries[half]->name, name);
    if (cmpVal < 0) {  // entries[half] is before name
      low = half + 1;
      if (low == table->size) {
        return table->size;
      }
    } else if (cmpVal > 0) {  // entries[half] is after name
      if (half == 0) {        // should be before zero
        return 0;
      } else {
        high = half - 1;
      }
    } else {  // match!
      return half;
    }
  }

  // high == expected place
  int cmpVal = strcmp(table->entries[high]->name, name);
  if (cmpVal < 0) {  // entries[high] is before name
    return high + 1;
  } else {  // entries[high] is after name or equal to name
    return high;
  }
}
static size_t symbolTableLookupIndex(SymbolTable *table, char const *name) {
  if (table->size == 0) return SIZE_MAX;  // edge case

  size_t expectedIndex = symbolTableLookupExpectedIndex(table, name);
  if (expectedIndex >= table->size) return SIZE_MAX;

  return strcmp(table->entries[expectedIndex]->name, name) == 0 ? expectedIndex
                                                                : SIZE_MAX;
}
static int symbolTableInsert(SymbolTable *table, SymbolTableEntry *entry) {
  if (symbolTableLookupIndex(table, entry->name) != SIZE_MAX) return ST_EEXISTS;

  if (table->size == table->capacity) {
    table->capacity *= 2;
    SymbolTableEntry **newTable =
        malloc(table->capacity * sizeof(SymbolTableEntry *));

    size_t insertionIndex = symbolTableLookupExpectedIndex(
        table, entry->name);  // find where it should go
    printf("Inserting %s into %zd.\n", entry->name,
           insertionIndex);  // FIXME: debug only
    memcpy(newTable, table->entries,
           insertionIndex *
               sizeof(SymbolTableEntry *));  // copy everything before it
    newTable[insertionIndex] = entry;        // copy it
    memcpy(newTable + insertionIndex + 1, table->entries + insertionIndex,
           (table->size - insertionIndex) *
               sizeof(SymbolTableEntry *));  // copy everything after it

    free(table->entries);
    table->entries = newTable;
    table->size++;
    return ST_OK;
  } else {
    size_t insertionIndex = symbolTableLookupExpectedIndex(
        table, entry->name);  // find where it should go
    printf("Inserting %s into %zd.\n", entry->name,
           insertionIndex);  // FIXME: debug only
    memmove(table->entries + insertionIndex + 1,
            table->entries + insertionIndex,
            (table->size - insertionIndex) * sizeof(SymbolTableEntry *));
    // for (size_t idx = table->size; idx > insertionIndex; idx--)
    //   table->entries[idx] =
    //       table->entries[idx - 1];  // shift everything down by one

    table->entries[insertionIndex] = entry;
    table->size++;
    return ST_OK;
  }
}
int symbolTableInsertStruct(SymbolTable *table, char *name,
                            NodePairList *elements) {
  SymbolTableEntry *entry = structTypeSymbolTableEntryCreate(name, elements);
  entry->category = ST_STRUCT;
  int retVal = symbolTableInsert(table, entry);
  if (retVal != ST_OK) {
    symbolTableEntryDestroy(entry);
  }
  return retVal;
}
int symbolTableInsertUnion(SymbolTable *table, char *name,
                           NodePairList *options) {
  SymbolTableEntry *entry = unionTypeSymbolTableEntryCreate(name, options);
  entry->category = ST_UNION;
  int retVal = symbolTableInsert(table, entry);
  if (retVal != ST_OK) {
    symbolTableEntryDestroy(entry);
  }
  return retVal;
}
int symbolTableInsertEnum(SymbolTable *table, char *name, NodeList *elements) {
  SymbolTableEntry *entry = enumTypeSymbolTableEntryCreate(name, elements);
  entry->category = ST_ENUM;
  int retVal = symbolTableInsert(table, entry);
  if (retVal != ST_OK) {
    symbolTableEntryDestroy(entry);
  }
  return retVal;
}
int symbolTableInsertTypedef(SymbolTable *table, char *name, Node *actualType) {
  SymbolTableEntry *entry = typedefTypeSymbolTableEntryCreate(name, actualType);
  entry->category = ST_TYPEDEF;
  int retVal = symbolTableInsert(table, entry);
  if (retVal != ST_OK) {
    symbolTableEntryDestroy(entry);
  }
  return retVal;
}
int symbolTableInsertVar(SymbolTable *table, char *name, Node *type) {
  SymbolTableEntry *entry = varSymbolTableEntryCreate(name, type);
  entry->category = ST_VAR;
  int retVal = symbolTableInsert(table, entry);
  if (retVal != ST_OK) {
    symbolTableEntryDestroy(entry);
  }
  return retVal;
}
SymbolTableEntry *symbolTableLookup(SymbolTable *table, char const *name) {
  size_t index = symbolTableLookupIndex(table, name);
  return index == SIZE_MAX ? NULL : table->entries[index];
}

// Destructor
void symbolTableDestroy(SymbolTable *table) {
  for (size_t idx = 0; idx < table->size; idx++)
    symbolTableEntryDestroy(table->entries[idx]);
  free(table->entries);
  free(table);
}

SymbolTableTable *symbolTableTableCreate(void) {
  SymbolTableTable *table = malloc(sizeof(SymbolTableTable));
  table->size = 0;
  table->capacity = 1;
  table->tables = malloc(sizeof(SymbolTable *));
  table->names = malloc(sizeof(char *));
  return table;
}

int const STT_OK = 0;
int const STT_EEXISTS = 1;

static size_t symbolTableTableLookupExpectedIndex(SymbolTableTable *table,
                                                  char const *name) {
  if (table->size == 0) return 0;  // edge case
  size_t high = table->size - 1;   // hightest possible index that might match
  size_t low = 0;                  // lowest possible index that might match

  while (high >= low) {
    size_t half = low + (high - low) / 2;
    int cmpVal = strcmp(table->names[half], name);
    if (cmpVal < 0) {  // entries[half] is before name
      low = half + 1;
      if (low >= table->size) {
        return table->size;
      }
    } else if (cmpVal > 0) {  // entries[half] is after name
      if (half == 0) {        // should be before zero
        return 0;
      } else {
        high = half - 1;
      }
    } else {  // match!
      return half;
    }
  }

  // high == expected place
  int cmpVal = strcmp(table->names[high], name);
  if (cmpVal < 0) {  // entries[high] is before name
    return high + 1;
  } else {  // entries[high] is after name or equal to name
    return high;
  }
}
static size_t symbolTableTableLookupIndex(SymbolTableTable *table,
                                          char const *name) {
  if (table->size == 0) return SIZE_MAX;  // edge case

  size_t expectedIndex = symbolTableTableLookupExpectedIndex(table, name);
  if (expectedIndex >= table->size) return SIZE_MAX;

  return strcmp(table->names[expectedIndex], name) == 0 ? expectedIndex
                                                        : SIZE_MAX;
}
int symbolTableTableInsert(SymbolTableTable *table, char *name,
                           SymbolTable *toInsert) {
  if (symbolTableTableLookupIndex(table, name) != SIZE_MAX) {
    free(name);
    symbolTableDestroy(toInsert);
    return STT_EEXISTS;
  }

  if (table->size == table->capacity) {
    table->capacity *= 2;
    SymbolTable **newTables = malloc(table->capacity * sizeof(SymbolTable *));
    char **newNames = malloc(table->capacity * sizeof(char *));

    size_t insertionIndex = symbolTableTableLookupExpectedIndex(
        table, name);  // find where it should go
    memcpy(newTables, table->tables, insertionIndex * sizeof(SymbolTable *));
    memcpy(newNames, table->names,
           insertionIndex * sizeof(char *));  // copy everything before it
    newTables[insertionIndex] = toInsert;     // copy it
    newNames[insertionIndex] = name;
    memcpy(newTables + insertionIndex + 1, table->tables + insertionIndex,
           (table->size - insertionIndex) * sizeof(SymbolTable *));
    memcpy(newNames + insertionIndex + 1, table->names + insertionIndex,
           (table->size - insertionIndex) *
               sizeof(char *));  // copy everything after it

    free(table->tables);
    free(table->names);
    table->tables = newTables;
    table->names = newNames;
    table->size++;
    return STT_OK;
  } else {
    size_t insertionIndex = symbolTableTableLookupExpectedIndex(
        table, name);  // find where it should go
    for (size_t idx = table->size; idx > insertionIndex; idx--) {
      // shift everything down by one
      table->tables[idx] = table->tables[idx - 1];
      table->names[idx] = table->names[idx - 1];
    }

    table->tables[insertionIndex] = toInsert;
    table->names[insertionIndex] = name;
    table->size++;
    return STT_OK;
  }
}
SymbolTable *symbolTableTableLookup(SymbolTableTable *table, char const *name) {
  size_t index = symbolTableTableLookupIndex(table, name);
  return index == SIZE_MAX ? NULL : table->tables[index];
}

// destructor
void symbolTableTableDestroy(SymbolTableTable *table) {
  for (size_t idx = 0; idx < table->size; idx++) {
    free(table->names[idx]);
    symbolTableDestroy(table->tables[idx]);
  }
  free(table->names);
  free(table->tables);
  free(table);
}