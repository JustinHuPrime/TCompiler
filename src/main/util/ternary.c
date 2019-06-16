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

// implementation of three valued logic

#include "util/ternary.h"

#include <stdbool.h>

TernaryValue and (TernaryValue v1, TernaryValue v2) {
  switch (v1) {
    case YES:
      return v2;
    case NO:
      return NO;
    case INDETERMINATE: {
      switch (v2) {
        case YES:
          return INDETERMINATE;
        case NO:
          return NO;
        case INDETERMINATE:
          return INDETERMINATE;
      }
    }
  }
  return -1;  // not a ternary value; type safety violated
}
TernaryValue or (TernaryValue v1, TernaryValue v2) {
  switch (v1) {
    case YES:
      return YES;
    case NO:
      return v2;
    case INDETERMINATE: {
      switch (v2) {
        case YES:
          return YES;
        case NO:
          return INDETERMINATE;
        case INDETERMINATE:
          return INDETERMINATE;
      }
    }
  }
  return -1;  // not a ternary value; type safety violated
}
TernaryValue not(TernaryValue v) {
  switch (v) {
    case YES:
      return NO;
    case NO:
      return YES;
    case INDETERMINATE:
      return INDETERMINATE;
  }
  return -1;  // not a ternary value; type safety violated
}