// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "sdlaudio.h"

#include <SDL.h>
#include <config.h>
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

static const char *preferred_devices[] = {
  "HDMI",
  "codec",
  "analog",
};

void SDLAudio::init(int sample_rate) {
  static bool inited = false;

  if (inited) {
    SDL_LockAudio();
    SDL_CloseAudioDevice(m_audio_device_id);
    SDL_UnlockAudio();
  }

  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;

  const char *audio_dev = nullptr;

  if (CONFIG.audio_device == BString("default")) {
    if (strstr(SDL_GetCurrentAudioDriver(), "alsa")) {
      // SDL doesn't do a good-enough job on embedded systems to find a
      // viable output device, so we try to do it ourselves.

      for (auto pref : preferred_devices) {
        for (int i = 0; i < SDL_GetNumAudioDevices(0); ++i) {
          const char *dev = SDL_GetAudioDeviceName(i, 0);

          printf("outdev %d %s\n", i, dev);
          if (strcasestr(dev, pref)) {
            printf("matches %s\n", pref);
            audio_dev = dev;
            break;
          }
        }

        if (audio_dev)
          break;
      }

      if (!audio_dev) {
        audio_dev = SDL_GetAudioDeviceName(0, 0);
        printf("no match, using first device\n");
      }
    }
  } else {
    audio_dev = CONFIG.audio_device.c_str();
  }

  printf("chosen audio device: %s\n", audio_dev ? : "default");

  SDL_AudioSpec desired, obtained;

  desired.freq = sample_rate;
  desired.format = AUDIO_S16;
  desired.channels = SOUND_CHANNELS;
  desired.samples = SOUND_BUFLEN * SOUND_CHANNELS;
  desired.callback = fillAudioBuffer;

  if (!(m_audio_device_id = SDL_OpenAudioDevice(audio_dev, 0, &desired, &obtained, 0)))
    fprintf(stderr, "Couldn't open audio device %s: %s", audio_dev, SDL_GetError());
  else {
    inited = true;
    SDL_PauseAudioDevice(m_audio_device_id, 0);
  }
}

void SDLAudio::begin() {
}
