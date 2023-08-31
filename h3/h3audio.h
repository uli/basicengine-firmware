// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _H3AUDIO_H
#define _H3AUDIO_H

#define SOUND_BUFLEN 480
#define SOUND_CHANNELS 2

#define SOUND_BUFFERS 16

#include "../ttbasic/audio.h"
#include <string.h>

extern "C" void hook_audio_get_sample(int16_t *l, int16_t *r);

class H3Audio {
public:
  //  AudioSystem *audioSystem;

  void init(int sample_rate);

  inline void setBlockSize(int size) {
    m_block_size = size;
  }
  inline int getBlockSize() {
    return m_block_size;
  }

  inline sample_t *currBuf() {
    return m_curr_buf;
  }
  inline int currBufPos() {
    return m_curr_buf_pos;
  }

  inline bool isBufEmpty() {
    return m_curr_buf_pos == 0;
  }

  inline void setBufFull() {
    m_curr_buf_pos = m_block_size;
  }

  inline void clearBufs() {
    memset(m_sound_buf, 0, sizeof(m_sound_buf));
  }

  friend void ::hook_audio_get_sample(int16_t *l, int16_t *r);

private:
  static void timerInterrupt(H3Audio *audioOutput);

  static int m_curr_buf_pos;
  static sample_t *m_curr_buf;

  static int m_block_size;
  static sample_t m_sound_buf[SOUND_BUFFERS][SOUND_BUFLEN * SOUND_CHANNELS];
};

extern H3Audio audio;

#endif	// _H3AUDIO_H
