//
// File: TFlash.h
// Arduino STM32 内部フラッシュメモリ書き込みライブラリ V1.0
// 作成日 2017/03/16 by たま吉さん
//

#ifndef __TFLASH_H__
#define __TFLASH_H__

// FLASHメモリステータス
typedef enum  {
    TFLASH_BUSY = 1,    // 処理中
    TFLASH_ERROR_PG,    // 書込み失敗
    TFLASH_ERROR_WRP,   // 書込み保護による書き込み失敗
    TFLASH_ERROR_OPT,   // 操作異常
    TFLASH_COMPLETE,    // 書込み完了
    TFLASH_TIMEOUT,     // 書込み完了待ちタイムアウト
    TFLASH_BAD_ADDRESS  // 不当操作
} TFLASH_Status;

  
class TFlash_Class {
  public:
    TFLASH_Status eracePage(uint32_t pageAddress);
    TFLASH_Status write(uint16_t* adr, uint16_t data);
    TFLASH_Status write(uint16_t* adr, uint8_t* data, uint16_t len);
    uint16_t read(uint16_t* adr) {return *((uint16_t*)adr);};
    
    void lock();
    void unlock();

  private:
    TFLASH_Status waitOperation(uint32_t tm);
    TFLASH_Status getStatus(); 
};

extern TFlash_Class TFlash;

#endif
