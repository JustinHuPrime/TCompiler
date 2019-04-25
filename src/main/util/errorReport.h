// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Error message handler

#ifndef TLC_UTIL_ERRORREPORT_H_
#define TLC_UTIL_ERRORREPORT_H_

#include "util/vector.h"

#include <stddef.h>

typedef struct {
  Vector *messages;
  size_t errors;
  size_t warnings;
} Report;

// constructor
Report *reportCreate(void);

// adds a message
void reportMessage(Report *, char *message);
// adds an error
void reportError(Report *, char *message);
// adds a warning
void reportWarning(Report *, char *message);

// displays all errors and warnings to stderr
void reportDisplay(Report *);

extern int const RPT_OK;
extern int const RPT_ERR;
extern int const RPT_WARN;
// returns: RPT_OK if no errors and no warnings,
//          RPT_ERR if any errors exist,
//          RPT_WARN if any warnings exist.
int reportState(Report *);

// destructor
void reportDestroy(Report *);

#endif  // TLC_UTIL_ERRORREPORT_H_