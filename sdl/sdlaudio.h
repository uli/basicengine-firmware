// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _SDLAUDIO_H
#define _SDLAUDIO_H

#include "../ttbasic/audio.h"
#include <SDL/SDL.h>

#define SOUND_BUFLEN 512
#define SOUND_CHANNELS 2

class SDLAudio {
public:
  void begin();

  void init(int sample_rate);

  inline void setBlockSize(int size) {
    m_block_size = size;
  }
  inline int getBlockSize() {
    return m_block_size;
  }

  inline void queueSample(sample_t sample) {
    m_curr_buf[m_curr_buf_pos++] = sample;
  }
  inline void setSampleAt(int buf, int idx, sample_t sample) {
    m_sound_buf[buf][idx] = sample;
  }

  inline int currBufPos() {
    return m_curr_buf_pos;
  }

  inline void clearBufs() {
    memset(m_sound_buf, 0, sizeof(m_sound_buf));
  }

  inline void pumpEvents() {
  }

private:
  static void fillAudioBuffer(void *userdata, Uint8 *samples, int len);

  static int m_curr_buf_pos;
  static sample_t *m_curr_buf;

  static int m_block_size;
  static sample_t m_sound_buf[2][SOUND_BUFLEN * SOUND_CHANNELS];
};

extern SDLAudio audio;

#endif  // _SDLAUDIO_H
