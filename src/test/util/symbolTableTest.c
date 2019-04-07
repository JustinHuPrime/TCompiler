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

  test(status, "[util] [symbolTable] [constructor] table created has size zero",
       table->size == 0);
  test(status,
       "[util] [symbolTable] [constructor] table created has capacity one",
       table->capacity == 1);
  test(status,
       "[util] [symbolTable] [constructor] table created has non-null pointer "
       "for entries",
       table->entries != NULL);

  symbolTableInsertStruct(table, strcpy(malloc(7), "struct"),
                          nodePairListCreate());
  test(status, "[util] [symbolTable] [insert] table insertion adds one to size",
       table->size == 1);
  symbolTableInsertUnion(table, strcpy(malloc(6), "union"),
                         nodePairListCreate());
  test(status,
       "[util] [symbolTable] [insert] table insertion has correct category",
       table->entries[1]->category == ST_UNION);
  symbolTableInsertEnum(table, strcpy(malloc(5), "enum"), nodeListCreate());
  test(status,
       "[util] [symbolTable] [insert] table insertion increases capacity "
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

  table = symbolTableCreate();
  symbolTableInsertStruct(table, strcpy(malloc(2), "a"), nodePairListCreate());
  symbolTableInsertStruct(table, strcpy(malloc(2), "b"), nodePairListCreate());
  symbolTableInsertStruct(table, strcpy(malloc(2), "d"), nodePairListCreate());
  symbolTableInsertStruct(table, strcpy(malloc(2), "c"), nodePairListCreate());
  test(status,
       "[util] [symbolTable] [insert] insertion complex case inserts into "
       "correct place",
       strcmp(table->entries[2]->name, "c") == 0);
  symbolTableDestroy(table);
}

void symbolTableTableTest(TestStatus *status) {
  SymbolTableTable *table = symbolTableTableCreate();
  test(status,
       "[util] [symbolTableTable] [constructor] table created has size zero",
       table->size == 0);
  test(status,
       "[util] [symbolTableTable] [constructor] table created has capacity one",
       table->capacity == 1);
  test(status,
       "[util] [symbolTableTable] [constructor] table created has non-null "
       "pointer for entries",
       table->tables != NULL);
  test(status,
       "[util] [symbolTableTable] [constructor] table created has non-null "
       "pointer for entries",
       table->names != NULL);
  SymbolTable *s1 = symbolTableCreate();
  SymbolTable *s2 = symbolTableCreate();
  SymbolTable *s3 = symbolTableCreate();
  SymbolTable *s4 = symbolTableCreate();
  symbolTableTableInsert(table, strcpy(malloc(2), "a"), s1);
  test(status, "[util] [symbolTableTable] [insert] insertion adds one to size",
       table->size == 1);
  int retVal = symbolTableTableInsert(table, strcpy(malloc(2), "c"), s3);
  test(status,
       "[util] [symbolTableTable] [insert] table insertion has correct return "
       "value",
       retVal == STT_OK);
  symbolTableTableInsert(table, strcpy(malloc(2), "d"), s4);
  test(status,
       "[util] [symbolTableTable] [insert] insertion increases capacity",
       table->capacity == 4);
  symbolTableTableInsert(table, strcpy(malloc(2), "b"), s2);
  test(status,
       "[util] [symbolTableTable] [insert] insertion inserts into correct "
       "location",
       strcmp(table->names[1], "b") == 0);
  test(status,
       "[util] [symbolTableTable] [insert] insertion inserts correct pointer",
       table->tables[1] == s2);

  retVal = symbolTableTableInsert(table, strcpy(malloc(2), "b"),
                                  symbolTableCreate());
  test(status,
       "[util] [symbolTableTable] [insert] table insertion fails when it "
       "already exists",
       retVal == STT_EEXISTS);

  SymbolTable *entry = symbolTableTableLookup(table, "b");
  test(status,
       "[util] [symbolTableTable] [lookup] return value for success is correct",
       entry == s2);

  entry = symbolTableTableLookup(table, "s6");
  test(status,
       "[util] [symbolTableTable] [lookup] return value for failure is null",
       entry == NULL);
  symbolTableTableDestroy(table);
}