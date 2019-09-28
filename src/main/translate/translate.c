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

// Implementation of translation

#include "translate/translate.h"
#include "constants.h"
#include "ir/frame.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "parser/parser.h"
#include "typecheck/symbolTable.h"
#include "util/format.h"
#include "util/internalError.h"
#include "util/nameUtils.h"
#include "util/tstring.h"

#include <stdlib.h>
#include <string.h>

// fragments
static Fragment *fragmentCreate(FragmentKind kind, char *label) {
  Fragment *f = malloc(sizeof(Fragment));
  f->kind = kind;
  f->label = label;
  return f;
}
Fragment *bssFragmentCreate(char *label, size_t size) {
  Fragment *f = fragmentCreate(FK_BSS, label);
  f->data.bss.size = size;
  return f;
}
Fragment *rodataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_RODATA, label);
  f->data.rodata.ir = irVectorCreate();
  return f;
}
Fragment *dataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_DATA, label);
  f->data.data.ir = irVectorCreate();
  return f;
}
Fragment *textFragmentCreate(char *label, Frame *frame) {
  Fragment *f = fragmentCreate(FK_TEXT, label);
  f->data.text.frame = frame;
  f->data.text.ir = irVectorCreate();
  return f;
}
void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_BSS: {
      break;
    }
    case FK_RODATA: {
      fragmentVectorDestroy(f->data.rodata.ir);
      break;
    }
    case FK_DATA: {
      fragmentVectorDestroy(f->data.data.ir);
      break;
    }
    case FK_TEXT: {
      fragmentVectorDestroy(f->data.text.ir);
      break;
    }
  }
  free(f->label);
  free(f);
}

// fragment vector
FragmentVector *fragmentVectorCreate(void) { return vectorCreate(); }
void fragmentVectorInsert(FragmentVector *v, Fragment *f) {
  vectorInsert(v, f);
}
void fragmentVectorDestroy(FragmentVector *v) {
  vectorDestroy(v, (void (*)(void *))fragmentDestroy);
}

void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *key) {
  return hashMapGet(map, key);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char const *key,
                             FragmentVector *vector) {
  return hashMapPut(map, key, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  hashMapUninit(map, (void (*)(void *))fragmentVectorDestroy);
}

// typeKindof
static AllocHint typeKindof(Type const *type) {
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_PTR:
    case K_FUNCTION_PTR:
    case K_ENUM: {
      return AH_GP;
    }
    case K_FLOAT:
    case K_DOUBLE: {
      return AH_SSE;
    }
    case K_STRUCT:
    case K_UNION:
    case K_ARRAY: {
      return AH_MEM;
    }
    case K_CONST: {
      return typeKindof(type->data.modifier.type);
    }
    case K_TYPEDEF: {
      return typeKindof(
          type->data.reference.referenced->data.type.data.typedefType.type);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}

// name stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static char *mangleModuleName(char const *moduleName) {
  char *buffer = strdup("__Z");
  StringVector *exploded = explodeName(moduleName);
  for (size_t idx = 0; idx < exploded->size; idx++) {
    char *newBuffer = format("%s%zu%s", buffer, strlen(exploded->elements[idx]),
                             (char const *)exploded->elements[idx]);
    free(buffer);
    buffer = newBuffer;
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
      return strdup("v");
    }
    case K_UBYTE: {
      return strdup("ub");
    }
    case K_BYTE: {
      return strdup("sb");
    }
    case K_CHAR: {
      return strdup("c");
    }
    case K_USHORT: {
      return strdup("us");
    }
    case K_SHORT: {
      return strdup("ss");
    }
    case K_UINT: {
      return strdup("ui");
    }
    case K_INT: {
      return strdup("si");
    }
    case K_WCHAR: {
      return strdup("w");
    }
    case K_ULONG: {
      return strdup("ul");
    }
    case K_LONG: {
      return strdup("sl");
    }
    case K_FLOAT: {
      return strdup("f");
    }
    case K_DOUBLE: {
      return strdup("d");
    }
    case K_BOOL: {
      return strdup("B");
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
      error(__FILE__, __LINE__,
            "attempted to mangle an unexpressable type (aggregate init type?)");
    }
  }
}
static char *mangleTypeString(TypeVector const *args) {
  char *buffer = strdup("");
  for (size_t idx = 0; idx < args->size; idx++) {
    char *mangledType = mangleType(args->elements[idx]);
    char *newBuffer = format("%s%s", buffer, mangledType);
    free(buffer);
    buffer = newBuffer;
    free(mangledType);
  }
  return buffer;
}
static char *mangleVarName(char const *moduleName, char const *id) {
  char *mangledModuleName = mangleModuleName(moduleName);
  char *retVal = format("%s%zu%s", mangledModuleName, strlen(id), id);
  free(mangledModuleName);
  return retVal;
}
static char *mangleFunctionName(char const *moduleName, char const *id,
                                TypeVector *argumentTypes) {
  char *mangledModuleName = mangleModuleName(moduleName);
  char *mangledArgumentTypes = mangleTypeString(argumentTypes);
  char *retVal = format("%s%zu%s%s", mangledModuleName, strlen(id), id,
                        mangledArgumentTypes);
  free(mangledModuleName);
  free(mangledArgumentTypes);
  return retVal;
}

// constant stuff
static bool constantIsZero(Node *initializer) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          return initializer->data.constExp.value.ubyteVal == 0;
        }
        case CT_BYTE: {
          return initializer->data.constExp.value.byteVal == 0;
        }
        case CT_CHAR: {
          return initializer->data.constExp.value.charVal == 0;
        }
        case CT_USHORT: {
          return initializer->data.constExp.value.ushortVal == 0;
        }
        case CT_SHORT: {
          return initializer->data.constExp.value.shortVal == 0;
        }
        case CT_UINT: {
          return initializer->data.constExp.value.uintVal == 0;
        }
        case CT_INT: {
          return initializer->data.constExp.value.intVal == 0;
        }
        case CT_WCHAR: {
          return initializer->data.constExp.value.wcharVal == 0;
        }
        case CT_ULONG: {
          return initializer->data.constExp.value.ulongVal == 0;
        }
        case CT_LONG: {
          return initializer->data.constExp.value.longVal == 0;
        }
        case CT_FLOAT: {
          return initializer->data.constExp.value.floatBits == 0;
        }
        case CT_DOUBLE: {
          return initializer->data.constExp.value.doubleBits == 0;
        }
        case CT_BOOL: {
          return initializer->data.constExp.value.boolVal == false;
        }
        case CT_STRING:
        case CT_WSTRING: {
          return false;
        }
        case CT_NULL: {
          return true;
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        if (!constantIsZero(elements->elements[idx])) {
          return false;
        }
      }
      return true;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}
static void constantToData(Node *initializer, IRVector *out,
                           FragmentVector *fragments,
                           LabelGenerator *labelGenerator) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          IR(out, CONST(BYTE_WIDTH,
                        UBYTE(initializer->data.constExp.value.ubyteVal)));
          return;
        }
        case CT_BYTE: {
          IR(out,
             CONST(BYTE_WIDTH, BYTE(initializer->data.constExp.value.byteVal)));
          return;
        }
        case CT_CHAR: {
          IR(out, CONST(CHAR_WIDTH,
                        UBYTE(initializer->data.constExp.value.charVal)));
          return;
        }
        case CT_USHORT: {
          IR(out, CONST(SHORT_WIDTH,
                        USHORT(initializer->data.constExp.value.ushortVal)));
          return;
        }
        case CT_SHORT: {
          IR(out, CONST(SHORT_WIDTH,
                        SHORT(initializer->data.constExp.value.shortVal)));
          return;
        }
        case CT_UINT: {
          IR(out,
             CONST(INT_WIDTH, UINT(initializer->data.constExp.value.uintVal)));
          return;
        }
        case CT_INT: {
          IR(out,
             CONST(INT_WIDTH, INT(initializer->data.constExp.value.intVal)));
          return;
        }
        case CT_WCHAR: {
          IR(out, CONST(WCHAR_WIDTH,
                        UINT(initializer->data.constExp.value.wcharVal)));
          return;
        }
        case CT_ULONG: {
          IR(out, CONST(LONG_WIDTH,
                        ULONG(initializer->data.constExp.value.ulongVal)));
          return;
        }
        case CT_LONG: {
          IR(out,
             CONST(LONG_WIDTH, LONG(initializer->data.constExp.value.longVal)));
          return;
        }
        case CT_FLOAT: {
          IR(out, CONST(FLOAT_WIDTH,
                        FLOAT(initializer->data.constExp.value.floatBits)));
          return;
        }
        case CT_DOUBLE: {
          IR(out, CONST(DOUBLE_WIDTH,
                        DOUBLE(initializer->data.constExp.value.doubleBits)));
          return;
        }
        case CT_BOOL: {
          IR(out,
             CONST(BYTE_WIDTH,
                   UBYTE(initializer->data.constExp.value.boolVal ? 1 : 0)));
          return;
        }
        case CT_STRING: {
          Fragment *f = rodataFragmentCreate(
              labelGenerator->vtable->generateDataLabel(labelGenerator));
          IR(f->data.rodata.ir,
             CONST(0, STRING(tstrdup(
                          initializer->data.constExp.value.stringVal))));
          fragmentVectorInsert(fragments, f);
          IR(out, CONST(POINTER_WIDTH, LABEL(f->label)));
          return;
        }
        case CT_WSTRING: {
          Fragment *f = rodataFragmentCreate(
              labelGenerator->vtable->generateDataLabel(labelGenerator));
          IR(f->data.rodata.ir,
             CONST(0, WSTRING(twstrdup(
                          initializer->data.constExp.value.wstringVal))));
          fragmentVectorInsert(fragments, f);
          IR(out, CONST(POINTER_WIDTH, LABEL(f->label)));
          return;
        }
        case CT_NULL: {
          IR(out, CONST(POINTER_WIDTH, ULONG(0)));
          return;
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        constantToData(elements->elements[idx], out, fragments, labelGenerator);
      }
      return;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}

// global accesses
static void addGlobalAccesses(SymbolTable *stab, char const *moduleName,
                              GlobalAccessCtor globalAccessCtor,
                              FunctionAccessCtor functionAccessCtor) {
  for (size_t idx = 0; idx < stab->capacity; idx++) {
    if (stab->keys[idx] != NULL) {
      SymbolInfo *info = stab->values[idx];
      if (info->kind == SK_FUNCTION) {
        OverloadSet *set = &info->data.function.overloadSet;
        for (size_t overloadIdx = 0; overloadIdx < set->size; overloadIdx++) {
          OverloadSetElement *elm = set->elements[overloadIdx];
          elm->access = functionAccessCtor(mangleFunctionName(
              moduleName, stab->keys[idx], &elm->argumentTypes));
        }
      } else if (info->kind == SK_VAR) {
        info->data.var.access = globalAccessCtor(
            typeSizeof(info->data.var.type), typeAlignof(info->data.var.type),
            typeKindof(info->data.var.type),
            mangleVarName(moduleName, stab->keys[idx]));
      }
    }
  }
}

// translation
static void translateGlobalVar(Node *varDecl, FragmentVector *fragments,
                               char const *moduleName,
                               LabelGenerator *labelGenerator) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *id = idValuePairs->firstElements[idx];
    Node *initializer = idValuePairs->secondElements[idx];
    Access *access = id->data.id.symbol->data.var.access;
    char *mangledName =
        access->vtable->getLabel(id->data.id.symbol->data.var.access);
    Fragment *f;
    if (initializer == NULL || constantIsZero(initializer)) {
      f = bssFragmentCreate(mangledName,
                            typeSizeof(id->data.id.symbol->data.var.type));
    } else if (id->data.id.symbol->data.var.type->kind == K_CONST) {
      f = rodataFragmentCreate(mangledName);
      constantToData(initializer, f->data.rodata.ir, fragments, labelGenerator);
    } else {
      f = dataFragmentCreate(mangledName);
      constantToData(initializer, f->data.data.ir, fragments, labelGenerator);
    }

    fragmentVectorInsert(fragments, f);
  }

  return;
}
static void translateFunction(Node *function, FragmentVector *fragments,
                              char const *moduleName, FrameCtor frameCtor,
                              LabelGenerator *labelGenerator) {
  // Frame *frame = frameCtor();
  // Access *functionAccess =
  // function->data.function.id->data.id.overload->access; char *mangledName =
  // functionAccess->vtable->getLabel(functionAccess); Fragment *fragment =
  // functionFragmentCreate(mangledName, frame); IRStmVector *body =
  // irStmVectorCreate();

  // Node *statements = function->data.function.body;
  // Access *outArg = frame->vtable->allocOutArg(
  //     frame,
  //     typeSizeof(function->data.function.id->data.id.overload->returnType));
  // for (size_t idx = 0; idx < function->data.function.formals->size; idx++)
  // {
  //   Node *id = function->data.function.formals->secondElements[idx];
  //   SymbolInfo *info = id->data.id.symbol;
  //   // info->data.var.access = frame->vtable->allocInArg(
  //   //     frame, typeSizeof(info->data.var.type), info->data.var.escapes);
  // }

  // char *exitLabel =
  // labelGenerator->vtable->generateCodeLabel(labelGenerator);
  // translateStmt(statements, body, frame, outArg, NULL, NULL, exitLabel,
  //               labelGenerator, tempGenerator,
  //               function->data.function.id->data.id.overload->returnType);

  // outArg->vtable->dtor(outArg);

  // body = frame->vtable->generateEntryExit(frame, body, exitLabel);
  // irStmVectorUninit(&fragment->data.function.body);
  // memcpy(&fragment->data.function.body, body, sizeof(IRStmVector));

  // fragmentVectorInsert(fragments, fragment);
  // return;
}
static void translateBody(Node *body, FragmentVector *fragments,
                          char const *moduleName, FrameCtor frameCtor,
                          LabelGenerator *labelGenerator) {
  switch (body->type) {
    case NT_VARDECL: {
      translateGlobalVar(body, fragments, moduleName, labelGenerator);
      return;
    }
    case NT_FUNCTION: {
      translateFunction(body, fragments, moduleName, frameCtor, labelGenerator);
      return;
    }
    default: { return; }
  }
}
static FragmentVector *translateFile(Node *file, FrameCtor frameCtor,
                                     LabelGenerator *labelGenerator) {
  FragmentVector *fragments = fragmentVectorCreate();

  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    translateBody(bodies->elements[idx], fragments,
                  file->data.module.id->data.id.id, frameCtor, labelGenerator);
  }

  return fragments;
}

void translate(FileFragmentVectorMap *fragmentMap, ModuleAstMapPair *asts,
               LabelGeneratorCtor labelGeneratorCtor, FrameCtor frameCtor,
               GlobalAccessCtor globalAccessCtor,
               FunctionAccessCtor functionAccessCtor) {
  fileFragmentVectorMapInit(fragmentMap);

  for (size_t idx = 0; idx < asts->decls.capacity; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      Node *file = asts->decls.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor, functionAccessCtor);
    }
  }
  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor, functionAccessCtor);
    }
  }

  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      fileFragmentVectorMapPut(
          fragmentMap, codeFilenameToAssembyFilename(file->data.file.filename),
          translateFile(file, frameCtor, labelGeneratorCtor()));
    }
  }
}