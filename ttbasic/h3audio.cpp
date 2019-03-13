#ifdef H3

#include "h3audio.h"

AudioOutput audio;

int AudioOutput::m_curr_buf_pos;
uint8_t *AudioOutput::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

uint8_t AudioOutput::m_sound_buf[2][SOUND_BUFLEN];
int AudioOutput::m_block_size;

void AudioOutput::timerInterrupt(AudioOutput *audioOutput)
{
}  

void AudioOutput::init(int sample_rate)//AudioSystem &audioSystem)
{
  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;
}

#endif	// H3
