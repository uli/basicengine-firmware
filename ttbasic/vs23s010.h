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

#ifndef __VS23S010_H
#define __VS23S010_H

#include "ntsc.h"
#include <Arduino.h>
#include <SPI.h>
#include "bgengine.h"

#define SC_DEFAULT 0

#define VS23_MAX_X 456
#define VS23_MAX_Y 224

//#define DEBUG_BM

enum {
  LINE_BROKEN,
  LINE_SOLID,
};

// ntscビデオ表示クラス定義
class VS23S010 : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);
  void end();                    // NTSCビデオ表示終了
  void cls();                    // 画面クリア
  void delay_frame(uint16_t x);  // フレーム換算時間待ち

  inline uint16_t width() {
    return XPIXELS;
  }
  inline uint16_t height() {
    return YPIXELS;
  }
  uint16_t vram_size();
  uint16_t screen();
  void adjust(int16_t cnt);

  inline uint32_t piclineByteAddress(int line) {
    return m_first_line_addr + m_pitch * line;
  }
  inline uint16_t piclinePitch() {
    return m_pitch;
  }
  inline uint32_t pixelAddr(int x, int y) {
    return m_first_line_addr + m_pitch * y + x;
  }

  uint16_t ICACHE_RAM_ATTR currentLine();

  inline void setFrame(uint32_t f) {
    m_frame = f;
  }

  inline bool isPal() {
    return m_pal;
  }

  void setPixel(uint16_t x, uint16_t y, pixel_t c);
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    setPixel(x, y, (pixel_t)(PIXEL_TYPE)c);
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos,
                   uint8_t r, uint8_t g, uint8_t b);
  pixel_t getPixel(uint16_t x, uint16_t y);

  int numModes();
  void setColorSpace(uint8_t palette);
  const struct video_mode_t *modes();
  bool setMode(uint8_t mode);
  void calibrateVsync();
  void setSyncLine(uint16_t line);

  void videoInit();
  void SetLineIndex(uint16_t line, uint16_t wordAddress);
  void SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress);
  void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   uint16_t rgb);
  uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width,
                              uint16_t *pixels);
  uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width,
                                  uint16_t *pixels);
  void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                pixel_t color);
  void blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                 uint8_t width, uint8_t height);

  inline void writeBytes(uint32_t address, uint8_t *data, uint32_t len) {
    SpiRamWriteBytes(address, data, len);
  }
  inline void setPixels(uint32_t address, pixel_t *data, uint32_t len) {
    writeBytes(address, (uint8_t *)data, len * sizeof(pixel_t));
  }
  inline void setPixelsIndexed(uint32_t address, ipixel_t *data, uint32_t len) {
    writeBytes(address, (uint8_t *)data, len);
  }

  inline void setInterlace(bool interlace) {
    m_interlace = interlace;
  }
  inline void setLowpass(bool lowpass) {
    m_lowpass = lowpass;
  }
  uint32_t getSpiClock() {
    return SPI1CLK;
  }
  void setSpiClock(uint32_t div) {
    SPI1CLK = div;
  }
  void setSpiClockRead() {
    SPI1CLK = m_current_mode.max_spi_freq;
  }
  void setSpiClockWrite() {
    SPI1CLK = m_def_spi_div;
  }
  void setSpiClockMax() {
    SPI1CLK = m_min_spi_div;
  }

  static inline void startBlockMove() {
#ifdef DEBUG_BM
    if (!blockFinished()) {
      Serial.println("overmove!!");
    }
#endif
    VS23_SELECT;
    SPI.write(0x36);
    VS23_DESELECT;
  }

  void pinMode(uint8_t pin, uint8_t mode) {
    uint8_t bit = (1 << (pin + 4));
    if (mode == OUTPUT)
      m_gpio_state |= bit;
    else
      m_gpio_state &= ~bit;
    SpiRamWriteRegister(0x82, m_gpio_state);
  }

  void digitalWrite(uint8_t pin, uint8_t val) {
    uint8_t bit = (1 << pin);
    if (val)
      m_gpio_state |= bit;
    else
      m_gpio_state &= ~bit;
    SpiRamWriteRegister(0x82, m_gpio_state);
  }

  int digitalRead(uint8_t pin) {
    uint8_t bits = SpiRamReadRegister(0x86);
    if (bits & (1 << (pin + 4)))
      return HIGH;
    else
      return LOW;
  }

#ifdef USE_BG_ENGINE
  void updateBg();

  void resetSprites() override;

  inline void setSpriteOpaque(uint8_t num, bool enable) {
    struct sprite_t *s = &m_sprite[num];
    if (enable == (s->pat == NULL))
      return;

    m_bg_modified = true;
    if (enable) {
      // Opaque sprites don't need a pattern, no need to reload it
      // even if previous operations would have made that necessary.
      s->must_reload = false;
      if (s->pat) {
        s->pat->ref--;
        s->pat = NULL;
      }
      s->p.opaque = true;
    } else {
      s->must_reload = true;
      s->p.opaque = false;
    }
  }

  inline bool spriteReload(uint8_t num) {
    if (m_sprite[num].must_reload)
      return loadSpritePattern(num);
    else
      return true;
  }

  uint8_t spriteCollision(uint8_t collidee, uint8_t collider);
#endif

  const static struct video_mode_t modes_ntsc[];
  const static struct video_mode_t modes_pal[];

  void reset() override;

  void setBorder(uint8_t y, uint8_t uv);
  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w);
  inline uint16_t borderWidth() {
    return FRPORCH - BLANKEND;
  }

  uint32_t cyclesPerFrame() {
    return m_cycles_per_frame;
  }
  uint32_t cyclesPerFrameCalculated() {
    return m_cycles_per_frame_calculated;
  }
#ifndef HOSTED
private:
#endif
  void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                 uint8_t width, uint8_t height, uint8_t dir);

  static void ICACHE_RAM_ATTR vsyncHandler(void);
  bool m_vsync_enabled;
  uint32_t m_cycles_per_frame;
  uint32_t m_cycles_per_frame_calculated;

  int m_min_spi_div;  // smallest valid divider, i.e. max. frequency
  int m_def_spi_div;  // divider safe for writing
  uint8_t m_gpio_state;

  uint32_t m_pitch;  // Distance between piclines in bytes
  uint32_t m_first_line_addr;
  uint16_t m_sync_line;

#ifdef USE_BG_ENGINE
  void drawBg(struct bg_t *bg, int y1, int y2);

  struct sprite_pattern *m_patterns[MAX_SPRITES];

  bool loadSpritePattern(uint8_t num);
  struct sprite_pattern *allocateSpritePattern(struct sprite_props *p);
#endif

  bool m_interlace;
  bool m_pal;
  bool m_lowpass;
  inline uint16_t lowpass() {
    return m_lowpass ? BLOCKMVC1_PYF : 0;
  }
};

extern VS23S010 vs23;  // グローバルオブジェクト利用宣言

#endif
