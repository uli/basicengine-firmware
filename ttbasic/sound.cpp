// SPDX-License-Identifier: MIT
/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2019 Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include "ttconfig.h"
#include <Arduino.h>
#include "sound.h"
#include "audio.h"

#ifndef AUDIO_SAMPLE_RATE
#define AUDIO_SAMPLE_RATE 16000
#endif

#ifdef HAVE_TSF
#define TSF_NO_STDIO
#define TSF_IMPLEMENTATION
#define TSF_MEMCPY os_memcpy
#define TSF_MEMSET memset
#include "tsf.h"
#endif

#ifdef HAVE_MML
MML BasicSound::m_mml[SOUND_CHANNELS];
MML_OPTION BasicSound::m_mml_opt[SOUND_CHANNELS];
uint32_t BasicSound::m_next_event[SOUND_CHANNELS];
bool BasicSound::m_finished[SOUND_CHANNELS];
#endif
#ifdef HAVE_TSF
uint32_t BasicSound::m_all_done_time;
#endif
uint32_t BasicSound::m_sam_done_time;

#ifdef HAVE_MML
uint32_t BasicSound::m_off_time[SOUND_CHANNELS];
uint8_t BasicSound::m_off_key[SOUND_CHANNELS];
uint8_t BasicSound::m_off_inst[SOUND_CHANNELS];
uint8_t BasicSound::m_ch_inst[SOUND_CHANNELS];
uint16_t BasicSound::m_bpm[SOUND_CHANNELS];
uint8_t BasicSound::m_velocity[SOUND_CHANNELS];
#endif

#ifdef HAVE_TSF
BString BasicSound::m_font_name;
#endif

uint16_t BasicSound::m_beep_period;
uint8_t BasicSound::m_beep_vol;
uint8_t *BasicSound::m_beep_env;
uint16_t BasicSound::m_beep_pos;

#ifdef HAVE_TSF
void BasicSound::noteOn(int ch, int inst, int note, float vel, int ticks) {
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
#endif

#ifdef HAVE_MML
inline uint32_t BasicSound::mmlGetNoteLength(int ch, uint32_t note_ticks) {
  return (60000) * note_ticks / m_bpm[ch] / m_mml_opt[ch].bticks;
}

void GROUP(basic_sound) BasicSound::mmlCallback(MML_INFO *p, void *extobj) {
  uint32_t now = millis();
  int ch = (int)extobj;
  m_next_event[ch] = now;

  switch (p->type) {
    case MML_TYPE_NOTE:
      {
        MML_ARGS_NOTE *args = &(p->args.note);
//        dbg_snd("[NOTE  : Number=%3d, Ticks=%4d]\n", args->number, args->ticks);
        noteOn(ch, m_ch_inst[ch], args->number, m_velocity[ch] / 15.0f, mmlGetNoteLength(ch, args->ticks));
      }
      break;
    case MML_TYPE_REST:
      {
        MML_ARGS_REST *args = &(p->args.rest);
//        dbg_snd("[REST  :             Ticks=%4d]\n", args->ticks);
        m_next_event[ch] += mmlGetNoteLength(ch, args->ticks);
      }
      break;
    case MML_TYPE_USER_EVENT:
      {
        MML_ARGS_USER_EVENT *args = &(p->args.user_event);
        dbg_snd("[USER_EVENT: Name='%s' Type '%s']\r\n", args->name, args->type);
        if (!strcmp(args->type, "ADSR")) {
          int adsr[4] = {0, 0, 0, 0};
          int a = 0;

          char *tok = strtok(args->name, " ");
          while (tok && a < 4) {
            adsr[a] = strtonum(tok, 0);
            dbg_snd("adsr %d <- %d\r\n", a, adsr[a]);

            if (adsr[a] < 0)
              adsr[a] = 0;
            else if (adsr[a] > 15)
              adsr[a] = 15;

            tok = strtok(NULL, " ");
            ++a;
          };
          // obsolete
        } else if (isDigit(args->type[0])) {
          m_ch_inst[ch] = strtonum(args->type, 0);
        }
      }
      break;
    case MML_TYPE_TEMPO:
      {
        MML_ARGS_TEMPO *args = &(p->args.tempo);
        dbg_snd("[TEMPO : Value=%3d]\r\n", args->value);
        m_bpm[ch] = args->value;
      }
      break;
#ifdef DEBUG_SOUND
    case MML_TYPE_LENGTH:
      {
        MML_ARGS_LENGTH *args = &(p->args.length);
        dbg_snd("[LENGTH: Value=%3d]\r\n", args->value);
      }
      break;
    case MML_TYPE_VOLUME:
      {
        MML_ARGS_VOLUME *args = &(p->args.volume);
        dbg_snd("[VOLUME: Value=%3d]\r\n", args->value);
        m_velocity[ch] = min(0, max(args->value, 15));
      }
      break;
    case MML_TYPE_OCTAVE:
      {
        MML_ARGS_OCTAVE *args = &(p->args.octave);
        dbg_snd("[OCTAVE: Value=%3d]\r\n", args->value);
      }
      break;
    case MML_TYPE_OCTAVE_UP:
      {
        MML_ARGS_OCTAVE *args = &(p->args.octave_up);
        dbg_snd("[OCTAVE: Value=%3d(Up)]\r\n", args->value);
      }
      break;
    case MML_TYPE_OCTAVE_DOWN:
      {
        MML_ARGS_OCTAVE *args = &(p->args.octave_down);
        dbg_snd("[OCTAVE: Value=%3d(Down)]\r\n", args->value);
      }
      break;
#else
    default:
      break;
#endif
  }
}
#endif  // HAVE_MML

#ifdef HAVE_TSF
int BasicSound::tsfile_read(void *data, void *ptr, unsigned int size) {
  return fread((char *)ptr, 1, size, (FILE *)data);
}
int BasicSound::tsfile_tell(void *data) {
  return ftell((FILE *)data);
}
int BasicSound::tsfile_skip(void *data, unsigned int count) {
  return !fseek((FILE *)data, count, SEEK_CUR);
}
int BasicSound::tsfile_seek(void *data, unsigned int pos) {
  return !fseek((FILE *)data, pos, SEEK_SET);
}
int BasicSound::tsfile_close(void *data) {
  return !fclose((FILE *)data);
}
int BasicSound::tsfile_size(void *data) {
  long now = ftell((FILE *)data);
  fseek((FILE *)data, 0, SEEK_END);
  long end = ftell((FILE *)data);
  fseek((FILE *)data, now, SEEK_SET);
  return end;
}
struct tsf_stream BasicSound::m_sf2;
FILE *BasicSound::m_sf2_file;
tsf *BasicSound::m_tsf;

void BasicSound::loadFont() {
#ifdef SDL
  char *root = getenv("ENGINEBASIC_ROOT");
  if (root && m_font_name[0] != '/')
    m_sf2_file = fopen((BString(root) + BString('/') + m_font_name).c_str(), "r");
  else
    m_sf2_file = fopen(m_font_name.c_str(), "r");
#else
  if (m_font_name[0] == '/') {
    m_sf2_file = fopen(m_font_name.c_str(), "r");
  } else {
    m_sf2_file = fopen(
      (BString(SD_PREFIX) + BString('/') + m_font_name).c_str(),
      "r");
#ifndef H3
    if (!m_sf2_file) {
      m_sf2_file = fopen(
        (BString(FLASH_PREFIX) + BString('/') + m_font_name).c_str(),
        "r");
    }
#endif
  }
#endif
  if (!m_sf2_file)
    return;

  m_sf2.data = m_sf2_file;
  m_sf2.read = tsfile_read;
  m_sf2.tell = tsfile_tell;
  m_sf2.skip = tsfile_skip;
  m_sf2.seek = tsfile_seek;
  m_sf2.close = tsfile_close;
  m_sf2.size = tsfile_size;
  m_tsf = tsf_load(&m_sf2);
  if (m_tsf)
    tsf_set_output(m_tsf, TSF_MONO, AUDIO_SAMPLE_RATE, -10);
  m_all_done_time = 0;
  audio.setBlockSize(SOUND_BUFLEN);
}

void GROUP(basic_sound) BasicSound::unloadFont() {
  if (m_tsf) {
    tsf_close(m_tsf);
    m_tsf = NULL;
  }
}
#endif  // HAVE_TSF

void BasicSound::begin(void) {
#ifdef HAVE_TSF
  m_font_name = F("1mgm.sf2");
#endif
#ifdef HAVE_MML
  for (int i = 0; i < SOUND_CHANNELS; ++i) {
    mml_init(&m_mml[i], mmlCallback, (void *)i);
    MML_OPTION_INITIALIZER_DEFAULT(&m_mml_opt[i]);
    defaults(i);
  }
#endif
  m_beep_env = NULL;
  m_sam = NULL;
}

#ifdef HAVE_MML
void BasicSound::defaults(int ch) {
  m_off_time[ch] = 0;
  m_ch_inst[ch] = ch * 3;
  m_next_event[ch] = 0;
  m_bpm[ch] = 120;
  m_velocity[ch] = 15;
}

void BasicSound::playMml(int ch, const char *data) {
  if (!m_tsf)
    loadFont();
  mml_setup(&m_mml[ch], &m_mml_opt[ch], (char *)data);
  defaults(ch);
  m_next_event[ch] = millis();
}

void BasicSound::stopMml(int ch) {
  m_next_event[ch] = 0;
}
#endif

#ifdef HAVE_TSF
static short staging_buf[SOUND_BUFLEN];
#endif

void GROUP(basic_sound) BasicSound::pumpEvents() {
#if defined(HAVE_TSF) || defined(HAVE_MML)
  uint32_t now = millis();
#endif
#ifdef HAVE_MML
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
#endif

#ifdef HAVE_TSF
  // Unload driver if nothing has been played for a few seconds.
  if (m_tsf && !tsf_playing(m_tsf)) {
    if (m_all_done_time) {
      if (now > m_all_done_time + SOUND_IDLE_TIMEOUT)
        unloadFont();
    } else
      m_all_done_time = now;
  } else
    m_all_done_time = 0;
#endif

  if (m_beep_env) {
    uint8_t vol = m_beep_env[m_beep_pos];
    if (!vol) {
      noBeep();
    } else {
      setBeep(m_beep_period, vol * m_beep_vol / 16);
      m_beep_pos++;
    }
  }
}

#ifdef HAVE_TSF
void GROUP(basic_sound) BasicSound::render() {
  // This can not be done in the I2S interrupt handler because it may need
  // soundfont file access to cache samples.
  if (m_sam && audio.currBufPos() == 0) {
    if (m_sam->finished()) {
      if (!m_sam_done_time) {
        m_sam_done_time = millis();
      } else if (millis() > m_sam_done_time + SOUND_IDLE_TIMEOUT) {
        delete m_sam;
        m_sam = NULL;
        audio.init(AUDIO_SAMPLE_RATE);
      }
    }
    if (m_sam) {
      for (int i = 0; i < SOUND_BUFLEN; ++i) {
        audio.queueSample(m_sam->getSample());
      }
    }
  } else if (m_tsf && audio.currBufPos() == 0) {
    tsf_render_short_fast(m_tsf, staging_buf, SOUND_BUFLEN, TSF_FALSE);
    if (m_tsf->out_of_memory) {
      unloadFont();
      err = ERR_OOM;
      return;
    }
    for (int i = 0; i < SOUND_BUFLEN; ++i) {
      int idx = (staging_buf[i] >> 5) + 128;
      if (idx < 0)
        idx = 0;
      else if (idx > 255)
        idx = 255;
      audio.queueSample(idx);
    }
  }
}

BString BasicSound::instName(int index) {
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
#endif

void BasicSound::setBeep(int period, int vol) {
  period = period * AUDIO_SAMPLE_RATE / 16000;

  if (vol > 15)
    vol = 15;
  else if (vol < 0)
    vol = 0;

  uint32_t sample = vol * 17;

  audio.setBlockSize(period);

#ifndef HOSTED
  for (int b = 0; b < 2; ++b) {
    for (int i = 0; i < period / 2; ++i) {
      audio.setSampleAt(b, i, 0);
    }
    for (int i = period / 2; i < period; ++i) {
      audio.setSampleAt(b, i, sample);
    }
  }
#endif
}

void BasicSound::beep(int period, int vol, const uint8_t *env) {
  if (period == 0) {
    noBeep();
    return;
  }
  setBeep(period, vol);
  if (env) {
    free(m_beep_env);
    m_beep_env = (uint8_t *)strdup((const char *)env);
    m_beep_pos = 0;
    m_beep_period = period;
    m_beep_vol = vol;
  } else {
    free(m_beep_env);
    m_beep_env = NULL;
  }
}

void BasicSound::noBeep() {
  free(m_beep_env);
  m_beep_env = NULL;
#if !defined(HOSTED)
  audio.clearBufs();
  audio.setBlockSize(SOUND_BUFLEN);
#endif
}

ESP8266SAM *BasicSound::m_sam;

BasicSound sound;
