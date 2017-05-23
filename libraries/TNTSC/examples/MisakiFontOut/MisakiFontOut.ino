//
// Arduino STM32 NTSCビデオ出力 サンプル V2.2
// 最終更新日 2017/02/17 たま吉さん
// Blue Pillボード(STM32F103C8)にて動作確認
//
//  修正履歴
//    2017/02/17 水平・垂直同期信号をPWM出力で行うように変更,解像度を224x216に変更
//    2017/02/20 NTSCビデオ表示部をライブラリ化
//    2017/03/03 解像度モード追加
//

#include <TNTSC.h>
#include <misakiUTF16.h>  // 美咲フォントライブラリ
   
// フォント描画
void drawFont(int x, int y, uint8_t* font) {
  uint8_t* ptr = &TNTSC.VRAM()[y*(TNTSC.width()/8)*8+x];
  for (int i=0; i<8; i++) {
    *ptr = *font;
    ptr+=(TNTSC.width()/8);
    font++;
  }
}

// 文字列描画
void drawText(int x, int y, char* str) {
  uint8_t  fnt[8];
  while(*str) {
    if (x>=(TNTSC.width()/8))
      break;
    if (! (str = getFontData(fnt, str, true)) )  {
         Serial.println("Error"); 
         break;
    }
    drawFont(x,y ,fnt);
    x++;
  }  
}

void setup(){

}

void loop(){
  char str[32];
  for (uint8_t i = 0; i <=4; i++) {
    TNTSC.begin(i);
  
    // 画面表示
    TNTSC.cls();
    drawText(0, 0,"左上");drawText((TNTSC.width()/8)-2, 0,"右上");
    drawText(0,(TNTSC.height()/8)-1,"左下");drawText((TNTSC.width()/8)-2,(TNTSC.height()/8)-1,"右下");
    drawText(0,2,"　　■■ＮＴＳＣ出力■■　　");
    drawText(0,4,"　ねこにコ・ン・バ・ン・ワ　");

    sprintf(str, "解像度：%dx%dドット", TNTSC.width(), TNTSC.height());
    drawText(0,6, str);
    drawText(0,9,"　まだまだ色々と調整中です　");

    delay(5000);   
    TNTSC.end();
  }  
}
