//
// 豊四季Tiny BASIC for Arduino STM32 シリアルデバイス制御
// 2017/07/19 by たま吉さん
//

#include <Arduino.h>
#include "tSerialDev.h"

void tSerialDev::Serial_open(uint32_t baud) {
  if (serialMode == 0) 
    Serial1.begin(baud);
//  else if (serialMode == 1) 
//    Serial.begin(baud);  
}

// シリアルポートクローズ
void tSerialDev::Serial_close() {
  if (serialMode == 0) 
    Serial1.end();  
//  else if (serialMode == 1) 
//    Serial.end();  
}

// シリアル1バイト出力
void tSerialDev::Serial_write(uint8_t c) {
  if (serialMode == 0) {
     Serial1.write( (uint8_t)c);
  } else if (serialMode == 1) {
     Serial.write( (uint8_t)c);  
  }
}

// シリアル1バイト入力
int16_t tSerialDev::Serial_read() {
  if (serialMode == 0) 
    return Serial1.read();
  else if (serialMode == 1) 
    return Serial.read();
}

// シリアル改行出力 
void tSerialDev::Serial_newLine() {
  if (serialMode == 0) {
    Serial1.write(0x0d);
    Serial1.write(0x0a);
  } else if (serialMode == 1) {
    Serial.write(0x0d);
    Serial.write(0x0a);
  }
}

// シリアルデータ着信チェック
uint8_t tSerialDev::Serial_available() {
  if (serialMode == 0) 
     return Serial1.available();
  else if (serialMode == 1) 
     return Serial.available();
}

// シリアルポートモード設定 
void tSerialDev::Serial_mode(uint8_t c, uint32_t b) {
  serialMode = c;
  if (serialMode == 1) {
    Serial1.begin(b);
  } else {
    //Serial1.end();
  }
}

