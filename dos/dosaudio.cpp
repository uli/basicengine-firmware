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
