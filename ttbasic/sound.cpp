#include "ttconfig.h"
#include <Arduino.h>
#include "sound.h"

#define TSF_NO_STDIO
#define TSF_IMPLEMENTATION
#define TSF_MEMCPY os_memcpy
#define TSF_MEMSET memset
#include "tsf.h"

MML BasicSound::m_mml[SOUND_CHANNELS];
MML_OPTION BasicSound::m_mml_opt[SOUND_CHANNELS];
uint32_t BasicSound::m_next_event[SOUND_CHANNELS];
bool BasicSound::m_finished[SOUND_CHANNELS];
uint32_t BasicSound::m_all_done_time;

uint32_t BasicSound::m_off_time[SOUND_CHANNELS];
uint8_t BasicSound::m_off_key[SOUND_CHANNELS];
uint8_t BasicSound::m_off_inst[SOUND_CHANNELS];
uint8_t BasicSound::m_ch_inst[SOUND_CHANNELS];

BString BasicSound::m_font_name;

void BasicSound::noteOn(int ch, int inst, int note, float vel, int ticks)
{
  if (!m_tsf || ch >= SOUND_CHANNELS)
    return;
  uint32_t now = millis();
  if (m_off_time[ch]) {
    tsf_note_off(m_tsf, m_off_inst[ch], m_off_key[ch]);
  }
  m_next_event[ch] = now + ticks;
  m_off_time[ch] = now + ticks;
  m_off_key[ch] = note;
  m_off_inst[ch] = inst;
  tsf_note_on(m_tsf, inst, note, vel);
  if (m_tsf->out_of_memory) {
    unloadFont();
    err = ERR_OOM;
  }
}

void GROUP(basic_sound) BasicSound::mmlCallback(MML_INFO *p, void *extobj)
{
  uint32_t now = millis();
  int ch = (int)extobj;
  m_next_event[ch] = now;

  switch (p->type) {
    case MML_TYPE_NOTE:
      {
        MML_ARGS_NOTE *args = &(p->args.note);
        noteOn(ch, m_ch_inst[ch], args->number, 1.0, args->ticks);
      }
      break;
    case MML_TYPE_REST:
      {
        MML_ARGS_REST *args = &(p->args.rest);
        m_next_event[ch] += args->ticks;
      }
      break;
    case MML_TYPE_USER_EVENT:
      {
        MML_ARGS_USER_EVENT *args = &(p->args.user_event);
        if (!strcmp(args->type, "ADSR")) {
          int adsr[4] = {0, 0, 0, 0};
          int a = 0;

          char *tok = strtok(args->name, " ");
          while (tok && a < 4) {
            adsr[a] = strtonum(tok, 0);

            if (adsr[a] < 0)
              adsr[a] = 0;
            else if (adsr[a] > 15)
              adsr[a] = 15;

            tok = strtok(NULL, " ");
            ++a;
          };
          // obsolete
        } else if (args->type[1] == '\0') {
          // XXX: numbers > 9?
          m_ch_inst[ch] = args->type[0] - '0';
        }
      }
      break;
  }
}

int BasicSound::tsfile_read(void *data, void *ptr, unsigned int size) {
  return ((Unifile *)data)->read((char *)ptr, size);
}
int BasicSound:: tsfile_tell(void *data) {
  return ((Unifile *)data)->position();
}
int BasicSound::tsfile_skip(void *data, unsigned int count) {
  return ((Unifile *)data)->seekSet(((Unifile *)data)->position() + count);
}
int BasicSound::tsfile_seek(void *data, unsigned int pos) {
  return ((Unifile *)data)->seekSet(pos);
}
int BasicSound::tsfile_close(void *data) {
  ((Unifile *)data)->close();
  return 0;
}
int BasicSound::tsfile_size(void *data) {
  return ((Unifile *)data)->fileSize();
}
struct tsf_stream BasicSound::m_sf2;
Unifile BasicSound::m_sf2_file;
tsf *BasicSound::m_tsf;

void BasicSound::loadFont()
{
  m_sf2_file = Unifile::open(m_font_name.c_str(), FILE_READ);
  m_sf2.data = &m_sf2_file;
  m_sf2.read = tsfile_read;
  m_sf2.tell = tsfile_tell;
  m_sf2.skip = tsfile_skip;
  m_sf2.seek = tsfile_seek;
  m_sf2.close = tsfile_close;
  m_sf2.size = tsfile_size;
  m_tsf = tsf_load(&m_sf2);
  if (m_tsf)
    tsf_set_output(m_tsf, TSF_MONO, 16000, -10);
  m_all_done_time = 0;
}

void GROUP(basic_sound) BasicSound::unloadFont()
{
  if (m_tsf) {
    tsf_close(m_tsf);
    m_tsf = NULL;
  }
}

void BasicSound::begin(void)
{
  m_font_name = "1mgm.sf2";
  for (int i = 0; i < SOUND_CHANNELS; ++i) {
    mml_init(&m_mml[i], mmlCallback, (void *)i);
    MML_OPTION_INITIALIZER_DEFAULT(&m_mml_opt[i]);
    defaults(i);
  }
}

void BasicSound::defaults(int ch)
{
  m_off_time[ch] = 0;
  m_ch_inst[ch] = ch;
  m_next_event[ch] = 0;
}

void BasicSound::playMml(int ch, const char *data)
{
  if (!m_tsf)
    loadFont();
  mml_setup(&m_mml[ch], &m_mml_opt[ch], (char *)data);
  defaults(ch);
  m_next_event[ch] = millis();
}

void BasicSound::stopMml(int ch)
{
  m_next_event[ch] = 0;
}

// Array with 32-bit values which have one bit more set to '1' in every
// consecutive array index value
// Taken from Espressif MP3 decoder demo.
const uint32_t GROUP(basic_data) fakePwm[]={
        0x00000010, 0x00000410, 0x00400410, 0x00400C10, 0x00500C10,
        0x00D00C10, 0x20D00C10, 0x21D00C10, 0x21D80C10, 0xA1D80C10,
        0xA1D80D10, 0xA1D80D30, 0xA1DC0D30, 0xA1DC8D30, 0xB1DC8D30,
        0xB9DC8D30, 0xB9FC8D30, 0xBDFC8D30, 0xBDFE8D30, 0xBDFE8D32,
        0xBDFE8D33, 0xBDFECD33, 0xFDFECD33, 0xFDFECD73, 0xFDFEDD73,
        0xFFFEDD73, 0xFFFEDD7B, 0xFFFEFD7B, 0xFFFFFD7B, 0xFFFFFDFB,
        0xFFFFFFFB, 0xFFFFFFFF
};

static short staging_buf[I2S_BUFLEN];

void GROUP(basic_sound) BasicSound::pumpEvents()
{
  uint32_t now = millis();
  for (int i = 0; i < SOUND_CHANNELS; ++i) {
    m_finished[i] = false;
    if (m_next_event[i] && now >= m_next_event[i]) {
      if (mml_fetch(&m_mml[i]) != MML_RESULT_OK) {
        m_next_event[i] = 0;
        m_finished[i] = true;
      }
    }
    if (m_off_time[i] && m_off_time[i] <= now) {
      if (m_tsf)
        tsf_note_off(m_tsf, m_off_inst[i], m_off_key[i]);
      m_off_time[i] = 0;
    }
  }
  
  // Unload driver if nothing has been played for a few seconds.
  if (m_tsf && !tsf_playing(m_tsf)) {
    if (m_all_done_time) {
      if (now > m_all_done_time + 5000)
        unloadFont();
    } else
      m_all_done_time = now;
  } else
    m_all_done_time = 0;
}

void GROUP(basic_sound) BasicSound::render()
{
  // This can not be done in the I2S interrupt handler because it may need
  // soundfont file access to cache samples.
  if (m_tsf && nosdk_i2s_curr_buf_pos == 0) {
    tsf_render_short_fast(m_tsf, staging_buf, I2S_BUFLEN, TSF_FALSE);
    if (m_tsf->out_of_memory) {
      unloadFont();
      err = ERR_OOM;
      return;
    }
    for (int i = 0; i < I2S_BUFLEN; ++i) {
      int idx = (staging_buf[i] >> 8) + 16;
      if (idx < 0)
        idx = 0;
      else if (idx > 31)
        idx = 31;
      nosdk_i2s_curr_buf[i] = pgm_read_dword(&fakePwm[idx]);
    }
    nosdk_i2s_curr_buf_pos = I2S_BUFLEN;
  }
}

BString BasicSound::instName(int index)
{
  BString name;

  if (!m_tsf)
    loadFont();
  if (m_tsf && index < instCount()) {
    name = tsf_get_presetname(m_tsf, index);
    if (m_tsf->out_of_memory) {
      unloadFont();
      err = ERR_OOM;
    }
  }

  return name;
}

BasicSound sound;
