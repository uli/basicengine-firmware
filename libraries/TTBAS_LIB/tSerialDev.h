//
// 豊四季Tiny BASIC for Arduino STM32 シリアルデバイス制御
// 2017/07/19 by たま吉さん
//

#ifndef __tSerialDev_h__
#define __tSerialDev_h__

class tSerialDev {
  protected:
    uint8_t serialMode;         // シリアルポートモード
    uint8_t allowCtrl;          // シリアルからの入力制御許可

  public:
    // シリアルポート制御
    void    Serial_open(uint32_t baud) ;  // シリアルポートオープン
    void    Serial_close();               // シリアルポートクローズ
    void    Serial_write(uint8_t c);      // シリアル1バイト出力
    int16_t Serial_read();                // シリアル1バイト入力
    uint8_t Serial_available();           // シリアルデータ着信チェック
    void    Serial_newLine();             // シリアル改行出力
    void    Serial_mode(uint8_t c, uint32_t b);    // シリアルポートモード設定
	uint8_t getSerialMode() { return serialMode;}; // シリアルポートモードの参照
};

#endif

