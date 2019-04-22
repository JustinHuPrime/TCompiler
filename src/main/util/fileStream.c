// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// implementation of file streams for a linux based system

#include "util/fileStream.h"

#include <fcntl.h>
#include <stdlib.h>

size_t const FS_BUFFER_SIZE = 4096;

FileStream *fsOpen(char const *fileName) {
  // try to open the file
  int fd = open(fileName, O_RDONLY | O_CLOEXEC);
  if (fd == -1) return NULL;

  // setup the data structure
  FileStream *fs = malloc(sizeof(FileStream));
  fs->buffer = malloc(FS_BUFFER_SIZE);
  fs->fd = fd;
  fs->offset = 0;
  fs->bufferMax = 0;
  fs->eof = false;

  // try to read
  ssize_t nbytes = read(fs->fd, fs->buffer, FS_BUFFER_SIZE);
  if (nbytes == -1) {
    // error - free and return error
    fsClose(fs);
    return NULL;
  } else if (nbytes == 0) {
    // check for eof
    fs->eof = true;
  } else {
    // update size if no eof
    fs->bufferMax += (size_t)nbytes;
  }

  return fs;
}

char const FS_OK = -1;
char const FS_EOF = -2;
char const FS_ERR = -3;
char fsGet(FileStream *fs) {
  if (fs->eof) {
    // check for eof
    return FS_EOF;
  } else if (fs->offset == fs->bufferMax) {
    // need to read(2) more, may be at eof
    ssize_t nbytes = read(fs->fd, fs->buffer, FS_BUFFER_SIZE);
    if (nbytes == -1) {
      // error - not automatically free'd
      return FS_ERR;
    } else if (nbytes == 0) {
      fs->eof = true;
      return FS_EOF;
    } else {
      fs->bufferMax += (size_t)nbytes;
      return fs->buffer[fs->offset++ % FS_BUFFER_SIZE];
    }
  } else {
    // isn't at eof
    return fs->buffer[fs->offset++ % FS_BUFFER_SIZE];
  }
}

int fsUnget(FileStream *fs) {
  fs->eof = false;  // can't be at eof when attempting to back up.
                    // if file was empty, then lseek(2) would error

  if (fs->offset % FS_BUFFER_SIZE != 0) {
    // current buffer isn't a fresh buffer
    fs->offset--;
    return FS_OK;
  } else {
    // read the previous buffer
    // find out how large the current one is
    size_t bufferSize = fs->bufferMax - (fs->bufferMax % FS_BUFFER_SIZE);
    if (bufferSize == 0) bufferSize = FS_BUFFER_SIZE;

    // back up
    lseek(fs->fd, -(off_t)bufferSize, SEEK_CUR);
    fs->bufferMax -= bufferSize;
    // read it again
    ssize_t nbytes = read(fs->fd, fs->buffer, FS_BUFFER_SIZE);
    if (nbytes == -1 || nbytes == 0) {
      // failed, or inconsistent eof - error either way
      return FS_ERR;
    } else {
      // reset the buffer size and back up
      fs->bufferMax += (size_t)nbytes;
      fs->offset--;
      return FS_OK;
    }
  }
}

void fsClose(FileStream *fs) {
  close(fs->fd);
  free(fs->buffer);
  free(fs);
}