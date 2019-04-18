// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Information attached to modules created during the dependency-finding phase

#include "dependencyGraph/grapher.h"

#include "util/format.h"

#include <stdlib.h>

ModuleInfo *moduleInfoCreate(bool isCode, char const *fileName) {
  ModuleInfo *info = malloc(sizeof(ModuleInfo));
  info->fileName = fileName;
  info->isCode = isCode;
  info->numDependencies = 0;
  info->capacityDependencies = 1;
  info->dependencyNames = malloc(sizeof(char *));
  info->dependencyLines = malloc(sizeof(size_t));
  info->dependencyColumns = malloc(sizeof(size_t));
  return info;
}
void moduleInfoAddDependency(ModuleInfo *info, char *name, size_t line,
                             size_t column) {
  if (info->capacityDependencies == info->numDependencies) {
    info->capacityDependencies *= 2;
    info->dependencyNames = realloc(
        info->dependencyNames, info->capacityDependencies * sizeof(char *));
    info->dependencyLines = realloc(
        info->dependencyLines, info->capacityDependencies * sizeof(size_t));
    info->dependencyColumns = realloc(
        info->dependencyColumns, info->capacityDependencies * sizeof(size_t));
  }
  info->dependencyNames[info->numDependencies] = name;
  info->dependencyLines[info->numDependencies] = line;
  info->dependencyColumns[info->numDependencies] = column;
  info->numDependencies++;
}
void moduleInfoDestroy(ModuleInfo *info) {
  free(info->moduleName);
  for (size_t idx = 0; idx < info->numDependencies; idx++) {
    free(info->dependencyNames[idx]);
  }
  free(info->dependencyNames);
  free(info);
}

// static ModuleInfoTable *buildModuleInfoTable(Report *report, size_t numFiles,
//                                              char const **files) {
//   ModuleInfoTable *table = hashMapCreate();

//   for (size_t idx = 0; idx < numFiles; idx++) {
//     ModuleInfo *info = moduleInfoCreate(false, files[idx]);

//     FILE *inFile = fopen(files[idx], "r");
//     if (inFile == NULL) {
//       reportError(report, format("%s: error: cannot open file", files[idx]));
//       continue;
//     }

//     yyscan_t scanner;
//     dglex_init(&scanner);
//     dgset_in(inFile, scanner);

//     if (dgparse(scanner, info) == 1) {
//       reportError(
//           report,
//           format("%s: error: cannot parse file to read module information",
//                  files[idx]));

//       dglex_destroy(scanner);
//       fclose(inFile);
//       continue;
//     }

//     dglex_destroy(scanner);
//     fclose(inFile);

//     moduleInfoTablePut(table, info->moduleName, info);
//   }

//   return table;
// }
// static void checkDuplicatedRequires(Report *report, Options *options,
//                                     ModuleInfoTable *table) {
//   for (size_t idx = 0; idx < table->size; idx++) {
//     if (table->keys[idx] == NULL) continue;
//     ModuleInfo const *info = table->values[idx];
//     for (size_t currIdx = 0; idx < info->numDependencies; currIdx++) {
//       for (size_t findIdx = currIdx + 1; idx < info->numDependencies;
//            findIdx++) {
//         if (strcmp(info->dependencyNames[currIdx],
//                    info->dependencyNames[findIdx]) == 0) {
//           // findIdx duplicates currIdx
//           if (getOpt(options, OPT_WARN_DUPLICATE_IMPORT)) {
//             if (getOpt(options, OPT_WARN_DUPLICATE_FILE_ERROR)) {
//               reportError(
//                   report,
//                   format("%s:%zd:%zd: error: duplicated import at %zd:%zd",
//                          info->fileName, info->dependencyLines[currIdx],
//                          info->dependencyColumns[currIdx],
//                          info->dependencyLines[findIdx],
//                          info->dependencyColumns[findIdx]));
//             } else {
//               reportWarning(
//                   report,
//                   format("%s:%zd:%zd: warning: duplicated import at %zd:%zd",
//                          info->fileName, info->dependencyLines[currIdx],
//                          info->dependencyColumns[currIdx],
//                          info->dependencyLines[findIdx],
//                          info->dependencyColumns[findIdx]));
//             }
//           }
//         }
//       }
//     }
//   }
// }
// static void checkDependencies(Report *report, ModuleInfoTable *table,
//                               char const *target, char const *checkFor,
//                               const char *checkName, size_t checkLine,
//                               size_t checkColumn) {
//   ModuleInfo const *info = moduleInfoTableGet(table, target);
//   for (size_t idx = 0; idx < info->numDependencies; idx++) {
//     if (strcmp(info->dependencyNames[idx], checkFor) == 0) {
//       // target's dependencies contains the name we are checking for.
//       reportError(
//           report,
//           format("%s:%zd:%zd: error: module '%s' has a circular dependency "
//                  "with module '%s', declared at %s:%zd:%zd",
//                  checkName, checkLine, checkColumn, checkFor,
//                  info->moduleName, info->fileName,
//                  info->dependencyLines[idx], info->dependencyColumns[idx]));
//     } else {
//       checkDependencies(report, table, info->dependencyNames[idx], checkFor,
//                         checkName, checkLine, checkColumn);
//     }
//   }
// }
// ModuleInfoTable *moduleInfoTableCreate(Report *report, Options *options,
//                                        FileList *files) {
//   ModuleInfoTable *declTable =
//       buildModuleInfoTable(report, files->numDecl, files->decls);
//   ModuleInfoTable *codeTable =
//       buildModuleInfoTable(report, files->numCode, files->codes);

//   // check for duplicated decl modules
//   for (size_t currIdx = 0; currIdx < declTable->size; currIdx++) {
//     if (declTable->keys[currIdx] == NULL) continue;

//     for (size_t findIdx = currIdx + 1; findIdx < declTable->size; findIdx++)
//     {
//       if (declTable->keys[findIdx] == NULL) continue;
//       if (strcmp(declTable->keys[currIdx], declTable->keys[findIdx]) == 0) {
//         // findIdx duplicates currIdx
//         ModuleInfo const *currInfo = declTable->values[currIdx];
//         ModuleInfo const *foundInfo = declTable->values[findIdx];
//         reportError(report, format("%s:%zd:%zd: error: module '%s' also "
//                                    "declared at %s:%zd:%zd",
//                                    currInfo->fileName, currInfo->moduleLine,
//                                    currInfo->moduleColumn,
//                                    currInfo->moduleName, foundInfo->fileName,
//                                    foundInfo->moduleLine,
//                                    foundInfo->moduleColumn));
//       }
//     }
//   }

//   // check for duplicated requires
//   checkDuplicatedRequires(report, options, declTable);
//   checkDuplicatedRequires(report, options, codeTable);

//   // check for circular dependency (decls only - codes can't be circular
//   unless
//   // decls are)
//   for (size_t idx = 0; idx < declTable->size; idx++) {
//     if (declTable->keys[idx] != NULL) {
//       ModuleInfo const *info = declTable->values[idx];
//       checkDependencies(report, declTable, declTable->keys[idx],
//                         declTable->keys[idx], info->fileName,
//                         info->moduleLine, info->moduleColumn);
//     }
//   }

//   return declTable;
// }
ModuleInfo *moduleInfoTableGet(ModuleInfoTable *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleInfoTablePut(ModuleInfoTable *table, char const *key,
                       ModuleInfo *data) {
  return hashMapPut(table, key, data, (void (*)(void *))moduleInfoDestroy);
}
void moduleInfoTableDestroy(ModuleInfoTable *table) {
  hashMapDestroy(table, (void (*)(void *))moduleInfoDestroy);
}