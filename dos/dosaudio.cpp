#include "dosaudio.h"

DOSAudio audio;

int DOSAudio::m_curr_buf_pos;
uint8_t *DOSAudio::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

uint8_t DOSAudio::m_sound_buf[2][SOUND_BUFLEN];
int DOSAudio::m_block_size;

void DOSAudio::init(int sample_rate)
{
  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;
}
