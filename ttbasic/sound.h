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

#ifndef _SOUND_H
#define _SOUND_H

#include "mml.h"

#include "audio.h"

#if !defined(ESP8266) || defined(ESP8266_NOWIFI)
#define HAVE_TSF
#define HAVE_MML
#endif

//#define DEBUG_SOUND

#ifdef DEBUG_SOUND
//#define DEBUG_TSF_MEMORY
#define dbg_snd(x...) dbg_printf("snd: " x)
#else
#define dbg_snd(x...) do {} while(0)
#endif

#ifdef LOWMEM
#define SOUND_IDLE_TIMEOUT 5000
#else
#define SOUND_IDLE_TIMEOUT 300000
#endif

#ifdef HAVE_TSF

#ifdef DEBUG_TSF_MEMORY
static inline void *dbg_malloc(size_t s, int line) {
  void *x = malloc(s);
  printf("malloc %d@%d -> %p\r\n", s, line, x);
  return x;
}
#define TSF_MALLOC(a) dbg_malloc((a), __LINE__)
#define TSF_REALLOC(a, b) \
  (printf("realloc %p -> %d@%d\r\n", (a), (b), __LINE__), realloc((a), (b)))
#define TSF_FREE(a)                          \
  do {                                       \
    printf("free %p@%d\r\n", (a), __LINE__); \
    free((a));                               \
  } while (0)
#endif

#define TSF_NO_STDIO
#include "tsf.h"

#endif  // HAVE_TSF

#define MML_CHANNELS 3

#include "basic.h"

extern "C" {
#include <sts_mixer.h>
}

void refill_stream_beep(sts_mixer_sample_t *sample, void *userdata);
void refill_stream_tsf(sts_mixer_sample_t *sample, void *userdata);

class BasicSound {
public:
    static void begin(void);
#ifdef HAVE_MML
    static void playMml(int ch, const char *data);
    static void stopMml(int ch);
#endif
    static void pumpEvents();
#ifdef HAVE_TSF
    static void render();
#endif

#ifdef HAVE_MML
    static inline bool isPlaying(int ch) {
        return m_next_event[ch] != 0;
    }
    static inline bool isFinished(int ch) {
        return m_finished[ch];
    }
#endif

#ifdef HAVE_TSF
    static inline bool needSamples() {
        return audio.currBufPos() == 0;
    }

    static inline int instCount() {
        if (!m_tsf)
          loadFont();
        if (m_tsf)
          return tsf_get_presetcount(m_tsf);
        else
          return 0;
    }
    static BString instName(int inst);
    static void noteOn(int ch, int inst, int note, float vel, int ticks);

    static void loadFont();
    static void unloadFont();
    static inline void setFontName(BString &n) {
      m_font_name = n;
    }
#endif

    static void beep(int period, int vol = 15, const uint8_t *env = NULL);
    static void noBeep();

    static sts_mixer_t *getMixer() {
      return &m_mixer;
    }

private:
    static void setBeep(int period, int vol);

    static uint8_t *m_beep_env;
    static uint16_t m_beep_pos;
    static uint16_t m_beep_period;
    static uint8_t m_beep_vol;
    static sample_t m_beep_buf[SOUND_BUFLEN * 2];
    static sts_mixer_stream_t m_beep_stream;

#ifdef HAVE_TSF
    static tsf *m_tsf;
    static sample_t m_tsf_buf[SOUND_BUFLEN * 2];
    static sts_mixer_stream_t m_tsf_stream;
    static struct tsf_stream m_sf2;
    static FILE *m_sf2_file;
    static int tsfile_read(void *data, void *ptr, unsigned int size);
    static int tsfile_tell(void *data);
    static int tsfile_skip(void *data, unsigned int count);
    static int tsfile_seek(void *data, unsigned int pos);
    static int tsfile_close(void *data);
    static int tsfile_size(void *data);
#endif

#ifdef HAVE_MML
    static void defaults(int ch);
    static void ICACHE_RAM_ATTR mmlCallback(MML_INFO *p, void *extobj);
    static uint32_t mmlGetNoteLength(int ch, uint32_t note_ticks);

    static MML m_mml[MML_CHANNELS];
    static MML_OPTION m_mml_opt[MML_CHANNELS];
    static uint32_t m_next_event[MML_CHANNELS];

    static uint32_t m_off_time[MML_CHANNELS];
    static uint8_t m_off_key[MML_CHANNELS];
    static uint8_t m_off_inst[MML_CHANNELS];
    static uint8_t m_ch_inst[MML_CHANNELS];
    static bool m_finished[MML_CHANNELS];
    static uint16_t m_bpm[MML_CHANNELS];
    static uint8_t m_velocity[MML_CHANNELS];
#endif
#ifdef HAVE_TSF
    static uint32_t m_all_done_time;
    static BString m_font_name;
#endif

    static sts_mixer_t m_mixer;
    friend void ::refill_stream_beep(sts_mixer_sample_t *sample, void *userdata);
    friend void ::refill_stream_tsf(sts_mixer_sample_t *sample, void *userdata);
};

extern BasicSound sound;

#endif
