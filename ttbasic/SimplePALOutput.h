#pragma once
#include <Arduino.h>
#include "driver/i2s.h"
#include "colorspace.h"

const int lineSamples = 854;
const int memSamples = 856;
const int syncSamples = 64;
const int burstSamples = 38;

const int burstStart = 70;
const int imageSamples = 640;

const int syncLevel = 0;
const int blankLevel = 23;
const int burstAmp = 8;//12;
const int maxLevel = 54;
const int maxUVLevel = 27;//54;
const float burstPerSample = (2 * M_PI) / (13333333 / 4433618.75);
const float colorFactor = (M_PI * 2) / 16;
const float burstPhase = M_PI / 4 * 3;

#include "RGB2YUV.h"

#define SPO_NUM_MODES	13

class SimplePALOutput
{
  public:    
  unsigned short longSync[memSamples];
  unsigned short shortSync[memSamples];
  unsigned short line[2][memSamples];
  unsigned short blank[memSamples];
  short SIN[imageSamples];
  short COS[imageSamples];

  short YLUT[16];
  short UVLUT[16];
  uint16_t yuv2u[256];
  uint16_t yuv2v[256];
  short yuv2y[256];

  static const i2s_port_t I2S_PORT = (i2s_port_t)I2S_NUM_0;
    
  void setMode(const struct video_mode_t &mode);
  void init();

  void sendLine(unsigned short *l);
  void sendSync1(int blank_lines);
  void sendSync2(int blank_lines);
  void sendFrame(const struct video_mode_t *mode, uint8_t **frame);
  void sendFrame1ppc(const struct video_mode_t *mode, uint8_t **frame);
  void sendFrame4ppc(const struct video_mode_t *mode, uint8_t **frame);
};
