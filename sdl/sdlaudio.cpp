#include "sdlaudio.h"

#include <SDL/SDL.h>

SDLAudio audio;

int SDLAudio::m_curr_buf_pos;
uint8_t *SDLAudio::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

uint8_t SDLAudio::m_sound_buf[2][SOUND_BUFLEN];
int SDLAudio::m_block_size;

void SDLAudio::init(int sample_rate)
{
  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;
}

void SDLAudio::begin()
{
}
