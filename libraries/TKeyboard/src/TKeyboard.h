//
// file: TKeyboard.h
// Arduino STM32用 PS/2 キーボード制御 by たま吉さん
// 作成日 2017/01/31
// 修正日 2017/02/05, setPriority()関数の追加
// 修正日 2017/02/05, KEY_SPACEのキーコード変更(32=>35)
//

#ifndef __TKEYBOARD_H__
#define __TKEYBOARD_H__
#include <TPS2.h>
#include <Arduino.h>

#if defined(ESP8266) && !defined(__FLASH__)
#define __FLASH__ ICACHE_RODATA_ATTR
#endif

// 状態管理用
#define BREAK_CODE       0x0100  // BREAKコード
#define PS2KEY_CODE         0x0200  // KEYコード
#define SHIFT_CODE       0x0400  // SHIFTあり
#define CTRL_CODE        0x0800  // CTRLあり
#define ALT_CODE         0x1000  // ALTあり
#define GUI_CODE         0x2000  // GUIあり

#define PS2KEY_ERROR          0xFF  // キーコードエラー
#define PS2KEY_NONE              0  // 継続またはバッファに変換対象なし

// キーコード定義(JIS配列キーボード)
#define PS2KEY_L_Alt		    1	// [左Alt]
#define PS2KEY_L_Shift		  2	// [左Shift]
#define PS2KEY_L_Ctrl		  3	// [左Ctrl]
#define PS2KEY_R_Shift		  4	// [右Shift]
#define PS2KEY_R_Alt		    5	// [右Alt]
#define PS2KEY_R_Ctrl		  6	// [右Ctrl]
#define PS2KEY_L_GUI		    7	// [左Windowsキー]
#define PS2KEY_R_GUI		    8	// [右Windowsキー]
#define PS2KEY_NumLock		  9	// [NumLock]
#define PS2KEY_ScrollLock	10	// [ScrollLock]
#define PS2KEY_CapsLock	  11	// [CapsLock]
#define PS2KEY_PrintScreen	12	// [PrintScreen]
#define PS2KEY_HanZen		  13	// [半角/全角 漢字]
#define PS2KEY_Insert		  14	// [Insert]
#define PS2KEY_Home		    15	// [Home]
#define PS2KEY_Pause		    16	// [Pause]
#define PS2KEY_Romaji		  17	// [カタカナ ひらがな ローマ字]
#define PS2KEY_APP			    18	// [メニューキー]
#define PS2KEY_Henkan		  19	// [変換]
#define PS2KEY_Muhenkan	  20	// [無変換]
#define PS2KEY_PageUp		  21	// [PageUp]
#define PS2KEY_PageDown	  22	// [PageDown]
#define PS2KEY_End			    23	// [End]
#define PS2KEY_L_Arrow		  24	// [←]
#define PS2KEY_Up_Arrow	  25	// [↑]
#define PS2KEY_R_Arrow		  26	// [→]
#define PS2KEY_Down_Arrow	27	// [↓]

#define PS2KEY_ESC			    30	// [ESC]
#define PS2KEY_Tab 		    31	// [Tab]
#define PS2KEY_Enter       32  // [Enter]
#define PS2KEY_Space		    35	// [空白]
#define PS2KEY_Backspace	  33	// [BackSpace]
#define PS2KEY_Delete		  34	// [Delete]

#define PS2KEY_Colon		    36	// [: *]
#define PS2KEY_Semicolon	  37	// [; +]
#define PS2KEY_Kamma		    38	// [, <]
#define PS2KEY_minus		    39	// [- =]
#define PS2KEY_Dot			    40	// [. >]
#define PS2KEY_Question	  41	// [/ ?]
#define PS2KEY_AT			    42	// [@ `]
#define PS2KEY_L_brackets	43	// [[ {]
#define PS2KEY_Pipe		    44	// [\ |]
#define PS2KEY_R_brackets	45	// [] }]
#define PS2KEY_Hat			    46	// [^ ~]
#define PS2KEY_Ro			    47	// [\ _ ろ]

#define PS2KEY_0			48	// [0 )]
#define PS2KEY_1			49	// [1 !]
#define PS2KEY_2			50	// [2 @]
#define PS2KEY_3			51	// [3 #]
#define PS2KEY_4			52	// [4 $]
#define PS2KEY_5			53	// [5 %]
#define PS2KEY_6			54	// [6 ^]
#define PS2KEY_7			55	// [7 &]
#define PS2KEY_8			56	// [8 *]
#define PS2KEY_9			57	// [9 (]
#define PS2KEY_Pipe2 58  // [\ |] (USキーボード用)
#define PS2KEY_A			65	// [a A]
#define PS2KEY_B			66	// [b B]
#define PS2KEY_C			67	// [c C]
#define PS2KEY_D			68	// [d D]
#define PS2KEY_E			69	// [e E]
#define PS2KEY_F			70	// [f F]
#define PS2KEY_G			71	// [g G]
#define PS2KEY_H			72	// [h H]
#define PS2KEY_I			73	// [i I]
#define PS2KEY_J			74	// [j J]
#define PS2KEY_K			75	// [k K]
#define PS2KEY_L			76	// [l L]
#define PS2KEY_M			77	// [m M]
#define PS2KEY_N			78	// [n N]
#define PS2KEY_O			79	// [o O]
#define PS2KEY_P			80	// [p P]
#define PS2KEY_Q			81	// [q Q]
#define PS2KEY_R			82	// [r R]
#define PS2KEY_S			83	// [s S]
#define PS2KEY_T			84	// [t T]
#define PS2KEY_U			85	// [u U]
#define PS2KEY_V			86	// [v V]
#define PS2KEY_W			87	// [w W]
#define PS2KEY_X			88	// [x X]
#define PS2KEY_Y			89	// [y Y]
#define PS2KEY_Z			90	// [z Z]

// テンキー
#define PS2KEY_PAD_Equal	94	// [=]
#define PS2KEY_PAD_Enter	95	// [Enter]
#define PS2KEY_PAD_0		96  	// [0/Insert]
#define PS2KEY_PAD_1		97  	// [1/End]
#define PS2KEY_PAD_2		98  	// [2/DownArrow]
#define PS2KEY_PAD_3		99  	// [3/PageDown]
#define PS2KEY_PAD_4		100 	// [4/LeftArrow]
#define PS2KEY_PAD_5		101 	// [5]
#define PS2KEY_PAD_6		102 	// [6/RightArrow]
#define PS2KEY_PAD_7		103 	// [7/Home]
#define PS2KEY_PAD_8		104 	// [8/UPArrow]
#define PS2KEY_PAD_9		105	  // [9/PageUp]
#define PS2KEY_PAD_Multi	106	// [*]
#define PS2KEY_PAD_Plus	107	// [+]
#define PS2KEY_PAD_Kamma	108	// [,]
#define PS2KEY_PAD_Minus	109	// [-]
#define PS2KEY_PAD_DOT		110	// [./Delete]
#define PS2KEY_PAD_Slash	111	// [/]

// ファンクションキー
#define PS2KEY_F1 		112	// [F1]
#define PS2KEY_F2 		113	// [F2]
#define PS2KEY_F3 		114	// [F3]
#define PS2KEY_F4 		115	// [F4]
#define PS2KEY_F5 		116	// [F5]
#define PS2KEY_F6 		117	// [F6]
#define PS2KEY_F7 		118	// [F7]
#define PS2KEY_F8 		119	// [F8]
#define PS2KEY_F9 		120	// [F9]
#define PS2KEY_F10 	121	// [F10]
#define PS2KEY_F11 	122	// [F11]
#define PS2KEY_F12 	123	// [F12]
#define PS2KEY_F13 	124	// [F13]
#define PS2KEY_F14 	125	// [F14]
#define PS2KEY_F15 	126	// [F15]
#define PS2KEY_F16 	127	// [F16]
#define PS2KEY_F17 	128	// [F17]
#define PS2KEY_F18 	129	// [F18]
#define PS2KEY_F19 	130	// [F19]
#define PS2KEY_F20 	131	// [F20]
#define PS2KEY_F21 	132	// [F21]
#define PS2KEY_F22 	133	// [F22]
#define PS2KEY_F23 	134	// [F23]

// マルチマディアキー
#define PS2KEY_PrevTrack		  135	// 前のトラック
#define PS2KEY_WWW_Favorites	136	// ブラウザお気に入り
#define PS2KEY_WWW_Refresh		137	// ブラウザ更新表示
#define PS2KEY_VolumeDown		138	// 音量を下げる
#define PS2KEY_Mute			    139	// ミュート
#define PS2KEY_WWW_Stop		  140	// ブラウザ停止
#define PS2KEY_Calc			    141	// 電卓
#define PS2KEY_WWW_Forward		142	// ブラウザ進む
#define PS2KEY_VolumeUp		  143	// 音量を上げる
#define PS2KEY_PLAY			    144	// 再生
#define PS2KEY_POWER			    145	// 電源ON
#define PS2KEY_WWW_Back		  146	// ブラウザ戻る
#define PS2KEY_WWW_Home		  147	// ブラウザホーム
#define PS2KEY_Sleep			    148	// スリープ
#define PS2KEY_Mycomputer		149	// マイコンピュータ
#define PS2KEY_Mail			    150	// メーラー起動
#define PS2KEY_NextTrack		  151	// 次のトラック
#define PS2KEY_MEdiaSelect		152	// メディア選択
#define PS2KEY_Wake			    153	// ウェイクアップ
#define PS2KEY_Stop			    154	// 停止
#define PS2KEY_WWW_Search		155	// ウェブ検索

// キーボードイベント構造体
typedef struct  {
    uint8_t code  : 8; // code
    uint8_t BREAK : 1; // BREAKコード
    uint8_t KEY   : 1; // KEYコード判定
    uint8_t SHIFT : 1; // SHIFTあり
    uint8_t CTRL  : 1; // CTRLあり
    uint8_t ALT   : 1; // ALTあり
    uint8_t GUI   : 1; // GUIあり
    uint8_t dumy  : 2; // ダミー
} keyEvent;

// キーボードイベント共用体
typedef union {
  uint16_t  value;
  keyEvent  kevt; 
} keyinfo;

// クラス定義
class TKeyboard {

  private:
     // キーコード検索
     static uint8_t findcode(uint8_t c);
   
     static uint8_t m_key_state[256/8];
   
  public:
    // キーボード利用開始
    static uint8_t begin(uint8_t clk, uint8_t dat, uint8_t flgLED=false, uint8_t layout = 0);
    static void setLayout(uint8_t layout);
   
    // キーボード利用終了
    static void end();
   
    // キーボード初期化
    static uint8_t init();
        
    // スキャンコード・キーコード変換
    static uint16_t scanToKeycode() ;
    
    // キーボード入力の読み込み
    static keyEvent read();
    
    // キーボード上LED制御
    static uint8_t ctrl_LED(uint8_t swCaps, uint8_t swNum, uint8_t swScrol);

    // CLKピン割り込み制御
    inline static void enableInterrupts();      // CLK変化割り込み許可
    inline static void disableInterrupts();     // CLK変化割り込み禁止
    static void setPriority(uint8_t n);  // 割り込み優先レベルの設定

    // PS/2ライン制御
    inline static void  mode_idole();     // アイドル状態に設定
    inline static void  mode_stop();      // 通信禁止
    inline static void  mode_send();      // ホスト送信モード
    
    inline static bool state(uint8_t keycode) {
      return !!(m_key_state[keycode/8] & (1 << keycode % 8));
    }
};

#endif
