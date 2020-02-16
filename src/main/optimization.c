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

// Values of container optimization constants

#include "optimization.h"

// vectors of pointers start with 64 bytes allocated to reduce memory churn
size_t const PTR_VECTOR_INIT_CAPACITY = 8;
// vectors of bytes start with 16 bytes allocated to reduce memory churn
size_t const BYTE_VECTOR_INIT_CAPACITY = 16;
