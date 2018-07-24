#pragma once
#include <Arduino.h>
#include "driver/i2s.h"
#include "colorspace.h"

const int memSamples = 856;
const int imageSamples = 640;

#define SPO_NUM_MODES	13

class SimplePALOutput
{
  public:    
  unsigned short longSync[memSamples];
  unsigned short shortSync[memSamples];
  unsigned short line[2][memSamples];
  unsigned short blank[memSamples];
  static short SIN[imageSamples];
  static short COS[imageSamples];

  short YLUT[16];
  short UVLUT[16];
  uint16_t yuv2u[256];
  uint16_t yuv2v[256];
  short yuv2y[256];

  static const i2s_port_t I2S_PORT = (i2s_port_t)I2S_NUM_0;
    
  void setMode(const struct video_mode_t &mode);
  void setColorSpace(uint8_t palette);
  void init();

  void sendLine(unsigned short *l);
  void sendSync1(int blank_lines);
  void sendSync2(int blank_lines);
  void sendFrame(const struct video_mode_t *mode, uint8_t **frame);
  void sendFrame1ppc(const struct video_mode_t *mode, uint8_t **frame);
  void sendFrame4ppc(const struct video_mode_t *mode, uint8_t **frame);
  void sendFrame2pp5c(const struct video_mode_t *mode, uint8_t **frame);
};
