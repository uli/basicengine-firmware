// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "c_api.h"
#include "c_io.h"

#ifdef ESP8266
uint16_t pcf_state = 0xffff;
#endif
#ifdef H3
#include <ports.h>
#endif

int c_gpio_set_pin(int portno, int pinno, int data) {
#ifdef ESP8266
  if (check_param(pinno, 0, 15))
    return -1;
  if (check_param(data, 0, 1))
    return -1;

  pcf_state = (pcf_state & ~(1 << pinno)) | (data << pinno);

  // SDA is multiplexed with MVBLK0, so we wait for block move to finish
  // to avoid interference.
  while (!blockFinished()) {}

  // XXX: frequency is higher when running at 160 MHz because F_CPU is wrong
  Wire.beginTransmission(0x20);
  Wire.write(pcf_state & 0xff);
  Wire.write(pcf_state >> 8);

#ifdef DEBUG_GPIO
  int ret =
#endif
  Wire.endTransmission();
#ifdef DEBUG_GPIO
  Serial.printf("wire st %d pcf 0x%x\n", ret, pcf_state);
#endif
  return 0;

#elif defined(H3)

  if (check_param(portno, 0, 7) ||
      check_param(pinno, 0, 31) ||
      check_param(data, 0, 1))
    return -1;

  set_pin_data(PIO_PORT(portno), pinno, data);

  return 0;
#else
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}

int c_gpio_get_pin(int portno, int pinno) {
#ifdef ESP8266
  if (check_param(pinno, 0, 15)) return -1;
  // SDA is multiplexed with MVBLK0, so we wait for block move to finish
  // to avoid interference.
  while (!blockFinished()) {}

  if (Wire.requestFrom(0x20, 2) != 2) {
    err = ERR_IO;
    return 0;
  }
  uint16_t state = Wire.read();
  state |= Wire.read() << 8;
  return !!(state & (1 << a));
#elif defined(H3)
  if (check_param(portno, 0, 7) ||
      check_param(pinno, 0, 31))
    return -1;
  return get_pin_data(PIO_PORT(portno), pinno);
#else
  err = E_NOT_SUPPORTED;
  return -1;
#endif
}

int c_gpio_set_pin_mode(int portno, int pinno, int mode) {
#ifdef H3
  if (check_param(portno, 0, 7) ||
      check_param(pinno, 0, 31) ||
      check_param(mode, 0, 7))
    return -1;
  set_pin_mode(PIO_PORT(portno), pinno, mode);
  return 0;
#else
  err = E_NOT_SUPPORTED;
  return -1;
#endif
}
