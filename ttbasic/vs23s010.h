/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2018 Ulrich Hecht.
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
#include "GuillotineBinPack.h"

#ifdef ESP8266_NOWIFI
#define VS23_BG_ENGINE
#endif

#define SC_DEFAULT 0

struct vs23_mode_t {
  uint16_t x;
  uint16_t y;
  uint8_t top;
  uint8_t left;
  uint8_t vclkpp;
  uint8_t bextra;
  // Maximum SPI frequency for this mode; translated to minimum clock
  // divider on boot.
  uint32_t max_spi_freq;
};

#define VS23_MAX_X	456
#define VS23_MAX_Y	224

#define VS23_MAX_BG	4
#define VS23_MAX_SPRITES 32
#define VS23_MAX_SPRITE_W 32
#define VS23_MAX_SPRITE_H 32

#define VS23_MAX_PRIO	(VS23_MAX_BG-1)

//#define DEBUG_BM

enum {
  LINE_BROKEN,
  LINE_SOLID,
};

struct sprite_line {
  uint8_t *pixels;
  uint8_t off;
  uint8_t len;
  uint8_t type;
};

// ntscビデオ表示クラス定義
class VS23S010 {    
  public:
    void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);
    void end();                            // NTSCビデオ表示終了 
    void cls();                            // 画面クリア
    void delay_frame(uint16_t x);          // フレーム換算時間待ち

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

#ifdef HOSTED
    uint32_t frame();
#else
    inline uint32_t frame() {
      return m_frame;
    }
#endif

    inline void setFrame(uint32_t f) {
      m_frame = f;
    }

    inline bool isPal() {
      return m_pal;
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t c);
    void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
    uint8_t getPixel(uint16_t x, uint16_t y);

    int numModes();
    void setColorSpace(uint8_t palette);
    const struct vs23_mode_t *modes();
    void setMode(uint8_t mode);
    void calibrateVsync();
    void setSyncLine(uint16_t line);
 
    void videoInit();
    void SetLineIndex(uint16_t line, uint16_t wordAddress);
    void SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress);
    void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t rgb);
    uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    void TvFilledRectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *texture, uint16_t color);
    void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);

    inline void writeBytes(uint32_t address, uint8_t *data, uint32_t len) {
      SpiRamWriteBytes(address, data, len);
    }

    inline void setInterlace(bool interlace) {
      m_interlace = interlace;
    }
    inline void setLowpass(bool lowpass) {
      m_lowpass = lowpass;
    }
    inline void setLineAdjust(int8_t line_adjust) {
      m_line_adjust = line_adjust;
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

#ifdef VS23_BG_ENGINE
    bool setBgSize(uint8_t bg, uint16_t width, uint16_t height);

    inline uint8_t bgWidth(uint8_t bg) {
      return m_bg[bg].w;
    }
    inline uint8_t bgHeight(uint8_t bg) {
      return m_bg[bg].h;
    }

    inline void setBgTileSize(uint8_t bg_idx, uint8_t tile_size_x, uint8_t tile_size_y) {
      struct bg_t *bg = &m_bg[bg_idx];
      bg->tile_size_x = tile_size_x;
      bg->tile_size_y = tile_size_y;
      m_bg_modified = true;
    }

    inline void setBgPattern(uint8_t bg_idx, uint16_t pat_x, uint16_t pat_y, uint16_t pat_w) {
      struct bg_t *bg = &m_bg[bg_idx];
      bg->pat_x = pat_x;
      bg->pat_y = pat_y;
      bg->pat_w = pat_w;
      m_bg_modified = true;
    }
    
    inline void setBgPriority(uint8_t bg_idx, uint8_t prio) {
      m_bg[bg_idx].prio = prio;
      m_bg_modified = true;
    }

    inline uint8_t bgTileSizeX(uint8_t bg) {
      return m_bg[bg].tile_size_x;
    }
    inline uint8_t bgTileSizeY(uint8_t bg) {
      return m_bg[bg].tile_size_y;
    }

    inline void getBgTileSize(uint8_t bg_idx, uint8_t &tsx, uint8_t &tsy) {
      struct bg_t *bg = &m_bg[bg_idx];
      tsx = bg->tile_size_x;
      tsy = bg->tile_size_y;
    }

    inline uint8_t *bgTiles(uint8_t bg) {
      return m_bg[bg].tiles;
    }

    void resetBgs();
    void enableBg(uint8_t bg);
    void disableBg(uint8_t bg);
    void freeBg(uint8_t bg);

    void setBgWin(uint8_t bg, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    inline uint16_t bgWinX(uint8_t bg) {
      return m_bg[bg].win_x;
    }
    inline uint16_t bgWinY(uint8_t bg) {
      return m_bg[bg].win_y;
    }
    inline uint16_t bgWinWidth(uint8_t bg) {
      return m_bg[bg].win_w;
    }
    inline uint16_t bgWinHeight(uint8_t bg) {
      return m_bg[bg].win_h;
    }
    inline bool bgEnabled(uint8_t bg) {
      return m_bg[bg].enabled;
    }
    inline int bgScrollX(uint8_t bg) {
      return m_bg[bg].scroll_x;
    }
    inline int bgScrollY(uint8_t bg) {
      return m_bg[bg].scroll_y;
    }
    
    void setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t);
    void setBgTiles(uint8_t bg_idx, uint16_t x, uint16_t y, const uint8_t *tiles, int count);
    void mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to);

    inline void scroll(uint8_t bg_idx, int x, int y) {
      struct bg_t *bg = &m_bg[bg_idx];
      while (x < 0)
        x += bg->tile_size_x * bg->w;
      while (y < 0)
        y += bg->tile_size_y * bg->h;
      if (bg->scroll_x != x || bg->scroll_y != y) {
        bg->scroll_x = x;
        bg->scroll_y = y;
        m_bg_modified = true;
      }
    }
    void updateBg();

    void resetSprites();
    void setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y);
    void resizeSprite(uint8_t num, uint8_t w, uint8_t h);
    void moveSprite(uint8_t num, int16_t x, int16_t y);
    void setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y = 0, bool flip_x = false, bool flip_y = false);
    void setSpriteKey(uint8_t num, int16_t key);
    void enableSprite(uint8_t num);
    void disableSprite(uint8_t num);
    inline uint8_t spriteFrameX(uint8_t num) {
      return m_sprite[num].p.frame_x;
    }
    inline uint8_t spriteFrameY(uint8_t num) {
      return m_sprite[num].p.frame_y;
    }
    inline bool spriteFlipX(uint8_t num) {
      return m_sprite[num].p.flip_x;
    }
    inline bool spriteFlipY(uint8_t num) {
      return m_sprite[num].p.flip_y;
    }
    inline bool spriteOpaque(uint8_t num) {
      return !m_sprite[num].pat;
    }
    inline int16_t spriteX(uint8_t num) {
      return m_sprite[num].pos_x;
    }
    inline int16_t spriteY(uint8_t num) {
      return m_sprite[num].pos_y;
    }

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
      } else
        s->must_reload = true;
    }

    inline bool spriteEnabled(uint8_t num) {
      return m_sprite[num].enabled;
    }

    inline void setSpritePriority(uint8_t num, uint8_t prio) {
      m_sprite[num].prio = prio;
      m_bg_modified = true;
    }

    inline bool spriteReload(uint8_t num) {
      if (m_sprite[num].must_reload)
        return loadSpritePattern(num);
      else
        return true;
    }

    void spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t *tiles, uint8_t num_tiles);
    uint8_t spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile);
    uint8_t spriteCollision(uint8_t collidee, uint8_t collider);
#endif

    const static struct vs23_mode_t modes_ntsc[];
    const static struct vs23_mode_t modes_pal[];
    
    inline uint16_t lastLine() { return m_last_line; }

    bool allocBacking(int w, int h, int &x, int &y);
    void freeBacking(int x, int y, int w, int h);

    void reset();

    void setBorder(uint8_t y, uint8_t uv);
    void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w);
    inline uint16_t borderWidth() {
      return FRPORCH - BLANKEND;
    }

#ifdef VS23_BG_ENGINE
    inline void forceRedraw() {
      m_bg_modified = true;
    }
    
    inline void setFrameskip(uint32_t v) {
      m_frameskip = v;
    }
    inline uint32_t getFrameskip() {
      return m_frameskip;
    }
#endif

    uint32_t cyclesPerFrame() {
      return m_cycles_per_frame;
    }
    uint32_t cyclesPerFrameCalculated() {
      return m_cycles_per_frame_calculated;
    }
#ifndef HOSTED
private:
#endif
    static void ICACHE_RAM_ATTR vsyncHandler(void);
    bool m_vsync_enabled;
    uint32_t m_cycles_per_frame;
    uint32_t m_cycles_per_frame_calculated;
    
    int m_min_spi_div;	// smallest valid divider, i.e. max. frequency
    int m_def_spi_div;	// divider safe for writing
    uint8_t m_gpio_state;

    struct vs23_mode_t m_current_mode;
    uint32_t m_pitch;	// Distance between piclines in bytes
    uint32_t m_first_line_addr;
    int m_last_line;
    uint16_t m_sync_line;

#ifdef VS23_BG_ENGINE
    struct bg_t {
      uint8_t *tiles;
      uint16_t pat_x, pat_y, pat_w;
      uint8_t w, h;
      uint8_t tile_size_x, tile_size_y;
      int scroll_x, scroll_y;
      uint16_t win_x, win_y, win_w, win_h;
      bool enabled;
      uint8_t prio;
      uint8_t *tile_map;
    } m_bg[VS23_MAX_BG];

    void drawBg(struct bg_t *bg,
                            int dest_addr_start,
                            uint32_t pat_start_addr,
                            uint32_t win_start_addr,
                            int tile_start_x,
                            int tile_start_y,
                            int tile_end_x,
                            int tile_end_y,
                            uint32_t xpoff,
                            uint32_t ypoff,
                            int skip_x,
                            int skip_y);
    void drawBgTop(struct bg_t *bg,
                            int dest_addr_start,
                            uint32_t pat_start_addr,
                            int tile_start_x,
                            int tile_start_y,
                            int tile_end_x,
                            uint32_t xpoff,
                            uint32_t ypoff);

    void drawBgBottom(struct bg_t *bg,
                                      int tile_start_x,
                                      int tile_end_x,
                                      int tile_end_y,
                                      uint32_t xpoff,
                                      uint32_t ypoff,
                                      int skip_x);

    struct sprite_props {
      uint16_t pat_x, pat_y;
      uint8_t w, h;
      uint8_t frame_x, frame_y;
      uint8_t key;
      bool flip_x:1, flip_y:1;
    };
      
    struct sprite_pattern {
      struct sprite_props p;
      uint32_t last;
      uint8_t ref;
      struct sprite_line lines[0];
    };
    struct sprite_pattern *m_patterns[VS23_MAX_SPRITES];
    
    struct sprite_t {
      struct sprite_pattern *pat;
      struct sprite_props p;
      int16_t pos_x, pos_y;
      uint8_t prio;
      bool enabled:1, must_reload:1;
    } m_sprite[VS23_MAX_SPRITES];
    struct sprite_t *m_sprites_ordered[VS23_MAX_SPRITES];
    static int cmp_sprite_y(const void *one, const void *two);

    bool loadSpritePattern(uint8_t num);
    struct sprite_pattern *allocateSpritePattern(struct sprite_props *p);
#endif
    
    uint32_t m_frame;
#ifdef VS23_BG_ENGINE
    uint32_t m_frameskip;

    bool m_bg_modified;
#endif
    
    GuillotineBinPack m_bin;
    
    bool m_interlace;
    bool m_pal;
    bool m_lowpass;
    int8_t m_line_adjust;
    inline uint16_t lowpass() {
      return m_lowpass ? BLOCKMVC1_PYF : 0;
    }
};

extern VS23S010 vs23; // グローバルオブジェクト利用宣言

#endif
