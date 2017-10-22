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

#define SC_DEFAULT 0

struct vs23_mode_t {
  uint16_t x;
  uint16_t y;
  uint8_t top;
  uint8_t left;
  uint8_t vclkpp;
  uint8_t bextra;
};

#define VS23_MAX_X	456
#define VS23_MAX_Y	224

#define VS23_MAX_BG	2
#define VS23_MAX_SPRITES 16

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

    inline uint32_t piclineByteAddress(uint16_t line) {
        return PICLINE_BYTE_ADDRESS(line);
    }
    inline uint16_t piclinePitch() {
        return PICLINE_LENGTH_BYTES + BEXTRA;
    }

    inline uint16_t currentLine() {
        return SpiRamReadRegister(CURLINE) & 0xfff;
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t c);
    uint8_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b);

    void setMode(uint8_t mode);
    void calibrateVsync();
    void setSyncLine(uint16_t line);
 
    void SpiRamVideoInit();
    void SetPixyuv(uint16_t xpos, uint16_t ypos, uint16_t yuv);
    void SetPixel(uint16_t xpos, uint16_t ypos, uint16_t r, uint16_t g, uint16_t b);
    void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t g, uint16_t b);
    void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t rgb);
    uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels);
    void TvFilledRectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *texture, uint16_t color);
    void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);
    void MoveBlockFast(uint16_t x_src, uint16_t y_src, int16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height);
    
    bool setBg(uint8_t bg, uint16_t width, uint16_t height,
                     uint8_t tile_size_x, uint8_t tile_size_y,
                     uint16_t pat_x, uint16_t pat_y, uint16_t pat_w);
    void enableBg(uint8_t bg);
    void disableBg(uint8_t bg);
    inline void scroll(uint8_t bg, uint16_t x, uint16_t y) {
      m_bg[bg].scroll_x = x;
      m_bg[bg].scroll_y = y;
    }
    void updateBg();

    void sprite(uint8_t num, uint16_t pat_x, uint16_t pat_y, uint16_t pos_x, uint16_t pos_y, uint8_t w, uint8_t h);

    const struct vs23_mode_t *currentMode;
    static const uint8_t numModes;
    static const struct vs23_mode_t modes[];

private:
    bool m_vsync_enabled;
    uint32_t cyclesPerFrame;
    static void ICACHE_RAM_ATTR vsyncHandler(void);
    uint16_t syncLine;

    struct bg_t {
      uint8_t *tiles;
      uint16_t pat_x, pat_y, pat_w;
      uint8_t w, h;
      uint8_t tile_size_x, tile_size_y;
      uint16_t scroll_x, scroll_y;
      uint16_t old_scroll_x, old_scroll_y;
      uint16_t win_x, win_y, win_w, win_h;
      bool enabled;
      bool force_redraw;
    } m_bg[VS23_MAX_BG];

    void ICACHE_RAM_ATTR drawBg(struct bg_t *bg,
                            uint32_t pitch,
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
                            uint32_t pitch,
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
                                                   uint32_t ypoff);

    struct sprite_t {
      uint16_t pat_x, pat_y;
      uint16_t pos_x, pos_y;
      bool enabled;
      uint16_t old_pos_x, old_pos_y;
      bool old_enabled;
      uint8_t w, h;
      uint8_t *pattern;
    } m_sprite[VS23_MAX_SPRITES];
    
    uint32_t m_frame;
};

extern VS23S010 vs23; // グローバルオブジェクト利用宣言

#endif
