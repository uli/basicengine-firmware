// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include <TKeyboard.h>
#include "TPS2.h"
#include <SDL.h>
#include <ring_buffer.h>

TPS2 pb;

static const SDL_Keycode ps2_to_sdl[] = {
SDL_SCANCODE_UNKNOWN,	// PS2KEY_NONE              0  // 継続またはバッファに変換対象なし
SDL_SCANCODE_LALT,	// PS2KEY_L_Alt		    1	// [左Alt]
SDL_SCANCODE_LSHIFT,	// PS2KEY_L_Shift		  2	// [左Shift]
SDL_SCANCODE_LCTRL,	// PS2KEY_L_Ctrl		  3	// [左Ctrl]
SDL_SCANCODE_RSHIFT,	// PS2KEY_R_Shift		  4	// [右Shift]
SDL_SCANCODE_RALT,	// PS2KEY_R_Alt		    5	// [右Alt]
SDL_SCANCODE_RCTRL,	// PS2KEY_R_Ctrl		  6	// [右Ctrl]
SDL_SCANCODE_LGUI,	// PS2KEY_L_GUI		    7	// [左Windowsキー]
SDL_SCANCODE_RGUI,	// PS2KEY_R_GUI		    8	// [右Windowsキー]
SDL_SCANCODE_NUMLOCKCLEAR,// PS2KEY_NumLock		  9	// [NumLock]
SDL_SCANCODE_SCROLLLOCK,// PS2KEY_ScrollLock	10	// [ScrollLock]
SDL_SCANCODE_CAPSLOCK,	// PS2KEY_CapsLock	  11	// [CapsLock]
// XXX: There is also SDL_SCANCODE_SYSREQ, which is the same key...
SDL_SCANCODE_PRINTSCREEN,// PS2KEY_PrintScreen	12	// [PrintScreen]
SDL_SCANCODE_GRAVE,	// PS2KEY_HanZen		  13	// [半角/全角 漢字]
SDL_SCANCODE_INSERT,	// PS2KEY_Insert		  14	// [Insert]
SDL_SCANCODE_HOME,	// PS2KEY_Home		    15	// [Home]
SDL_SCANCODE_PAUSE,	// PS2KEY_Pause		    16	// [Pause]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Romaji		  17	// [カタカナ ひらがな ローマ字]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_APP			    18	// [メニューキー]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Henkan		  19	// [変換]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Muhenkan	  20	// [無変換]
SDL_SCANCODE_PAGEUP,	// PS2KEY_PageUp		  21	// [PageUp]
SDL_SCANCODE_PAGEDOWN,	// PS2KEY_PageDown	  22	// [PageDown]
SDL_SCANCODE_END,	// PS2KEY_End			    23	// [End]
SDL_SCANCODE_LEFT,	// PS2KEY_L_Arrow		  24	// [←]
SDL_SCANCODE_UP,	// PS2KEY_Up_Arrow	  25	// [↑]
SDL_SCANCODE_RIGHT,	// PS2KEY_R_Arrow		  26	// [→]
SDL_SCANCODE_DOWN,	// PS2KEY_Down_Arrow	27	// [↓]
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_ESCAPE,	// PS2KEY_ESC			    30	// [ESC]
SDL_SCANCODE_TAB,	// PS2KEY_Tab 		    31	// [Tab]
SDL_SCANCODE_RETURN,	// PS2KEY_Enter       32  // [Enter]
SDL_SCANCODE_BACKSPACE,	// PS2KEY_Backspace	  33	// [BackSpace]
SDL_SCANCODE_DELETE,	// PS2KEY_Delete		  34	// [Delete]
SDL_SCANCODE_SPACE,	// PS2KEY_Space		    35	// [空白]
SDL_SCANCODE_APOSTROPHE,	// PS2KEY_Colon		    36	// [: *]
// XXX: There is also SDL_SCANCODE_SEMICOLON, which are on the same key in US layout...
SDL_SCANCODE_SEMICOLON,	// PS2KEY_Semicolon	  37	// [; +]
SDL_SCANCODE_COMMA,	// PS2KEY_Kamma		    38	// [, <]
SDL_SCANCODE_MINUS,	// PS2KEY_minus		    39	// [- =]
SDL_SCANCODE_PERIOD,	// PS2KEY_Dot			    40	// [. >]
SDL_SCANCODE_SLASH,	// PS2KEY_Question	  41	// [/ ?]
SDL_SCANCODE_LEFTBRACKET,	// PS2KEY_AT			    42	// [@ `]
SDL_SCANCODE_RIGHTBRACKET,	// PS2KEY_L_brackets	43	// [[ {]
SDL_SCANCODE_BACKSLASH,	// PS2KEY_Pipe		    44	// [\ |]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_R_brackets	45	// [] }]
SDL_SCANCODE_EQUALS,	// PS2KEY_Hat			    46	// [^ ~]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Ro			    47	// [\ _ ろ]
SDL_SCANCODE_0,	// PS2KEY_0			48	// [0 )]
SDL_SCANCODE_1,	// PS2KEY_1			49	// [1 !]
SDL_SCANCODE_2,	// PS2KEY_2			50	// [2 @]
SDL_SCANCODE_3,	// PS2KEY_3			51	// [3 #]
SDL_SCANCODE_4,	// PS2KEY_4			52	// [4 $]
SDL_SCANCODE_5,	// PS2KEY_5			53	// [5 %]
SDL_SCANCODE_6,	// PS2KEY_6			54	// [6 ^]
SDL_SCANCODE_7,	// PS2KEY_7			55	// [7 &]
SDL_SCANCODE_8,	// PS2KEY_8			56	// [8 *]
SDL_SCANCODE_9,	// PS2KEY_9			57	// [9 (]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Pipe2 58  // [\ |] (USキーボード用)
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_A,	// PS2KEY_A			65	// [a A]
SDL_SCANCODE_B,	// PS2KEY_B			66	// [b B]
SDL_SCANCODE_C,	// PS2KEY_C			67	// [c C]
SDL_SCANCODE_D,	// PS2KEY_D			68	// [d D]
SDL_SCANCODE_E,	// PS2KEY_E			69	// [e E]
SDL_SCANCODE_F,	// PS2KEY_F			70	// [f F]
SDL_SCANCODE_G,	// PS2KEY_G			71	// [g G]
SDL_SCANCODE_H,	// PS2KEY_H			72	// [h H]
SDL_SCANCODE_I,	// PS2KEY_I			73	// [i I]
SDL_SCANCODE_J,	// PS2KEY_J			74	// [j J]
SDL_SCANCODE_K,	// PS2KEY_K			75	// [k K]
SDL_SCANCODE_L,	// PS2KEY_L			76	// [l L]
SDL_SCANCODE_M,	// PS2KEY_M			77	// [m M]
SDL_SCANCODE_N,	// PS2KEY_N			78	// [n N]
SDL_SCANCODE_O,	// PS2KEY_O			79	// [o O]
SDL_SCANCODE_P,	// PS2KEY_P			80	// [p P]
SDL_SCANCODE_Q,	// PS2KEY_Q			81	// [q Q]
SDL_SCANCODE_R,	// PS2KEY_R			82	// [r R]
SDL_SCANCODE_S,	// PS2KEY_S			83	// [s S]
SDL_SCANCODE_T,	// PS2KEY_T			84	// [t T]
SDL_SCANCODE_U,	// PS2KEY_U			85	// [u U]
SDL_SCANCODE_V,	// PS2KEY_V			86	// [v V]
SDL_SCANCODE_W,	// PS2KEY_W			87	// [w W]
SDL_SCANCODE_X,	// PS2KEY_X			88	// [x X]
SDL_SCANCODE_Y,	// PS2KEY_Y			89	// [y Y]
SDL_SCANCODE_Z,	// PS2KEY_Z			90	// [z Z]
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_UNKNOWN,
SDL_SCANCODE_KP_EQUALS,	// PS2KEY_PAD_Equal	94	// [=]
SDL_SCANCODE_KP_ENTER,	// PS2KEY_PAD_Enter	95	// [Enter]
SDL_SCANCODE_KP_0,	// PS2KEY_PAD_0		96  	// [0/Insert]
SDL_SCANCODE_KP_1,	// PS2KEY_PAD_1		97  	// [1/End]
SDL_SCANCODE_KP_2,	// PS2KEY_PAD_2		98  	// [2/DownArrow]
SDL_SCANCODE_KP_3,	// PS2KEY_PAD_3		99  	// [3/PageDown]
SDL_SCANCODE_KP_4,	// PS2KEY_PAD_4		100 	// [4/LeftArrow]
SDL_SCANCODE_KP_5,	// PS2KEY_PAD_5		101 	// [5]
SDL_SCANCODE_KP_6,	// PS2KEY_PAD_6		102 	// [6/RightArrow]
SDL_SCANCODE_KP_7,	// PS2KEY_PAD_7		103 	// [7/Home]
SDL_SCANCODE_KP_8,	// PS2KEY_PAD_8		104 	// [8/UPArrow]
SDL_SCANCODE_KP_9,	// PS2KEY_PAD_9		105	  // [9/PageUp]
SDL_SCANCODE_KP_MULTIPLY,	// PS2KEY_PAD_Multi	106	// [*]
SDL_SCANCODE_KP_PLUS,	// PS2KEY_PAD_Plus	107	// [+]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_PAD_Kamma	108	// [,]
SDL_SCANCODE_KP_MINUS,	// PS2KEY_PAD_Minus	109	// [-]
SDL_SCANCODE_KP_PERIOD,	// PS2KEY_PAD_DOT		110	// [./Delete]
SDL_SCANCODE_KP_DIVIDE,	// PS2KEY_PAD_Slash	111	// [/]
SDL_SCANCODE_F1,	// PS2KEY_F1 		112	// [F1]
SDL_SCANCODE_F2,	// PS2KEY_F2 		113	// [F2]
SDL_SCANCODE_F3,	// PS2KEY_F3 		114	// [F3]
SDL_SCANCODE_F4,	// PS2KEY_F4 		115	// [F4]
SDL_SCANCODE_F5,	// PS2KEY_F5 		116	// [F5]
SDL_SCANCODE_F6,	// PS2KEY_F6 		117	// [F6]
SDL_SCANCODE_F7,	// PS2KEY_F7 		118	// [F7]
SDL_SCANCODE_F8,	// PS2KEY_F8 		119	// [F8]
SDL_SCANCODE_F9,	// PS2KEY_F9 		120	// [F9]
SDL_SCANCODE_F10,	// PS2KEY_F10 	121	// [F10]
SDL_SCANCODE_F11,	// PS2KEY_F11 	122	// [F11]
SDL_SCANCODE_F12,	// PS2KEY_F12 	123	// [F12]
SDL_SCANCODE_F13,	// PS2KEY_F13 	124	// [F13]
SDL_SCANCODE_F14,	// PS2KEY_F14 	125	// [F14]
SDL_SCANCODE_F15,	// PS2KEY_F15 	126	// [F15]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F16 	127	// [F16]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F17 	128	// [F17]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F18 	129	// [F18]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F19 	130	// [F19]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F20 	131	// [F20]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F21 	132	// [F21]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F22 	133	// [F22]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_F23 	134	// [F23]
SDL_SCANCODE_UNKNOWN,	// PS2KEY_PrevTrack		  135	// 前のトラック
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Favorites	136	// ブラウザお気に入り
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Refresh		137	// ブラウザ更新表示
SDL_SCANCODE_UNKNOWN,	// PS2KEY_VolumeDown		138	// 音量を下げる
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Mute			    139	// ミュート
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Stop		  140	// ブラウザ停止
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Calc			    141	// 電卓
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Forward		142	// ブラウザ進む
SDL_SCANCODE_UNKNOWN,	// PS2KEY_VolumeUp		  143	// 音量を上げる
SDL_SCANCODE_UNKNOWN,	// PS2KEY_PLAY			    144	// 再生
SDL_SCANCODE_POWER,	// PS2KEY_POWER			    145	// 電源ON
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Back		  146	// ブラウザ戻る
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Home		  147	// ブラウザホーム
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Sleep			    148	// スリープ
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Mycomputer		149	// マイコンピュータ
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Mail			    150	// メーラー起動
SDL_SCANCODE_UNKNOWN,	// PS2KEY_NextTrack		  151	// 次のトラック
SDL_SCANCODE_UNKNOWN,	// PS2KEY_MEdiaSelect		152	// メディア選択
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Wake			    153	// ウェイクアップ
SDL_SCANCODE_UNKNOWN,	// PS2KEY_Stop			    154	// 停止
SDL_SCANCODE_UNKNOWN,	// PS2KEY_WWW_Search		155	// ウェブ検索
};

static int keyboard_layout = 0;

static const int32_t usb2jp[] = {
     0,   0,   0,   0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
   'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', '0','\r',   0,   0,   0, ' ', '-', '^', '@',
   '[', ']',   0, ';', ':', '`', ',', '.', '/',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '/', '*', '-', '+','\r', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', '0', '.','\\',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',
   '#', '$', '%', '&','\'', '(', ')',   0,   0,   0,   0,   0,   0, '=', '~', '`',
   '{', '}',   0, '+', '*', '~', '<', '>', '?',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '_',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2jp_altgr[240] = {
  0
};

static const int32_t usb2us[] = {
     0,   0,   0,   0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
   'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', '0','\r',   0,   0,   0, ' ', '-', '=', '[',
   ']','\\',   0, ';','\'', '`', ',', '.', '/',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '/', '*', '-', '+','\r', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', '0', '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
   '#', '$', '%', '^', '&', '*', '(', ')',   0,   0,   0,   0,   0, '_', '+', '{',
   '}', '|',   0, ':', '"', '~', '<', '>', '?',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2us_altgr[240] = {
  0
};

static const int32_t usb2de[] = {
     0,   0,   0,   0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
   'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'z', 'y', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', '0','\r',   0,   0,   0, ' ',U'ß',U'´',U'ü',
   '+', '#',   0,U'ö',U'ä', '^', ',', '.', '-',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '/', '*', '-', '+','\r', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', '0', '.', '<',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Z', 'Y', '!', '"',
  U'§', '$', '%', '&', '/', '(', ')', '=',   0,   0,   0,   0,   0, '?', '`',U'Ü',
   '*','\'',   0,U'Ö',U'Ä',U'°', ';', ':', '_',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '>',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2de_altgr[] = {
     0,   0,   0,   0,   0,   0,   0,   0,U'€',   0,   0,   0,   0,   0,   0,   0,
  U'µ',   0,   0,   0, '@',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,U'²',
  U'³',   0,   0,   0, '{', '[', ']', '}',   0,   0,   0,   0,   0,'\\',   0,   0,
   '~',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '|',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2fr[] = {
     0,   0,   0,   0, 'q', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
   ',', 'n', 'o', 'p', 'a', 'r', 's', 't', 'u', 'v', 'z', 'x', 'y', 'w', '&',U'é',
   '"','\'', '(', '-',U'è', '_',U'ç',U'à','\r',   0,   0,   0, ' ', ')', '=', '^',
   '$', '*',   0, 'm',U'ù', '`', ';', ':', '!',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '/', '*', '-', '+','\r', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', '0', '.', '<',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, 'Q', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   '?', 'N', 'O', 'P', 'A', 'R', 'S', 'T', 'U', 'V', 'Z', 'X', 'Y', 'W', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', '0',   0,   0,   0,   0,   0,U'°', '+',U'¨',
  U'£',U'µ',   0, 'M', '%', '~', '.', '/',U'§',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '>',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2fr_altgr[] = {
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '~',
   '#', '{', '[', '|', '`','\\', '^', '@',   0,   0,   0,   0,   0, ']', '}',   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2es[] = {
     0,   0,   0,   0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
   'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
   '3', '4', '5', '6', '7', '8', '9', '0','\r',   0,   0,   0, ' ','\'',U'¡', '`',
   '+',U'ç',   0,U'ñ',   0, '`', ',', '.', '-',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '/', '*', '-', '+','\r', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', '0', '.', '<',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
   'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
   '#', '$', '%', '&', '/', '(', ')', '=',   0,   0,   0,   0,   0, '?',U'¿', '^',
   '*',U'Ç',   0,U'Ñ',   0, '~', ';', ':', '_',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0, '>',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static const int32_t usb2es_altgr[] = {
     0,   0,   0,   0,   0,   0,   0,   0,U'€',   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '|', '@',
   '#', '~',U'½',U'¬',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '[',
   ']', '}',   0,   0, '{','\\',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

static std::vector<const int32_t *> usb2ascii = {
  usb2jp,
  usb2us,
  usb2de,
  usb2fr,
  usb2es,
};

static std::vector<const int32_t *> usb2ascii_altgr = {
  usb2jp_altgr,
  usb2us_altgr,
  usb2de_altgr,
  usb2fr_altgr,
  usb2es_altgr,
};

static std::vector<const char *> usb2ascii_names = {
  "Japanese", "English (US)", "Deutsch", "Français", "Español",
};

bool TKeyboard::state(uint8_t keycode) {
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  int sdlcode = 0;
  if (keycode < sizeof(ps2_to_sdl) / sizeof(*ps2_to_sdl))
    sdlcode = ps2_to_sdl[keycode];
  return state[sdlcode];
}

std::deque<SDL_Event> kbd_events;

static int key_events_pending = 0;
bool TKeyboard::m_layout_visible = false;

uint8_t TPS2::available() {
  for (auto &e : kbd_events) {
    if (e.type == SDL_KEYDOWN)
      return 1;
  }
  return 0;
}

#include <sdlgfx.h>

keyEvent TKeyboard::read() {
  keyinfo ki;
  ki.value = 0;
  SDL_Event event;

  if (kbd_events.size() > 0) {
    event = kbd_events.front();
    kbd_events.pop_front();
    int unicode = 0;
    int kc = event.key.keysym.scancode;
    if (kc >= SDL_SCANCODE_LCTRL && kc <= SDL_SCANCODE_MODE)
      kc = 0;
    uint8_t kc_off = kc + ((event.key.keysym.mod & KMOD_SHIFT) ? 0x80 : 0);

    if (event.type == SDL_KEYUP)
      ki.kevt.BREAK = 1;
    if (event.key.keysym.mod & KMOD_CTRL)
      ki.kevt.CTRL = 1;
    if (event.key.keysym.mod & KMOD_SHIFT)
      ki.kevt.SHIFT = 1;
    if (event.key.keysym.mod & KMOD_LALT)
      ki.kevt.ALT = 1;
    if (event.key.keysym.mod & KMOD_RALT)
      ki.kevt.ALTGR = 1;

    if (event.key.keysym.mod & KMOD_RALT) {
      unicode = usb2ascii_altgr[keyboard_layout][kc];
    } else if (kc_off < sizeof(usb2us) / sizeof(usb2us[0])) {
      unicode = usb2ascii[keyboard_layout][kc_off];
      if (event.key.keysym.mod & KMOD_CAPS)
        unicode = toupper(unicode);
    } else
      unicode = 0;

    if ((event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) &&
        event.key.keysym.sym == SDLK_PAUSE) {
      exit(0);
    }

    if ((event.key.keysym.mod & KMOD_LALT) &&
        event.key.keysym.sym == SDLK_RETURN && event.type == SDL_KEYDOWN) {
      vs23.toggleFullscreen();
    }

    if (event.type == SDL_KEYDOWN &&
        (event.key.keysym.mod & (KMOD_RALT|KMOD_RCTRL)) == (KMOD_RALT|KMOD_RCTRL) &&
        event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
      int layout = event.key.keysym.sym;
      layout = layout == SDLK_0 ? 10 : layout - SDLK_1 + 1;
      setLayout(layout - 1);
      unicode = 0;
    }

    if (unicode && unicode != 127)
      ki.kevt.code = unicode;
    else {
      ki.kevt.KEY = 1;
      for (unsigned int i = 0; i < sizeof(ps2_to_sdl) / sizeof(*ps2_to_sdl); ++i) {
        if (ps2_to_sdl[i] == event.key.keysym.scancode) {
          ki.kevt.code = i;
          break;
        }
      }
    }
  }
  return ki.kevt;
}

void TKeyboard::setLayout(uint8_t layout) {
  keyboard_layout = layout < usb2ascii.size() ? layout : 0;

  if (m_layout_surf) {
    uint32_t *surf = m_layout_surf;
    m_layout_surf = NULL;
    free(surf);
  }
}

void TKeyboard::showLayout(bool onoff) {
  if (m_layout_visible != onoff) {
    m_layout_visible = onoff;
  }
}

void TKeyboard::toggleLayout() {
  m_layout_visible = !m_layout_visible;
}

SDL_Texture *TKeyboard::m_layout_tex;
uint32_t *TKeyboard::m_layout_surf;

#include <stb_image.h>
#include <tvutil.h>

// mapping of USB key codes to keyboard rows (-1 is disabled)
static const int usb_key_row[] = {
  -1, -1, -1, -1,  3,  4,  4,  3,  2,  3,  3,  3,  2,  3,  3,  3,
   4,  4,  2,  2,  2,  2,  3,  2,  2,  4,  2,  4,  2,  4,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  3, -1, -1, -1,  5,  1,  1,  2,
   2,  3, -1,  3,  3,  1,  4,  4,  4, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,  1,  1,  1,  3,  5,  4,  4,  4,  3,  3,  3,  2,
   2,  2,  5,  5,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

// size of keyboard template
#define TEMPLATE_WIDTH 900
#define TEMPLATE_HEIGHT 300

// size of key labels (one glyph)
#define LABEL_SIZE 16

// key spacing
#define KEY_SPACE_H 60
#define KEY_SPACE_V 60

// offsets within the key cell
#define LABEL_OFF_H (KEY_SPACE_H / 6)
#define LABEL_OFF_V (KEY_SPACE_V / 6)
#define LABEL_SHIFT_OFF_V (KEY_SPACE_V * 4 / 10)	// shifted character

// width of first key in each row
static const int usb_row_shift[] = {
  (int)KEY_SPACE_H,  (int)KEY_SPACE_H,  90,  105,  75,  90,
};

// mapping of USB key codes to keyboard columns
static const int usb_key_col[] = {
    0,        0,        0,        0,        1 /*a*/,  6 /*b*/,  4 /*c*/,  3 /*d*/,  3 /*e*/,   4 /*f*/,  5 /*g*/,  6 /*h*/,  8 /*i*/,  7 /*j*/,  8 /*k*/,  9 /*l*/,
    8 /*m*/,  7 /*n*/,  9 /*o*/, 10 /*p*/,  1 /*q*/,  4 /*r*/,  2 /*s*/,  5 /*t*/,  7 /*u*/,   5 /*v*/,  2 /*w*/,  3 /*x*/,  6 /*y*/,  2 /*z*/,  1 /*1*/,  2 /*2*/,
    3 /*3*/,  4 /*4*/,  5 /*5*/,  6 /*6*/,  7 /*7*/,  8 /*8*/,  9 /*9*/, 10 /*0*/, 13 /*\r*/,  0,        0,        0,        4 /* */, 11 /*-*/, 12 /*=*/, 11 /*[*/,
   12 /*]*/, 12 /*\*/,  0,       10 /*;*/, 11 /*'*/,  0 /*`*/,  9 /*,*/, 10 /*.*/, 11 /*/*/,   0,        0,        0,        0,        0,        0,        0,
    0,        0,        0,        0,        0,        0,        0,        0,        0,         0,        0,        0,        0,        0,        0,        0,
    0,        0,        0,        0,       21 /*/*/, 22 /***/, 23 /*-*/, 23 /*+*/, 23 /*\r*/, 20 /*1*/, 21 /*2*/, 22 /*3*/, 20 /*4*/, 21 /*5*/, 22 /*6*/, 20 /*7*/,
   21 /*8*/, 22 /*9*/, 20 /*0*/, 22 /*.*/,  1,        0,        0,        0,        0,         0,        0,        0,        0,        0,        0,        0,
    0,        0,        0,        0,        0,        0,        0,        0,        0,         0,        0,        0,        0,        0,        0,        0,
};

void TKeyboard::drawLayout(SDL_Renderer *renderer, SDL_Rect *viewport) {
  if (!m_layout_visible) {
    // clean up texture and surface if any
    if (m_layout_tex) {
      SDL_DestroyTexture(m_layout_tex);
      m_layout_tex = NULL;
    }
    if (m_layout_surf) {
      free(m_layout_surf);
      m_layout_surf = NULL;
    }
    return;
  }

  if (!m_layout_surf) {
    // draw the keyboard layout to a texture
    int w,h,ch;

    FILE *lf = fopen((std::string(getenv("ENGINEBASIC_ROOT")) +
                      std::string("/sys/ui/KB_Template.png")).c_str(), "rb");
    if (!lf)
      return;

    m_layout_surf = (uint32_t *)stbi_load_from_file(lf, &w, &h, &ch, 4);

    fclose(lf);

    if (!m_layout_surf)
      return;

    pixel_t fg_save = fg_color;
    pixel_t bg_save = bg_color;
    // XXX: tv_write_px_ex() doesn't support the alpha channel; colored
    // template background will be overwritten.
    tv_setcolor(0x00000000, 0xffffffff);

    for (int i = 0; i < sizeof(usb_key_row)/sizeof(usb_key_row[0]); ++i) {
      if (usb_key_row[i] == -1)
        continue;

      utf8_int32_t sym = usb2ascii[keyboard_layout][i];
      utf8_int32_t sym_shift = usb2ascii[keyboard_layout][i + 128];
      utf8_int32_t sym_altgr = usb2ascii_altgr[keyboard_layout][i];
      utf8_int32_t sym_altgr_shift = usb2ascii_altgr[keyboard_layout][i + 128];

      // account for the variably sized keys at the start of each row
      int shift = usb_key_col[i] == 0 ? 0 : usb_row_shift[usb_key_row[i]] - KEY_SPACE_H;

      int x = shift + KEY_SPACE_H * usb_key_col[i] + LABEL_OFF_H;
      int y = (usb_key_row[i] - 1) * KEY_SPACE_V + LABEL_OFF_V;

      if (y >= 0 && y < TEMPLATE_HEIGHT - LABEL_SIZE - LABEL_SHIFT_OFF_V && x >= 0 && x <= TEMPLATE_WIDTH - LABEL_SIZE) {
        if (sym_shift >= 32)
          tv_write_px_ex(x, y, LABEL_SIZE, LABEL_SIZE, sym_shift, m_layout_surf, TEMPLATE_WIDTH);
        if (sym >= 32)
          tv_write_px_ex(x, y + LABEL_SHIFT_OFF_V, LABEL_SIZE, LABEL_SIZE, sym, m_layout_surf, TEMPLATE_WIDTH);
        if (sym_altgr >= 32)
          tv_write_px_ex(x + LABEL_SHIFT_OFF_V, y + LABEL_SHIFT_OFF_V, LABEL_SIZE, LABEL_SIZE, sym_altgr, m_layout_surf, TEMPLATE_WIDTH);
        if (sym_altgr_shift >= 32)
          tv_write_px_ex(x + LABEL_SHIFT_OFF_V, y, LABEL_SIZE, LABEL_SIZE, sym_altgr_shift, m_layout_surf, TEMPLATE_WIDTH);
      }
    }

    // write layout name on the space bar
    int cpos = 0;
    int start_x = 430 - utf8len(usb2ascii_names[keyboard_layout]) * LABEL_SIZE / 2;
    for (const char *cp = usb2ascii_names[keyboard_layout]; *cp;) {
      utf8_int32_t c;
      cp = (const char *)utf8codepoint(cp, &c);
      tv_write_px_ex(start_x + cpos, TEMPLATE_HEIGHT - LABEL_SIZE - LABEL_OFF_V, LABEL_SIZE, LABEL_SIZE, c, m_layout_surf, TEMPLATE_WIDTH);
      cpos += LABEL_SIZE;
    }

    tv_setcolor(fg_save, bg_save);

    // blend it in a little
    for (int i = 0; i < TEMPLATE_WIDTH * TEMPLATE_HEIGHT; ++i) {
      m_layout_surf[i] = (m_layout_surf[i] & 0x00ffffff) | 0xd0000000;
    }

    if (m_layout_tex)
      SDL_DestroyTexture(m_layout_tex);

    m_layout_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                     SDL_TEXTUREACCESS_STREAMING, TEMPLATE_WIDTH, TEMPLATE_HEIGHT);
    if (!m_layout_tex) {
      free(m_layout_surf);
      m_layout_surf = NULL;
      return;
    }

    SDL_SetTextureBlendMode(m_layout_tex, SDL_BLENDMODE_BLEND);

    SDL_UpdateTexture(m_layout_tex, NULL, m_layout_surf, TEMPLATE_WIDTH * 4);
  }

  SDL_Rect layout_rect =  {
    .x = viewport->w / ((960 - TEMPLATE_WIDTH) / 2) + viewport->x,
    .y = viewport->h - (TEMPLATE_HEIGHT * viewport->h / 540) - viewport->h / 54 + viewport->y,
    .w = TEMPLATE_WIDTH * viewport->w / 960,
    .h = TEMPLATE_HEIGHT * viewport->h / 540,
  };

  SDL_RenderSetViewport(renderer, &layout_rect);
  SDL_RenderCopy(renderer, m_layout_tex, NULL, NULL);
  SDL_RenderSetViewport(renderer, viewport);
}

uint8_t TKeyboard::begin(uint8_t clk, uint8_t dat, uint8_t flgLED,
                         uint8_t layout) {
  setLayout(layout);
  return 0;
}

void TKeyboard::end() {
}
