//
// File: TFlash.cpp
// Arduino STM32 内部フラッシュメモリ書き込みライブラリ V1.0
// 作成日 2017/03/16 by たま吉さん
//

#include <Arduino.h>
#include "libmaple/util.h"
#include "libmaple/flash.h"
#include <wirish.h>
#include <TFlash.h>

#define IS_TFLASH_ADDRESS(ADDRESS) (((ADDRESS) >= 0x08002000) && ((ADDRESS) < 0x0807FFFF))

#define FLASH_KEY1      ((uint32)0x45670123)
#define FLASH_KEY2      ((uint32)0xCDEF89AB)

#define EraseTimeout    ((uint32)0x00000FFF)
#define ProgramTimeout  ((uint32)0x0000001F)

// 命令実行時間待ち
static void delay(void) {
  for(uint8_t i = 0xFF; i != 0; i--) { }
}

// ステータスの取得
TFLASH_Status TFlash_Class::getStatus() {
  if ((FLASH_BASE->SR & FLASH_SR_BSY) == FLASH_SR_BSY)
    return TFLASH_BUSY;

  if ((FLASH_BASE->SR & FLASH_SR_PGERR) != 0)
    return TFLASH_ERROR_PG;

  if ((FLASH_BASE->SR & FLASH_SR_WRPRTERR) != 0 )
    return TFLASH_ERROR_WRP;

  if ((FLASH_BASE->SR & FLASH_OBR_OPTERR) != 0 )
    return TFLASH_ERROR_OPT;

  return TFLASH_COMPLETE;
}

// 処理完了待ち(タイムアウト付き)
TFLASH_Status TFlash_Class::waitOperation(uint32_t tm) {
  TFLASH_Status status;

  status = getStatus();
  while ((status == TFLASH_BUSY) && (tm != 0x00)) {
    delay();
    status = getStatus();
    tm--;
  }
  if (tm == 0)
    status = TFLASH_TIMEOUT;
  return status;  
}

// 指定ページの消去
TFLASH_Status TFlash_Class::eracePage(uint32_t pageAddress) {
  TFLASH_Status status = TFLASH_COMPLETE;

  // アドレスの有効性チェック
  if (!IS_TFLASH_ADDRESS(pageAddress))
    return TFLASH_ERROR_PG;
 
  status = waitOperation(EraseTimeout);
  
  if(status == TFLASH_COMPLETE)  {
    // 指定ページのデータ消去の実行
    FLASH_BASE->CR |= FLASH_CR_PER;
    FLASH_BASE->AR = pageAddress;
    FLASH_BASE->CR |= FLASH_CR_STRT;

    // 消去完了待ち
    status = waitOperation(EraseTimeout);
    if(status != TFLASH_TIMEOUT) {
      FLASH_BASE->CR &= ~FLASH_CR_PER;
    }
    FLASH_BASE->SR = (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
  }
  return status;  
}

// 指定アドレスへの16ビットデータ書き込み
TFLASH_Status TFlash_Class::write(uint16_t* adr, uint16_t data) {
  TFLASH_Status status = TFLASH_BAD_ADDRESS;
  if (IS_TFLASH_ADDRESS(((uint32_t)adr))) {
    status = waitOperation(ProgramTimeout);
    if(status == TFLASH_COMPLETE)  {
      // データの書込み
      FLASH_BASE->CR |= FLASH_CR_PG;
      *(__io uint16*)adr = data;
      // 書込み完了待ち
      status = waitOperation(ProgramTimeout);
      if(status != TFLASH_TIMEOUT) {
        FLASH_BASE->CR &= ~FLASH_CR_PG; // 書込み禁止
      }
      FLASH_BASE->SR = (FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
    }
  }
  return status;  
}

// 指定アドレスへのバイトデータ列の書込み
TFLASH_Status TFlash_Class::write(uint16_t* adr, uint8_t* data, uint16_t len) {
  TFLASH_Status status = TFLASH_COMPLETE;
  uint16_t d1,d2;
  
  for (uint32_t i = 0; i < len/2; i++) {
    d1 = *data++;
    d2 = *data++;
    status = write(adr++, d1|(d2<<8));
    if (status != TFLASH_COMPLETE) 
      return status;
  }
  if (len&1) {
    // 端数バイトの処理
    d1 = *data;
    d2 = 0xff;
    status = write(adr, d1|(d2<<8));    
  }
  return status;
}

// 書込みロック
void TFlash_Class::lock() {
  FLASH_BASE->CR |= FLASH_CR_LOCK;  
}

// 書込みアンロック
void TFlash_Class::unlock() {
  FLASH_BASE->KEYR = FLASH_KEY1;
  FLASH_BASE->KEYR = FLASH_KEY2;  
}

TFlash_Class TFlash;

