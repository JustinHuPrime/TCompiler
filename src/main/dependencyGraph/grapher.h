// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Generates the dependency graph

#ifndef TLC_DEPENDENCYGRAPH_GRAPHER_H_
#define TLC_DEPENDENCYGRAPH_GRAPHER_H_

#include "util/errorReport.h"
#include "util/fileList.h"
#include "util/hashMap.h"
#include "util/options.h"

// holds the module name and its dependencies
typedef struct {
  char *moduleName;  // used to hack ownership
  size_t moduleLine, moduleColumn;
  char const *fileName;
  bool isCode;
  size_t numDependencies;
  size_t capacityDependencies;
  char **dependencyNames;
  size_t *dependencyLines;
  size_t *dependencyColumns;
} ModuleInfo;

// ctor
ModuleInfo *moduleInfoCreate(bool isCode, char const *fileName);
// add
void moduleInfoAddDependency(ModuleInfo *, char *name, size_t line,
                             size_t column);
// dtor
void moduleInfoDestroy(ModuleInfo *);

// hashMap between module name and ast node
// specialization of a generic
typedef HashMap ModuleInfoTable;
// ctor
ModuleInfoTable *moduleInfoTableCreate(Report *report, Options *options,
                                       FileList *files);
// get
// returns the node, or NULL if the key is not in the table
ModuleInfo *moduleInfoTableGet(ModuleInfoTable *, char const *key);
// put - note that key is not owned by the table, but the node is
// returns: HT_OK if the insertion was successful
//          HT_EEXISTS if the key exists
int moduleInfoTablePut(ModuleInfoTable *, char const *key, ModuleInfo *data);
// dtor
void moduleInfoTableDestroy(ModuleInfoTable *);

#endif  // TLC_DEPENDENCYGRAPH_GRAPHER_H_