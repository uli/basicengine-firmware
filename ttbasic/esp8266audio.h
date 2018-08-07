#ifndef _ESP8266AUDIO_H
#define _ESP8266AUDIO_H

#include "ttconfig.h"
#include <Arduino.h>
extern "C" {
#include "nosdki2s.h"
};

extern const uint32_t fakePwm[];

#define SOUND_BUFLEN I2S_BUFLEN

class ESP8266Audio
{
public:
  void init(int sample_rate) {
    InitI2S(sample_rate);
    SendI2S();
  }

  inline void setBlockSize(int size) {
    nosdk_i2s_set_blocksize(size * 4);
  }
  
  inline void queueSample(uint8_t sample) {
    nosdk_i2s_curr_buf[nosdk_i2s_curr_buf_pos++] =
      pgm_read_dword(&fakePwm[sample >> 3]);
  }

  inline int currBufPos() {
    return nosdk_i2s_curr_buf_pos;
  }
  
  void setSampleAt(int buf, int idx, uint8_t sample) {
    i2sBufDesc[buf].buf_ptr[idx] =
      pgm_read_dword(&fakePwm[sample >> 3]);
  }

  void clearBufs() {
    memset((void *)i2sBufDesc[0].buf_ptr, 0xaa, SOUND_BUFLEN * 4);
    memset((void *)i2sBufDesc[1].buf_ptr, 0xaa, SOUND_BUFLEN * 4);
  }
};

extern ESP8266Audio audio;

#endif
