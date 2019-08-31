#ifndef _DOSAUDIO_H
#define _DOSAUDIO_H

#define SOUND_BUFLEN 2048

#include <TPS2.h>

class DOSAudio
{
public:
  void begin();
  
  void init(int sample_rate);

  inline void setBlockSize(int size) {
    m_block_size = size;
  }
  inline int getBlockSize() {
    return m_block_size;
  }

  inline void queueSample(uint8_t sample) {
    m_curr_buf[m_curr_buf_pos++] = sample;
  }
  inline void setSampleAt(int buf, int idx, uint8_t sample) {
    m_sound_buf[buf][idx] = sample;
  }

  inline int currBufPos() {
    return m_curr_buf_pos;
  }

  inline void clearBufs() {
    memset(m_sound_buf, 0, sizeof(m_sound_buf));
  }

//  friend void ::hook_audio_get_sample(int16_t *l, int16_t *r);

  inline void pumpEvents() {
    if (m_backend == AU_PCI)
    	doFillPciBuffer();
  }

private:
  static void timerInterrupt(DOSAudio *audioOutput);

  static int m_curr_buf_pos;
  static uint8_t *m_curr_buf;

  static int m_block_size;
  static uint8_t m_sound_buf[2][SOUND_BUFLEN];

  void doFillPciBuffer();

  enum {
    AU_NONE,
    AU_PCI,
    AU_ALLEGRO,
  } m_backend;
};

extern DOSAudio audio;

#endif	// _DOSAUDIO_H
