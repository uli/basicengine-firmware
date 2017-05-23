//
// FILE stm31_testFlash
// フラッシュメモリ書き込みテスト for Arduino STM32
// 作成日 2017/03/16 by たま吉さん
//

#define FLASH_PAGE_SIZE        1024
#define FLASH_START_ADDRESS    ((uint32)(0x8000000))

#include <string.h>
#include "stm32_hexedit.h"
#include "TFlash.h"

uint8_t str1[] = "1234567890A";
uint8_t str2[] = "abcdefghij";

void Arduino_putchar(uint8_t c) {
  Serial.write(c);
}

char Arduino_getchar() {
  char c;
  while (!Serial.available());
  return Serial.read();
}

uint32_t adr0 = FLASH_START_ADDRESS + FLASH_PAGE_SIZE *  63;
uint32_t adr1 = FLASH_START_ADDRESS + FLASH_PAGE_SIZE *  127;

void setup() {
  Serial.begin(115200);
  while (!Serial.isConnected()) delay(100);
  setFunction_putchar(Arduino_putchar); 
  setFunction_getchar(Arduino_getchar); 
  initscr();

  // フラッシュメモリ書き込みテスト
  TFlash.unlock();
  TFlash.eracePage(adr0);
  TFlash.write((uint16_t*)adr0, str1, strlen((char*)str1));
  TFlash.eracePage(adr1);
  TFlash.write((uint16_t*)adr1, str2, strlen((char*)str2));
  TFlash.lock();
}

void loop() {
  // 64kバイトフラッシュメモリ最終ページの参照
  clear();
  hexedit2 (adr0, false);

  // 128kバイトフラッシュメモリ最終ページの参照
  clear();
  hexedit2 (adr1, false);  
}
