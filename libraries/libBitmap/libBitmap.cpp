// libBitmap.cpp
// ビットマップ操作ライブラリ V 0.1
// 作成 2015/01/25 by たま吉さん
// 修正 2016/05/16 by たま吉さん, rotateBitmap()のバグ修正
// 修正 2016/06/26 by たま吉さん, setBitmapAt()のバグ修正
// 修正 2016/08/19 by たま吉さん, Arduino IDE用ライブラリ化
// 修正 2016/08/19 by たま吉さん, setBitmapAt()のバグ修正(8x8ドット時に不具合発生)
// 修正 2016/08/19 by たま吉さん, scrollInFont()の追加
// 修正 2016/09/17 by たま吉さん, scrollInFont()のmode 0時の無駄処理修正

#include <Arduino.h>
#include <libBitmap.h>
// 任意サイズのビットマップにドットをセット
void setDotAt(uint8_t* bmp, uint16_t w, uint16_t h, int16_t x, int16_t y, uint8_t d) {
  if (x < 0 || y < 0 || x >= w || y >= h)
    return;

  uint16_t bl = (w+7)>>3;           // 横必要バイト数
  uint16_t addr = bl*y + (x>>3);    // 書込みアドレス
  if (d) 
    bmp[addr] |= 0x80>>(x%8);
  else
    bmp[addr] &= ~(0x80>>(x%8));
}


// 任意サイズのビットマップに任意サイズのパターンをセット(OR書き)
void setBitmapAt(
 uint8_t *dstbmp, uint16_t dstw, uint16_t dsth, int16_t dstx, int16_t dsty,
 uint8_t *srcbmp, uint16_t srcw, uint16_t srch)
{
  if (dsty+srch <=0 || dstx+srcw <=0 || dsty >= (uint8_t)dsth || dstx >= (uint8_t)dstw )
     return;
 
  uint16_t src_bl = (srcw+7)>>3;      // パターンの横バイト数
  uint16_t dst_bl = (dstw+7)>>3;      // バッファの横バイト数
  uint16_t src_addr, dst_addr;
  uint8_t d;
  
  for (int16_t y=0; y < srch; y++) {
    if (dsty+y < dsth) {
      for (int16_t x=0; x < src_bl; x++) {
        if (((dstx/8)+x >=0) && ((dstx/8)+x < dst_bl)) {
          src_addr = src_bl*y + x;        
          dst_addr = dst_bl*(dsty+y) + (dstx/8)+x;
          d = srcbmp[src_addr];
          if (dstx>=0) {
            d>>=(dstx%8);
          } else {
          	d<<=((-dstx)%8);          
          }          
          dstbmp[dst_addr] |= d;

          if (dst_bl>1) { 	
	          if (dstx>=0 && x > 0) {
	            d = srcbmp[src_addr-1];
	            d<<=(8-(dstx%8));
	          } else if (dstx<0 && x < src_bl) {
	            d = srcbmp[src_addr+1];
	            d>>=(8-(-dstx%8));
	          }                  
	          dstbmp[dst_addr] |= d;          
          }
        }
      }
    }
  }  
}

// 任意サイズバッファのスクロール
//  bmp: スクロール対象バッファ
//  w:   バッファの幅(ドット)
//  h:   バッファの高さ(ドット)
//  mode: B0001 左 ,B0010 右, B0100 上, B1000 下 … OR で同時指定可能
void scrollBitmap(uint8_t *bmp, uint16_t w, uint16_t h, uint8_t mode) {
  uint16_t bl = (w+7)>>3;           // 横バイト数
  uint16_t addr;                    // データアドレス
  uint8_t prv_bit;
  uint8_t d;

  // 横
  if ((mode & B11) == B01) {  // 左スクロール
    addr=0;
    for (uint8_t i=0; i < h;i++) {
      prv_bit = 0;
      for (int8_t j=0; j < bl; j++) {
        d = bmp[addr+bl-1-j];
        bmp[addr+bl-1-j]<<=1;
        if (j>0)
          bmp[addr+bl-1-j] |= prv_bit;
        prv_bit=d>>7;
      }
      addr+=bl;
    }
  } else if ((mode & B11) == B10) { // 右スクロール
    addr=0;
    for (uint8_t i=0; i < h;i++) {
      prv_bit = 0;
      for (int8_t j=0; j < bl; j++) {
        d = bmp[addr+j];
        bmp[addr+j]>>=1;
        if (j>0)
          bmp[addr+j] |= prv_bit;
        prv_bit=d<<7;
        
       // Serial.print("addr+j=");
       // Serial.println(addr+j,DEC);
      }
      addr+=bl;
    } 
  }

  // 縦
  if ((mode & B1100) == B0100) {  // 上スクロール
    addr=0;   
    for (uint8_t i=0; i<h-1;i++) {
      for (int8_t j=0; j < bl; j++) {
        bmp[addr+j] = bmp[addr+j+bl];
      }
      addr+=bl;
    }
    for (int8_t j=0; j < bl; j++) {
      bmp[addr+j] = 0;
    }
  } else if ((mode & B1100) == B1000) { // 下スクロール
    addr=bl*(h-1);
    for (uint8_t i=0; i<h-1;i++) {
      for (int8_t j=0; j < bl; j++) {
        bmp[addr+j] = bmp[addr-bl+j];
      }
      addr-=bl;
    }
    for (int8_t j=0; j < bl; j++) {
      bmp[j] = 0;
    } 
  }
}

// 任意サイズビットマップのドットON/OFF反転
//  bmp: スクロール対象バッファ
//  w:   バッファの幅(ドット)
//  h:   バッファの高さ(ドット)
void revBitmap(uint8_t *bmp, uint16_t w, uint16_t h) {
  uint16_t bl = (w+7)>>3;           // 横バイト数
  uint16_t addr;                    // データアドレス
  uint8_t d;
  addr=0;
  for (uint8_t i=0; i <h; i++) {
    for (uint8_t j=0; j <bl; j++) {
      d = ~bmp[addr+j];     
      if (j+1 == bl && (w%8)!=0) {
        d &= 0xff<<(8-(w%8));
      }
      bmp[addr+j]=d;
    }
    addr+=bl;
  }
}

// 任意サイズビットマップの指定座標のドットを取得
//  bmp:  スクロール対象バッファ
//  w:    バッファの幅(ドット)
//  h:    バッファの高さ(ドット)
//  x,y:  座標
uint8_t getdotBitmap(uint8_t *bmp, uint16_t w, uint16_t h, int16_t x, int16_t y) {
  if (x>=w || y>=h || x <0 ||  y < 0) 
    return 0;

  uint16_t bl = (w+7)>>3;           // 横バイト数    
  uint8_t d;
  
  d = bmp[y*bl + (x/8)];
  if (d & (0x80>>(x%8)))
    return 1;
  else 
    return 0;
}

// 任意サイズビットマップの回転
//  bmp:  スクロール対象バッファ
//  w:    バッファの幅(ドット)
//  h:    バッファの高さ(ドット)
//  mode: B00 なし, B01 反時計90° B10 反時計180° B11 反時計270°  
void rotateBitmap(uint8_t *bmp, uint16_t w, uint16_t h, uint8_t mode) {
  if (mode == B00 || w != h || mode > B11)
   return;  

  uint16_t bl = (w+7)>>3;           // 横バイト数  
  uint8_t tmpbmp[h*bl];
  uint8_t d;
  memset(tmpbmp,0,h*bl);
  if (mode == B01) {  // 反時計90°
    for (int16_t x = 0; x < w; x++) {
      for (int16_t y = 0; y < h; y++) {
       d = getdotBitmap(bmp, w, h, x, y);
       setDotAt(tmpbmp, w, h, w-y-1, x, d);
      }
    }
  } else if (mode == B10) { // 反時計180°
    for (int16_t x = 0; x < w; x++) {
      for (int16_t y = 0; y < h; y++) {
       d = getdotBitmap(bmp, w, h, x, y);
       setDotAt(tmpbmp, w, h, w-x-1, h-y-1, d);
      }
    }
  } else {  // 反時計270°
    for (int16_t x = 0; x < w; x++) {
      for (int16_t y = 0; y < h; y++) {
       d = getdotBitmap(bmp, w, h, x, y);
       setDotAt(tmpbmp, w, h, y, h-x-1,d);
      }
    }
  }
  memcpy(bmp,tmpbmp,h*bl);
}

// 任意サイズのビットマップの任意領域のクリア
void clearBitmapAt(uint8_t* bmp, uint16_t w, uint16_t h, int16_t x, int16_t y, uint8_t cw, uint8_t ch) {
 
  uint16_t bl = (w+7)>>3;           // 横バイト数  
  uint8_t tmpbmp[h*bl];
  uint8_t d;
  
  //if (x < 0 || y < 0 || x+cw >w || y+ch >h)
  //  return;
 
  for (uint16_t i=0; i <ch; i++) {
    for (uint16_t j=0; j <cw; j++) {
       setDotAt(bmp, w, h, x+j, y+i, 0);
    }
  } 
}

// 1文字分スクロール挿入表示
void scrollInFont(uint8_t*dst, uint8_t dw, uint8_t dh, uint8_t *src, uint8_t sw, uint8_t sh, uint16_t dt, uint8_t mode) {
  int16_t px = 0,py = 0;
  int16_t s;

  if (mode & B11) {
    s = sw;
  } else if (mode & B1100) {
    s = sh;
  } 

  if (mode) {
    for (int16_t t = 0; t < s; t++) {
      scrollBitmap(dst, dw, dh, mode); 
      if (mode & B0001) px = sw-t-1;
      if (mode & B0010) px = t-sw+1;
      if (mode & B0100) py = sh-t-1;
      if (mode & B1000) py = t-sh+1;  
      clearBitmapAt(dst, dw, dh, px, py, sw, sh);
      setBitmapAt(dst, dw, dh, px, py, src, sw, sh);
      delay(dt);
   }  
  } else {
    clearBitmapAt(dst, dw, dh, 0, 0, sw, sh);
    setBitmapAt(dst, dw, dh, 0, 0, src, sw, sh); 
  	delay(dt*sw);
  }	
}
