#include "sdlaudio.h"

#include <SDL/SDL.h>

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
    stream[i] = m_curr_buf[(i + off) % m_block_size];
  }
  off = (off + len) % m_block_size;
  m_curr_buf_pos = 0;
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
  desired.samples = SOUND_BUFLEN * 2;
  desired.callback = fillAudioBuffer;

  SDL_AudioSpec obtained;
  if (SDL_OpenAudio(&desired, &obtained) < 0) {
    fprintf(stderr, "Couldn't open audio: %s", SDL_GetError());
    exit(1);
  }
  inited = true;
  SDL_PauseAudio(0);
}

void SDLAudio::begin()
{
}
