// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A vector-style symbol table

#ifndef TLC_UTIL_SYMBOLTABLE_H_
#define TLC_UTIL_SYMBOLTABLE_H_

#include "ast/ast.h"

#include <stddef.h>

// the category of a symbol
typedef enum {
  ST_STRUCT,
  ST_UNION,
  ST_ENUM,
  ST_TYPEDEF,
  ST_VAR,
} SymbolCategory;

typedef struct {
  char *name;
  SymbolCategory category;
  union {
    struct {
      NodePairList *elements;  // pair of type, id
    } structType;
    struct {
      NodePairList *options;  // pair of type, id
    } unionType;
    struct {
      NodeList *elements;
    } enumType;
    struct {
      Node *actualType;
    } typedefType;
    struct {
      Node *type;
    } var;
  } data;
} SymbolTableEntry;

// Constructors (polymorphic)
SymbolTableEntry *structTypeSymbolTableEntryCreate(char *name,
                                                   NodePairList *elements);
SymbolTableEntry *unionTypeSymbolTableEntryCreate(char *name,
                                                  NodePairList *options);
SymbolTableEntry *enumTypeSymbolTableEntryCreate(char *name,
                                                 NodeList *elements);
SymbolTableEntry *typedefTypeSymbolTableEntryCreate(char *name,
                                                    Node *actualType);
SymbolTableEntry *varSymbolTableEntryCreate(char *name, Node *type);

void symbolTableEntryDestroy(SymbolTableEntry *);

typedef struct {
  size_t size;
  size_t capacity;
  SymbolTableEntry **entries;
} SymbolTable;

// Constructor
SymbolTable *symbolTableCreate(void);

extern int const ST_OK;
extern int const ST_EEXISTS;

// inserts an entry of the given type into the table
// returns: ST_OK if insertion was successful,
//          ST_EEXISTS if element already exists
// Note that all pointers are owning instances (i.e. you should deep copy the
// AST nodes)
int symbolTableInsertStruct(SymbolTable *, char *name, NodePairList *elements);
int symbolTableInsertUnion(SymbolTable *, char *name, NodePairList *options);
int symbolTableInsertEnum(SymbolTable *, char *name, NodeList *elements);
int symbolTableInsertTypedef(SymbolTable *, char *name, Node *actualType);
int symbolTableInsertVar(SymbolTable *, char *name, Node *type);
// Finds an entry in the table, and returns it, as a non-owning instance.
// Returns NULL if no entry could be found
SymbolTableEntry *symbolTableLookup(SymbolTable *, char const *name);

// Destructor
void symbolTableDestroy(SymbolTable *);

// a table of tables associated some name
typedef struct {
  size_t size;
  size_t capacity;
  SymbolTable **tables;
  char **names;
} SymbolTableTable;

// constructor
SymbolTableTable *symbolTableTableCreate(void);

extern int const STT_OK;
extern int const STT_EEXISTS;

// inserts a table (owning instance) into the table of tables, associated with
// some name (owning instance)
// returns: STT_OK if insertion was successful
//          STT_EEXISTS if a table with that name already exists
// Note that both pointers should be owning instances
int symbolTableTableInsert(SymbolTableTable *, char *name, SymbolTable *table);
// finds an entry in the table, and returns a non-owning pointer to it
// returns NULL if no entry could be found.
SymbolTable *symbolTableTableLookup(SymbolTableTable *, char const *name);

// destructor
void symbolTableTableDestroy(SymbolTableTable *);

typedef struct {
  size_t localTablesSize;
  size_t localTablesCapacity;
  SymbolTable **localTables;
  SymbolTableTable *
      globalTables;  // due to destructor, this is a non-owning SymbolTableTable
} Environment;

// constructor
Environment *environmentCreate(void);

// adds a table to the top of the localTables stack
void environmentPush(Environment *);
// removes a table from the top of the LocalTables stack
void environmentPop(Environment *);
// gets the top table, as a non-owning instance
SymbolTable *environmentTop(Environment *);
// looks up an element in the environment
// returns null if no such element can be found
SymbolTableEntry *environmentLookup(Environment *, char const *);

// destructor
void environmentDestroy(Environment *);

#endif  // TLC_UTIL_SYMBOLTABLE_H_