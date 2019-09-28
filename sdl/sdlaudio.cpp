// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "sdlaudio.h"

#include <SDL/SDL.h>
#include <sound.h>

SDLAudio audio;

int SDLAudio::m_curr_buf_pos;
uint8_t *SDLAudio::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

uint8_t SDLAudio::m_sound_buf[2][SOUND_BUFLEN];
int SDLAudio::m_block_size;

void SDLAudio::fillAudioBuffer(void *userdata, Uint8 *stream, int len)
{
  static int off = 0;

  for (int i = 0; i < len; ++i) {
    if ((i + off) % m_block_size == 0) {
      m_curr_buf_pos = 0;
      sound.render();
    }
    stream[i] = m_curr_buf[(i + off) % m_block_size];
  }
  if (len == m_block_size)
    off = 0;
  else
    off = (off + len) % m_block_size;
}

void SDLAudio::init(int sample_rate)
{
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
  desired.format = AUDIO_U8;
  desired.channels = 1;
  desired.samples = SOUND_BUFLEN;
  desired.callback = fillAudioBuffer;

  if (SDL_OpenAudio(&desired, NULL) < 0)
    fprintf(stderr, "Couldn't open audio: %s", SDL_GetError());
  else {
    inited = true;
    SDL_PauseAudio(0);
  }
}

void SDLAudio::begin()
{
}
