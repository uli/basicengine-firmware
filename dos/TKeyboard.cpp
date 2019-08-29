#include <TKeyboard.h>
#include "TPS2.h"

#include <allegro.h>
#include <tscreenBase.h>

TPS2 pb;

static const int ps2_to_allegro[] = {
0,	// PS2KEY_NONE              0  // 継続またはバッファに変換対象なし
KEY_ALT,	// PS2KEY_L_Alt		    1	// [左Alt]
KEY_LSHIFT,	// PS2KEY_L_Shift		  2	// [左Shift]
KEY_LCONTROL,	// PS2KEY_L_Ctrl		  3	// [左Ctrl]
KEY_RSHIFT,	// PS2KEY_R_Shift		  4	// [右Shift]
KEY_ALTGR,	// PS2KEY_R_Alt		    5	// [右Alt]
KEY_RCONTROL,	// PS2KEY_R_Ctrl		  6	// [右Ctrl]
KEY_LWIN,	// PS2KEY_L_GUI		    7	// [左Windowsキー]
KEY_RWIN,	// PS2KEY_R_GUI		    8	// [右Windowsキー]
KEY_NUMLOCK,	// PS2KEY_NumLock		  9	// [NumLock]
KEY_SCRLOCK,	// PS2KEY_ScrollLock	10	// [ScrollLock]
KEY_CAPSLOCK,	// PS2KEY_CapsLock	  11	// [CapsLock]
KEY_PRTSCR,	// PS2KEY_PrintScreen	12	// [PrintScreen]
KEY_TILDE,	// PS2KEY_HanZen		  13	// [半角/全角 漢字]
KEY_INSERT,	// PS2KEY_Insert		  14	// [Insert]
KEY_HOME,	// PS2KEY_Home		    15	// [Home]
KEY_PAUSE,	// PS2KEY_Pause		    16	// [Pause]
0, // XXX?	// PS2KEY_Romaji		17	// [カタカナ ひらがな ローマ字]
0, // XXX?	// PS2KEY_APP	18	// [メニューキー]
0, // XXX?	// PS2KEY_Henkan		  19	// [変換]
0, // XXX?	// PS2KEY_Muhenkan	  20	// [無変換]
KEY_PGUP,	// PS2KEY_PageUp		  21	// [PageUp]
KEY_PGDN,	// PS2KEY_PageDown	  22	// [PageDown]
KEY_END,	// PS2KEY_End			    23	// [End]
KEY_LEFT,	// PS2KEY_L_Arrow		  24	// [←]
KEY_UP,	// PS2KEY_Up_Arrow	  25	// [↑]
KEY_RIGHT,	// PS2KEY_R_Arrow		  26	// [→]
KEY_DOWN,	// PS2KEY_Down_Arrow	27	// [↓]
0,
0,
KEY_ESC,	// PS2KEY_ESC			    30	// [ESC]
KEY_TAB,	// PS2KEY_Tab 		    31	// [Tab]
KEY_ENTER,	// PS2KEY_Enter       32  // [Enter]
KEY_BACKSPACE,	// PS2KEY_Backspace	  33	// [BackSpace]
KEY_DEL,	// PS2KEY_Delete		  34	// [Delete]
KEY_SPACE,	// PS2KEY_Space		    35	// [空白]
KEY_QUOTE,	// PS2KEY_Colon		    36	// [: *]
KEY_COLON,	// PS2KEY_Semicolon	  37	// [; +]
KEY_COMMA,	// PS2KEY_Kamma		    38	// [, <]
KEY_MINUS,	// PS2KEY_minus		    39	// [- =]
KEY_STOP,	// PS2KEY_Dot			    40	// [. >]
KEY_SLASH,	// PS2KEY_Question	  41	// [/ ?]
KEY_OPENBRACE,	// PS2KEY_AT			    42	// [@ `]
KEY_CLOSEBRACE,	// PS2KEY_L_brackets	43	// [[ {]
KEY_BACKSLASH,	// PS2KEY_Pipe		    44	// [\ |]
0,	// PS2KEY_R_brackets	45	// [] }]
KEY_EQUALS,	// PS2KEY_Hat			    46	// [^ ~]
KEY_BACKSLASH2,	// XXX: guess	// PS2KEY_Ro			    47	// [\ _ ろ]
KEY_0,	// PS2KEY_0			48	// [0 )]
KEY_1,	// PS2KEY_1			49	// [1 !]
KEY_2,	// PS2KEY_2			50	// [2 @]
KEY_3,	// PS2KEY_3			51	// [3 #]
KEY_4,	// PS2KEY_4			52	// [4 $]
KEY_5,	// PS2KEY_5			53	// [5 %]
KEY_6,	// PS2KEY_6			54	// [6 ^]
KEY_7,	// PS2KEY_7			55	// [7 &]
KEY_8,	// PS2KEY_8			56	// [8 *]
KEY_9,	// PS2KEY_9			57	// [9 (]
// According to MS translation table, this key does not exist.
0,	// PS2KEY_Pipe2 58  // [\ |] (USキーボード用)
0,
0,
0,
0,
0,
0,
KEY_A,	// PS2KEY_A			65	// [a A]
KEY_B,	// PS2KEY_B			66	// [b B]
KEY_C,	// PS2KEY_C			67	// [c C]
KEY_D,	// PS2KEY_D			68	// [d D]
KEY_E,	// PS2KEY_E			69	// [e E]
KEY_F,	// PS2KEY_F			70	// [f F]
KEY_G,	// PS2KEY_G			71	// [g G]
KEY_H,	// PS2KEY_H			72	// [h H]
KEY_I,	// PS2KEY_I			73	// [i I]
KEY_J,	// PS2KEY_J			74	// [j J]
KEY_K,	// PS2KEY_K			75	// [k K]
KEY_L,	// PS2KEY_L			76	// [l L]
KEY_M,	// PS2KEY_M			77	// [m M]
KEY_N,	// PS2KEY_N			78	// [n N]
KEY_O,	// PS2KEY_O			79	// [o O]
KEY_P,	// PS2KEY_P			80	// [p P]
KEY_Q,	// PS2KEY_Q			81	// [q Q]
KEY_R,	// PS2KEY_R			82	// [r R]
KEY_S,	// PS2KEY_S			83	// [s S]
KEY_T,	// PS2KEY_T			84	// [t T]
KEY_U,	// PS2KEY_U			85	// [u U]
KEY_V,	// PS2KEY_V			86	// [v V]
KEY_W,	// PS2KEY_W			87	// [w W]
KEY_X,	// PS2KEY_X			88	// [x X]
KEY_Y,	// PS2KEY_Y			89	// [y Y]
KEY_Z,	// PS2KEY_Z			90	// [z Z]
0,
0,
0,
KEY_EQUALS_PAD, //XXX?	// PS2KEY_PAD_Equal	94	// [=]
KEY_ENTER_PAD,	// PS2KEY_PAD_Enter	95	// [Enter]
KEY_0_PAD,	// PS2KEY_PAD_0		96  	// [0/Insert]
KEY_1_PAD,	// PS2KEY_PAD_1		97  	// [1/End]
KEY_2_PAD,	// PS2KEY_PAD_2		98  	// [2/DownArrow]
KEY_3_PAD,	// PS2KEY_PAD_3		99  	// [3/PageDown]
KEY_4_PAD,	// PS2KEY_PAD_4		100 	// [4/LeftArrow]
KEY_5_PAD,	// PS2KEY_PAD_5		101 	// [5]
KEY_6_PAD,	// PS2KEY_PAD_6		102 	// [6/RightArrow]
KEY_7_PAD,	// PS2KEY_PAD_7		103 	// [7/Home]
KEY_8_PAD,	// PS2KEY_PAD_8		104 	// [8/UPArrow]
KEY_9_PAD,	// PS2KEY_PAD_9		105	  // [9/PageUp]
KEY_ASTERISK,	// PS2KEY_PAD_Multi	106	// [*]
KEY_PLUS_PAD,	// PS2KEY_PAD_Plus	107	// [+]
// This is the "Keypad ," (aka "Brazilian ."), not the "PC9800 Keypad ,"
// What a pain in the ass this keyboard stuff is!
0, //XXX?	// PS2KEY_PAD_Kamma	108	// [,]
KEY_MINUS_PAD,	// PS2KEY_PAD_Minus	109	// [-]
KEY_DEL_PAD,	// PS2KEY_PAD_DOT		110	// [./Delete]
KEY_SLASH_PAD,	// PS2KEY_PAD_Slash	111	// [/]
KEY_F1,	// PS2KEY_F1 		112	// [F1]
KEY_F2,	// PS2KEY_F2 		113	// [F2]
KEY_F3,	// PS2KEY_F3 		114	// [F3]
KEY_F4,	// PS2KEY_F4 		115	// [F4]
KEY_F5,	// PS2KEY_F5 		116	// [F5]
KEY_F6,	// PS2KEY_F6 		117	// [F6]
KEY_F7,	// PS2KEY_F7 		118	// [F7]
KEY_F8,	// PS2KEY_F8 		119	// [F8]
KEY_F9,	// PS2KEY_F9 		120	// [F9]
KEY_F10,	// PS2KEY_F10 	121	// [F10]
KEY_F11,	// PS2KEY_F11 	122	// [F11]
KEY_F12,	// PS2KEY_F12 	123	// [F12]
0,	// PS2KEY_F13 	124	// [F13]
0,	// PS2KEY_F14 	125	// [F14]
0,	// PS2KEY_F15 	126	// [F15]
0,	// PS2KEY_F16 	127	// [F16]
0,	// PS2KEY_F17 	128	// [F17]
0,	// PS2KEY_F18 	129	// [F18]
0,	// PS2KEY_F19 	130	// [F19]
0,	// PS2KEY_F20 	131	// [F20]
0,	// PS2KEY_F21 	132	// [F21]
0,	// PS2KEY_F22 	133	// [F22]
0,	// PS2KEY_F23 	134	// [F23]
// From USB HID to PS/2 Scan Code Translation Table
// https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf
// Some of these have two-byte usage IDs, I don't know how that works.
0,	// PS2KEY_PrevTrack	  135	// 前のトラック
0,	// XXX: 0x22a	PS2KEY_WWW_Favorites	136	// ブラウザお気に入り
0,	// XXX: 0x227	PS2KEY_WWW_Refresh		137	// ブラウザ更新表示
0,	// PS2KEY_VolumeDown		138	// 音量を下げる
0,	// PS2KEY_Mute			    139	// ミュート
0,	// PS2KEY_WWW_Stop		  140	// ブラウザ停止
0,	// XXX: 0x192	PS2KEY_Calc			    141	// 電卓
0,	// XXX: 0x225 PS2KEY_WWW_Forward		142	// ブラウザ進む
0,	// PS2KEY_VolumeUp		  143	// 音量を上げる
0,	// PS2KEY_PLAY			    144	// 再生
0,	// PS2KEY_POWER			    145	// 電源ON
0,	// XXX: 0x224 PS2KEY_WWW_Back		  146	// ブラウザ戻る
0,	// XXX: 0x223 PS2KEY_WWW_Home		  147	// ブラウザホーム
0,	// PS2KEY_Sleep			    148	// スリープ
0,	// XXX: 0x194 PS2KEY_Mycomputer		149	// マイコンピュータ
0,	// XXX: 0x18a	PS2KEY_Mail			    150	// メーラー起動
0,	// PS2KEY_NextTrack		  151	// 次のトラック
0,	// PS2KEY_MEdiaSelect		152	// メディア選択
0,	// PS2KEY_Wake			    153	// ウェイクアップ
0,	// PS2KEY_Stop			    154	// 停止
0,	// XXX: 0x221 PS2KEY_WWW_Search		155	// ウェブ検索
};

bool TKeyboard::state(uint8_t keycode) {
  if (keycode < sizeof(ps2_to_allegro)/sizeof(*ps2_to_allegro))
    return key[ps2_to_allegro[keycode]];
  else
    return false;
}

keyEvent ps22tty_last_key;

#include <allegro.h>

int peeked_key = -1;

uint16_t ps2read()
{
  if (peeked_key >= 0) {
    uint16_t k = peeked_key;
    peeked_key = -1;
    return k;
  }

  if (keypressed()) {
    ps22tty_last_key = {};
    ps22tty_last_key.CTRL = !!(key_shifts & KB_CTRL_FLAG);
    ps22tty_last_key.ALT = !!(key_shifts & KB_ALT_FLAG);
    ps22tty_last_key.SHIFT = !!(key_shifts & KB_SHIFT_FLAG);
    
    int scancode;
    int ukey = ureadkey(&scancode);
    
    if (ukey == 0 && scancode) {
      switch (scancode) {
      case KEY_UP:	ukey = SC_KEY_UP; break;
      case KEY_DOWN:	ukey = SC_KEY_DOWN; break;
      case KEY_LEFT:	ukey = SC_KEY_LEFT; break;
      case KEY_RIGHT:	ukey = SC_KEY_RIGHT; break;
      case KEY_HOME:	ukey = SC_KEY_HOME; break;
      case KEY_DEL:	ukey = SC_KEY_DC; break;
      case KEY_INSERT:	ukey = SC_KEY_IC; break;
      case KEY_PGDN:	ukey = SC_KEY_NPAGE; break;
      case KEY_PGUP:	ukey = SC_KEY_PPAGE; break;
      case KEY_END:	ukey = SC_KEY_END; break;
      case KEY_F1:	ukey = SC_KEY_F(1); break;
      case KEY_F2:	ukey = SC_KEY_F(2); break;
      case KEY_F3:	ukey = SC_KEY_F(3); break;
      case KEY_F4:	ukey = SC_KEY_F(4); break;
      case KEY_F5:	ukey = SC_KEY_F(5); break;
      case KEY_F6:	ukey = SC_KEY_F(6); break;
      case KEY_F7:	ukey = SC_KEY_F(7); break;
      case KEY_F8:	ukey = SC_KEY_F(8); break;
      case KEY_F9:	ukey = SC_KEY_F(9); break;
      case KEY_F10:	ukey = SC_KEY_F(10); break;
      case KEY_F11:	ukey = SC_KEY_F(11); break;
      case KEY_F12:	ukey = SC_KEY_F(12); break;

      default:	// probably ALT + key
        if (key_shifts & KB_ALT_FLAG)
          ukey = scancode_to_ascii(scancode);
        break;
      }
    }

//    printf("%d %08x\n", scancode, ukey); delay(500);
    return ukey;
  }

  return 0;
}

uint16_t ps2peek()
{
  if (peeked_key >= 0)
    return peeked_key;

  if (keypressed()) {
    peeked_key = ps2read();
    return peeked_key;
  }
  
  return 0;
}

TKeyboard kb;

void setupPS2(uint8_t kb_type = 0) {
  install_keyboard();
  // XXX: keymap?
}

void endPS2() {
}
