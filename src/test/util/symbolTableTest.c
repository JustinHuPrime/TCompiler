// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for the symbol table

#include "util/symbolTable.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void symbolTableTest(TestStatus *status) {
  SymbolTable *table = symbolTableCreate();

  test(status,
       "[util] [symbolTable] [constructor] sTable created has size zero",
       table->size == 0);
  test(status,
       "[util] [symbolTable] [constructor] sTable created has capacity one",
       table->capacity == 1);
  test(status,
       "[util] [symbolTable] [constructor] sTable created has non-null pointer "
       "for entries",
       table->entries != NULL);

  symbolTableInsertStruct(table, strcpy(malloc(7), "struct"),
                          nodePairListCreate());
  test(status,
       "[util] [symbolTable] [insert] sTable insertion adds one to size",
       table->size == 1);
  symbolTableInsertUnion(table, strcpy(malloc(6), "union"),
                         nodePairListCreate());
  test(status,
       "[util] [symbolTable] [insert] sTable insertion has correct category",
       table->entries[1]->category == ST_UNION);
  symbolTableInsertEnum(table, strcpy(malloc(5), "enum"), nodeListCreate());
  test(status,
       "[util] [symbolTable] [insert] sTable insertion increases capacity "
       "exponentially",
       table->capacity == 4);
  int retVal =
      symbolTableInsertTypedef(table, strcpy(malloc(8), "typedef"),
                               keywordTypeNodeCreate(0, 0, TYPEKWD_BOOL));
  test(status,
       "[util] [symbolTable] [insert] return value for success is correct",
       retVal == ST_OK);
  Node *varValue = keywordTypeNodeCreate(0, 0, TYPEKWD_BOOL);
  symbolTableInsertVar(table, strcpy(malloc(4), "var"), varValue);
  retVal = symbolTableInsertVar(table, strcpy(malloc(4), "var"),
                                keywordTypeNodeCreate(0, 0, TYPEKWD_BOOL));
  test(status,
       "[util] [symbolTable] [insert] return value for failure is correct",
       retVal == ST_EEXISTS);

  SymbolTableEntry *entry = symbolTableLookup(table, "var");
  test(status,
       "[util] [symbolTable] [lookup] return value for success has correct "
       "category",
       entry->category == ST_VAR);
  test(
      status,
      "[util] [symbolTable] [lookup] return value for success has correct name",
      strcmp(entry->name, "var") == 0);
  test(
      status,
      "[util] [symbolTable] [lookup] return value for success has correct data",
      entry->data.var.type == varValue);

  entry = symbolTableLookup(table, "ptruct");
  test(status, "[util] [symbolTable] [lookup] return value for failure is null",
       entry == NULL);

  symbolTableDestroy(table);
}