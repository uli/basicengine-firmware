// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifdef H3

#include "h3audio.h"
#include <h3_codec.h>
#include <audio.h>

#ifdef JAILHOUSE
#include <video_encoder.h>

#define VENC_BUFFERS 16
sample_t venc_sound_buffer[VENC_BUFFERS * SOUND_BUFLEN * SOUND_CHANNELS];
int venc_buf_pos;
#endif

H3Audio audio;

int H3Audio::m_curr_buf_pos;
sample_t *H3Audio::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

sample_t H3Audio::m_sound_buf[2][SOUND_BUFLEN * SOUND_CHANNELS];
int H3Audio::m_block_size;

void H3Audio::timerInterrupt(H3Audio *audioOutput) {
}

void H3Audio::init(int sample_rate) {
  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;

  audio_start(SOUND_BUFLEN * SOUND_CHANNELS);
#ifdef JAILHOUSE
  video_encoder->audio_size = (VENC_BUFFERS * SOUND_BUFLEN / 2) * SOUND_CHANNELS * sizeof(sample_t);
#endif
}

void hook_audio_get_sample(int16_t *l, int16_t *r) {
  *l = audio.m_sound_buf[m_read_buf][m_read_pos * SOUND_CHANNELS];
  *r = audio.m_sound_buf[m_read_buf][m_read_pos * SOUND_CHANNELS + 1];
  m_read_pos++;

  if (m_read_pos >= audio.m_block_size) {
    m_read_buf ^= 1;
    audio.m_curr_buf = audio.m_sound_buf[m_read_buf ^ 1];
    audio.m_curr_buf_pos = 0;
    m_read_pos = 0;

    h3_codec_push_data(audio.m_curr_buf);

#ifdef JAILHOUSE
    if (video_encoder->enabled) {
      // The video encoder cannot handle incoming audio at the rate we're sending it (every 10ms).
      // We therefore have to buffer it to slow down the frame rate to a reasonable level.

      memcpy(&venc_sound_buffer[venc_buf_pos * SOUND_BUFLEN * SOUND_CHANNELS], audio.m_curr_buf, SOUND_BUFLEN * SOUND_CHANNELS * sizeof(sample_t));

      venc_buf_pos = (venc_buf_pos + 1) % VENC_BUFFERS;

      if (venc_buf_pos == VENC_BUFFERS / 2) {
        video_encoder->audio_buffer = venc_sound_buffer;
        ++video_encoder->audio_frame_no;
        asm("sev");
      } else if (venc_buf_pos == 0) {
        video_encoder->audio_buffer = &venc_sound_buffer[VENC_BUFFERS * SOUND_BUFLEN * SOUND_CHANNELS / 2];
        ++video_encoder->audio_frame_no;
        asm("sev");
      }
    }
#endif
  }
}

#endif	// H3
