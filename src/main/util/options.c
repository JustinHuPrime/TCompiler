// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implemntation of command line options manager

#include "util/options.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Options *optionsParseCmdlineArgs(int argc, char *argv[]) {
  Options *options = malloc(sizeof(Options));
  options->flagSize = 0;
  options->flagCapacity = 1;
  options->flags = malloc(sizeof(char const *));
  options->optionSize = 0;
  options->optionCapacity = 1;
  options->keys = malloc(sizeof(char const *));
  options->values = malloc(sizeof(char const *));
  options->argSize = 0;
  options->argCapacity = 1;
  options->args = malloc(sizeof(char const *));

  // TODO: actually parse the args
  return options;
}

static size_t optionsLookupExpectedIndex(size_t size, char const **list,
                                         char const *target) {
  if (size == 0) return 0;  // edge case
  size_t high = size - 1;   // hightest possible index that might match
  size_t low = 0;           // lowest possible index that might match

  while (high >= low) {
    size_t half = low + (high - low) / 2;
    int cmpVal = strcmp(list[half], target);
    if (cmpVal < 0) {  // entries[half] is before name
      low = half + 1;
      if (low >= size) {  // should be after the end
        return size;
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
  int cmpVal = strcmp(list[high], target);
  if (cmpVal < 0) {  // entries[high] is before name
    return high + 1;
  } else {  // entries[high] is after name or equal to name
    return high;
  }
}
static size_t optionsLookupIndex(size_t size, char const **list,
                                 char const *target) {
  if (size == 0) return SIZE_MAX;  // edge case

  size_t expectedIndex = optionsLookupExpectedIndex(size, list, target);
  if (expectedIndex >= size) return SIZE_MAX;

  return strcmp(list[expectedIndex], target) == 0 ? expectedIndex : SIZE_MAX;
}
bool optionsHasFlag(Options *options, char const *name) {
  return optionsLookupIndex(options->flagSize, options->flags, name) !=
         SIZE_MAX;
}
char const *optionsGetValue(Options *options, char const *name) {
  size_t index = optionsLookupIndex(options->optionSize, options->keys, name);
  if (index != SIZE_MAX) {
    return options->values[index];
  } else {
    return NULL;
  }
}

void optionsDestroy(Options *options) {
  free(options->flags);
  free(options->keys);
  free(options->keys);
  free(options->values);
  free(options);
}