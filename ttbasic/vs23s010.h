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
    void begin();  // NTSCビデオ表示開始
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
        return SpiRamReadRegister(CURLINE) & 0xfff;
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t c);
    void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t c);
    void drawLineRgb(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t r, uint8_t g, uint8_t b);
    uint8_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b);

    void setMode(uint8_t mode);
    void calibrateVsync();
    void setSyncLine(uint16_t line);
 
    void SpiRamVideoInit();
    void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t rgb);
    uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    void TvFilledRectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *texture, uint16_t color);
    void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);

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
    
    bool setBgSize(uint8_t bg, uint16_t width, uint16_t height);

    inline void setBgTileSize(uint8_t bg_idx, uint8_t tile_size_x, uint8_t tile_size_y) {
      struct bg_t *bg = &m_bg[bg_idx];
      bg->tile_size_x = tile_size_x;
      bg->tile_size_y = tile_size_y;
    }

    inline void setBgPat(uint8_t bg_idx, uint16_t pat_x, uint16_t pat_y, uint16_t pat_w) {
      struct bg_t *bg = &m_bg[bg_idx];
      bg->pat_x = pat_x;
      bg->pat_y = pat_y;
      bg->pat_w = pat_w;
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

    void resetBgs();
    void enableBg(uint8_t bg);
    void disableBg(uint8_t bg);
    void freeBg(uint8_t bg);
    void setBgWin(uint8_t bg, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t);

    inline void scroll(uint8_t bg, uint16_t x, uint16_t y) {
      m_bg[bg].scroll_x = x;
      m_bg[bg].scroll_y = y;
    }
    void updateBg();

    void resetSprites();
    void setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y);
    void resizeSprite(uint8_t num, uint8_t w, uint8_t h);
    void moveSprite(uint8_t num, int16_t x, int16_t y);
    void setSpriteFrame(uint8_t num, uint8_t frame);
    void enableSprite(uint8_t num);
    void disableSprite(uint8_t num);

    static const uint8_t numModes;
    static struct vs23_mode_t modes[];
    
    inline uint16_t lastLine() { return m_last_line; }

    bool allocBacking(int w, int h, int &x, int &y);
    void freeBacking(int x, int y, int w, int h);
private:
    static void ICACHE_RAM_ATTR vsyncHandler(void);
    bool m_vsync_enabled;
    uint32_t m_cycles_per_frame;

    const struct vs23_mode_t *m_current_mode;
    uint32_t m_pitch;	// Distance between piclines in bytes
    uint32_t m_first_line_addr;
    int m_last_line;
    uint16_t m_sync_line;

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
                            uint32_t skip_x,
                            uint32_t skip_y);
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
                                                   uint32_t skip_x);

    struct sprite_t {
      struct sprite_line *pattern;
      uint16_t pat_x, pat_y;
      int16_t pos_x, pos_y;
      bool enabled;
      bool transparent;
      uint8_t w, h;
      uint8_t frame;
    } m_sprite[VS23_MAX_SPRITES];
    struct sprite_t *m_sprites_ordered[VS23_MAX_SPRITES];
    static int cmp_sprite_y(const void *one, const void *two);

    void loadSpritePattern(uint8_t num);
    void allocateSpritePattern(struct sprite_t *s);
    void freeSpritePattern(struct sprite_t *s);
    
    uint32_t m_frame;
    
    GuillotineBinPack m_bin;
};

extern VS23S010 vs23; // グローバルオブジェクト利用宣言

#endif
