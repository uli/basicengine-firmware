// SPDX-License-Identifier: MIT
// Copyright (c) 2017, 2018 Ulrich Hecht

#ifndef _LOCK_H
#define _LOCK_H

// Arduino SPI locking is fake, doesn't actually do anything! WTF?!?

// Don't want to do the locking manually for every SD card method call,
// so we allow more than one owner. The main purpose of this is to
// prevent asynchronous SPI accesses by interrupt handlers while we're
// in the middle of something else.

extern volatile int spi_lock;

static inline void SpiLock(void) {
  ++spi_lock;
}

static inline void SpiUnlock(void) {
  --spi_lock;
  if (spi_lock < 0) {
#ifdef DEBUG_LOCK
    Serial.println("WARN neg SPI refcnt");
#endif
    spi_lock = 0;
  }
}

static inline bool SpiLocked(void) {
  return spi_lock > 0;
}

#endif
