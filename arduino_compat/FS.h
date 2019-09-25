// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifndef __FS_H
#define __FS_H

#include "WString.h"

namespace fs {

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File {
public:
    void close() {
    }
    size_t write(uint8_t) {
      return 0;
    }
    size_t write(const uint8_t *buf, size_t size) {
      return 0;
    }
    int read() {
      return -1;
    }
    size_t read(uint8_t* buf, size_t size) {
      return 0;
    }
    size_t position() const {
      return 0;
    }
    int available() {
      return -1;
    }
    int peek() {
      return -1;
    }
    bool seek(uint32_t pos, SeekMode mode) {
      return false;
    }
    bool seek(uint32_t pos) {
        return seek(pos, SeekSet);
    }
    size_t size() const {
      return 0;
    }
    operator bool() const {
      return false;
    }
};

class Dir {
public:
    bool next() {
      return false;
    }
    String fileName() {
      return String("");
    }
    size_t fileSize() {
      return 0;
    }
};

class FS {
public:
    bool begin(bool format = true) {
      return true;
    }
    bool format() {
      return false;
    }
    File open(const char* path, const char* mode) {
      File tmpFile;
      return tmpFile;
    }
    bool rename(const char* pathFrom, const char* pathTo) {
      return false;
    }
    bool remove(const char* path) {
      return false;
    }
    bool exists(const char* path) {
      return false;
    }
    Dir openDir(const char* path) {
      Dir tmpDir;
      return tmpDir;
    }
};
};

extern fs::FS SPIFFS;

#endif
