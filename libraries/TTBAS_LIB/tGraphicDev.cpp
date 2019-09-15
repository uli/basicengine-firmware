//
// 豊四季Tiny BASIC for Arduino STM32 グラフィック描画デバイス
// 2017/07/19 by たま吉さん
//

#include "../../ttbasic/ttconfig.h"

#include "tGraphicDev.h"
#include "tvutil.h"

// 初期化
void tGraphicDev::init() {
  gwidth   = tv_get_gwidth();
  gheight  = tv_get_gheight();  
}

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
void tGraphicDev::circle(int16_t x, int16_t y, int16_t r, uint8_t c, int f) {
  tv_circle(x, y, r, c, f);
}

// 四角の描画
void tGraphicDev::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int f) {
  tv_rect(x, y, w, h, c, f);
}

// グラフィックスクロール
void tGraphicDev::gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  tv_gscroll(x, y, w, h, mode);
}

void tGraphicDev::set_gcursor(uint16_t x, uint16_t y) {
  tv_set_gcursor(x,y);
}

void tGraphicDev::gputch(uint8_t c) {
 tv_write(c); 
}
