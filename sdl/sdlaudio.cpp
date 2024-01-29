// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "sdlaudio.h"

#include <SDL.h>
#include <sound.h>

SDLAudio audio;

int SDLAudio::m_curr_buf_pos;
sample_t *SDLAudio::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

sample_t SDLAudio::m_sound_buf[2][SOUND_BUFLEN * SOUND_CHANNELS];
int SDLAudio::m_block_size;

void SDLAudio::fillAudioBuffer(void *userdata, Uint8 *stream, int len) {
  static int off = 0;
  sample_t *str = (sample_t *)stream;

  for (unsigned int i = 0; i < len / sizeof(sample_t); ++i) {
    if ((i + off) % (m_block_size * SOUND_CHANNELS) == 0) {
      m_curr_buf_pos = 0;
      sound.render();
    }
    str[i] = m_curr_buf[(i + off) % (m_block_size * SOUND_CHANNELS)];
  }
  if (len / (int)sizeof(sample_t) == m_block_size * SOUND_CHANNELS)
    off = 0;
  else
    off = (off + len / sizeof(sample_t)) % (m_block_size * SOUND_CHANNELS);
}

void SDLAudio::init(int sample_rate) {
  static bool inited = false;

  if (inited) {
    SDL_LockAudio();
    SDL_CloseAudio();
    SDL_UnlockAudio();
  }

  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;

  SDL_AudioSpec desired;
  desired.freq = sample_rate;
  desired.format = AUDIO_S16;
  desired.channels = SOUND_CHANNELS;
  desired.samples = SOUND_BUFLEN * SOUND_CHANNELS;
  desired.callback = fillAudioBuffer;

  if (SDL_OpenAudio(&desired, NULL) < 0)
    fprintf(stderr, "Couldn't open audio: %s", SDL_GetError());
  else {
    inited = true;
    SDL_PauseAudio(0);
  }
}

void SDLAudio::begin() {
}
