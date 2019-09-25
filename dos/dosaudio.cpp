// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "dosaudio.h"

#include <allegro.h>
extern "C" {
#include <sound.lib/au.h>
};

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
  
  if (m_backend == AU_PCI) {
    unsigned int rate = sample_rate;
    unsigned int bits = 16;
    unsigned int chans = 1;
    AU_setrate(&rate, &bits, &chans);
    AU_setmixer_all(100);
    AU_start();
  }
}

void DOSAudio::doFillPciBuffer()
{
  if (m_curr_buf_pos > 0) {
    if (AU_cardbuf_space() >= 0) {
      short foo[SOUND_BUFLEN];
      for (int i = 0; i < SOUND_BUFLEN; ++i) {
        foo[i] = rand();//m_curr_buf[i] * 257 - 32768;
      }
      AU_writedata((char *)foo, SOUND_BUFLEN, 0);
      m_curr_buf_pos = 0;
    }
  }
}

void DOSAudio::begin()
{
  char *pci_sound = AU_search(1);
  if (pci_sound) {
    puts(pci_sound);
    m_backend = AU_PCI;
  } else if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) == 0) {
    m_backend = AU_ALLEGRO;
  } else {
    m_backend = AU_NONE;
  }
}
