// FILE: TNTSC.h
// Arduino STM32 用 NTSCビデオ出力ライブラリ by たま吉さん
// 作成日 2017/02/20, Blue Pillボード(STM32F103C8)にて動作確認
// 更新日 2017/02/27, delay_frame()の追加、
// 更新日 2017/02/27, フック登録関数追加
// 更新日 2017/03/03, 解像度モード追加
// 更新日 2017/04/05, クロック48MHz対応
// 更新日 2017/04/27, NTSC走査線数補正関数adjust()追加
// 更新日 2017/04/30, SPI1,SPI2の選択指定を可能に修正
// 更新日 2017/06/25, 外部確保VRAMの指定を可能に修正
//

#ifndef __VS23S010_H
#define __VS23S010_H

#include "ntsc.h"
#include <Arduino.h>
#include <SPI.h>
#include "GuillotineBinPack.h"

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

#define VS23_MAX_BG	2
#define VS23_MAX_SPRITES 16
#define VS23_MAX_SPRITE_W 16
#define VS23_MAX_SPRITE_H 16

#define VS23_NUM_COLORSPACES	2

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
    void begin(bool interlace = false, bool lowpass = false);  // NTSCビデオ表示開始
    void end();                            // NTSCビデオ表示終了 
    void cls();                            // 画面クリア
    void delay_frame(uint16_t x);          // フレーム換算時間待ち
	void setBktmStartHook(void (*func)()); // ブランキング期間開始フック設定
    void setBktmEndHook(void (*func)());   // ブランキング期間終了フック設定

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

    inline uint16_t currentLine() {
        uint16_t cl = SpiRamReadRegister(CURLINE) & 0xfff;
        if (m_interlace && cl >= 262)
          cl -= 262;
        return cl;
    }
    
    inline uint32_t frame() {
      return m_frame;
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t c);
    void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
    uint8_t getPixel(uint16_t x, uint16_t y);
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t c);
    void drawLineRgb(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t r, uint8_t g, uint8_t b);
    void drawRect(int x0, int y0, int w, int h, uint8_t c, int fc);
    void drawCircle(int x0, int y0, int radius, uint8_t c, int fc);
    uint8_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b);
    inline uint8_t colorFromRgb(uint8_t *c) {
      return colorFromRgb(c[0], c[1], c[2]);
    }
    static void setColorConversion(int yuvpal, int h_weight, int s_weight, int v_weight, bool fixup);
    void setColorSpace(uint8_t palette);
    inline uint8_t getColorspace() {
      return m_colorspace;
    }
    uint8_t *paletteData(uint8_t colorspace);

    void setMode(uint8_t mode);
    void calibrateVsync();
    void setSyncLine(uint16_t line);
 
    void SpiRamVideoInit();
    void SetLineIndex(uint16_t line, uint16_t wordAddress);
    void SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress);
    void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t rgb);
    uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    void TvFilledRectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *texture, uint16_t color);
    void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);

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
      SPI1CLK = m_current_mode->max_spi_freq;
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
    
    void setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t);

    inline void scroll(uint8_t bg, uint16_t x, uint16_t y) {
      m_bg[bg].scroll_x = x;
      m_bg[bg].scroll_y = y;
      m_bg_modified = true;
    }
    void updateBg();

    void resetSprites();
    void setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y);
    void resizeSprite(uint8_t num, uint8_t w, uint8_t h);
    void moveSprite(uint8_t num, int16_t x, int16_t y);
    void setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y = 0);
    void setSpriteKey(uint8_t num, int16_t key);
    void enableSprite(uint8_t num);
    void disableSprite(uint8_t num);

    inline void setSpriteOpaque(uint8_t num, bool enable) {
      m_sprite[num].transparent = !enable;
      m_bg_modified = true;
    }
    inline bool spriteEnabled(uint8_t num) {
      return m_sprite[num].enabled;
    }

    void spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t *tiles, uint8_t num_tiles);
    uint8_t spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile);
    uint8_t spriteCollision(uint8_t collidee, uint8_t collider);

    static const uint8_t numModes;
    static struct vs23_mode_t modes[];
    
    inline uint16_t lastLine() { return m_last_line; }

    bool allocBacking(int w, int h, int &x, int &y);
    void freeBacking(int x, int y, int w, int h);

    void reset();

    void setBorder(uint8_t y, uint8_t uv);
    void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w);
    inline uint16_t borderWidth() {
      return FRPORCH - BLANKEND;
    }

    inline void forceRedraw() {
      m_bg_modified = true;
    }
    
    inline void setFrameskip(uint32_t v) {
      m_frameskip = v;
    }
    inline uint32_t getFrameskip() {
      return m_frameskip;
    }

    uint32_t cyclesPerFrame() {
      return m_cycles_per_frame;
    }
    uint32_t cyclesPerFrameCalculated() {
      return m_cycles_per_frame_calculated;
    }
private:
    static void ICACHE_RAM_ATTR vsyncHandler(void);
    bool m_vsync_enabled;
    uint32_t m_cycles_per_frame;
    uint32_t m_cycles_per_frame_calculated;
    
    int m_min_spi_div;	// smallest valid divider, i.e. max. frequency
    int m_def_spi_div;	// divider safe for writing
    uint8_t m_gpio_state;

    const struct vs23_mode_t *m_current_mode;
    uint32_t m_pitch;	// Distance between piclines in bytes
    uint32_t m_first_line_addr;
    int m_last_line;
    uint16_t m_sync_line;

    uint8_t colorFromRgbSlow(uint8_t r, uint8_t g, uint8_t b);

    struct bg_t {
      uint8_t *tiles;
      uint16_t pat_x, pat_y, pat_w;
      uint8_t w, h;
      uint8_t tile_size_x, tile_size_y;
      int scroll_x, scroll_y;
      uint16_t win_x, win_y, win_w, win_h;
      bool enabled;
    } m_bg[VS23_MAX_BG];

    void ICACHE_RAM_ATTR drawBg(struct bg_t *bg,
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
    void ICACHE_RAM_ATTR drawBgTop(struct bg_t *bg,
                            int dest_addr_start,
                            uint32_t pat_start_addr,
                            int tile_start_x,
                            int tile_start_y,
                            int tile_end_x,
                            uint32_t xpoff,
                            uint32_t ypoff);

    void ICACHE_RAM_ATTR drawBgBottom(struct bg_t *bg,
                                      int tile_start_x,
                                      int tile_end_x,
                                      int tile_end_y,
                                      uint32_t xpoff,
                                      uint32_t ypoff,
                                      int skip_x);

    struct sprite_t {
      struct sprite_line *pattern;
      uint16_t pat_x, pat_y;
      int16_t pos_x, pos_y;
      bool enabled;
      bool transparent;
      uint8_t w, h;
      uint8_t frame_x, frame_y;
      int16_t key;
    } m_sprite[VS23_MAX_SPRITES];
    struct sprite_t *m_sprites_ordered[VS23_MAX_SPRITES];
    static int cmp_sprite_y(const void *one, const void *two);

    void loadSpritePattern(uint8_t num);
    void allocateSpritePattern(struct sprite_t *s);
    void freeSpritePattern(struct sprite_t *s);
    
    uint32_t m_frame;
    uint32_t m_frameskip;

    bool m_bg_modified;
    
    GuillotineBinPack m_bin;
    
    uint8_t m_colorspace;
    
    bool m_interlace;
    bool m_pal;
    bool m_lowpass;
    inline uint16_t lowpass() {
      return m_lowpass ? BLOCKMVC1_PYF : 0;
    }
};

extern VS23S010 vs23; // グローバルオブジェクト利用宣言

#endif
