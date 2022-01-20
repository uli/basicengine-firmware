// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_api.h"
#include "eb_io.h"

#ifdef ESP8266
#include <Wire.h>
uint16_t pcf_state = 0xffff;
#endif

#ifdef H3
#include <ports.h>
#include <h3_i2c.h>
#endif

#if !defined(__linux__)
int eb_gpio_set_pin(int portno, int pinno, int data) {
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

int eb_gpio_get_pin(int portno, int pinno) {
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
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}

int eb_gpio_set_pin_mode(int portno, int pinno, int mode) {
#ifdef H3
  if (check_param(portno, 0, 7) ||
      check_param(pinno, 0, 31) ||
      check_param(mode, 0, 7))
    return -1;
  set_pin_mode(PIO_PORT(portno), pinno, mode);
  return 0;
#else
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}
#endif // __linux__

#ifndef __linux__
int eb_i2c_write(unsigned char addr, const char *data, int count) {
#ifdef ESP8266
  // SDA is multiplexed with MVBLK0, so we wait for block move to finish
  // to avoid interference.
  while (!blockFinished()) {}

  // I2Cデータ送信
  Wire.beginTransmission(i2cAdr);
  if (out.length()) {
    for (uint32_t i = 0; i < out.length(); i++)
      Wire.write(out[i]);
  }
  return Wire.endTransmission();
#elif defined(H3)
  h3_i2c_set_slave_address(addr);
  return h3_i2c_write(data, count);
#else
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}

int eb_i2c_read(unsigned char addr, char *data, int count) {
  BString in;

#ifdef ESP8266
  // SDA is multiplexed with MVBLK0, so we wait for block move to finish
  // to avoid interference.
  while (!blockFinished()) {}

  // I2Cデータ送受信
  Wire.beginTransmission(i2cAdr);
  Wire.requestFrom(i2cAdr, rdlen);
  int idx = 0;
  while (Wire.available()) {
    data[idx++] = Wire.read();
  }

  return Wire.endTransmission();
#elif defined(H3)
  h3_i2c_set_slave_address(addr);
  return h3_i2c_read(data, count);
#else
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}

#ifdef H3
#include <h3_spi.h>
#endif

void eb_spi_write(const char *out_data, unsigned int count) {
#ifdef H3
  h3_spi_writenb(out_data, count);
#endif
}

void eb_spi_transfer(const char *out_data, char *in_data, unsigned int count) {
#ifdef H3
  h3_spi_transfernb((char *)out_data, in_data, count);
#endif
}

void eb_spi_set_bit_order(int bit_order) {
#ifdef H3
  h3_spi_setBitOrder((h3_spi_bit_order_t)bit_order);
#endif
}

void eb_spi_set_freq(int freq) {
#ifdef H3
  h3_spi_set_speed_hz(freq);
#endif
}

void eb_spi_set_mode(int mode) {
#ifdef H3
  h3_spi_setDataMode(mode);
#endif
}

int eb_i2c_select_bus(unsigned char bus) {
  return 0;
}

#endif	// __linux__
