// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Ulrich Hecht

class TwoWire {
public:
  static void begin(int sda, int scl) {
  }
  void beginTransmission(uint8_t) {
  }
  uint8_t endTransmission(void) {
    return 0;
  }
  virtual size_t write(uint8_t) {
    return 0;
  }
  virtual size_t write(const uint8_t *, size_t) {
    return 0;
  }
  uint8_t requestFrom(uint8_t adr, uint8_t len) {
    return 0;
  }
  virtual int available(void) {
    return 0;
  }
  virtual int read(void) {
    return -1;
  }
};

extern TwoWire Wire;
