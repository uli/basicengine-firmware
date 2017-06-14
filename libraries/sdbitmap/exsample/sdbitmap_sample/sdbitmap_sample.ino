//
// sdbitmap_sample.ino
// SDカードビットマップファイル操作ライブラリサンプルスケッチ
// たま吉さん 2016/06/21
//
// 概要: SDカード内の画像ファイル LOGO1.BMP,LOGO2.BMP の内容をコンソール出力します。
//

#include <SD.h>
#include <sdbitmap.h>

#define CS_SD     (10)    // SDカードモジュールCS

// ビットパターン表示
// d: 8ビットパターンデータ
void bitdisp(uint8_t d) {
  for (uint8_t i=0; i<8;i++) {
    if (d & 0x80>>i) 
      Serial.print("#");
    else
      Serial.print(" ");
  }
}

// ビットデータデータの表示
// buf:ビットデータ 
// w  :幅(ピクセル）
// h  :高(ピクセル）
//
void bmpdisp(uint8_t *buf, uint16_t w, uint16_t h) {

  uint16_t bn = (w+7)>>3; // 1ライン当たりのバイト数
  uint16_t ln = bn*h;     // 総バイト数

  Serial.print(w,DEC);
  Serial.print("x");      
  Serial.println(h,DEC);      
  for (uint16_t i = 0; i < ln; i += bn ) {
      for (uint16_t j = 0; j < bn; j++) {
        bitdisp(buf[i+j]);
      }
      Serial.println();
  }
  Serial.println();
} 

//
// getBitmap(uint8_t*bmp, uint8_t mode) のテスト
//
void test1() {
  uint8_t tmpbuf[600];        // 表示パターンバッファデータ
  uint16_t w,h,sz;

  memset(tmpbuf,0,600);
  sdbitmap bitmap;
  bitmap.setFilename("LOGO2.BMP");

  // ビットマップデータオープン
  bitmap.open();
  w  = bitmap.getWidth();
  h  = bitmap.getHeight();
  sz = bitmap.getImageSize();
 
  // ビットデータロード
  bitmap.getBitmap(tmpbuf,1);

  // パターン表示
  Serial.print("FILE:LOGO1.BMP size=");
  Serial.print(w,DEC);
  Serial.print("x");
  Serial.print(h,DEC);
  Serial.print(" ");
  Serial.print(sz,DEC);
  Serial.println("byte");
  bmpdisp(tmpbuf,w,h);
  bitmap.close(); 
}

//
//  sdbitmap::getBitmapEx(uint8_t*bmp, uint16_t x, uint16_t y, uint8_t w, uint8_t h, uint8_t mode) のテスト
//
void test2() {
  uint8_t tmpbuf[100];        // 表示パターンバッファデータ
  uint16_t w,h,sz;

  memset(tmpbuf,0,100);
  sdbitmap bitmap;
  bitmap.setFilename("LOGO1.BMP");

  // ビットマップデータオープン
  bitmap.open();
  w  = bitmap.getWidth();
  h  = bitmap.getHeight();
  sz = bitmap.getImageSize();

  Serial.print("FILE:LOGO1.BMP size=");
  Serial.print(w,DEC);
  Serial.print("x");
  Serial.print(h,DEC);
  Serial.print(" ");
  Serial.print(sz,DEC);
  Serial.println("byte");

  for (uint16_t i = 0; i < h; i+=17) {
    for (uint16_t j = 0; j < w; j+= 32) {
      // ビットデータロード
      bitmap.getBitmapEx(tmpbuf, j, i, 32, 17,1);
      // パターン表示
      bmpdisp(tmpbuf,32, 17);
    }
  }   
  bitmap.close(); 
}

void setup() {
  Serial.begin(115200);
  SD.begin(CS_SD);
  test1();
  test2();
}

void loop() {

}
