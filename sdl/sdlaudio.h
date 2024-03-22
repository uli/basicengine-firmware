// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _SDLAUDIO_H
#define _SDLAUDIO_H

#include "../ttbasic/audio.h"
#include <SDL.h>

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

  inline void pumpEvents() {
  }

  BString deviceAvailable(int idx) {
    return BString(SDL_GetAudioDeviceName(idx, 0));
  }

  BString deviceUsed() {
    return BString(SDL_GetAudioDeviceName(m_audio_device_idx, 0));
  }

private:
  static void fillAudioBuffer(void *userdata, Uint8 *samples, int len);

  static int m_curr_buf_pos;
  static sample_t *m_curr_buf;

  static int m_block_size;
  static sample_t m_sound_buf[2][SOUND_BUFLEN * SOUND_CHANNELS];

  int m_audio_device_idx;	// index to SDL_GetAudioDeviceName(), not SDL audio device ID!
};

extern SDLAudio audio;

#endif  // _SDLAUDIO_H
