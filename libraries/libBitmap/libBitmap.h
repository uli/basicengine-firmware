// libBitmap.h
// ビットマップ操作ライブラリ V 0.2
// 作成 2015/01/25 by たま吉さん
// 修正 2016/05/16 by たま吉さん, rotateBitmap()のバグ修正
// 修正 2016/06/26 by たま吉さん, setBitmapAt()のバグ修正
// 修正 2016/06/26 by たま吉さん, setBitmapAt()のバグ修正
// 修正 2016/08/19 by たま吉さん, Arduino IDE用ライブラリ化
// 修正 2016/08/19 by たま吉さん, setBitmapAt()のバグ修正(8x8ドット時に不具合発生)
// 修正 2016/08/19 by たま吉さん, scrollInFont()の追加

#ifndef ___libBitmap_h___
 #define ___libBitmap_h___

#include <Arduino.h>

//
// ビットマップ操作関数宣言
//
 
void setDotAt(uint8_t* bmp, uint16_t w, uint16_t h, int16_t x, int16_t y, uint8_t d);
void setBitmapAt(uint8_t *dstbmp, uint16_t dstw, uint16_t dsth, int16_t dstx, int16_t dsty,uint8_t *srcbmp, uint16_t srcw, uint16_t srch);
void scrollBitmap(uint8_t *bmp, uint16_t w, uint16_t h, uint8_t mode);
void revBitmap(uint8_t *bmp, uint16_t w, uint16_t h);
void clearBitmapAt(uint8_t* bmp, uint16_t w, uint16_t h, int16_t x, int16_t y, uint8_t cw, uint8_t ch);
uint8_t getdotBitmap(uint8_t *bmp, uint16_t w, uint16_t h, int16_t x, int16_t y);
void rotateBitmap(uint8_t *bmp, uint16_t w, uint16_t h, uint8_t mode) ;
void scrollInFont(uint8_t*dst, uint8_t dw, uint8_t dh, uint8_t *src, uint8_t sw, uint8_t sh, uint16_t dt, uint8_t mode);
#endif
