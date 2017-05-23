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

// 状態管理用
#define BREAK_CODE       0x0100  // BREAKコード
#define KEY_CODE         0x0200  // KEYコード
#define SHIFT_CODE       0x0400  // SHIFTあり
#define CTRL_CODE        0x0800  // CTRLあり
#define ALT_CODE         0x1000  // ALTあり
#define GUI_CODE         0x2000  // GUIあり

#define KEY_ERROR          0xFF  // キーコードエラー
#define KEY_NONE              0  // 継続またはバッファに変換対象なし

// キーコード定義(JIS配列キーボード)
#define KEY_L_Alt		    1	// [左Alt]
#define KEY_L_Shift		  2	// [左Shift]
#define KEY_L_Ctrl		  3	// [左Ctrl]
#define KEY_R_Shift		  4	// [右Shift]
#define KEY_R_Alt		    5	// [右Alt]
#define KEY_R_Ctrl		  6	// [右Ctrl]
#define KEY_L_GUI		    7	// [左Windowsキー]
#define KEY_R_GUI		    8	// [右Windowsキー]
#define KEY_NumLock		  9	// [NumLock]
#define KEY_ScrollLock	10	// [ScrollLock]
#define KEY_CapsLock	  11	// [CapsLock]
#define KEY_PrintScreen	12	// [PrintScreen]
#define KEY_HanZen		  13	// [半角/全角 漢字]
#define KEY_Insert		  14	// [Insert]
#define KEY_Home		    15	// [Home]
#define KEY_Pause		    16	// [Pause]
#define KEY_Romaji		  17	// [カタカナ ひらがな ローマ字]
#define KEY_APP			    18	// [メニューキー]
#define KEY_Henkan		  19	// [変換]
#define KEY_Muhenkan	  20	// [無変換]
#define KEY_PageUp		  21	// [PageUp]
#define KEY_PageDown	  22	// [PageDown]
#define KEY_End			    23	// [End]
#define KEY_L_Arrow		  24	// [←]
#define KEY_Up_Arrow	  25	// [↑]
#define KEY_R_Arrow		  26	// [→]
#define KEY_Down_Arrow	27	// [↓]

#define KEY_ESC			    30	// [ESC]
#define KEY_Tab 		    31	// [Tab]
#define KEY_Enter       32  // [Enter]
#define KEY_Space		    35	// [空白]
#define KEY_Backspace	  33	// [BackSpace]
#define KEY_Delete		  34	// [Delete]

#define KEY_Colon		    36	// [: *]
#define KEY_Semicolon	  37	// [; +]
#define KEY_Kamma		    38	// [, <]
#define KEY_minus		    39	// [- =]
#define KEY_Dot			    40	// [. >]
#define KEY_Question	  41	// [/ ?]
#define KEY_AT			    42	// [@ `]
#define KEY_L_brackets	43	// [[ {]
#define KEY_Pipe		    44	// [\ |]
#define KEY_R_brackets	45	// [] }]
#define KEY_Hat			    46	// [^ ~]
#define KEY_Ro			    47	// [\ _ ろ]

#define KEY_0			48	// [0 )]
#define KEY_1			49	// [1 !]
#define KEY_2			50	// [2 @]
#define KEY_3			51	// [3 #]
#define KEY_4			52	// [4 $]
#define KEY_5			53	// [5 %]
#define KEY_6			54	// [6 ^]
#define KEY_7			55	// [7 &]
#define KEY_8			56	// [8 *]
#define KEY_9			57	// [9 (]
#define KEY_Pipe2 58  // [\ |] (USキーボード用)
#define KEY_A			65	// [a A]
#define KEY_B			66	// [b B]
#define KEY_C			67	// [c C]
#define KEY_D			68	// [d D]
#define KEY_E			69	// [e E]
#define KEY_F			70	// [f F]
#define KEY_G			71	// [g G]
#define KEY_H			72	// [h H]
#define KEY_I			73	// [i I]
#define KEY_J			74	// [j J]
#define KEY_K			75	// [k K]
#define KEY_L			76	// [l L]
#define KEY_M			77	// [m M]
#define KEY_N			78	// [n N]
#define KEY_O			79	// [o O]
#define KEY_P			80	// [p P]
#define KEY_Q			81	// [q Q]
#define KEY_R			82	// [r R]
#define KEY_S			83	// [s S]
#define KEY_T			84	// [t T]
#define KEY_U			85	// [u U]
#define KEY_V			86	// [v V]
#define KEY_W			87	// [w W]
#define KEY_X			88	// [x X]
#define KEY_Y			89	// [y Y]
#define KEY_Z			90	// [z Z]

// テンキー
#define KEY_PAD_Equal	94	// [=]
#define KEY_PAD_Enter	95	// [Enter]
#define KEY_PAD_0		96  	// [0/Insert]
#define KEY_PAD_1		97  	// [1/End]
#define KEY_PAD_2		98  	// [2/DownArrow]
#define KEY_PAD_3		99  	// [3/PageDown]
#define KEY_PAD_4		100 	// [4/LeftArrow]
#define KEY_PAD_5		101 	// [5]
#define KEY_PAD_6		102 	// [6/RightArrow]
#define KEY_PAD_7		103 	// [7/Home]
#define KEY_PAD_8		104 	// [8/UPArrow]
#define KEY_PAD_9		105	  // [9/PageUp]
#define KEY_PAD_Multi	106	// [*]
#define KEY_PAD_Plus	107	// [+]
#define KEY_PAD_Kamma	108	// [,]
#define KEY_PAD_Minus	109	// [-]
#define KEY_PAD_DOT		110	// [./Delete]
#define KEY_PAD_Slash	111	// [/]

// ファンクションキー
#define KEY_F1 		112	// [F1]
#define KEY_F2 		113	// [F2]
#define KEY_F3 		114	// [F3]
#define KEY_F4 		115	// [F4]
#define KEY_F5 		116	// [F5]
#define KEY_F6 		117	// [F6]
#define KEY_F7 		118	// [F7]
#define KEY_F8 		119	// [F8]
#define KEY_F9 		120	// [F9]
#define KEY_F10 	121	// [F10]
#define KEY_F11 	122	// [F11]
#define KEY_F12 	123	// [F12]
#define KEY_F13 	124	// [F13]
#define KEY_F14 	125	// [F14]
#define KEY_F15 	126	// [F15]
#define KEY_F16 	127	// [F16]
#define KEY_F17 	128	// [F17]
#define KEY_F18 	129	// [F18]
#define KEY_F19 	130	// [F19]
#define KEY_F20 	131	// [F20]
#define KEY_F21 	132	// [F21]
#define KEY_F22 	133	// [F22]
#define KEY_F23 	134	// [F23]

// マルチマディアキー
#define KEY_PrevTrack		  135	// 前のトラック
#define KEY_WWW_Favorites	136	// ブラウザお気に入り
#define LEY_WWW_Refresh		137	// ブラウザ更新表示
#define KEY_VolumeDown		138	// 音量を下げる
#define KEY_Mute			    139	// ミュート
#define KEY_WWW_Stop		  140	// ブラウザ停止
#define KEY_Calc			    141	// 電卓
#define KEY_WWW_Forward		142	// ブラウザ進む
#define KEY_VolumeUp		  143	// 音量を上げる
#define KEY_PLAY			    144	// 再生
#define KEY_POWER			    145	// 電源ON
#define KEY_WWW_Back		  146	// ブラウザ戻る
#define KEY_WWW_Home		  147	// ブラウザホーム
#define KEY_Sleep			    148	// スリープ
#define KEY_Mycomputer		149	// マイコンピュータ
#define KEY_Mail			    150	// メーラー起動
#define KEY_NextTrack		  151	// 次のトラック
#define KEY_MEdiaSelect		152	// メディア選択
#define KEY_Wake			    153	// ウェイクアップ
#define KEY_Stop			    154	// 停止
#define KEY_WWW_Search		155	// ウェブ検索

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
   
  public:
    // キーボード利用開始
    static uint8_t begin(uint8_t clk, uint8_t dat, uint8_t flgLED=false, uint8_t flgUS=false);
   
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
};

#endif
