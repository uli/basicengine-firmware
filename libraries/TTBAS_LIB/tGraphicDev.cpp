//
// 豊四季Tiny BASIC for Arduino STM32 グラフィック描画デバイス
// 2017/07/19 by たま吉さん
//

#include "../../ttbasic/ttconfig.h"

#include "tGraphicDev.h"

uint16_t tv_get_gwidth();
uint16_t tv_get_gheight();
void    tv_pset(int16_t x, int16_t y, uint8_t c);
void    tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c);
void    tv_circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f);
void    tv_rect(int16_t x, int16_t y, int16_t h, int16_t w, uint8_t c, int8_t f) ;
void    tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d);
void    tv_set_gcursor(uint16_t x, uint16_t y);
void    tv_gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) ;
int16_t tv_gpeek(int16_t x, int16_t y);
#if USE_VS23 == 0
uint8_t* tv_getGVRAM();
uint16_t tv_getGVRAMSize();
#endif
void    tv_write(uint8_t c);

// 初期化
void tGraphicDev::init() {
  gwidth   = tv_get_gwidth();
  gheight  = tv_get_gheight();  
}

#if USE_VS23 == 0
// グラフィク表示用メモリアドレス参照
uint8_t* tGraphicDev::getGRAM() {
  return tv_getGVRAM();
}

// グラフィク表示用メモリサイズ取得
int16_t tGraphicDev::getGRAMsize() {
  return tv_getGVRAMSize();
}
#endif

// グラフックスクリーン横幅取得
uint16_t tGraphicDev::getGWidth() {
	return gwidth;
}

// グラフックスクリーン縦幅取得
uint16_t tGraphicDev::getGHeight() {
	return gheight;
} 

// ドット描画
void tGraphicDev::pset(int16_t x, int16_t y, uint8_t c) {
  tv_pset(x,y,c);
}

// 線の描画
void tGraphicDev::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
 tv_line(x1,y1,x2,y2,c);
}

// 円の描画
void tGraphicDev::circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  tv_circle(x, y, r, c, f);
}

// 四角の描画
void tGraphicDev::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f) {
  tv_rect(x, y, w, h, c, f);
}

// グラフィックスクロール
void tGraphicDev::gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  tv_gscroll(x, y, w, h, mode);
}

// 指定位置のピクセル情報取得
int16_t tGraphicDev::gpeek(int16_t x, int16_t y) {
  return tv_gpeek(x,y);
}

void tGraphicDev::set_gcursor(uint16_t x, uint16_t y) {
  tv_set_gcursor(x,y);
}

void tGraphicDev::gputch(uint8_t c) {
 tv_write(c); 
}
