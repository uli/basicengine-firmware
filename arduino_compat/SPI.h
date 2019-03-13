#ifndef _SPI_H
#define _SPI_H

#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>

const uint8_t SPI_MODE0 = 0x00; ///<  CPOL: 0  CPHA: 0
const uint8_t SPI_MODE1 = 0x01; ///<  CPOL: 0  CPHA: 1
const uint8_t SPI_MODE2 = 0x10; ///<  CPOL: 1  CPHA: 0
const uint8_t SPI_MODE3 = 0x11; ///<  CPOL: 1  CPHA: 1

class SPIClass {
public:
  SPIClass() {
    SPI1CLK = 38;
  }
  void begin() {
  }
  void write(uint8_t data);
  void setFrequency(uint32_t freq) {
    SPI1CLK = freq / 1000000;
  }
  bool pins(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) {
    return true;
  }
  void setDataMode(uint8_t dataMode) {
  }
  uint32_t SPI1CLK;
};

extern SPIClass SPI;

#define SPI1CLK SPI.SPI1CLK

#endif
