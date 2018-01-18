//
// file: TPS2.h
// Arduino STM32用 PS/2インタフェース by たま吉さん
// 作成日 2017/01/31
// 修正日 2017/02/04, rcev()関数の追加
// 修正日 2017/02/05, send(),setPriority()関数の追加
//

#ifndef __TPS2_H__
#define __TPS2_H__

#include <Arduino.h>
#define QUEUESIZE 128 // キーバッファサイズ

// PS/2バスの定義

class TPS2 {
  // 定数
  public:
    static const uint8_t D_IN  = 0;  // 入出力方向(IN)
    static const uint8_t D_OUT = 1;  // 入出力方向(OUT)

    static const uint8_t B_IDL =  0; // バス_アイドル
    static const uint8_t B_STOP = 1; // バス_通信禁止
    static const uint8_t B_HSND = 2; // バス_ホスト送信
    
  // メンバー変数
  private:

  // メンバー関数
  public:

    // ポート設定
    inline void  setPort(uint8_t clk, uint8_t dat); // GPIOピンの設定

    void  begin(uint8_t clk, uint8_t dat); // 初期化
    void  end(); // 終了

    // 入出力方向設定
    inline static void  clkSet_Out();     // CLKを出力モードに設定
    inline static void  datSet_Out();     // DATを出力モードに設定
    inline static void  clkSet_In();      // CLKを入力モードに設定
    inline static void  datSet_In();      // DATを入力モードに設定

    // 入出力
    inline static void  Clk_Out(uint8_t val); // CLKから出力
    inline static void  Dat_Out(uint8_t val); // DATから出力
    inline static uint8_t Clk_In();           // CLKから入力
    inline static uint8_t Dat_In();           // データから入力
    
    // ラインの制御
    static void  mode_idole(uint8_t dir);     // アイドル状態に設定
    static void  mode_stop();                 // 通信禁止
    static void  mode_send();                 // ホスト送信モード

    // 状態待ち
    static uint8_t wait_Clk(uint8_t val, uint32_t tm);  // CLKの状態変化待ち
    static uint8_t wait_Dat(uint8_t val, uint32_t tm);  // DATの状態待ち

    // データの入出力
    static uint8_t hostSend(uint8_t data); // データ送信
    static uint8_t HostRcev(uint8_t* rcv); // データ受信
    static uint8_t response(uint8_t* rcv); // 応答受信
    static uint8_t rcev(uint8_t* rcv); // データ受信((キューからの取り出し、割り込み経由)
    static uint8_t send(uint8_t data); // データ送信(割り込み経由)
    
    // 割り込み制御
    static void enableInterrupts();   // CLK変化割り込み許可
    static void disableInterrupts();  // CLK変化割り込み禁止
    static void setPriority(uint8_t n);  // 割り込み優先レベルの設定
    
    // 割り込みハンドラ
    static void clkPinHandle();     // クロックピン状態変化ハンドラ
    static void clkPinHandleSend(); // クロックピン状態変化ハンドラ送信サブルーチン
    
    // キーバッファ操作
    static void clear_queue();             // キューのクリア
    static uint8_t enqueue(uint8_t data);  // キューへの挿入
    static uint8_t dequeue();              // キューからの取出し
    static uint8_t available();            // 取出し可能チェック

    // エラー情報取得
    inline static uint8_t error() ;

};
#endif
