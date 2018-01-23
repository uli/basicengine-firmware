#include "ttconfig.h"
#include <Arduino.h>
#include "sound.h"
#include "SID.h"
#include "mml.h"

SID sid;
MML mml;
MML_OPTION mml_opt;
uint32_t next_event;

uint32_t sid_off[3];

const uint16_t sidfreq[] PROGMEM = {
0x0115 /* C-0 */, 0x0125 /* C#0 */, 0x0137 /* D-0 */, 0x0149 /* D#0 */, 
0x015D /* E-0 */, 0x0172 /* F-0 */, 0x0188 /* F#0 */, 0x019F /* G-0 */, 
0x01B8 /* G#0 */, 0x01D2 /* A-0 */, 0x01EE /* A#0 */, 0x020B /* H-0 */, 

0x022A /* C-1 */, 0x024B /* C#1 */, 0x026E /* D-1 */, 0x0293 /* D#1 */, 
0x02BA /* E-1 */, 0x02E3 /* F-1 */, 0x030F /* F#1 */, 0x033E /* G-1 */, 
0x036F /* G#1 */, 0x03A4 /* A-1 */, 0x03DB /* A#1 */, 0x0416 /* H-1 */, 

0x0454 /* C-2 */, 0x0496 /* C#2 */, 0x04DC /* D-2 */, 0x0526 /* D#2 */, 
0x0574 /* E-2 */, 0x05C7 /* F-2 */, 0x061F /* F#2 */, 0x067C /* G-2 */, 
0x06DF /* G#2 */, 0x0747 /* A-2 */, 0x07B6 /* A#2 */, 0x082C /* H-2 */, 

0x08A8 /* C-3 */, 0x092C /* C#3 */, 0x09B7 /* D-3 */, 0x0A4B /* D#3 */, 
0x0AE8 /* E-3 */, 0x0B8E /* F-3 */, 0x0C3E /* F#3 */, 0x0CF8 /* G-3 */, 
0x0DBE /* G#3 */, 0x0E8F /* A-3 */, 0x0F6C /* A#3 */, 0x1057 /* H-3 */,

0x1150 /* C-4 */, 0x1258 /* C#4 */, 0x136F /* D-4 */, 0x1496 /* D#4 */,
0x15D0 /* E-4 */, 0x171C /* F-4 */, 0x187C /* F#4 */, 0x19F0 /* G-4 */,
0x1B7B /* G#4 */, 0x1D1E /* A-4 */, 0x1ED9 /* A#4 */, 0x20AE /* H-4 */,

0x22A0 /* C-5 */, 0x24AF /* C#5 */, 0x26DD /* D-5 */, 0x292D /* D#5 */,
0x2BA0 /* E-5 */, 0x2E38 /* F-5 */, 0x30F8 /* F#5 */, 0x33E1 /* G-5 */,
0x36F6 /* G#5 */, 0x3A3B /* A-5 */, 0x3DB2 /* A#5 */, 0x415C /* H-5 */,

0x4540 /* C-6 */, 0x495E /* C#6 */, 0x4DBB /* D-6 */, 0x525A /* D#6 */,
0x573F /* E-6 */, 0x5C6F /* F-6 */, 0x61EF /* F#6 */, 0x67C2 /* G-6 */,
0x6DED /* G#6 */, 0x7476 /* A-6 */, 0x7B63 /* A#6 */, 0x82B9 /* H-6 */,

0x8A7F /* C-7 */, 0x92BC /* C#7 */, 0x9B75 /* D-7 */, 0xA4B4 /* D#7 */,
0xAE7F /* E-7 */, 0xB8DF /* F-7 */, 0xC3DE /* F#7 */, 0xCF84 /* G-7 */,
0xDBD9 /* G#7 */, 0xE8ED /* A-7 */, 0xF6C6 /* A#7 */,
};

static void ICACHE_RAM_ATTR mml_callback(MML_INFO *p, void *extobj)
{
  uint32_t now = millis();
  next_event = now;
  switch (p->type) {
    case MML_TYPE_NOTE:
      {
        MML_ARGS_NOTE *args = &(p->args.note);
        sid_off[0] = now + args->ticks/2;
        next_event += args->ticks;
        if ((unsigned int)args->number < sizeof(sidfreq)/sizeof(*sidfreq)) {
          int freq = pgm_read_word(&sidfreq[args->number]);
          sid.set_register(0, freq & 0xff);
          sid.set_register(1, freq >> 8);
          sid.set_register(4, 0x11);
        }
      }
      break;
    case MML_TYPE_REST:
      {
        MML_ARGS_REST *args = &(p->args.rest);
        next_event += args->ticks;
      }
      break;
  }
}

void sound_init(void)
{
  sid.begin();
  mml_init(&mml, mml_callback, 0);
  MML_OPTION_INITIALIZER_DEFAULT(&mml_opt);
}

void sound_defaults(void)
{
  for (int ch = 0; ch < 3; ++ch) {
    int ch_off = ch * 7;
    sid.set_register(ch_off + 0, 0);
    sid.set_register(ch_off + 1, 0);
    sid.set_register(ch_off + 4, 0x11);
    sid.set_register(ch_off + 5, 0x33);
    sid.set_register(ch_off + 6, 0xd5);
    sid_off[ch] = 0;
  }
  sid.set_register(0x18, 0xf);
  next_event = 0;
}

void sound_play_mml(const char *data)
{
  mml_setup(&mml, &mml_opt, (char *)data);
  sound_defaults();
  next_event = millis();
}

void sound_stop_mml(void)
{
  next_event = 0;
}

void ICACHE_RAM_ATTR sound_pump_events(void)
{
  uint32_t now = millis();
  if (next_event && now >= next_event) {
    if (mml_fetch(&mml) != MML_RESULT_OK)
      next_event = 0;
  }
  for (int i = 0; i < 3; ++i) {
    if (sid_off[i] && sid_off[i] <= now) {
      sid_off[i] = 0;
      sid.set_register(i*7+4, 0x10);
    }
  }
}

// Array with 32-bit values which have one bit more set to '1' in every
// consecutive array index value
// Taken from Espressif MP3 decoder demo.
const uint32_t ICACHE_RODATA_ATTR fakePwm[]={
        0x00000010, 0x00000410, 0x00400410, 0x00400C10, 0x00500C10,
        0x00D00C10, 0x20D00C10, 0x21D00C10, 0x21D80C10, 0xA1D80C10,
        0xA1D80D10, 0xA1D80D30, 0xA1DC0D30, 0xA1DC8D30, 0xB1DC8D30,
        0xB9DC8D30, 0xB9FC8D30, 0xBDFC8D30, 0xBDFE8D30, 0xBDFE8D32,
        0xBDFE8D33, 0xBDFECD33, 0xFDFECD33, 0xFDFECD73, 0xFDFEDD73,
        0xFFFEDD73, 0xFFFEDD7B, 0xFFFEFD7B, 0xFFFFFD7B, 0xFFFFFDFB,
        0xFFFFFFFB, 0xFFFFFFFF
};

extern "C" void ICACHE_RAM_ATTR fill_audio_buffer(uint32_t *buf, int samples)
{
  for (int i = 0; i < samples; ++i) {
    uint8_t s = sid.getSample();
    buf[i] = pgm_read_dword(&fakePwm[s >> 3]);
  }
}
