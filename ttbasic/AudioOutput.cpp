#ifdef ESP32

#include "AudioOutput.h"

#include "soc/i2s_reg.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

AudioOutput audio;

int AudioOutput::m_curr_buf_pos;
uint8_t *AudioOutput::m_curr_buf;
static int m_read_buf;
static int m_read_pos;

uint8_t AudioOutput::m_sound_buf[2][SOUND_BUFLEN];
int AudioOutput::m_block_size;

void IRAM_ATTR AudioOutput::timerInterrupt(AudioOutput *audioOutput) {
  static bool tick;
  uint32_t intStatus = TIMERG0.int_st_timers.val;
  if (intStatus & BIT(TIMER_0)) {
    TIMERG0.hw_timer[TIMER_0].update = 1;
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[TIMER_0].config.alarm_en = 1;

    tick = !tick;
    if (tick) {
      //WRITE_PERI_REG(I2S_CONF_SIGLE_DATA_REG(0), audioOutput->audioSystem->nextSample() << 24);
      WRITE_PERI_REG(I2S_CONF_SIGLE_DATA_REG(0),
                     m_sound_buf[m_read_buf][m_read_pos++] << 24);
      if (m_read_pos >= m_block_size) {
        m_curr_buf = m_sound_buf[m_read_buf];
        m_curr_buf_pos = 0;
        m_read_buf ^= 1;
        m_read_pos = 0;
      }
    }
    TPS2::checkKbd();
  }
}

void AudioOutput::init(int sample_rate)
{
  m_curr_buf_pos = 0;
  m_read_pos = 0;
  m_read_buf = 0;
  m_curr_buf = m_sound_buf[m_read_buf ^ 1];
  m_block_size = SOUND_BUFLEN;
  timer_config_t config;
  config.alarm_en = 1;
  config.auto_reload = 1;
  config.counter_dir = TIMER_COUNT_UP;
  config.divider = 16;
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_en = TIMER_PAUSE;
  timer_init((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0, &config);
  timer_pause((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
  timer_set_counter_value((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0,
                          0x00000000ULL);
  timer_set_alarm_value((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0,
                        1.0 / (sample_rate * 2) * TIMER_BASE_CLK /
                                config.divider);
  timer_enable_intr((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
  timer_isr_register((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0,
                     (void (*)(void *))timerInterrupt, (void *)this,
                     ESP_INTR_FLAG_IRAM, NULL);
  timer_start((timer_group_t)TIMER_GROUP_0, (timer_idx_t)TIMER_0);
}

#endif  // ESP32
