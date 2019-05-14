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

// implementation of files

#include "util/file.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

// File

size_t const F_BUFFER_SIZE = 4096;

File *fOpen(char const *fileName) {
  // try to open the file
  int fd = open(fileName, O_RDONLY | O_CLOEXEC);
  if (fd == -1) return NULL;

  // setup the data structure
  File *f = malloc(sizeof(File));
  f->buffer = malloc(F_BUFFER_SIZE);
  f->fd = fd;
  f->offset = 0;
  f->bufferMax = 0;
  f->eof = false;

  // try to read
  ssize_t nbytes = read(f->fd, f->buffer, F_BUFFER_SIZE);
  if (nbytes == -1) {
    // error - free and return error
    fClose(f);
    return NULL;
  } else if (nbytes == 0) {
    // check for eof
    f->eof = true;
  } else {
    // update size if no eof
    f->bufferMax += (size_t)nbytes;
  }

  return f;
}

#undef F_OK
char const F_OK = -1;
char const F_EOF = -2;
char const F_ERR = -3;
char fGet(File *f) {
  if (f->eof) {
    // check for eof
    return F_EOF;
  } else if (f->offset == f->bufferMax) {
    // need to read(2) more, may be at eof
    ssize_t nbytes = read(f->fd, f->buffer, F_BUFFER_SIZE);
    if (nbytes == -1) {
      // error - not automatically free'd
      return F_ERR;
    } else if (nbytes == 0) {
      f->eof = true;
      return F_EOF;
    } else {
      f->bufferMax += (size_t)nbytes;
      return f->buffer[f->offset++ % F_BUFFER_SIZE];
    }
  } else {
    // isn't at eof
    return f->buffer[f->offset++ % F_BUFFER_SIZE];
  }
}

int fUnget(File *f) {
  f->eof = false;  // can't be at eof when attempting to back up.
                   // if file was empty, then lseek(2) would error

  if (f->offset % F_BUFFER_SIZE != 0) {
    // current buffer isn't a fresh buffer
    f->offset--;
    return F_OK;
  } else {
    // read the previous buffer
    // find out how large the current one is
    size_t bufferSize = f->bufferMax - (f->bufferMax % F_BUFFER_SIZE);
    if (bufferSize == 0) bufferSize = F_BUFFER_SIZE;

    // back up
    lseek(f->fd, -(off_t)bufferSize, SEEK_CUR);
    f->bufferMax -= bufferSize;
    // read it again
    ssize_t nbytes = read(f->fd, f->buffer, F_BUFFER_SIZE);
    if (nbytes == -1 || nbytes == 0) {
      // failed, or inconsistent eof - error either way
      return F_ERR;
    } else {
      // reset the buffer size and back up
      f->bufferMax += (size_t)nbytes;
      f->offset--;
      return F_OK;
    }
  }
}

void fClose(File *f) {
  close(f->fd);
  free(f->buffer);
  free(f);
}