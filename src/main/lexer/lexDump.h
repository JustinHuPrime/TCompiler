// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Dumps the tokens for a file

#ifndef TLC_LEXER_LEXDUMP_H_
#define TLC_LEXER_LEXDUMP_H_

#include "util/errorReport.h"
#include "util/fileList.h"

// dumps the tokens from all files to stdout
void lexDump(Report *report, FileList *files);

#endif  // TLC_LEXER_LEXDUMP_H_