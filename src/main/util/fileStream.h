// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// a file stream for a linux based system

#ifndef TLC_UTIL_FILESTREAM_H_
#define TLC_UTIL_FILESTREAM_H_

#include <stdbool.h>
#include <unistd.h>

extern size_t const FS_BUFFER_SIZE;

typedef struct {
  char *buffer;
  int fd;
  size_t offset;     // offset from start of file of next character to be read
  size_t bufferMax;  // first offset from start of file that is invalid
  bool eof;          // eof flag - set when a read fails.
} FileStream;

// opens a file. returns NULL if failed
FileStream *fileOpen(char const *fileName);

extern char const FS_OK;
extern char const FS_EOF;
extern char const FS_ERR;
// gets a character from the file
// returns FS_EOF on end of file.
//         FS_ERR on an error.
// FS_ERR and FS_EOF are guarenteed not to infringe on ASCII text.
// returning FS_ERR indicates the stream may be in an inconsistent state, and
// can only be safely closed.
char fileGet(FileStream *);
// backs up a character.
// returns FS_ERR if there is an error, i.e. backing up before start of file
// returning FS_ERR indicates the stream may be in an inconsistent state, and
// can only be safely closed.
int fileUnget(FileStream *);

// closes a file. silently fails if unsuccessful; no recovery is possible
void fileClose(FileStream *);

#endif  // TLC_UTIL_FILESTREAM_H_