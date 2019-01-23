#include <TKeyboard.h>
#include "TPS2.h"
#include <SDL/SDL.h>
#include <ring_buffer.h>

TPS2 pb;

static const SDLKey ps2_to_sdl[] = {
SDLK_UNKNOWN,	// PS2KEY_NONE              0  // 継続またはバッファに変換対象なし
SDLK_LALT,	// PS2KEY_L_Alt		    1	// [左Alt]
SDLK_LSHIFT,	// PS2KEY_L_Shift		  2	// [左Shift]
SDLK_LCTRL,	// PS2KEY_L_Ctrl		  3	// [左Ctrl]
SDLK_RSHIFT,	// PS2KEY_R_Shift		  4	// [右Shift]
SDLK_RALT,	// PS2KEY_R_Alt		    5	// [右Alt]
SDLK_RCTRL,	// PS2KEY_R_Ctrl		  6	// [右Ctrl]
SDLK_LSUPER,	// PS2KEY_L_GUI		    7	// [左Windowsキー]
SDLK_RSUPER,	// PS2KEY_R_GUI		    8	// [右Windowsキー]
SDLK_NUMLOCK,	// PS2KEY_NumLock		  9	// [NumLock]
SDLK_SCROLLOCK,	// PS2KEY_ScrollLock	10	// [ScrollLock]
SDLK_CAPSLOCK,	// PS2KEY_CapsLock	  11	// [CapsLock]
SDLK_PRINT,	// PS2KEY_PrintScreen	12	// [PrintScreen]
SDLK_UNKNOWN,	// PS2KEY_HanZen		  13	// [半角/全角 漢字]
SDLK_INSERT,	// PS2KEY_Insert		  14	// [Insert]
SDLK_HOME,	// PS2KEY_Home		    15	// [Home]
SDLK_BREAK,	// PS2KEY_Pause		    16	// [Pause]
SDLK_UNKNOWN,	// PS2KEY_Romaji		  17	// [カタカナ ひらがな ローマ字]
SDLK_UNKNOWN,	// PS2KEY_APP			    18	// [メニューキー]
SDLK_UNKNOWN,	// PS2KEY_Henkan		  19	// [変換]
SDLK_UNKNOWN,	// PS2KEY_Muhenkan	  20	// [無変換]
SDLK_PAGEUP,	// PS2KEY_PageUp		  21	// [PageUp]
SDLK_PAGEDOWN,	// PS2KEY_PageDown	  22	// [PageDown]
SDLK_END,	// PS2KEY_End			    23	// [End]
SDLK_LEFT,	// PS2KEY_L_Arrow		  24	// [←]
SDLK_UP,	// PS2KEY_Up_Arrow	  25	// [↑]
SDLK_RIGHT,	// PS2KEY_R_Arrow		  26	// [→]
SDLK_DOWN,	// PS2KEY_Down_Arrow	27	// [↓]
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_ESCAPE,	// PS2KEY_ESC			    30	// [ESC]
SDLK_TAB,	// PS2KEY_Tab 		    31	// [Tab]
SDLK_RETURN,	// PS2KEY_Enter       32  // [Enter]
SDLK_BACKSPACE,	// PS2KEY_Backspace	  33	// [BackSpace]
SDLK_DELETE,	// PS2KEY_Delete		  34	// [Delete]
SDLK_SPACE,	// PS2KEY_Space		    35	// [空白]
SDLK_COLON,	// PS2KEY_Colon		    36	// [: *]
SDLK_SEMICOLON,	// PS2KEY_Semicolon	  37	// [; +]
SDLK_COMMA,	// PS2KEY_Kamma		    38	// [, <]
SDLK_MINUS,	// PS2KEY_minus		    39	// [- =]
SDLK_PERIOD,	// PS2KEY_Dot			    40	// [. >]
SDLK_QUESTION,	// PS2KEY_Question	  41	// [/ ?]
SDLK_AT,	// PS2KEY_AT			    42	// [@ `]
SDLK_LEFTBRACKET,	// PS2KEY_L_brackets	43	// [[ {]
SDLK_BACKSLASH,	// PS2KEY_Pipe		    44	// [\ |]
SDLK_RIGHTBRACKET,	// PS2KEY_R_brackets	45	// [] }]
SDLK_CARET,	// PS2KEY_Hat			    46	// [^ ~]
SDLK_UNKNOWN,	// PS2KEY_Ro			    47	// [\ _ ろ]
SDLK_0,	// PS2KEY_0			48	// [0 )]
SDLK_1,	// PS2KEY_1			49	// [1 !]
SDLK_2,	// PS2KEY_2			50	// [2 @]
SDLK_3,	// PS2KEY_3			51	// [3 #]
SDLK_4,	// PS2KEY_4			52	// [4 $]
SDLK_5,	// PS2KEY_5			53	// [5 %]
SDLK_6,	// PS2KEY_6			54	// [6 ^]
SDLK_7,	// PS2KEY_7			55	// [7 &]
SDLK_8,	// PS2KEY_8			56	// [8 *]
SDLK_9,	// PS2KEY_9			57	// [9 (]
SDLK_BACKSLASH,	// PS2KEY_Pipe2 58  // [\ |] (USキーボード用)
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_a,	// PS2KEY_A			65	// [a A]
SDLK_b,	// PS2KEY_B			66	// [b B]
SDLK_c,	// PS2KEY_C			67	// [c C]
SDLK_d,	// PS2KEY_D			68	// [d D]
SDLK_e,	// PS2KEY_E			69	// [e E]
SDLK_f,	// PS2KEY_F			70	// [f F]
SDLK_g,	// PS2KEY_G			71	// [g G]
SDLK_h,	// PS2KEY_H			72	// [h H]
SDLK_i,	// PS2KEY_I			73	// [i I]
SDLK_j,	// PS2KEY_J			74	// [j J]
SDLK_k,	// PS2KEY_K			75	// [k K]
SDLK_l,	// PS2KEY_L			76	// [l L]
SDLK_m,	// PS2KEY_M			77	// [m M]
SDLK_n,	// PS2KEY_N			78	// [n N]
SDLK_o,	// PS2KEY_O			79	// [o O]
SDLK_p,	// PS2KEY_P			80	// [p P]
SDLK_q,	// PS2KEY_Q			81	// [q Q]
SDLK_r,	// PS2KEY_R			82	// [r R]
SDLK_s,	// PS2KEY_S			83	// [s S]
SDLK_t,	// PS2KEY_T			84	// [t T]
SDLK_u,	// PS2KEY_U			85	// [u U]
SDLK_v,	// PS2KEY_V			86	// [v V]
SDLK_w,	// PS2KEY_W			87	// [w W]
SDLK_x,	// PS2KEY_X			88	// [x X]
SDLK_y,	// PS2KEY_Y			89	// [y Y]
SDLK_z,	// PS2KEY_Z			90	// [z Z]
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_UNKNOWN,
SDLK_EQUALS,	// PS2KEY_PAD_Equal	94	// [=]
SDLK_KP_ENTER,	// PS2KEY_PAD_Enter	95	// [Enter]
SDLK_KP0,	// PS2KEY_PAD_0		96  	// [0/Insert]
SDLK_KP1,	// PS2KEY_PAD_1		97  	// [1/End]
SDLK_KP2,	// PS2KEY_PAD_2		98  	// [2/DownArrow]
SDLK_KP3,	// PS2KEY_PAD_3		99  	// [3/PageDown]
SDLK_KP4,	// PS2KEY_PAD_4		100 	// [4/LeftArrow]
SDLK_KP5,	// PS2KEY_PAD_5		101 	// [5]
SDLK_KP6,	// PS2KEY_PAD_6		102 	// [6/RightArrow]
SDLK_KP7,	// PS2KEY_PAD_7		103 	// [7/Home]
SDLK_KP8,	// PS2KEY_PAD_8		104 	// [8/UPArrow]
SDLK_KP9,	// PS2KEY_PAD_9		105	  // [9/PageUp]
SDLK_KP_MULTIPLY,	// PS2KEY_PAD_Multi	106	// [*]
SDLK_KP_PLUS,	// PS2KEY_PAD_Plus	107	// [+]
SDLK_UNKNOWN,	// PS2KEY_PAD_Kamma	108	// [,]
SDLK_KP_MINUS,	// PS2KEY_PAD_Minus	109	// [-]
SDLK_KP_PERIOD,	// PS2KEY_PAD_DOT		110	// [./Delete]
SDLK_KP_DIVIDE,	// PS2KEY_PAD_Slash	111	// [/]
SDLK_F1,	// PS2KEY_F1 		112	// [F1]
SDLK_F2,	// PS2KEY_F2 		113	// [F2]
SDLK_F3,	// PS2KEY_F3 		114	// [F3]
SDLK_F4,	// PS2KEY_F4 		115	// [F4]
SDLK_F5,	// PS2KEY_F5 		116	// [F5]
SDLK_F6,	// PS2KEY_F6 		117	// [F6]
SDLK_F7,	// PS2KEY_F7 		118	// [F7]
SDLK_F8,	// PS2KEY_F8 		119	// [F8]
SDLK_F9,	// PS2KEY_F9 		120	// [F9]
SDLK_F10,	// PS2KEY_F10 	121	// [F10]
SDLK_F11,	// PS2KEY_F11 	122	// [F11]
SDLK_F12,	// PS2KEY_F12 	123	// [F12]
SDLK_F13,	// PS2KEY_F13 	124	// [F13]
SDLK_F14,	// PS2KEY_F14 	125	// [F14]
SDLK_F15,	// PS2KEY_F15 	126	// [F15]
SDLK_UNKNOWN,	// PS2KEY_F16 	127	// [F16]
SDLK_UNKNOWN,	// PS2KEY_F17 	128	// [F17]
SDLK_UNKNOWN,	// PS2KEY_F18 	129	// [F18]
SDLK_UNKNOWN,	// PS2KEY_F19 	130	// [F19]
SDLK_UNKNOWN,	// PS2KEY_F20 	131	// [F20]
SDLK_UNKNOWN,	// PS2KEY_F21 	132	// [F21]
SDLK_UNKNOWN,	// PS2KEY_F22 	133	// [F22]
SDLK_UNKNOWN,	// PS2KEY_F23 	134	// [F23]
SDLK_UNKNOWN,	// PS2KEY_PrevTrack		  135	// 前のトラック
SDLK_UNKNOWN,	// PS2KEY_WWW_Favorites	136	// ブラウザお気に入り
SDLK_UNKNOWN,	// PS2KEY_WWW_Refresh		137	// ブラウザ更新表示
SDLK_UNKNOWN,	// PS2KEY_VolumeDown		138	// 音量を下げる
SDLK_UNKNOWN,	// PS2KEY_Mute			    139	// ミュート
SDLK_UNKNOWN,	// PS2KEY_WWW_Stop		  140	// ブラウザ停止
SDLK_UNKNOWN,	// PS2KEY_Calc			    141	// 電卓
SDLK_UNKNOWN,	// PS2KEY_WWW_Forward		142	// ブラウザ進む
SDLK_UNKNOWN,	// PS2KEY_VolumeUp		  143	// 音量を上げる
SDLK_UNKNOWN,	// PS2KEY_PLAY			    144	// 再生
SDLK_POWER,	// PS2KEY_POWER			    145	// 電源ON
SDLK_UNKNOWN,	// PS2KEY_WWW_Back		  146	// ブラウザ戻る
SDLK_UNKNOWN,	// PS2KEY_WWW_Home		  147	// ブラウザホーム
SDLK_UNKNOWN,	// PS2KEY_Sleep			    148	// スリープ
SDLK_UNKNOWN,	// PS2KEY_Mycomputer		149	// マイコンピュータ
SDLK_UNKNOWN,	// PS2KEY_Mail			    150	// メーラー起動
SDLK_UNKNOWN,	// PS2KEY_NextTrack		  151	// 次のトラック
SDLK_UNKNOWN,	// PS2KEY_MEdiaSelect		152	// メディア選択
SDLK_UNKNOWN,	// PS2KEY_Wake			    153	// ウェイクアップ
SDLK_UNKNOWN,	// PS2KEY_Stop			    154	// 停止
SDLK_UNKNOWN,	// PS2KEY_WWW_Search		155	// ウェブ検索
};

bool TKeyboard::state(uint8_t keycode) {
  const Uint8 *state = SDL_GetKeyState(NULL);
  int sdlcode = 0;
  if (keycode < sizeof(ps2_to_sdl)/sizeof(*ps2_to_sdl))
    sdlcode = ps2_to_sdl[keycode];
  return state[sdlcode];
}

uint8_t TPS2::available() {
  SDL_Event events[8];
  SDL_PumpEvents();
  return SDL_PeepEvents(events, 8, SDL_PEEKEVENT, SDL_KEYUPMASK|SDL_KEYDOWNMASK);
}

keyEvent TKeyboard::read() {
  keyinfo ki;
  ki.value = 0;
  SDL_Event event;
  SDL_PumpEvents();
  if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYUPMASK|SDL_KEYDOWNMASK) == 1) {
    int unicode = event.key.keysym.unicode;
    if (unicode)
      ki.kevt.code = unicode;
    else {
      ki.kevt.KEY = 1;
      for (unsigned int i = 0; i < sizeof(ps2_to_sdl)/sizeof(*ps2_to_sdl); ++i) {
        if (ps2_to_sdl[i] == event.key.keysym.sym) {
          ki.kevt.code = i;
          break;
        }
      }
    }
    if (event.type == SDL_KEYUP)
      ki.kevt.BREAK = 1;
    ki.kevt.ALT = !!(event.key.keysym.mod & (KMOD_LALT | KMOD_RALT));
  }
  return ki.kevt;
}

uint8_t TKeyboard::begin(uint8_t clk, uint8_t dat, uint8_t flgLED, uint8_t layout) {
  return 0;
}

void TKeyboard::end() {
}
