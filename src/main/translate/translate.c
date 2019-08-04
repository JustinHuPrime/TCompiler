// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// implemetation of the translation phase

#include "translate/translate.h"

#include "util/container/stringBuilder.h"
#include "util/format.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

// data structures
void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *file) {
  return hashMapGet(map, file);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char *file,
                             FragmentVector *vector) {
  return hashMapPut(map, file, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
      fragmentVectorDestroy(map->values[idx]);
    }
  }
  free(map->values);
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
      free((char *)(map->keys[idx]));
#pragma GCC diagnostic pop
    }
  }
  free(map->keys);
}

// helpers
static char *mangleModuleName(char const *moduleName) {
  char *buffer = strcpy(malloc(4), "__Z");
  StringVector *exploded = explodeName(moduleName);
  for (size_t idx = 0; idx < exploded->size; idx++) {
    buffer = format("%s%zu%s", buffer, strlen(exploded->elements[idx]),
                    (char const *)exploded->elements[idx]);
  }
  stringVectorDestroy(exploded, true);
  return buffer;
}
static char *mangleTypeName(const char *moduleName, const char *typeName) {
  return format("%s%zu%s", mangleModuleName(moduleName), strlen(typeName),
                typeName);
}
static char *mangleTypeString(TypeVector const *);
static char *mangleType(Type const *type) {
  switch (type->kind) {
    case K_VOID: {
      return strcpy(malloc(2), "v");
    }
    case K_UBYTE: {
      return strcpy(malloc(3), "ub");
    }
    case K_BYTE: {
      return strcpy(malloc(3), "sb");
    }
    case K_CHAR: {
      return strcpy(malloc(2), "c");
    }
    case K_USHORT: {
      return strcpy(malloc(3), "us");
    }
    case K_SHORT: {
      return strcpy(malloc(3), "ss");
    }
    case K_UINT: {
      return strcpy(malloc(3), "ui");
    }
    case K_INT: {
      return strcpy(malloc(3), "si");
    }
    case K_WCHAR: {
      return strcpy(malloc(2), "w");
    }
    case K_ULONG: {
      return strcpy(malloc(3), "ul");
    }
    case K_LONG: {
      return strcpy(malloc(3), "sl");
    }
    case K_FLOAT: {
      return strcpy(malloc(2), "f");
    }
    case K_DOUBLE: {
      return strcpy(malloc(2), "d");
    }
    case K_BOOL: {
      return strcpy(malloc(2), "B");
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      char *mangledTypeName =
          mangleTypeName(type->data.reference.referenced->module,
                         type->data.reference.referenced->data.type.name);
      char *buffer = format("T%zu%s", strlen(mangledTypeName), mangledTypeName);
      free(mangledTypeName);
      return buffer;
    }
    case K_CONST: {
      char *mangledType = mangleType(type->data.modifier.type);
      char *buffer = format("C%s", mangledType);
      free(mangledType);
      return buffer;
    }
    case K_ARRAY: {
      char *mangledType = mangleType(type->data.array.type);
      char *buffer = format("A%zu%s", type->data.array.size, mangledType);
      free(mangledType);
      return buffer;
    }
    case K_PTR: {
      char *mangledType = mangleType(type->data.modifier.type);
      char *buffer = format("P%s", mangledType);
      free(mangledType);
      return buffer;
    }
    case K_FUNCTION_PTR: {
      char *mangledReturnType = mangleType(type->data.functionPtr.returnType);
      char *mangledArgTypes =
          mangleTypeString(type->data.functionPtr.argumentTypes);
      char *buffer = format("F%s%s", mangledReturnType, mangledArgTypes);
      free(mangledReturnType);
      free(mangledArgTypes);
      return buffer;
    }
    default: {
      return NULL;  // can't express this type!
    }
  }
}
static char *mangleTypeString(TypeVector const *args) {
  char *buffer = strcpy(malloc(1), "");
  for (size_t idx = 0; idx < args->size; idx++) {
    char *mangledType = mangleType(args->elements[idx]);
    buffer = format("%s%s", buffer, mangledType);
    free(mangledType);
  }
  return buffer;
}
static char *mangleVarName(char const *moduleName, Node const *id) {
  return format("%s%zu%s", mangleModuleName(moduleName), strlen(id->data.id.id),
                id->data.id.id);
}
static char *mangleFunctionName(char const *moduleName, Node const *id) {
  return format("%s%zu%s%s", mangleModuleName(moduleName),
                strlen(id->data.id.id), id->data.id.id,
                mangleTypeString(&id->data.id.overload->argumentTypes));
}

// top level stuff
static bool constantNotZero(Node *initializer) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          return initializer->data.constExp.value.ubyteVal != 0;
        }
        case CT_BYTE: {
          return initializer->data.constExp.value.byteVal != 0;
        }
        case CT_CHAR: {
          return initializer->data.constExp.value.charVal != 0;
        }
        case CT_USHORT: {
          return initializer->data.constExp.value.ushortVal != 0;
        }
        case CT_SHORT: {
          return initializer->data.constExp.value.shortVal != 0;
        }
        case CT_UINT: {
          return initializer->data.constExp.value.uintVal != 0;
        }
        case CT_INT: {
          return initializer->data.constExp.value.intVal != 0;
        }
        case CT_WCHAR: {
          return initializer->data.constExp.value.wcharVal != 0;
        }
        case CT_ULONG: {
          return initializer->data.constExp.value.ulongVal != 0;
        }
        case CT_LONG: {
          return initializer->data.constExp.value.longVal != 0;
        }
        case CT_FLOAT: {
          return initializer->data.constExp.value.floatBits != 0;
        }
        case CT_DOUBLE: {
          return initializer->data.constExp.value.doubleBits != 0;
        }
        case CT_BOOL: {
          return initializer->data.constExp.value.boolVal != false;
        }
        case CT_STRING:
        case CT_WSTRING: {
          return false;
        }
        case CT_NULL: {
          return true;
        }
        default: {
          return false;  // invalid enum
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        if (constantNotZero(elements->elements[idx])) {
          return true;
        }
      }
      return false;
    }
    default: { return false; }
  }
}
static void constantToData(Node *initializer, IRExpVector *out,
                           FragmentVector *fragments) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          irExpVectorInsert(
              out,
              ubyteConstIRExpCreate(initializer->data.constExp.value.ubyteVal));
          return;
        }
        case CT_BYTE: {
          irExpVectorInsert(out, byteConstIRExpCreate(
                                     initializer->data.constExp.value.byteVal));
          return;
        }
        case CT_CHAR: {
          irExpVectorInsert(out, ubyteConstIRExpCreate(
                                     initializer->data.constExp.value.charVal));
          return;
        }
        case CT_USHORT: {
          irExpVectorInsert(out,
                            ushortConstIRExpCreate(
                                initializer->data.constExp.value.ushortVal));
          return;
        }
        case CT_SHORT: {
          irExpVectorInsert(
              out,
              shortConstIRExpCreate(initializer->data.constExp.value.shortVal));
          return;
        }
        case CT_UINT: {
          irExpVectorInsert(out, uintConstIRExpCreate(
                                     initializer->data.constExp.value.uintVal));
          return;
        }
        case CT_INT: {
          irExpVectorInsert(out, intConstIRExpCreate(
                                     initializer->data.constExp.value.intVal));
          return;
        }
        case CT_WCHAR: {
          irExpVectorInsert(
              out,
              uintConstIRExpCreate(initializer->data.constExp.value.wcharVal));
          return;
        }
        case CT_ULONG: {
          irExpVectorInsert(
              out,
              ulongConstIRExpCreate(initializer->data.constExp.value.ulongVal));
          return;
        }
        case CT_LONG: {
          irExpVectorInsert(out, longConstIRExpCreate(
                                     initializer->data.constExp.value.longVal));
          return;
        }
        case CT_FLOAT: {
          irExpVectorInsert(
              out,
              uintConstIRExpCreate(initializer->data.constExp.value.floatBits));
          return;
        }
        case CT_DOUBLE: {
          irExpVectorInsert(out,
                            doubleConstIRExpCreate(
                                initializer->data.constExp.value.doubleBits));
          return;
        }
        case CT_BOOL: {
          irExpVectorInsert(
              out, ubyteConstIRExpCreate(
                       initializer->data.constExp.value.boolVal ? 1 : 0));
          return;
        }
        case CT_STRING:
        case CT_WSTRING: {
          // TODO: write this
          return;
        }
        case CT_NULL: {
          irExpVectorInsert(out, ulongConstIRExpCreate(0));
          return;
        }
        default: {
          return;  // invalid enum
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        constantToData(elements->elements[idx], out, fragments);
      }
      return;
    }
    default: { return; }
  }
}
static void translateGlobalVar(Node *varDecl, FragmentVector *fragments,
                               char const *moduleName,
                               GlobalAccessCtor globalAccessCtor) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *id = idValuePairs->firstElements[idx];
    Node *initializer = idValuePairs->secondElements[idx];
    char *mangledName = mangleVarName(moduleName, id);
    Fragment *f;
    if (initializer == NULL || !constantNotZero(initializer)) {
      f = bssDataFragmentCreate(mangledName,
                                typeSizeof(id->data.id.symbol->data.var.type));
    } else if (id->data.id.symbol->data.var.type->kind == K_CONST) {
      f = roDataFragmentCreate(mangledName);
      constantToData(initializer, &f->data.roData.data, fragments);
    } else {
      f = dataFragmentCreate(mangledName);
      constantToData(initializer, &f->data.data.data, fragments);
    }
    id->data.id.symbol->data.var.access = globalAccessCtor(mangledName);

    fragmentVectorInsert(fragments, f);
  }

  return;
}
static void translateFunction(Node *function, FragmentVector *fragments,
                              char const *moduleName, FrameCtor frameCtor,
                              GlobalAccessCtor globalAccessCtor) {
  Fragment *f = functionFragmentCreate(
      mangleFunctionName(moduleName, function->data.function.id));

  // TODO: write this

  fragmentVectorInsert(fragments, f);
  return;
}
static void translateBody(Node *body, FragmentVector *fragments,
                          char const *moduleName, FrameCtor frameCtor,
                          GlobalAccessCtor globalAccessCtor) {
  switch (body->type) {
    case NT_VARDECL: {
      translateGlobalVar(body, fragments, moduleName, globalAccessCtor);
      return;
    }
    case NT_FUNCTION: {
      translateFunction(body, fragments, moduleName, frameCtor,
                        globalAccessCtor);
      return;
    }
    default: { return; }
  }
}

// file level stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static FragmentVector *translateFile(Node *file, FrameCtor frameCtor,
                                     GlobalAccessCtor globalAccessCtor) {
  FragmentVector *fragments = fragmentVectorCreate();

  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    translateBody(bodies->elements[idx], fragments,
                  file->data.module.id->data.id.id, frameCtor,
                  globalAccessCtor);
  }

  return fragments;
}

void translate(FileFragmentVectorMap *fragments, ModuleAstMapPair *asts,
               FrameCtor frameCtor, GlobalAccessCtor globalAccessCtor) {
  fileFragmentVectorMapInit(fragments);
  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      fileFragmentVectorMapPut(
          fragments, codeFilenameToAssembyFilename(file->data.file.filename),
          translateFile(file, frameCtor, globalAccessCtor));
    }
  }
}