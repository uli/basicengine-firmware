/*
   TOYOSHIKI Tiny BASIC for Arduino
   (C)2012 Tetsuya Suzuki
   GNU General Public License
   2017/03/22, Modified by Tamakichi、for Arduino STM32
   Modified for BASIC Engine by Ulrich Hecht
   (C) 2017 Ulrich Hecht
 */

#include <Arduino.h>
#include <unistd.h>
#include <stdlib.h>
#include "ttconfig.h"
#include "vs23s010.h"

#ifndef os_memcpy
#define os_memcpy memcpy
#endif

#include "epigrams.h"

#define STR_EDITION "ESP8266"
#define STR_VARSION "V0.84"

#include "basic.h"

// TOYOSHIKI TinyBASIC プログラム利用域に関する定義
int size_list;
#define SIZE_VAR  256    // 利用可能変数サイズ(A-Z,A0:A6-Z0:Z6の26+26*7=208)
#define MAX_VAR_NAME 32  // maximum length of variable names
#define SIZE_ARRY 100    // 配列変数サイズ(@(0)～@(99)
#define SIZE_GSTK 30     // GOSUB stack size(2/nest) :10ネストまでOK
#define SIZE_LSTK 50     // FOR stack size(5/nest) :  10ネストまでOK
#define SIZE_IFSTK 16	 // IF stack
#define SIZE_ASTK 16	// argument stack

// *** フォント参照 ***************
const uint8_t* ttbasic_font = TV_DISPLAY_FONT;

#define MIN_FONT_SIZE_X 6
#define MIN_FONT_SIZE_Y 8
#define NUM_FONTS 3
static const uint8_t *fonts[] = {
  console_font_6x8,
  console_font_8x8,
  font6x8tt,
};

// **** スクリーン管理 *************
uint8_t workarea[VS23_MAX_X/MIN_FONT_SIZE_X * VS23_MAX_Y/MIN_FONT_SIZE_Y];
uint8_t scmode = USE_SCREEN_MODE;

#if USE_NTSC == 1 || USE_VS23 == 1
  #include "tTVscreen.h"
  tTVscreen   sc0; 
#endif

uint16_t tv_get_gwidth();
uint16_t tv_get_gheight();

// Saved background and text window dimensions.
// These values are set when a program is interrupted and the text window is
// forced to be visible at the bottom of the screen. That way, the original
// geometries can be restored when the program is CONTinued.
static int original_bg_height[VS23_MAX_BG];
static bool turn_bg_back_on[VS23_MAX_BG];
bool restore_bgs = false;
static uint16_t original_text_pos[2];
bool restore_text_window = false;

#define KEY_ENTER 13

// **** I2Cライブラリの利用設定 ****
#if OLD_WIRE_LIB == 1
  // Wireライブラリ変更前の場合
  #if I2C_USE_HWIRE == 0
    #include <Wire.h>
    #define I2C_WIRE  Wire
  #else
    #include <HardWire.h>
    HardWire HWire(1, I2C_FAST_MODE); // I2C1を利用
    #define I2C_WIRE  HWire
  #endif
#else 
  // Wireライブラリ変更ありの場合
  #if I2C_USE_HWIRE == 0
    #include <SoftWire.h>
    TwoWire SWire(SCL, SDA, SOFT_STANDARD);
    #define I2C_WIRE  SWire
  #else
    #include <Wire.h>
    #define I2C_WIRE  Wire
  #endif
#endif

// *** SDカード管理 ****************
#include "sdfiles.h"
#if USE_SD_CARD == 1
sdfiles bfs;
#endif

#define MAX_USER_FILES 16
Unifile user_files[MAX_USER_FILES];

// *** システム設定関連 **************
#define CONFIG_NTSC 65534  // EEPROM NTSC設定値保存番号
#define CONFIG_KBD  65533  // EEPROM キーボード設定
#define CONFIG_PRG  65532  // 自動起動設定

typedef struct {
  int16_t NTSC;        // NTSC設定 (0,1,2,3)
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  int16_t STARTPRG;    // 自動起動(-1,なし 0～9:保存プログラム番号)
} SystemConfig;
SystemConfig CONFIG;

// プロトタイプ宣言
void loadConfig();
void isaveconfig();
int32_t getNextLineNo(int32_t lineno);
void mem_putch(uint8_t c);
const uint8_t* tv_getFontAdr();
void tv_fontInit();
void tv_toneInit();
void tv_tone(int16_t freq, int16_t duration);
void tv_notone();
void tv_write(uint8_t c);
unsigned char* GROUP(basic_core) iexe();
num_t GROUP(basic_core) iexp(void);
BString istrexp(void);
void error(uint8_t flgCmd);

// **** RTC用宣言 ********************
#if USE_INNERRTC == 1
  #include <RTClock.h>
  #include <time.h>
  RTClock rtc(RTCSEL_LSE);
#endif

// 定数
#define CONST_HIGH   1
#define CONST_LOW    0
#define CONST_LSB    LSBFIRST
#define CONST_MSB    MSBFIRST
#define SRAM_TOP     0x20000000

// Terminal control(文字の表示・入力は下記の3関数のみ利用)
#define c_getch( ) sc0.get_ch()
#define c_kbhit( ) sc0.isKeyIn()

#include <TKeyboard.h>
extern TKeyboard kb;

// 文字の出力
inline void c_putch(uint8_t c, uint8_t devno) {
  if (devno == 0)
    sc0.putch(c);
  else if (devno == 1)
    Serial.write(c);
  else if (devno == 2)
    sc0.gputch(c);
  else if (devno == 3)
    mem_putch(c);
  else if (devno == 4)
    bfs.putch(c);
}

// 改行
void newline(uint8_t devno) {
  if (devno==0) {
    // XXX: this drains the keyboard buffer; is that a problem?
    c_kbhit();
    if (kb.state(PS2KEY_L_Ctrl)) {
      uint32_t m = millis() + 200;
      while (millis() < m) {
        c_kbhit();
        if (!kb.state(PS2KEY_L_Ctrl))
          break;
      }
    }
    sc0.newLine();
  } else if (devno == 1)
    Serial.println("");
  else if (devno == 2)
    sc0.gputch('\n');
  else if (devno == 3)
    mem_putch('\n');
  else if (devno == 4) {
    bfs.putch('\x0d');
    bfs.putch('\x0a');
  }
}

// tick用支援関数
void iclt() {
//  systick_uptime_millis = 0;
}

// 乱数
int getrnd(int value) {
  return random(value);
}

#ifdef ESP8266
#define __FLASH__ ICACHE_RODATA_ATTR
#endif

// キーワードテーブル
#include "kwtbl.h"

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))

// i-code(Intermediate code) assignment
#include "kwenum.h"

// List formatting condition
// 後ろに空白を入れない中間コード
const uint8_t i_nsa[] __FLASH__ = {
  I_RETURN, I_END,
  I_CLS,I_CLT,
  I_HIGH, I_LOW, I_CW, I_CH, I_GW, I_GH,
  I_UP, I_DOWN, I_RIGHT, I_LEFT,
  I_INKEY,I_VPEEK, I_CHR, I_ASC, I_HEX, I_BIN,I_LEN, I_STRSTR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT, I_LNOT, I_BITREV, I_XOR,
  I_RND, I_ABS, I_FREE, I_TICK, I_PEEK, I_I2CW, I_I2CR,
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F, I_PWM,
  I_DIN, I_ANA, I_MAP, I_DMP,
  I_LSB, I_MSB, I_EEPREAD, I_MPRG, I_MFNT,
  I_SREAD, I_SREADY, I_GPEEK, I_GINP,
  I_RET, I_ARG, I_ARGSTR,
};

// 前が定数か変数のとき前の空白をなくす中間コード
const uint8_t i_nsb[] __FLASH__ = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_DIVR, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT, I_OR, I_AND,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2,I_LT, I_LNOT, I_BITREV, I_XOR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL
};

// insert a blank before intermediate code
const uint8_t i_sf[] __FLASH__  = {
  I_ATTR, I_CLS, I_COLOR, I_DATE, I_END, I_FILES, I_TO, I_STEP,I_QUEST,I_LAND, I_LOR,
  I_GETDATE,I_GETTIME,I_GOSUB,I_GOTO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_SETDATE,I_WAIT,I_EEPFORMAT, I_EEPWRITE,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BLIT, I_SWRITE, I_SPRINT,I_SMODE,
  I_TONE, I_NOTONE, I_CSCROLL, I_GSCROLL,
};

// exception search function
char sstyle(uint8_t code,
            const uint8_t *table, uint8_t count) {
  while(count--) //中間コードの数だけ繰り返す
    if (code == pgm_read_byte(&table[count])) //もし該当の中間コードがあったら
      return 1;  //1を持ち帰る
  return 0; //（なければ）0を持ち帰る
}

// exception search macro
#define nospacea(c) sstyle(c, i_nsa, sizeof(i_nsa))
#define nospaceb(c) sstyle(c, i_nsb, sizeof(i_nsb))
#define spacef(c) sstyle(c, i_sf, sizeof(i_sf))

// エラーメッセージ定義
uint8_t err; // Error message index

#define ESTR(n,s) static const char _errmsg_ ## n[] PROGMEM = s;
#include "errdef.h"

#undef ESTR
#define ESTR(n,s) _errmsg_ ## n,
static const char* const errmsg[] PROGMEM = {
#include "errdef.h"
};

#undef ESTR

#include "error.h"

// RAM mapping
char lbuf[SIZE_LINE];          // コマンド入力バッファ
char tbuf[SIZE_LINE];          // テキスト表示用バッファ
int32_t tbuf_pos = 0;
unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer

NumVariables var;
VarNames var_names;
StringVariables svar;
VarNames svar_names;

#define MAX_ARRAY_DIMS 4
NumArrayVariables<num_t> num_arr;
VarNames num_arr_names;
StringArrayVariables<BString> str_arr;
VarNames str_arr_names;

VarNames proc_names;
Procedures proc;

unsigned char *listbuf; // Pointer to program list area

// macros for in/decrementing stack pointers with bounds checking
#define inc_stk(idx, size, errnum, ret) \
  do { \
    idx++;	\
    if (idx >= size) {	\
      err = errnum;	\
      return ret;	\
    }	\
  } while (0)

#define dec_stk(idx, errnum, ret) \
  do { \
    if (!idx) {	\
      err = errnum;	\
      return ret;	\
    }	\
    idx--;	\
  } while (0)


unsigned char* clp;               // Pointer current line
unsigned char* cip;               // Pointer current Intermediate code
unsigned char* gstk[SIZE_GSTK];   // GOSUB stack
unsigned char gstki;              // GOSUB stack index
num_t astk_num[SIZE_ASTK];
unsigned char astk_num_i;
BString astk_str[SIZE_ASTK];
unsigned char astk_str_i;
unsigned char* lstk[SIZE_LSTK];   // FOR stack
unsigned char lstki;              // FOR stack index

uint8_t *cont_clp = NULL;
uint8_t *cont_cip = NULL;

bool ifstk[SIZE_IFSTK];		  // IF stack
unsigned char ifstki;		  // IF stack index
#define inc_ifstk(ret)	inc_stk(ifstki, SIZE_IFSTK, ERR_IFSTKOF, ret)
#define dec_ifstk(ret)	dec_stk(ifstki,	ERR_IFSTKUF, ret)

#define MAX_RETVALS 4
num_t retval[MAX_RETVALS];        // multi-value returns (numeric)

uint8_t prevPressKey = 0;         // 直前入力キーの値(INKEY()、[ESC]中断キー競合防止用)
uint8_t lfgSerial1Opened = false;  // Serial1のオープン設定フラグ

// メモリへの文字出力
inline void mem_putch(uint8_t c) {
  if (tbuf_pos < SIZE_LINE) {
    tbuf[tbuf_pos] = c;
    tbuf_pos++;
  }
}

uint8_t* sanitize_addr(uint32_t vadr) {
  // XXX: This needs to be a lot smarter if we want it to reliably prevent
  // crashes from accidental memory accesses.
  if (vadr < 0x30000000U)
    return 0;
  return (uint8_t *)vadr;
}

// Standard C libraly (about) same functions
char c_isprint(char c) {
  //return(c >= 32 && c <= 126);
  return(c >= 32 && c!=127 );
}
char c_isspace(char c) {
  return(c == ' ' || (c <= 13 && c >= 9));
}

// 文字列の右側の空白文字を削除する
char* tlimR(char* str) {
  uint32_t len = strlen(str);
  for (uint32_t i = len - 1; i>0; i--) {
    if (str[i] == ' ') {
      str[i] = 0;
    } else {
      break;
    }
  }
  return str;
}

// コマンド引数取得(int32_t,引数チェックあり)
inline uint8_t getParam(num_t& prm, num_t v_min,  num_t v_max, token_t next_token) {
  prm = iexp();
  if (!err &&  (prm < v_min || prm > v_max))
    err = ERR_VALUE;
  else if (next_token != I_NONE && *cip++ != next_token) {
    err = ERR_SYNTAX;
  }
  return err;
}

inline uint32_t getParam(uint32_t& prm, uint32_t v_min, uint32_t v_max, token_t next_token) {
  prm = iexp();
  if (!err &&  (prm < v_min || prm > v_max))
    err = ERR_VALUE;
  else if (next_token != I_NONE && *cip++ != next_token) {
    err = ERR_SYNTAX;
  }
  return err;
}

#ifdef FLOAT_NUMS
// コマンド引数取得(int32_t,引数チェックあり)
inline uint8_t getParam(int32_t& prm, int32_t v_min,  int32_t v_max, token_t next_token) {
  num_t p = iexp();
  if (!err && (int)p != p)
    err = ERR_VALUE;
  prm = p;
  if (!err &&  (prm < v_min || prm > v_max || ((int)prm) != prm))
    err = ERR_VALUE;
  else if (next_token != I_NONE && *cip++ != next_token) {
    err = ERR_SYNTAX;
  }
  return err;
}
#endif

// コマンド引数取得(int32_t,引数チェックなし)
inline uint8_t getParam(uint32_t& prm, token_t next_token) {
  prm = iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    err = ERR_SYNTAX;
  }
  return err;
}

// コマンド引数取得(uint32_t,引数チェックなし)
inline uint8_t getParam(num_t& prm, token_t next_token) {
  prm = iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    if (next_token == I_OPEN || next_token == I_CLOSE)
      err = ERR_PAREN;
    else
      err = ERR_SYNTAX;
  }
  return err;
}

#ifdef FLOAT_NUMS
// コマンド引数取得(uint32_t,引数チェックなし)
inline uint8_t getParam(int32_t& prm, token_t next_token) {
  num_t p = iexp();
  if (!err && ((int)p) != p)
    err = ERR_SYNTAX;
  prm = p;
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    err = ERR_SYNTAX;
  }
  return err;
}
#endif

// '('チェック関数
inline uint8_t checkOpen() {
  if (*cip != I_OPEN) err = ERR_PAREN;
  else cip++;
  return err;
}

// ')'チェック関数
inline uint8_t checkClose() {
  if (*cip != I_CLOSE) err = ERR_PAREN;
  else cip++;
  return err;
}

inline bool is_strexp() {
  // XXX: does not detect string comparisons (numeric)
  return (*cip == I_STR ||
          (*cip == I_SVAR && cip[2] != I_SQOPEN) ||
          *cip == I_ARGSTR ||
          *cip == I_STRSTR ||
          *cip == I_CHR ||
          *cip == I_HEX ||
          *cip == I_BIN ||
          *cip == I_DMP ||
          *cip == I_LEFTSTR ||
          *cip == I_RIGHTSTR ||
          *cip == I_MIDSTR ||
          *cip == I_CWD ||
          *cip == I_INKEYSTR
         );
}

// 1桁16進数文字を整数に変換する
uint16_t hex2value(char c) {
  if (c <= '9' && c >= '0')
    return c - '0';
  else if (c <= 'f' && c >= 'a')
    return c - 'a' + 10;
  else if (c <= 'F' && c >= 'A')
    return c - 'A' + 10;
  return 0;
}

// 1文字出力
void c_puts(const char *s, uint8_t devno) {
  while (*s) c_putch(*s++, devno);  //終端でなければ出力して繰り返す
}
void c_puts_P(const char *s, uint8_t devno) {
  while (pgm_read_byte(s)) c_putch(pgm_read_byte(s++), devno);
}

// Print numeric specified columns
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'SNNNNN' S:符号 N:数値 or 空白
//  dで桁指定時は空白補完する
//
void putnum(num_t value, int8_t d, uint8_t devno) {
#ifdef FLOAT_NUMS
  char f[] = "%.g";
#else
  char f[] = "%.d";
#endif
  f[1] = '0' + d;
  sprintf(lbuf, f, value);
  c_puts(lbuf, devno);
}

// 16進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'XXXX' X:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
//
void putHexnum(uint32_t value, uint8_t d, uint8_t devno=0) {
  char s[] = "%0.X";
  s[2] = '0' + d;
  sprintf(lbuf, s, value);
  c_puts(lbuf,devno);
}

// 2進数の出力
// 引数
//  value : 出力対象数値
//  d     : 桁指定(0で指定無し)
//  devno : 出力先デバイスコード
// 機能
// 'BBBBBBBBBBBBBBBB' B:数値
//  dで桁指定時は0補完する
//  符号は考慮しない
//
void putBinnum(uint32_t value, uint8_t d, uint8_t devno=0) {
  uint32_t bin = (uint32_t)value;  // 符号なし16進数として参照利用する
  uint32_t b;
  uint32_t dig = 0;

  for (uint8_t i = 0; i < 16; i++) {
    b =(bin>>(15-i)) & 1;
    lbuf[i] = b ? '1' : '0';
    if (!dig && b)
      dig = 16-i;
  }
  lbuf[16] = 0;

  if (d > dig)
    dig = d;
  c_puts(&lbuf[16-dig],devno);
}

void get_input(bool numeric) {
  char c; //文字
  uint8_t len; //文字数

  len = 0; //文字数をクリア
  while(1) {
    c = c_getch();
    if (c == KEY_ENTER) {
      break;
    } else if (c == SC_KEY_CTRL_C || c==27) {
      err = ERR_CTR_C;
      break;
    } else
    //［BackSpace］キーが押された場合の処理（行頭ではないこと）
    if (((c == 8) || (c == 127)) && (len > 0)) {
      len--; //文字数を1減らす
      //c_putch(8); c_putch(' '); c_putch(8); //文字を消す
      sc0.movePosPrevChar();
      sc0.delete_char();
    } else
    //行頭の符号および数字が入力された場合の処理（符号込みで6桁を超えないこと）
    if (len < SIZE_LINE - 1 && (!numeric || c == '.' ||
        (len == 0 && (c == '+' || c == '-')) || isDigit(c)) ) {
      lbuf[len++] = c; //バッファへ入れて文字数を1増やす
      c_putch(c); //表示
    } else {
      sc0.beep();
    }
  }
  newline(); //改行
  lbuf[len] = 0; //終端を置く
}

// 数値の入力
num_t getnum() {
  num_t value;

  get_input(true);
  value = strtonum(lbuf, NULL);
  // XXX: say "EXTRA IGNORED" if there is unused input?

  return value;
}

BString getstr() {
  get_input();
  return BString(lbuf);
}

// キーワード検索
//[戻り値]
//  該当なし   : -1
//  見つかった : キーワードコード
int lookup(char* str) {
  for (uint8_t i = 0; i < SIZE_KWTBL; ++i) {
    if (kwtbl[i] && !strncasecmp_P(str, kwtbl[i], strlen_P(kwtbl[i])))
      return i;
  }
  return -1;
}

uint8_t parse_identifier(char *ptok, char *vname) {
  uint8_t var_len = 0;
  uint8_t v;
  char *p = ptok;
  do {
    v = vname[var_len++] = *p++;
  } while (isAlphaNumeric(v) && var_len < MAX_VAR_NAME-1);
  vname[--var_len] = 0;     // terminate C string
  return var_len;
}

uint32_t getlineno(unsigned char *lp);

//
// Convert text to intermediate code
// [Return value]
// 0 or intermediate code number of bytes
//
// If find_prg_text is true (default), variable and procedure names
// are designated as belonging to the program; if not, they are considered
// temporary, even if the input line starts with a number.
uint8_t SMALL toktoi(bool find_prg_text) {
  int16_t i;
  int key;
  uint8_t len = 0;	// length of sequence of intermediate code
  char *ptok;		// pointer to the inside of one word
  char *s = lbuf;	// pointer to the inside of the string buffer
  char c;		// Character used to enclose string (")
  num_t value;		// constant
  uint32_t hex;		// hexadecimal constant
  uint16_t hcnt;	// hexadecimal digit count
  uint8_t var_len;	// variable name length
  char vname [MAX_VAR_NAME];	// variable name

  bool is_prg_text = false;

  while (*s) {                  //文字列1行分の終端まで繰り返す
    while (c_isspace(*s)) s++;  //空白を読み飛ばす

    key = lookup(s);
    if (key >= 0) {
      // 該当キーワードあり
      if (len >= SIZE_IBUF - 1) {      // もし中間コードが長すぎたら
	err = ERR_IBUFOF;              // エラー番号をセット
	return 0;                      // 0を持ち帰る
      }
      ibuf[len++] = key;                 // 中間コードを記録
      s+= strlen_P(kwtbl[key]);

    } else {
      //err = ERR_SYNTAX; //エラー番号をセット
      //return 0;
    }

    // 16進数の変換を試みる $XXXX
    if (key == I_DOLLAR) {
      if (isHexadecimalDigit(*s)) {   // もし文字が16進数文字なら
	hex = 0;              // 定数をクリア
	hcnt = 0;             // 桁数
	do { //次の処理をやってみる
	  hex = (hex<<4) + hex2value(*s++); // 数字を値に変換
	  hcnt++;
	} while (isHexadecimalDigit(*s)); //16進数文字がある限り繰り返す

	if (hcnt > 8) {      // 桁溢れチェック
	  err = ERR_VOF;     // エラー番号オバーフローをセット
	  return 0;          // 0を持ち帰る
	}

	if (len >= SIZE_IBUF - 3) { // もし中間コードが長すぎたら
	  err = ERR_IBUFOF;         // エラー番号をセット
	  return 0;                 // 0を持ち帰る
	}
	//s = ptok; // 文字列の処理ずみの部分を詰める
	len--;    // I_DALLARを置き換えるために格納位置を移動
	ibuf[len++] = I_HEXNUM;  //中間コードを記録
	ibuf[len++] = hex & 255; //定数の下位バイトを記録
	ibuf[len++] = hex >> 8;  //定数の上位バイトを記録
	ibuf[len++] = hex >> 16;
	ibuf[len++] = hex >> 24;
      }
    }

    //コメントへの変換を試みる
    if(key == I_REM|| key == I_SQUOT) {       // もし中間コードがI_REMなら
      while (c_isspace(*s)) s++;         // 空白を読み飛ばす
      ptok = s;                          // コメントの先頭を指す

      for (i = 0; *ptok++; i++) ;        // コメントの文字数を得る
      if (len >= SIZE_IBUF - 2 - i) {    // もし中間コードが長すぎたら
	err = ERR_IBUFOF;                // エラー番号をセット
	return 0;                        // 0を持ち帰る
      }

      ibuf[len++] = i;                   // コメントの文字数を記録
      while (i--) {                      // コメントの文字数だけ繰り返す
	ibuf[len++] = *s++;              // コメントを記録
      }
      break;                             // 文字列の処理を打ち切る（終端の処理へ進む）
    }

    if (key == I_PROC) {
      if (!is_prg_text) {
        err = ERR_COM;
        return 0;
      }
      if (len >= SIZE_IBUF - 1) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF;
	return 0;
      }

      while (c_isspace(*s)) s++;
      s += parse_identifier(s, vname);

      int idx = proc_names.assign(vname, true);
      ibuf[len++] = idx;
      proc.reserve(proc_names.varTop());
    }
    
    if (key == I_CALL) {
      while (c_isspace(*s)) s++;
      s += parse_identifier(s, vname);
      int idx = proc_names.assign(vname, is_prg_text);
      proc.reserve(proc_names.varTop());
      ibuf[len++] = idx;
    }
    
    // done if keyword parsed
    if (key >= 0) {
      find_prg_text = false;
      continue;
    }

    // Attempt to convert to constant
    ptok = s;                            // Points to the beginning of a word
    if (isDigit(*ptok)) {
      if (len >= SIZE_IBUF - 3) { // If the intermediate code is too long
	err = ERR_IBUFOF;
	return 0;
      }
      value = strtonum(ptok, &ptok);
      s = ptok;			// Stuff the processed part of the character string
      ibuf[len++] = I_NUM;	// Record intermediate code
      os_memcpy(ibuf+len, &value, sizeof(num_t));
      len += sizeof(num_t);
      if (find_prg_text) {
        is_prg_text = true;
        find_prg_text = false;
      }
    } else if (*s == '\"' ) {
      // Attempt to convert to a character string

      c = *s++;		// Remember " and go to the next character
      ptok = s;		// Points to the beginning of the string

      // Get the number of characters in a string
      for (i = 0; (*ptok != c) && c_isprint(*ptok); i++)
	ptok++;

      if (len >= SIZE_IBUF - 1 - i) { // if the intermediate code is too long
	err = ERR_IBUFOF;
	return 0;
      }

      ibuf[len++] = I_STR;
      ibuf[len++] = i; // record the number of characters in the string
      while (i--) {
	ibuf[len++] = *s++; // Record character
      }
      if (*s == c)	// If the character is ", go to the next character
        s++;
    } else if (isAlpha(*ptok)) {
      // Try converting to variable
      var_len = 0;
      if (len >= SIZE_IBUF - 2) { // if the intermediate code is too long
	err = ERR_IBUFOF;
	return 0;
      }

      uint8_t v;
      char *p = ptok;
      do {
	v = vname[var_len++] = *p++;
#ifdef DEBUG_VAR
	Serial.printf("c %c (%d)\n", v, isAlphaNumeric(v));
#endif
      } while (isAlphaNumeric(v) && var_len < MAX_VAR_NAME-1);
      vname[--var_len] = 0;     // terminate C string

      p--;
      if (*p == '$' && p[1] == '(') {
        ibuf[len++] = I_STRARR;
        int idx = str_arr_names.assign(vname, is_prg_text);
        if (idx < 0)
          goto oom;
        ibuf[len++] = idx;
        if (str_arr.reserve(str_arr_names.varTop()))
          goto oom;
        s += var_len + 1;
        ptok++;
      } else if (*p == '$') {
	ibuf[len++] = I_SVAR;
	int idx = svar_names.assign(vname, is_prg_text);
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if (svar.reserve(svar_names.varTop()))
	  goto oom;
	s += var_len + 1;
	ptok++;
      } else if (*p == '(') {
        ibuf[len++] = I_VARARR;
        int idx = num_arr_names.assign(vname, is_prg_text);
        if (idx < 0)
          goto oom;
        ibuf[len++] = idx;
        if (num_arr.reserve(num_arr_names.varTop()))
          goto oom;
        s += var_len + 1;
        ptok++;
      } else {
	// Convert to intermediate code
	ibuf[len++] = I_VAR; //中間コードを記録
	int idx = var_names.assign(vname, is_prg_text);
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if (var.reserve(var_names.varTop()))
	  goto oom;
	s += var_len;
      }
    } else { // if none apply
      err = ERR_SYNTAX;
      return 0;
    }
  }

  ibuf[len++] = I_EOL;
  return len;

oom:
  err = ERR_OOM;
  return 0;
}


// Return free memory size
int list_free() {
  unsigned char* lp;

  for (lp = listbuf; *lp; lp += *lp) ;  // Move the pointer to the end of the list
  return listbuf + size_list - lp - 1; // Calculate the rest and return it
}

// Get line numbere by line pointer
uint32_t getlineno(unsigned char *lp) {
  num_t l;
  if(*lp == 0) //もし末尾だったら
    return (uint32_t)-1;
  os_memcpy(&l, lp+1, sizeof(num_t));
  return l;
}

// Search line by line number
unsigned char* getlp(uint32_t lineno) {
  unsigned char *lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp) //先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) //もし指定の行番号以上なら
      break;  //繰り返しを打ち切る

  return lp; //ポインタを持ち帰る
}

// ラベルでラインポインタを取得する
// pLabelは [I_STR][長さ][ラベル名] であること
unsigned char* getlpByLabel(uint8_t* pLabel) {
  unsigned char *lp; //ポインタ
  uint8_t len;
  pLabel++;
  len = *pLabel; // 長さ取得
  pLabel++;      // ラベル格納位置

  for (lp = listbuf; *lp; lp += *lp)  { //先頭から末尾まで繰り返す
    if ( *(lp+sizeof(num_t)+1) == I_STR ) {
      if (len == *(lp+sizeof(num_t)+2)) {
	if (strncmp((char*)pLabel, (char*)(lp+sizeof(num_t)+3), len) == 0) {
	  return lp;
	}
      }
    }
  }
  return NULL;
}

// 行番号から行インデックスを取得する
uint32_t getlineIndex(uint32_t lineno) {
  unsigned char *lp; //ポインタ
  uint32_t index = 0;
  uint32_t rc = INT32_MAX;
  for (lp = listbuf; *lp; lp += *lp) { // 先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) {         // もし指定の行番号以上なら
      rc = index;
      break;                                   // 繰り返しを打ち切る
    }
    index++;
  }
  return rc;
}

// ELSE中間コードをサーチする
// 引数   : 中間コードプログラムポインタ
// 戻り値 : NULL 見つからない
//          NULL以外 LESEの次のポインタ
//
uint8_t* getELSEptr(uint8_t* p) {
#define inc_lifstk	inc_stk(lifstki, SIZE_IFSTK, ERR_IFSTKOF, NULL)
#define dec_lifstk	dec_stk(lifstki, ERR_IFSTKUF, NULL)
  uint8_t* rc = NULL;
  uint8_t* lp;
  bool lifstk[SIZE_IFSTK];
  unsigned char lifstki = 1;
  lifstk[0] = ifstk[ifstki-1];

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (lp = p; ; ) {
    switch(*lp) {
    case I_IF:    // IF命令
      lp++;
      lifstk[lifstki] = false;
      if (*lp == I_THEN) {
        lp++;
        // If THEN is followed by EOL, it's a multiline IF.
        if (*lp == I_EOL) {
          lifstk[lifstki] = true;
          // XXX: Shouldn't this be taken care of by the I_EOL handler?
          clp += *clp;
          lp = cip = clp + sizeof(num_t) + 1;
        }
      }
      inc_lifstk;
      break;
    case I_ELSE:  // ELSE命令
      if (lifstki == 1) {
        // Found the highest-level ELSE, we're done.
        rc = lp+1;
        goto DONE;
      }
      lp++;
      break;
    case I_STR:     // 文字列
      lp += lp[1]+1;
      break;
    case I_NUM:     // 定数
      lp += sizeof(num_t) + 1;
      break;
    case I_HEXNUM:
      lp+=5;        // 整数2バイト+中間コード1バイト分移動
      break;
    case I_VAR:     // 変数
    case I_VARARR:
    case I_CALL:
    case I_PROC:
      lp+=2;        // 変数名
      break;
    case I_EOL:
      if (lifstk[lifstki-1] == false) {
        // Current IF is single-line, so it's finished here.
        dec_lifstk;
        if (!lifstki) {
          // This is the end of the last IF, and we have not found a
          // matching ELSE, meaning there is none. We're done.
          goto DONE;
        }
      }
      // fallthrough
    case I_REM:
    case I_SQUOT:
      // Continue at next line.
      clp += *clp;
      // XXX: What about EOT?
      lp = cip = clp + sizeof(num_t) + 1;
      break;
    case I_ENDIF:
      dec_lifstk;
      if (lifstk[lifstki] != true) {
        // Encountered ENDIF while not in a multiline IF.
        // Not sure what to do here; this could happen if someone
        // jumped into an ELSE block with another IF inside.
        // Probably screwed up enough to consider it an error.
        err = ERR_IFSTKUF;
        goto DONE;
      }
      lp++;
      if (!lifstki) {
        // End of the last IF, no ELSE found -> done.
        goto DONE;
      }
    default:        // その他
      lp++;
      break;
    }
  }
DONE:
  return rc;
}

// プログラム行数を取得する
uint32_t countLines(uint32_t st = 0, uint32_t ed = UINT32_MAX) {
  unsigned char *lp; //ポインタ
  uint32_t cnt = 0;
  uint32_t lineno;
  for (lp = listbuf; *lp; lp += *lp)  {
    lineno = getlineno(lp);
    if (lineno == (uint32_t)-1)
      break;
    if ( (lineno >= st) && (lineno <= ed))
      cnt++;
  }
  return cnt;
}

// Insert i-code to the list
// Insert [ibuf] in [listbuf]
//  [ibuf]: [1: data length] [1: I_NUM] [2: line number] [intermediate code]
//
void inslist() {
  unsigned char *insp;     // 挿入位置ポインタ
  unsigned char *p1, *p2;  // 移動先と移動元ポインタ
  int len;               // 移動の長さ

  cont_clp = cont_cip = NULL;
  proc.reset();

  // Empty check (If this is the case, it may be impossible to delete lines
  // when only line numbers are entered when there is insufficient space ..)
  // @Tamakichi)
  if (list_free() < *ibuf) { // If the vacancy is insufficient
    listbuf = (unsigned char *)realloc(listbuf, size_list + *ibuf);
    if (!listbuf) {
      err = ERR_LBUFOF;
      size_list = 0;
      return;
    }
    size_list += *ibuf;
  }

  insp = getlp(getlineno(ibuf)); // 挿入位置ポインタを取得

  // 同じ行番号の行が存在したらとりあえず削除
  if (getlineno(insp) == getlineno(ibuf)) { // もし行番号が一致したら
    p1 = insp;                              // p1を挿入位置に設定
    p2 = p1 + *p1;                          // p2を次の行に設定
    while ((len = *p2) != 0) {              // 次の行の長さが0でなければ繰り返す
      while (len--)                         // 次の行の長さだけ繰り返す
	*p1++ = *p2++;                      // 前へ詰める
    }
    *p1 = 0; // リストの末尾に0を置く
  }

  // 行番号だけが入力された場合はここで終わる
  if (*ibuf == sizeof(num_t)+2) // もし長さが4（[長さ][I_NUM][行番号]のみ）なら
    return;

  // 挿入のためのスペースを空ける

  for (p1 = insp; *p1; p1 += *p1) ;  // p1をリストの末尾へ移動
  len = p1 - insp + 1;             // 移動する幅を計算
  p2 = p1 + *ibuf;                 // p2を末尾より1行の長さだけ後ろに設定
  while (len--)                    // 移動する幅だけ繰り返す
    *p2-- = *p1--;                 // 後ろへズラす

  // 行を転送する
  len = *ibuf;     // 中間コードの長さを設定
  p1 = insp;       // 転送先を設定
  p2 = ibuf;       // 転送元を設定
  while (len--)    // 中間コードの長さだけ繰り返す
    *p1++ = *p2++;  // 転送
}

//指定中間コード行レコードのテキスト出力
void SMALL putlist(unsigned char* ip, uint8_t devno) {
  unsigned char i;  // ループカウンタ
  uint8_t var_code; // 変数コード

  while (*ip != I_EOL) { //行末でなければ繰り返す
    //キーワードの処理
    if (*ip < SIZE_KWTBL && kwtbl[*ip]) { //もしキーワードなら
      char kw[MAX_KW_LEN+1];
      strcpy_P(kw, kwtbl[*ip]);
      c_puts(kw, devno); //キーワードテーブルの文字列を表示
      if (*(ip+1) != I_COLON)
	if ( (!nospacea(*ip) || spacef(*(ip+1))) && (*ip != I_COLON) && (*ip != I_SQUOT)) //もし例外にあたらなければ
	  c_putch(' ',devno);  //空白を表示

      if (*ip == I_REM||*ip == I_SQUOT) { //もし中間コードがI_REMなら
	ip++; //ポインタを文字数へ進める
	i = *ip++; //文字数を取得してポインタをコメントへ進める
	while (i--) //文字数だけ繰り返す
	  c_putch(*ip++,devno);  //ポインタを進めながら文字を表示
	return;
      }
      
      if (*ip == I_PROC || *ip == I_CALL) {
        ip++;
        c_puts(proc_names.name(*ip), devno);
      }

      ip++; //ポインタを次の中間コードへ進める
    }
    else

    //定数の処理
    if (*ip == I_NUM) { //もし定数なら
      ip++; //ポインタを値へ進める
      num_t n;
      os_memcpy(&n, ip, sizeof(num_t));
      putnum(n, 0,devno); //値を取得して表示
      ip += sizeof(num_t); //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    }
    else

    //16進定数の処理
    if (*ip == I_HEXNUM) { //もし16進定数なら
      ip++; //ポインタを値へ進める
      c_putch('$',devno); //空白を表示
      putHexnum(ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24), 8,devno); //値を取得して表示
      ip += 4; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    }
    else

    //変数の処理(2017/07/26 変数名 A～Z 9対応)
    if (*ip == I_VAR) { //もし定数なら
      ip++; //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(var_names.name(var_code), devno);

      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_VARARR) {
      ip++;
      var_code = *ip++;
      c_puts(num_arr_names.name(var_code), devno);
      c_putch('(', devno);
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_SVAR) {
      ip++; //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(svar_names.name(var_code), devno);
      c_putch('$', devno);

      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_STRARR) {
      ip++;
      var_code = *ip++;
      c_puts(str_arr_names.name(var_code), devno);
      c_putch('$', devno);
      c_putch('(', devno);
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else

    //文字列の処理
    if (*ip == I_STR) { //もし文字列なら
      char c; //文字列の括りに使われている文字（「"」または「'」）

      //文字列の括りに使われている文字を調べる
      c = '\"'; //文字列の括りを仮に「"」とする
      ip++; //ポインタを文字数へ進める
      for (i = *ip; i; i--) //文字数だけ繰り返す
	if (*(ip + i) == '\"') { //もし「"」があれば
	  c = '\''; //文字列の括りは「'」
	  break; //繰り返しを打ち切る
	}

      //文字列を表示する
      c_putch(c,devno); //文字列の括りを表示
      i = *ip++; //文字数を取得してポインタを文字列へ進める
      while (i--) //文字数だけ繰り返す
	c_putch(*ip++,devno);  //ポインタを進めながら文字を表示
      c_putch(c,devno); //文字列の括りを表示
      if (*ip == I_VAR || *ip ==I_ELSE || *ip == I_AS || *ip == I_TO)
	c_putch(' ',devno);
    }

    else { //どれにも当てはまらなかった場合
      err = ERR_SYS; //エラー番号をセット
      return;
    }
  }
}

int get_array_dims(int *idxs);

// Get argument in parenthesis
num_t GROUP(basic_core) getparam() {
  num_t value; //値
  if (checkOpen()) return 0;
  if (getParam(value, I_NONE) ) return 0;
  if (checkClose()) return 0;
  return value; //値を持ち帰る
}

// INPUT handler
void SMALL iinput() {
  int dims = 0;
  int idxs[MAX_ARRAY_DIMS];
  num_t value;
  BString str_value;
  short index;          // Array subscript or variable number
  unsigned char prompt; // prompt display flag

  sc0.show_curs(1);
  for(;;) {
    prompt = 1;       // no prompt shown yet

    // Processing when a prompt is specified (w0t?)
    // We have to exclude string variables here because they may be lvalues.
    if(is_strexp() && *cip != I_SVAR) {
      c_puts(istrexp().c_str());
      prompt = 0;        // prompt shown

      if (*cip != I_SEMI) {
	err = ERR_SYNTAX;
	goto DONE;
      }
      cip++;
    }

    // Processing input values
    switch (*cip++) {
    case I_VAR:
    case I_VARARR:
      index = *cip;

      if (cip[-1] == I_VARARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
        value = num_arr.var(index).var(idxs);
      }
 
      cip++;

      // XXX: idiosyncrasy, never seen this in other BASICs
      if (prompt) {          // If you are not prompted yet
        if (dims)
          c_puts(num_arr_names.name(index));
        else
	  c_puts(var_names.name(index));
	c_putch(':');
      }

      value = getnum();
      if (err)
        return;

      if (dims)
        num_arr.var(index).var(idxs) = value;
      else
        var.var(index) = value;
      break;

    case I_SVAR:
      index = *cip;
      
      cip++;
      
      // XXX: idiosyncrasy, never seen this in other BASICs
      if (prompt) {          // If you are not prompted yet
        c_puts(svar_names.name(index));
	c_putch(':');
      }
      
      str_value = getstr();
      if (err)
        return;
      
      svar.var(index) = str_value;

      break;

    default: // 以上のいずれにも該当しなかった場合
      err = ERR_SYNTAX; // エラー番号をセット
      //return;            // 終了
      goto DONE;
    } // 中間コードで分岐の末尾

    //値の入力を連続するかどうか判定する処理
    switch (*cip) { // 中間コードで分岐
    case I_COMMA:    // コンマの場合
      cip++;         // 中間コードポインタを次へ進める
      break;         // 打ち切る
    case I_COLON:    //「:」の場合
    case I_EOL:      // 行末の場合
      //return;        // 終了
      goto DONE;
    default:      // 以上のいずれにも該当しなかった場合
      err = ERR_SYNTAX; // エラー番号をセット
      //return;           // 終了
      goto DONE;
    } // 中間コードで分岐の末尾
  }   // 無限に繰り返すの末尾

DONE:
  sc0.show_curs(0);
}

// Variable assignment handler
void GROUP(basic_core) ivar() {
  num_t value; //値
  short index; //変数番号

  index = *cip++; //変数番号を取得して次へ進む

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return;
  }
  cip++; //中間コードポインタを次へ進める
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  var.var(index) = value;
}

int get_array_dims(int *idxs) {
  int dims = 0;
  while (dims < MAX_ARRAY_DIMS) {
    if (getParam(idxs[dims], 0, 255, I_NONE))
      return -1;
    dims++;
    if (*cip == I_CLOSE)
      break;
    if (*cip != I_COMMA) {
      err = ERR_SYNTAX;
      return -1;
    }
    cip++;
  }
  cip++;
  return dims;
}

void idim() {
  int dims = 0;
  int idxs[MAX_ARRAY_DIMS];
  uint8_t index;

  if (*cip++ != I_VARARR) {
    err = ERR_SYNTAX;
    return;
  }

  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;

  if (num_arr.var(index).reserve(dims, idxs))
    err = ERR_OOM;

  if (*cip == I_EQ) {
    // Initializers
    if (dims > 1) {
      err = ERR_NOT_SUPPORTED;
    } else {
      num_t value;
      int cnt = 0;
      do {
        cip++;
        value = iexp();
        if (err)
          return;
        num_t &n = num_arr.var(index).var(&cnt);
        if (err)
          return;
        n = value;
        cnt++;
      } while(*cip == I_COMMA);
    }
  }

  return;
}

// Numeric array variable assignment handler
void GROUP(basic_core) ivararr() {
  num_t value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++; //変数番号を取得して次へ進む

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return;
  }
  cip++; //中間コードポインタを次へ進める
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  num_t &n = num_arr.var(index).var(idxs);
  if (err)
    return;
  n = value;
}

void isvar() {
  BString value;
  uint8_t index = *cip++;
  int32_t offset;
  uint8_t sval;

  if (*cip == I_SQOPEN) {
    // String character accessor
    ++cip;

    if (getParam(offset, 0, svar.var(index).length(), I_SQCLOSE))
      return;

    if (*cip != I_EQ) {
      err = ERR_VWOEQ;
      return;
    }
    cip++;
    
    sval = iexp();
    svar.var(index)[offset] = sval;

    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = istrexp();
  if (err)
    return;
  svar.var(index) = value;
}

// String array variable assignment handler
void GROUP(basic_core) istrarr() {
  BString value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = istrexp();
  if (err)
    return;

  BString &s = str_arr.var(index).var(idxs);
  if (err)
    return;
  s = value;
}

static inline bool end_of_statement()
{
  return *cip == I_EOL || *cip == I_COLON || *cip == I_ELSE;
}

int GROUP(basic_core) token_size(uint8_t *code) {
  switch (*code) {
  case I_STR:
    return code[1] + 1;
  case I_NUM:
    return sizeof(num_t) + 1;
  case I_HEXNUM:
    return 5;
  case I_VAR:
  case I_VARARR:
  case I_CALL:
  case I_PROC:
    return 2;
  case I_EOL:
  case I_REM:
  case I_SQUOT:
    return -1;
  default:
    return 1;
  }
}

void find_next_token(unsigned char **start_clp, unsigned char **start_cip, unsigned char tok)
{
  unsigned char *sclp = *start_clp;
  unsigned char *scip = *start_cip;
  int next;

  *start_clp = *start_cip = NULL;

  if (!scip)
    scip = sclp + sizeof(num_t) + 1;

  while (*scip != tok) {
    next = token_size(scip);
    if (next < 0) {
      sclp += *sclp;
      if (!*sclp)
        return;
      scip = sclp + sizeof(num_t) + 1;
    } else
      scip += next;
  }

  *start_clp = sclp;
  *start_cip = scip;
}

void initialize_proc_pointers(void)
{
  unsigned char *lp, *ip;

  lp = listbuf; ip = NULL;

  for (;;) {
    find_next_token(&lp, &ip, I_PROC);
    if (!lp)
      return;

    proc.proc(ip[1]).lp = lp;
    proc.proc(ip[1]).ip = ip + 2;

    ip += 2;
  }
}

unsigned char *data_ip;
unsigned char *data_lp;

bool find_next_data() {
  int next;

  if (!data_lp) {
    if (*listbuf)
      data_lp = listbuf;
    else
      return false;
  }
  if (!data_ip) {
    data_ip = data_lp + sizeof(num_t) + 1;
  }
  
  while (*data_ip != I_DATA && *data_ip != I_COMMA) {
    next = token_size(data_ip);
    if (next < 0) {
      data_lp += *data_lp;
      if (!*data_lp)
        return false;
      data_ip = data_lp + sizeof(num_t) + 1;
    } else
      data_ip += next;
  }
  return true;
}

void idata() {
  int next;

  // Skip over the DATA statement
  while (*cip != I_EOL && *cip != I_COLON) {
    next = token_size(cip);
    if (next < 0) {
      clp += *clp;
      if (!*clp)
        return;
      cip = clp + sizeof(num_t) + 1;
    } else
      cip += next;
  }
}

void iread() {
  unsigned char *cip_save;
  num_t value;
  BString svalue;
  uint8_t index;

  if (!find_next_data()) {
    err = ERR_OOD;
    return;
  }
  ++cip;

  switch (*cip++) {
  case I_VAR:
    ++cip;
    cip_save = cip;
    cip = data_ip;
    value = iexp();
    data_ip = cip;
    cip = cip_save;
    var.var(*cip++) = value;
    break;
    
  case I_VARARR: {
    ++cip;
    int idxs[MAX_ARRAY_DIMS];
    int dims = 0;
    
    index = *cip++;
    dims = get_array_dims(idxs);
    if (dims < 0)
      return;

    cip_save = cip;
    cip = data_ip;
    value = iexp();
    data_ip = cip;
    cip = cip_save;

    num_t &n = num_arr.var(index).var(idxs);
    if (err)
      return;
    n = value;
    break;
    }

  case I_SVAR:
    ++cip;
    cip_save = cip;
    cip = data_ip;
    svalue = istrexp();
    data_ip = cip;
    cip = cip_save;
    svar.var(*cip++) = svalue;
    break;
    
  default:
    err = ERR_SYNTAX;
    break;
  }
}

// LET handler
void ilet() {
  switch (*cip) { //中間コードで分岐
  case I_VAR: // 変数の場合
    cip++;     // 中間コードポインタを次へ進める
    ivar();    // 変数への代入を実行
    break;

  case I_VARARR:
    cip++;
    ivararr();
    break;

  case I_SVAR:
    cip++;
    isvar();
    break;

  default:      // 以上のいずれにも該当しなかった場合
    err = ERR_LETWOV; // エラー番号をセット
    break;            // 打ち切る
  }
}

static bool trace_enabled = false;

void itron() {
  trace_enabled = true;
}
void itroff() {
  trace_enabled = false;
}

void inew(uint8_t mode = 0);

// RUN command handler
void GROUP(basic_core) irun(uint8_t* start_clp = NULL, bool cont = false) {
  uint8_t*   lp;     // 行ポインタの一時的な記憶場所
  if (cont) {
    clp = cont_clp;
    cip = cont_cip;
    goto resume;
  }
  initialize_proc_pointers();

  gstki = 0;         // GOSUBスタックインデクスを0に初期化
  lstki = 0;         // FORスタックインデクスを0に初期化
  ifstki = 0;
  astk_num_i = 0;
  astk_str_i = 0;
  inew(2);

  if (start_clp != NULL) {
    clp = start_clp;
  } else {
    clp = listbuf;   // 行ポインタをプログラム保存領域の先頭に設定
  }

  while (*clp) {     // 行ポインタが末尾を指すまで繰り返す
    if (trace_enabled) {
      putnum(getlineno(clp), 0);
      c_putch(' ');
    }

    cip = clp + sizeof(num_t) + 1;   // 中間コードポインタを行番号の後ろに設定

resume:
    lp = iexe();     // 中間コードを実行して次の行の位置を得る
    if (err) {         // もしエラーを生じたら
      if (err == ERR_CTR_C) {
        cont_cip = cip;
        cont_clp = clp;
      } else {
        cont_cip = cont_clp = NULL;
      }
      return;
    }
    clp = lp;         // 行ポインタを次の行の位置へ移動
  }
}

num_t GROUP(basic_core) imul();

// LIST command
void SMALL ilist(uint8_t devno=0) {
  uint32_t lineno = 0;			// start line number
  uint32_t endlineno = UINT32_MAX;	// end line number
  uint32_t prnlineno;			// output line number
  unsigned char *lp;

  // Determine range
  if (!end_of_statement()) {
    if (*cip == I_MINUS) {
      // -<num> -> from start to line <num>
      cip++;
      if (getParam(endlineno, I_NONE)) return;
    } else {
      // <num>, <num>-, <num>-<num>

      // Slight hack: We don't know how to disambiguate between range and
      // minus operator. We therefore skip the +/- part of the expression
      // parser.
      // It is still possible to use an expression containing +/- by
      // enclosing it in parentheses.
      lineno = imul();
      if (err)
        return;

      if (*cip == I_MINUS) {
        // <num>-, <num>-<num>
        cip++;
        if (!end_of_statement()) {
          // <num>-<num> -> list specified range
          if (getParam(endlineno, lineno, UINT32_MAX, I_NONE)) return;
        } else {
          // <num>- -> list from line <num> to the end
        }
      } else {
        // <num> -> only one line
        endlineno = lineno;
      }
    }
  }

  // Skip until we reach the start line.
  for ( lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) ;

  //リストを表示する
  while (*lp) {               // 行ポインタが末尾を指すまで繰り返す
    prnlineno = getlineno(lp); // 行番号取得
    if (prnlineno > endlineno) // 表示終了行番号に達したら抜ける
      break;
    putnum(prnlineno, 0,devno); // 行番号を表示
    c_putch(' ',devno);        // 空白を入れる
    putlist(lp + sizeof(num_t) + 1,devno);    // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    newline(devno);            // 改行
    lp += *lp;               // 行ポインタを次の行へ進める
  }
}

// Argument 0: all erase, 1: erase only program, 2: erase variable area only
void inew(uint8_t mode) {
  data_ip = data_lp = NULL;

  if (mode != 1) {
    var.reset();
    svar.reset();
    num_arr.reset();
    str_arr.reset();
  }

  //変数と配列の初期化
  if (mode == 0|| mode == 2) {
    // forget variables assigned in direct mode

    // XXX: These reserve() calls always downsize (or same-size) the
    // variable pools. Can they fail doing so?
    svar_names.deleteDirect();
    svar.reserve(svar_names.varTop());
    var_names.deleteDirect();
    var.reserve(var_names.varTop());
    num_arr_names.deleteDirect();
    num_arr.reserve(num_arr_names.varTop());
    str_arr_names.deleteDirect();
    str_arr.reserve(str_arr_names.varTop());
  }
  //実行制御用の初期化
  if (mode !=2) {
    cont_cip = cont_clp = NULL;
    // forget all variables
    var_names.deleteAll();
    var.reserve(0);
    svar_names.deleteAll();
    svar.reserve(0);
    num_arr_names.deleteAll();
    num_arr.reserve(0);
    str_arr_names.deleteAll();
    str_arr.reserve(0);
    proc_names.deleteAll();
    proc.reserve(0);

    gstki = 0; //GOSUBスタックインデクスを0に初期化
    lstki = 0; //FORスタックインデクスを0に初期化
    ifstki = 0;
    astk_num_i = 0;
    astk_str_i = 0;

    if (listbuf)
      free(listbuf);
    // XXX: Should we be more generous here to avoid fragmentation?
    listbuf = (unsigned char *)malloc(1);
    if (!listbuf) {
      err = ERR_OOM;
      // If this fails, we're in deep shit...
      return;
    }
    *listbuf = 0;
    size_list = 1;
    clp = listbuf; //行ポインタをプログラム保存領域の先頭に設定
  }
}

// RENUME command handler
void SMALL irenum() {
  uint32_t startLineNo = 10;  // 開始行番号
  uint32_t increase = 10;     // 増分
  uint8_t* ptr;               // プログラム領域参照ポインタ
  uint32_t len;               // 行長さ
  uint32_t i;                 // 中間コード参照位置
  num_t newnum;            // 新しい行番号
  uint32_t num;               // 現在の行番号
  uint32_t index;             // 行インデックス
  uint32_t cnt;               // プログラム行数
  int toksize;

  // 開始行番号、増分引数チェック
  if (*cip == I_NUM) {               // もしRENUMT命令に引数があったら
    startLineNo = getlineno(cip);    // 引数を読み取って開始行番号とする
    cip += sizeof(num_t) + 1;
    if (*cip == I_COMMA) {
      cip++;                          // カンマをスキップ
      if (*cip == I_NUM) {            // 増分指定があったら
	increase = getlineno(cip);    // 引数を読み取って増分とする
      } else {
	err = ERR_SYNTAX;             // カンマありで引数なしの場合はエラーとする
	return;
      }
    }
  }

  // 引数の有効性チェック
  cnt = countLines()-1;
  if (startLineNo <= 0 || increase <= 0) {
    err = ERR_VALUE;
    return;
  }
  if (startLineNo + increase * cnt > INT32_MAX) {
    err = ERR_VALUE;
    return;
  }

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (  clp = listbuf; *clp; clp += *clp) {
    ptr = clp;
    len = *ptr;
    ptr++;
    i=0;
    // 行内検索
    while( i < len-1 ) {
      switch(ptr[i]) {
      case I_GOTO:    // GOTO命令
      case I_GOSUB:   // GOSUB命令
	i++;
	if (ptr[i] == I_NUM) {
	  num = getlineno(&ptr[i]);      // 現在の行番号を取得する
	  index = getlineIndex(num);     // 行番号の行インデックスを取得する
	  if (index == INT32_MAX) {
	    // 該当する行が見つからないため、変更は行わない
	    i += sizeof(num_t) + 1;
	    continue;
	  } else {
	    // とび先行番号を付け替える
	    newnum = startLineNo + increase*index;
	    os_memcpy(ptr+i+1, &newnum, sizeof(num_t));
	    i += sizeof(num_t) + 1;
	    continue;
	  }
	}
	break;
      default:
        toksize = token_size(ptr+i);
        if (toksize < 0)
          i = len + 1;	// skip rest of line
        else
          i += toksize;	// next token
	break;
      }
    }
  }

  // 各行の行番号の付け替え
  index = 0;
  for (  clp = listbuf; *clp; clp += *clp ) {
    newnum = startLineNo + increase * index;
    os_memcpy(clp+1, &newnum, sizeof(num_t));
    index++;
  }
}

// CONFIGコマンド
// CONFIG 項目番号,設定値
void SMALL iconfig() {
  int32_t itemNo;
  int32_t value;

  if ( getParam(itemNo, I_COMMA) ) return;
  if ( getParam(value, I_NONE) ) return;
  switch(itemNo) {
#if USE_NTSC == 1
  case 0: // NTSC補正
    if (value <0 || value >2)  {
      err = ERR_VALUE;
    } else {
      sc0.adjustNTSC(value);
      CONFIG.NTSC = value;
    }
    break;
  case 1: // キーボード補正
    if (value <0 || value >1)  {
      err = ERR_VALUE;
    } else {
      sc0.reset_kbd(value);
      CONFIG.KEYBOARD = value;
    }
    break;
#endif
#if 0
  case 2: // プログラム自動起動番号設定
    if (value < -1 || value >FLASH_SAVE_NUM-1)  {
      err = ERR_VALUE;
    } else {
      CONFIG.STARTPRG = value;
    }
    break;
#endif
  default:
    err = ERR_VALUE;
    break;
  }
}

void iloadconfig() {
  loadConfig();
}

void isavebg();

// "SAVE <file name>" or "SAVE BG ..."
void isave() {
  BString fname;
  int8_t rc;

  if (*cip == I_BG) {
    isavebg();
    return;
  }

  if(!(fname = getParamFname())) {
    return;
  }

#if USE_SD_CARD == 1
  // SDカードへの保存
  rc = bfs.tmpOpen((char *)fname.c_str(),1);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err =  ERR_FILE_OPEN;
    return;
  }
  ilist(4);
  bfs.tmpClose();
#endif
}

// テキスト形式のプログラムのロード
// 引数
//   fname  :  ファイル名
//   newmode:  初期化モード 0:初期化する 1:変数を初期化しない 2:追記モード
// 戻り値
//   0:正常終了
//   1:異常終了
uint8_t SMALL loadPrgText(char* fname, uint8_t newmode = 0) {
  int16_t rc;
  int32_t len;
  
  cont_clp = cont_cip = NULL;
  proc.reset();

#if USE_SD_CARD == 1
  rc = bfs.tmpOpen(fname,0);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return 1;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err =  ERR_FILE_OPEN;
    return 1;
  }

  if (newmode != 2)
    inew(newmode);
  while(bfs.readLine(lbuf)) {
    tlimR(lbuf);  // 2017/07/31 追記
    len = toktoi();
    if (err) {
      c_puts(lbuf);
      newline();
      error(false);
      continue;
    }
    if (*ibuf == I_NUM) {
      *ibuf = len;
      inslist();
      if (err)
	error(true);
      continue;
    }
  }
  bfs.tmpClose();
#endif
  return 0;
}

// フラシュメモリからのプログラムロード
// 引数
//  progno:    プログラム番号
//  newmmode:  初期化モード 0:初期化する 1:初期化しない
// 戻り値
//  0:正常終了
//  1:異常終了

uint8_t loadPrg(uint16_t prgno, uint8_t newmode=0) {
#if 0
  uint32_t flash_adr;
  flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG);

  // 指定領域に保存されているかチェックする
  if ( *((uint8_t*)flash_adr) == 0xff && *((uint8_t*)flash_adr+1) == 0xff) {
    err = ERR_NOPRG;
    return 1;
  }
  // 現在のプログラムの削除とロード
  inew(newmode);
  os_memcpy(listbuf, (uint8_t*)flash_adr, FLASH_PAGE_SIZE*FLASH_PAGE_PAR_PRG);
#endif
  return 0;
}

// Delete specified line
// DELETE line number
// DELETE start line number, end line number
void SMALL idelete() {
  uint32_t sNo, eNo;
  uint8_t  *lp;      // 削除位置ポインタ
  uint8_t *p1, *p2;  // 移動先と移動元ポインタ
  int32_t len;       // 移動の長さ

  cont_clp = cont_cip = NULL;
  proc.reset();

  if ( getParam(sNo, I_NONE) ) return;
  if (*cip == I_COMMA) {
    cip++;
    if ( getParam(eNo, I_NONE) ) return;
  } else {
    eNo = sNo;
  }

  if (eNo < sNo) {
    err = ERR_VALUE;
    return;
  }

  if (eNo == sNo) {
    lp = getlp(eNo); // 削除位置ポインタを取得
    if (getlineno(lp) == sNo) {
      // 削除
      p1 = lp;                              // p1を挿入位置に設定
      p2 = p1 + *p1;                        // p2を次の行に設定
      while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
	while (len--)                       // 次の行の長さだけ繰り返す
	  *p1++ = *p2++;                    // 前へ詰める
      }
      *p1 = 0; // リストの末尾に0を置く
    }
  } else {
    for (uint32_t i = sNo; i <= eNo; i++) {
      lp = getlp(i); // 削除位置ポインタを取得
      if (getlineno(lp) == i) {               // もし行番号が一致したら
	p1 = lp;                              // p1を挿入位置に設定
	p2 = p1 + *p1;                        // p2を次の行に設定
	while ((len = *p2) != 0) {            // 次の行の長さが0でなければ繰り返す
	  while (len--)                       // 次の行の長さだけ繰り返す
	    *p1++ = *p2++;                    // 前へ詰める
	}
	*p1 = 0; // リストの末尾に0を置く
      }
    }
  }
}

// プログラムファイル一覧表示 FILES ["ファイルパス"]
void ifiles() {
  BString fname;
  char wildcard[SD_PATH_LEN];
  char* wcard = NULL;
  char* ptr = NULL;
  uint8_t flgwildcard = 0;
  int16_t rc;

  if (!is_strexp())
    fname = "";
  else
    fname = getParamFname();

  if (fname.length() > 0) {
    for (int8_t i = fname.length()-1; i >= 0; i--) {
      if (fname[i] == '/') {
        ptr = &fname[i];
        break;
      }
      if (fname[i] == '*' || fname[i] == '?' || fname[i] == '.')
        flgwildcard = 1;
    }
    if (ptr != NULL && flgwildcard == 1) {
      strcpy(wildcard, ptr+1);
      wcard = wildcard;
      *(ptr+1) = 0;
    } else if (ptr == NULL && flgwildcard == 1) {
      strcpy(wildcard, fname.c_str());
      wcard = wildcard;
      fname = "";
    }
  }
#if USE_SD_CARD == 1
  rc = bfs.flist((char *)fname.c_str(), wcard, sc0.getWidth()/14);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_FILE_OPEN;
  }
#endif
}

void iformat() {
  static const char warn_spiffs[] PROGMEM =
    "This will ERASE ALL DATA on the internal flash file system!";
  static const char areyousure[] PROGMEM =
    "ARE YOU SURE? (Y/N) ";
  static const char aborted[] PROGMEM = "Aborted";
  static const char formatting[] PROGMEM = "Formatting... ";
  static const char success[] PROGMEM = "Success!";
  static const char failed[] PROGMEM = "Failed.";

  BString target = getParamFname();
  if (err)
    return;

  if (target == "/flash") {
    c_puts_P(warn_spiffs); newline();
    c_puts_P(areyousure);
    BString answer = getstr();
    if (answer != "Y" && answer != "y") {
      c_puts_P(aborted); newline();
      return;
    }
    c_puts_P(formatting);
    if (SPIFFS.format())
      c_puts_P(success);
    else
      c_puts_P(failed);
    newline();
  } else {
    err = ERR_NOT_SUPPORTED;
  }
}

// 画面クリア
void icls() {
  sc0.cls();
  sc0.locate(0,0);
}

#include "sound.h"

void ICACHE_RAM_ATTR pump_events(void)
{
  static uint32_t last_frame;
  if (vs23.frame() == last_frame)
    return;

  last_frame = vs23.frame();

  vs23.updateBg();
  sound_pump_events();

  uint8_t f = last_frame & 0x3f;
  if (f == 0) {
    sc0.drawCursor(0);
  } else if (f == 0x20) {
    sc0.drawCursor(1);
  }
}

// 時間待ち
void iwait() {
  int32_t tm;
  if ( getParam(tm, 0, INT32_MAX, I_NONE) ) return;
  uint32_t end = tm + millis();
  while (millis() < end) {
    pump_events();
  }
}

// カーソル移動 LOCATE x,y
void ilocate() {
  int32_t x,  y;
  if ( getParam(x, I_COMMA) ) return;
  if ( getParam(y, I_NONE) ) return;
  if ( x >= sc0.getWidth() )   // xの有効範囲チェック
    x = sc0.getWidth() - 1;
  else if (x < 0) x = 0;
  if( y >= sc0.getHeight() )   // yの有効範囲チェック
    y = sc0.getHeight() - 1;
  else if(y < 0) y = 0;

  // カーソル移動
  sc0.locate((uint16_t)x, (uint16_t)y);
}

// 文字色の指定 COLOLR fc,bc
void icolor() {
  int32_t fc,  bc = 0;
  if ( getParam(fc, 0, 255, I_NONE) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(bc, 0, 255, I_NONE) ) return;
  }
  // 文字色の設定
  sc0.setColor((uint16_t)fc, (uint16_t)bc);
}

// 文字属性の指定 ATTRコマンド
void iattr() {
  int32_t attr;
  if ( getParam(attr, 0, 4, I_NONE) ) return;
  sc0.setAttr(attr);
}

// キー入力文字コードの取得 INKEY()関数
int32_t iinkey() {
  int32_t rc = 0;

  if (prevPressKey) {
    // 一時バッファに入力済キーがあればそれを使う
    rc = prevPressKey;
    prevPressKey = 0;
  } else {
    rc = c_kbhit();
    if (rc == SC_KEY_CTRL_C)
      err = ERR_CTR_C;
  }

  return rc;
}

// メモリ参照　PEEK(adr[,bnk])
int32_t ipeek() {
  int32_t value = 0, vadr;
  uint8_t* radr;

  if (checkOpen()) return 0;
  if ( getParam(vadr, I_NONE) ) return 0;
  if (checkClose()) return 0;
  radr = sanitize_addr(vadr);
  if (radr)
    value = *radr;
  else
    err = ERR_RANGE;
  return value;
}

num_t iret() {
  int32_t r;

  if (checkOpen()) return 0;
  if ( getParam(r, 0, MAX_RETVALS-1, I_NONE) ) return 0;
  if (checkClose()) return 0;

  return retval[r];
}

num_t iarg() {
  int32_t a;
  if (astk_num_i == 0) {
    err = ERR_UNDEFARG;
    return 0;
  }
  uint16_t argc = (uint32_t)(gstk[gstki-1]) & 0xffff;

  if (checkOpen()) return 0;
  if ( getParam(a, 0, argc-1, I_NONE) ) return 0;
  if (checkClose()) return 0;

  return astk_num[astk_num_i-argc+a];
}

BString *iargstr() {
  int32_t a;
  if (astk_str_i == 0) {
    err = ERR_UNDEFARG;
    return NULL;
  }
  uint16_t argc = (uint32_t)(gstk[gstki-1]) >> 16;

  if (checkOpen()) return NULL;
  if ( getParam(a, 0, argc-1, I_NONE) ) return NULL;
  if (checkClose()) return NULL;

  return &astk_str[astk_str_i-argc+a];
}

// スクリーン座標の文字コードの取得 'VPEEK(X,Y)'
int32_t ivpeek() {
  int32_t value; // 値
  int32_t x, y;  // 座標

  if (checkOpen()) return 0;
  if ( getParam(x, I_COMMA) ) return 0;
  if ( getParam(y, I_NONE) ) return 0;
  if (checkClose()) return 0;
  value = (x < 0 || y < 0 || x >=sc0.getWidth() || y >=sc0.getHeight()) ? 0 : sc0.vpeek(x, y);
  return value;
}

uint16_t pcf_state = 0xffff;

// GPIO ピンデジタル出力
void idwrite() {
  int32_t pinno,  data;

  if ( getParam(pinno, 0, 15, I_COMMA) ) return;
  if ( getParam(data, I_NONE) ) return;
  data = !!data;

  pcf_state = (pcf_state & ~(1 << pinno)) | (data << pinno);

  // SDA is multiplexed with MVBLK0, so we wait for block move to finish
  // to avoid interference.
  while (!blockFinished()) {}

  // XXX: frequency is higher when running at 160 MHz because F_CPU is wrong
  Wire.beginTransmission(0x20);
  Wire.write(pcf_state & 0xff);
  Wire.write(pcf_state >> 8);

  int ret = Wire.endTransmission();
}

BString ihex() {
  int value; // 値

  if (checkOpen()) goto out;
  if (getParam(value, I_NONE)) goto out;
  if (checkClose()) goto out;
  return BString(value, 16);
out:
  return BString();
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
BString ibin() {
  int32_t value; // 値

  if (checkOpen()) goto out;
  if (getParam(value, I_NONE)) goto out;
  if (checkClose()) goto out;
  return BString(value, 2);
out:
  return BString();
}

// 小数点数値出力 DMP$(数値) or DMP(数値,小数部桁数) or DMP(数値,小数部桁数,整数部桁指定)
void idmp(uint8_t devno=0) {
  int32_t value;     // 値
  int32_t v1,v2;
  int32_t n = 2;    // 小数部桁数
  int32_t dn = 0;   // 整数部桁指定
  int32_t base=1;

  if (checkOpen()) return;
  if (getParam(value, I_NONE)) return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(n, 0,4, I_NONE)) return;
    if (*cip == I_COMMA) {
      cip++;
      if (getParam(dn,-6,6, I_NONE)) return;
    }
  }
  if (checkClose()) return;

  for (int32_t i=0; i<n; i++) {
    base*=10;
  }
  v1 = value / base;
  v2 = value % base;
  if (v1 == 0 && value <0)
    c_putch('-',devno);
  putnum(v1, dn, devno);
  if (n) {
    c_putch('.',devno);
    putnum(v2<0 ? -v2 : v2, -n, devno);
  }
}

// POKEコマンド POKE ADR,データ[,データ,..データ]
void ipoke() {
  uint8_t* adr;
  int32_t value;
  int32_t vadr;

  // アドレスの指定
  vadr = iexp(); if(err) return;
  if (vadr <= 0 ) { err = ERR_VALUE; return; }
  if(*cip != I_COMMA) { err = ERR_SYNTAX; return; }

  // 例: 1,2,3,4,5 の連続設定処理
  do {
    adr = sanitize_addr(vadr);
    if (!adr) {
      err = ERR_RANGE;
      break;
    }
    cip++;          // 中間コードポインタを次へ進める
    if (getParam(value, I_NONE)) return;
    *((uint8_t*)adr) = (uint8_t)value;
    vadr++;
  } while(*cip == I_COMMA);
}

// I2CW関数  I2CW(I2Cアドレス, コマンドアドレス, コマンドサイズ, データアドレス, データサイズ)
int32_t ii2cw() {
  int32_t i2cAdr;
  uint32_t ctop, clen, top, len;
  uint8_t* ptr;
  uint8_t* cptr;
  int16_t rc;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA)) return 0;
  if (getParam(ctop, I_COMMA)) return 0;
  if (getParam(clen, I_COMMA)) return 0;
  if (getParam(top, I_COMMA)) return 0;
  if (getParam(len, I_NONE)) return 0;
  if (checkClose()) return 0;

  ptr  = sanitize_addr(top);
  cptr = sanitize_addr(ctop);
  if (ptr == 0 || cptr == 0 || sanitize_addr(top+len) == 0 || sanitize_addr(ctop+clen) == 0)
  { err = ERR_RANGE; return 0; }

  // I2Cデータ送信
  I2C_WIRE.beginTransmission(i2cAdr);
  if (clen) {
    for (uint32_t i = 0; i < clen; i++)
      I2C_WIRE.write(*cptr++);
  }
  if (len) {
    for (uint32_t i = 0; i < len; i++)
      I2C_WIRE.write(*ptr++);
  }
  rc =  I2C_WIRE.endTransmission();
  return rc;
}

// I2CR関数  I2CR(I2Cアドレス, 送信データアドレス, 送信データサイズ,受信データアドレス,受信データサイズ)
int32_t ii2cr() {
  int32_t i2cAdr, sdtop, sdlen,rdtop,rdlen;
  uint8_t* sdptr;
  uint8_t* rdptr;
  int16_t rc;
  int32_t n;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA)) return 0;
  if (getParam(sdtop, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(sdlen, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(rdtop, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(rdlen, 0, INT32_MAX, I_NONE)) return 0;
  if (checkClose()) return 0;

  sdptr = sanitize_addr(sdtop);
  rdptr = sanitize_addr(rdtop);
  if (sdptr == 0 || rdptr == 0 || sanitize_addr(sdtop+sdlen) == 0 || sanitize_addr(rdtop+rdlen) == 0)
  { err = ERR_RANGE; return 0; }

  // I2Cデータ送受信
  I2C_WIRE.beginTransmission(i2cAdr);

  // 送信
  if (sdlen) {
    I2C_WIRE.write(sdptr, sdlen);
  }
  rc = I2C_WIRE.endTransmission();
  if (rdlen) {
    if (rc!=0)
      return rc;
    n = I2C_WIRE.requestFrom(i2cAdr, rdlen);
    while (I2C_WIRE.available()) {
      *(rdptr++) = I2C_WIRE.read();
    }
  }
  return rc;
}

// SETDATEコマンド  SETDATE 年,月,日,時,分,秒
void isetDate() {
#if USE_INNERRTC == 1
  struct tm t;
  int32_t p_year, p_mon, p_day;
  int32_t p_hour, p_min, p_sec;

  if ( getParam(p_year, 1900,2036, I_COMMA) ) return;  // 年
  if ( getParam(p_mon,     1,  12, I_COMMA) ) return;  // 月
  if ( getParam(p_day,     1,  31, I_COMMA) ) return;  // 日
  if ( getParam(p_hour,    0,  23, I_COMMA) ) return;  // 時
  if ( getParam(p_min,     0,  59, I_COMMA) ) return;  // 分
  if ( getParam(p_sec,     0,  61, I_NONE)) return;  // 秒

  // RTCの設定
  t.tm_isdst = 0;             // サーマータイム [1:あり 、0:なし]
  t.tm_year  = p_year - 1900; // 年   [1900からの経過年数]
  t.tm_mon   = p_mon - 1;     // 月   [0-11] 0から始まることに注意
  t.tm_mday  = p_day;         // 日   [1-31]
  t.tm_hour  = p_hour;        // 時   [0-23]
  t.tm_min   = p_min;         // 分   [0-59]
  t.tm_sec   = p_sec;         // 秒   [0-61] うるう秒考慮
  rtc.setTime(&t);            // 時刻の設定
#else
  err = ERR_SYNTAX; return;
#endif
}

// GETDATEコマンド  SETDATE 年格納変数,月格納変数, 日格納変数, 曜日格納変数
void igetDate() {
#if USE_INNERRTC == 1
  int16_t index;
  time_t tt;
  struct tm* st;
  tt = rtc.getTime();   // 時刻取得
  st = localtime(&tt);  // 時刻型変換

  int16_t v[] = {
    st->tm_year+1900,
    st->tm_mon+1,
    st->tm_mday,
    st->tm_wday
  };

  for (uint8_t i=0; i <4; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      var.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if(i != 3) {
      if (*cip != I_COMMA) {      // ','のチェック
	err = ERR_SYNTAX;
	return;
      }
      cip++;
    }
  }
#else
  err = ERR_SYNTAX;
#endif
}

// GETDATEコマンド  SETDATE 時格納変数,分格納変数, 秒格納変数
void igetTime() {
#if USE_INNERRTC == 1
  int16_t index;
  time_t tt;
  struct tm* st;
  tt = rtc.getTime();   // 時刻取得
  st = localtime(&tt);  // 時刻型変換

  int16_t v[] = {
    st->tm_hour,          // 時
    st->tm_min,           // 分
    st->tm_sec            // 秒
  };

  for (uint8_t i=0; i <3; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      var.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if(i != 2) {
      if (*cip != I_COMMA) {      // ','のチェック
	err = ERR_SYNTAX;
	return;
      }
      cip++;
    }
  }
#else
  err = ERR_SYNTAX;
#endif
}

// DATEコマンド
void idate() {
#if USE_INNERRTC == 1
  static const char *wday[] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
  time_t tt;
  struct tm* st;
  tt = rtc.getTime();    // 時刻取得
  st = localtime(&tt);   // 時刻型変換

  putnum(st->tm_year+1900, -4);
  c_putch('/');
  putnum(st->tm_mon+1, -2);
  c_putch('/');
  putnum(st->tm_mday, -2);
  c_puts(" [");
  c_puts(wday[st->tm_wday]);
  c_puts("] ");
  putnum(st->tm_hour, -2);
  c_putch(':');
  putnum(st->tm_min, -2);
  c_putch(':');
  putnum(st->tm_sec, -2);
  newline();
#else
  err = ERR_SYNTAX;
#endif
}

// EEPFORMAT コマンド
void ieepformat() {
#if 0
  uint16_t Status;
  Status = EEPROM.format();
  if (Status != EEPROM_OK) {
    switch(Status) {
    case EEPROM_OUT_SIZE:      err = ERR_EEPROM_OUT_SIZE; break;
    case EEPROM_BAD_ADDRESS:   err = ERR_EEPROM_BAD_ADDRESS; break;
    case EEPROM_NOT_INIT:      err = ERR_EEPROM_NOT_INIT; break;
    case EEPROM_NO_VALID_PAGE: err = ERR_EEPROM_NO_VALID_PAGE; break;
    case EEPROM_BAD_FLASH:
    default:                   err = ERR_EEPROM_BAD_FLASH; break;
    }
  }
#endif
}

// EEPWRITE アドレス,データ コマンド
void ieepwrite() {
#if 0
  int32_t adr;     // 書込みアドレス
  uint32_t data;   // データ
  uint16_t Status;

  if ( getParam(adr, 0, INT32_MAX, I_COMMA) ) return;  // アドレス
  if ( getParam(data, I_NONE) ) return;         // データ

  // データの書込み
  Status = EEPROM.write(adr, data);
  if (Status != EEPROM_OK) {
    switch(Status) {
    case EEPROM_OUT_SIZE:      err = ERR_EEPROM_OUT_SIZE; break;
    case EEPROM_BAD_ADDRESS:   err = ERR_EEPROM_BAD_ADDRESS; break;
    case EEPROM_NOT_INIT:      err = ERR_EEPROM_NOT_INIT; break;
    case EEPROM_NO_VALID_PAGE: err = ERR_EEPROM_NO_VALID_PAGE; break;
    case EEPROM_BAD_FLASH:
    default:                   err = ERR_EEPROM_BAD_FLASH; break;
    }
  }
#endif
}

// EEPREAD(アドレス) 関数
int32_t ieepread(uint32_t addr) {
#if 0
  uint16_t Status;
  uint32_t data;

  if (addr < 0 || addr > INT32_MAX) {
    err = ERR_VALUE;
    return 0;
  }

  Status = EEPROM.read(addr, &data);
  if (Status != EEPROM_OK) {
    switch(Status) {
    case EEPROM_OUT_SIZE:
      err = ERR_EEPROM_OUT_SIZE;
      break;
    case EEPROM_BAD_ADDRESS:
      //err = ERR_EEPROM_BAD_ADDRESS;
      data = 0;   // 保存データが無い場合は0を返す
      break;
    case EEPROM_NOT_INIT:
      err = ERR_EEPROM_NOT_INIT;
      break;
    case EEPROM_NO_VALID_PAGE:
      err = ERR_EEPROM_NO_VALID_PAGE;
      break;
    case EEPROM_BAD_FLASH:
    default:
      err = ERR_EEPROM_BAD_FLASH;
      break;
    }
  }
  return data;
#else
  return 0;
#endif
}

// ドットの描画 PSET X,Y,C
void ipset() {
  int32_t x,y,c;
  if (getParam(x, I_COMMA)||getParam(y, I_COMMA)||getParam(c, I_NONE))
    return;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= sc0.getGWidth()) x = sc0.getGWidth()-1;
  if (y >= sc0.getGHeight()) y = sc0.getGHeight()-1;
  sc0.pset(x,y,c);
}

// 直線の描画 LINE X1,Y1,X2,Y2,C
void iline() {
  int32_t x1,x2,y1,y2,c;
  if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(c, I_NONE))
    return;
  if (x1 < 0) x1 =0;
  if (y1 < 0) y1 =0;
  if (x2 < 0) x1 =0;
  if (y2 < 0) y1 =0;
  if (x1 >= sc0.getGWidth()) x1 = sc0.getGWidth()-1;
  if (y1 >= sc0.getGHeight()) y1 = sc0.getGHeight()-1;
  if (x2 >= sc0.getGWidth()) x2 = sc0.getGWidth()-1;
  if (y2 >= sc0.getGHeight()) y2 = sc0.getGHeight()-1;
  sc0.line(x1, y1, x2, y2, c);
}

// 円の描画 CIRCLE X,Y,R,C,F
void icircle() {
  int32_t x, y, r, c, f;
  if (getParam(x, I_COMMA)||getParam(y, I_COMMA)||getParam(r, I_COMMA)||getParam(c, I_COMMA)||getParam(f, I_NONE))
    return;
  if (x < 0) x =0;
  if (y < 0) y =0;
  if (x >= sc0.getGWidth()) x = sc0.getGWidth()-1;
  if (y >= sc0.getGHeight()) y = sc0.getGHeight()-1;
  if (r < 0) r = -r;
  sc0.circle(x, y, r, c, f);
}

// 四角の描画 RECT X1,Y1,X2,Y2,C,F
void irect() {
  int32_t x1,y1,x2,y2,c,f;
  if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(c, I_COMMA)||getParam(f, I_NONE))
    return;
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc0.getGWidth() || y2 >= sc0.getGHeight())  {
    err = ERR_VALUE;
    return;
  }
  sc0.rect(x1, y1, x2-x1, y2-y1, c, f);
}

// ビットマップの描画 BITMAP 横座標, 縦座標, アドレス, インデックス, 幅, 高さ [,倍率]
void iblit() {
  int32_t x,y,w,h,dx,dy;
  int32_t dir = 0;
  if (getParam(x, 0, sc0.getGWidth(), I_COMMA)) return;
  if (getParam(y, 0, vs23.lastLine(), I_TO)) return;
  if (getParam(dx, 0, sc0.getGWidth(), I_COMMA)) return;
  if (getParam(dy, 0, vs23.lastLine(), I_SIZE)) return;
  if (getParam(w, 0, sc0.getGWidth()-x, I_COMMA)) return;
  if (getParam(h, 0, vs23.lastLine()-y, I_NONE)) return;
  if (*cip == I_UP) {
    ++cip;
    dir = 1;
  } else if (*cip == I_DOWN) {
    ++cip;
    dir = 0;
  }
    
  vs23.MoveBlock(x, y, dx, dy, w, h, dir);
}

// キャラクタスクロール CSCROLL X1,Y1,X2,Y2,方向
// 方向 0: 上, 1: 下, 2: 右, 3: 左
void  icscroll() {
#if USE_NTSC == 1
  int32_t x1,y1,x2,y2,d;
  if (scmode) {
    if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(d, I_NONE))
      return;
    if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc0.getWidth() || y2 >= sc0.getHeight())  {
      err = ERR_VALUE;
      return;
    }
    if (d < 0 || d > 3) d = 0;
    sc0.cscroll(x1, y1, x2-x1+1, y2-y1+1, d);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// グラフィックスクロール GSCROLL X1,Y1,X2,Y2,方向
void igscroll() {
  int32_t x1,y1,x2,y2,d;
  if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(d, I_NONE))
    return;
  if (x1 < 0 || y1 < 0 ||
      x2 <= x1 || y2 <= y1 ||
      x2 >= sc0.getGWidth() || y2 >= sc0.getGHeight()) {
    err = ERR_VALUE;
    return;
  }
  if (d < 0 || d > 3) d = 0;
  sc0.gscroll(x1,y1,x2-x1+1, y2-y1+1, d);
}

// シリアル1バイト出力 : SWRITE データ
void iswrite() {
  int32_t c;
  if ( getParam(c, I_NONE) ) return;
  Serial.write(c);
}

void SMALL ismode() {
  int32_t baud, flags = SERIAL_8N1;
  if ( getParam(baud, 0, 921600, I_NONE) ) return;
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(flags, 0, 0x3f, I_NONE)) return;
  }
  Serial.begin(baud, (enum SerialConfig)flags);
}

// TONE 周波数 [,音出し時間]
void itone() {
  int32_t freq;   // 周波数
  int32_t tm = 0; // 音出し時間

  if ( getParam(freq, 0, INT32_MAX, I_NONE) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(tm, 0, INT32_MAX, I_NONE) ) return;
  }
  tv_tone(freq, tm);
}

#include "SID.h"
#include "mml.h"
extern SID sid;
void isound() {
  int32_t reg, val;
  if ( getParam(reg, 0, INT32_MAX, I_COMMA) ) return;
  if ( getParam(val, 0, INT32_MAX, I_NONE) ) return;
  sid.set_register(reg, val);
}

BString mml_text;
void iplay() {
  sound_stop_mml();
  mml_text = istrexp();
  sound_play_mml(mml_text.c_str());
}

//　NOTONE
void inotone() {
  tv_notone();
}

// GPEEK(X,Y)関数の処理
int32_t igpeek() {
#if USE_NTSC == 1
  int x, y;  // 座標
  if (scmode) {
    if (checkOpen()) return 0;
    if ( getParam(x, I_COMMA) || getParam(y, I_NONE) ) return 0;
    if (checkClose()) return 0;
    if (x < 0 || y < 0 || x >= sc0.getGWidth()-1 || y >= sc0.getGHeight()-1) return 0;
    return sc0.gpeek(x,y);
  } else {
    err = ERR_NOT_SUPPORTED;
    return 0;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// GINP(X,Y,H,W,C)関数の処理
int32_t iginp() {
#if USE_NTSC == 1
  int32_t x,y,w,h,c;
  if (scmode) {
    if (checkOpen()) return 0;
    if ( getParam(x, I_COMMA)||getParam(y, I_COMMA)||getParam(w, I_COMMA)||getParam(h, I_COMMA)||getParam(c, I_NONE) ) return 0;
    if (checkClose()) return 0;
    if (x < 0 || y < 0 || x >= sc0.getGWidth() || y >= sc0.getGHeight() || h < 0 || w < 0) return 0;
    if (x+w >= sc0.getGWidth() || y+h >= sc0.getGHeight() ) return 0;
    return sc0.ginp(x, y, w, h, c);
  } else {
    err = ERR_NOT_SUPPORTED;
    return 0;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// MAP(V,L1,H1,L2,H2)関数の処理
int32_t imap() {
  int32_t value,l1,h1,l2,h2,rc;
  if (checkOpen()) return 0;
  if ( getParam(value, I_COMMA)||getParam(l1, I_COMMA)||getParam(h1, I_COMMA)||getParam(l2, I_COMMA)||getParam(h2, I_NONE) )
    return 0;
  if (checkClose()) return 0;
  if (l1 >= h1 || l2 >= h2 || value < l1 || value > h1) {
    err = ERR_VALUE;
    return 0;
  }
  rc = (value-l1)*(h2-l2)/(h1-l1)+l2;
  return rc;
}

// ASC(文字列)
int32_t iasc() {
  int32_t value;

  if (checkOpen()) return 0;
  value = istrexp()[0];
  checkClose();

  return value;
}

// PRINT handler
void iprint(uint8_t devno=0,uint8_t nonewln=0) {
  num_t value;     //値
  int len;       //桁数
  int32_t filenum;
  BString str;

  len = 0; //桁数を初期化
  while (!end_of_statement()) {
    if (is_strexp()) {
      str = istrexp();
      c_puts(str.c_str(), devno);
    } else  switch (*cip) { //中間コードで分岐
    case I_USING: //「#
      cip++;
      str = istrexp();
      if (str[0] == '#')
        len = strtonum(str.c_str() + 1, NULL);
      if (err || *cip++ != I_SEMI)
	return;
      break;

    case I_SHARP:
      cip++;
      if (getParam(filenum, 0, MAX_USER_FILES, I_COMMA))
        return;
      if (!user_files[filenum]) {
        err = ERR_FILE_NOT_OPEN;
        return;
      }
      bfs.setTempFile(user_files[filenum]);
      devno = 4;
      break;
      
    case I_CHR: // CHR$()関数
      cip++;
      if (getParam(value, 0,255, I_NONE)) break;   // 括弧の値を取得
      c_putch(value, devno);
      break;

    case I_DMP:  cip++; idmp(devno); break; // DMP$()関数

    default:	// anything else is assumed to be a numeric expression
      value = iexp();
      if (err) {
	newline();
	return;
      }
      putnum(value, len,devno);
      break;
    }

    if (err)  {
      newline(devno);
      return;
    }

    // 文字列引数流用時はここで終了
    // "Ends here when diverting character string argument"
    // WTF is that supposed to mean?
    if (nonewln && *cip == I_COMMA) 
      return;

    if (*cip == I_ELSE) {
      // XXX: Why newline() if it's ELSE, and not otherwise?
      // And disregarding nonewln, for that matter.
      newline(devno);
      return;
    } else if (*cip == I_COMMA || *cip == I_SEMI) {
      cip++;
      if (end_of_statement())
	return;
    } else {
      if (!end_of_statement()) {
	err = ERR_SYNTAX;
	newline(devno);
	return;
      }
    }
  }
  if (!nonewln)
    newline(devno);
}

// GPRINT x,y,..
void igprint() {
#if USE_NTSC == 1
  int32_t x,y;
  if (scmode) {
    if ( getParam(x, 0, sc0.getGWidth(), I_COMMA) ) return;
    if ( getParam(y, 0, sc0.getGHeight(), I_COMMA) ) return;
    sc0.set_gcursor(x,y);
    iprint(2);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// ファイル名引数の取得
BString getParamFname() {
  BString fname = istrexp();
  if (fname.length() >= SD_PATH_LEN)
    err = ERR_LONGPATH;
  return fname;
}


void SMALL ildbmp() {
  BString fname;
  int32_t dx = -1, dy = -1;
  int32_t x = 0,y = 0,w = -1, h = -1;
  int32_t spr_from = -1, spr_to = -1;
  bool define_bg = false;
  int bg;

  if(!(fname = getParamFname())) {
    return;
  }

  for (;; ) {
    if (*cip == I_AS) {
      cip++;
      if (*cip == I_BG) {		// AS BG ...
        ++cip;
        dx = dy = -1;
        if (getParam(bg,  0, VS23_MAX_BG-1, I_NONE)) return;
        define_bg = true;
      } else if (*cip == I_SPRITE) {	// AS SPRITE ...
        ++cip;
        dx = dy = -1;
        if (getParam(spr_from, 0, VS23_MAX_SPRITES-1, I_NONE)) {
          spr_from = 0;
          spr_to = VS23_MAX_SPRITES - 1;
        } else if (*cip == I_MINUS) {
          ++cip;
          if (getParam(spr_to, spr_from, VS23_MAX_SPRITES-1, I_NONE))
            return;
        } else {
          spr_to = spr_from;
        }
      } else {
	err = ERR_SYNTAX;
	return;
      }
    } else if (*cip == I_TO) {
      // TO dx,dy
      cip++;
      if ( getParam(dx,  0, INT32_MAX, I_COMMA) ) return;
      if ( getParam(dy,  0, INT32_MAX, I_NONE) ) return;
    } else if (*cip == I_OFF) {
      // OFF x,y
      cip++;
      if ( getParam(x,  0, INT32_MAX, I_COMMA) ) return;
      if ( getParam(y,  0, INT32_MAX, I_NONE) ) return;
    } else if (*cip == I_SIZE) {
      // SIZE w,h
      cip++;
      if ( getParam(w,  0, INT32_MAX, I_COMMA) ) return;
      if ( getParam(h,  0, INT32_MAX, I_NONE) ) return;
    } else {
      break;
    }
  }

  // 仮想アドレスから実アドレスへの変換
  // サイズチェック
  if (x + w > sc0.getGWidth() || y + h > vs23.lastLine()) {
    err = ERR_RANGE;
    return;
  }

  // 画像のロード
  err = bfs.loadBitmap((char *)fname.c_str(), dx, dy, x, y, w, h);
  if (!err) {
    if (define_bg)
      vs23.setBgPattern(bg, dx, dy, w / vs23.bgTileSizeX(bg));
    if (spr_from != -1) {
      for (int i = spr_from; i < spr_to + 1; ++i) {
        vs23.setSpritePattern(i, dx, dy);
      }
    }
    retval[0] = dx;
    retval[1] = dy;
    retval[2] = w;
    retval[3] = h;
  }
}

// MKDIR "ファイル名"
void imkdir() {
  uint8_t rc;
  BString fname;

  if(!(fname = getParamFname())) {
    return;
  }

#if USE_SD_CARD == 1
  rc = bfs.mkdir((char *)fname.c_str());
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
#endif
}

// RMDIR "ファイル名"
void irmdir() {
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

#if USE_SD_CARD == 1
  rc = bfs.rmdir((char *)fname.c_str());
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
#endif
}

// RENAME <old$> TO <new$>
void irename() {
  uint8_t rc;

  BString old_fname = getParamFname();
  if (err)
    return;
  if (*cip != I_TO) {
    err = ERR_SYNTAX;
    return;
  }
  cip++;
  BString new_fname = getParamFname();
  if (err)
    return;

  rc = Unifile::rename(old_fname.c_str(), new_fname.c_str());
  if (rc)
    err = ERR_FILE_WRITE;
}

// REMOVE "ファイル名"
void iremove() {
  BString fname;

  if(!(fname = getParamFname())) {
    return;
  }

  bool rc = Unifile::remove(fname.c_str());
  if (!rc) {
    err = ERR_FILE_WRITE;	// XXX: something more descriptive?
    return;
  }
}

// BSAVE "ファイル名", アドレス
void SMALL ibsave() {
  //char fname[SD_PATH_LEN];
  uint8_t*radr;
  uint32_t vadr, len;
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_COMMA) {
    err = ERR_SYNTAX;
    return;
  }
  cip++;
  if ( getParam(vadr, I_COMMA) ) return;  // アドレスの取得
  if ( getParam(len, I_NONE) ) return;             // データ長の取得

  // アドレスの範囲チェック
  if ( !sanitize_addr(vadr) || !sanitize_addr(vadr + len) ) {
    err = ERR_RANGE;
    return;
  }

  // ファイルオープン
#if USE_SD_CARD == 1
  rc = bfs.tmpOpen((char *)fname.c_str(),1);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err =  ERR_FILE_OPEN;
    return;
  }

  // データの書込み
  for (uint32_t i = 0; i < len; i++) {
    radr = sanitize_addr(vadr);
    if (radr == NULL) {
      goto DONE;
    }
    if(bfs.putch(*radr)) {
      err = ERR_FILE_WRITE;
      goto DONE;
    }
    vadr++;
  }

DONE:
  bfs.tmpClose();
#endif
  return;
}

void SMALL ibload() {
  uint8_t*radr;
  uint32_t vadr, len;
  int32_t c;
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_COMMA) {
    err = ERR_SYNTAX;
    return;
  }
  cip++;
  if ( getParam(vadr, I_COMMA) ) return;  // アドレスの取得
  if ( getParam(len, I_NONE) ) return;              // データ長の取得

  // アドレスの範囲チェック
  if ( !sanitize_addr(vadr) || !sanitize_addr(vadr + len) ) {
    err = ERR_RANGE;
    return;
  }
#if USE_SD_CARD == 1
  // ファイルオープン
  rc = bfs.tmpOpen((char *)fname.c_str(),0);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err =  ERR_FILE_OPEN;
    return;
  }

  // データの読込み
  for (uint32_t i = 0; i < len; i++) {
    radr = sanitize_addr(vadr);
    if (radr == NULL) {
      goto DONE;
    }
    c = bfs.read();
    if (c <0 ) {
      err = ERR_FILE_READ;
      goto DONE;
    }
    *radr = c;
    vadr++;
  }

DONE:
  bfs.tmpClose();
#endif
  return;
}

// TYPE "ファイル名"
void  icat() {
  //char fname[SD_PATH_LEN];
  //uint8_t rc;
  int32_t line = 0;
  uint8_t c;

  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

#if USE_SD_CARD == 1
  while(1) {
    rc = bfs.textOut((char *)fname.c_str(), line, sc0.getHeight());
    if (rc < 0) {
      if (rc == -SD_ERR_OPEN_FILE) {
	err = ERR_FILE_OPEN;
	return;
      } else if (rc == -SD_ERR_INIT) {
	err = ERR_SD_NOT_READY;
	return;
      } else if (rc == -SD_ERR_NOT_FILE) {
	err =  ERR_BAD_FNAME;
	return;
      }
    } else if (rc == 0) {
      break;
    }

    c_puts("== More?('Y' or SPACE key) =="); newline();
    c = c_getch();
    if (c != 'y' && c!= 'Y' && c != 32)
      break;
    line += sc0.getHeight();
  }
#endif
}

// ターミナルスクリーンの画面サイズ指定 WINDOW X,Y,W,H
void iwindow() {
  int32_t x, y, w, h;

  // Discard the dimensions saved for CONTing.
  restore_text_window = false;

  if (*cip == I_OFF) {
    ++cip;
    sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
    return;
  }

  if ( getParam(x,  0, sc0.getScreenWidth() - 8, I_COMMA) ) return;   // x
  if ( getParam(y,  0, sc0.getScreenHeight() - 2, I_COMMA) ) return;        // y
  if ( getParam(w,  8, sc0.getScreenWidth() - x, I_COMMA) ) return;   // w
  if ( getParam(h,  2, sc0.getScreenHeight() - y, I_NONE) ) return;        // h

  sc0.setWindow(x, y, w, h);
  sc0.locate(0,0);
  sc0.cls();
}

void ifont() {
  int32_t idx;
  if (getParam(idx, 0, NUM_FONTS, I_NONE))
    return;
  sc0.setFont(fonts[idx]);
}

// スクリーンモード指定 SCREEN M
void SMALL iscreen() {
  int32_t m;

  if ( getParam(m,  0, vs23.numModes, I_NONE) ) return;   // m

  // Discard dimensions saved for CONTing.
  restore_bgs = false;
  restore_text_window = false;

  vs23.reset();

  sc0.setFont(fonts[0]);
  sc0.cls();
  sc0.show_curs(true);
  sc0.locate(0,0);

  if (scmode == m)
    return;

  sc0.end();
  scmode = m;

  // NTSCスクリーン設定
  sc0.init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, m - 1);

  sc0.cls();
  sc0.show_curs(false);
  sc0.draw_cls_curs();
  sc0.locate(0,0);
  sc0.refresh();
}

void ipalette() {
  int32_t p, hw, sw, vw, f;
  if (getParam(p, 0, VS23_NUM_COLORSPACES - 1, I_NONE)) return;
  vs23.setColorSpace(p);
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(hw, 0, 7, I_COMMA) ) return;
    if (getParam(sw, 0, 7, I_COMMA) ) return;
    if (getParam(vw, 0, 7, I_COMMA) ) return;
    if (getParam(f, 0, 1, I_NONE)) return;
    vs23.setColorConversion(p, hw, sw, vw, !!f);
  }
}

void iborder() {
  int32_t y, uv;
  if (getParam(uv, 0, 255, I_COMMA)) return;
  if (getParam(y, 0, 255 - 0x66, I_NONE)) return;
  vs23.setBorder(y, uv);
}

void ibg() {
  int32_t m;
  int32_t w, h, px, py, pw, tx, ty, wx, wy, ww, wh;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetBgs();
    return;
  }

  if (getParam(m, 0, VS23_MAX_BG, I_NONE)) return;

  // Background modified, do not restore the saved values when CONTing.
  original_bg_height[m] = -1;
  turn_bg_back_on[m] = false;

  for (;;) switch (*cip++) {
  case I_TILES:
    // XXX: valid range depends on tile size
    if (getParam(w, 0, sc0.getGWidth(), I_COMMA)) return;
    if (getParam(h, 0, sc0.getGWidth(), I_NONE)) return;
    if (vs23.setBgSize(m, w, h)) {
      err = ERR_OOM;
      return;
    }
    break;
  case I_PATTERN:
    if (getParam(px, 0, sc0.getGWidth(), I_COMMA)) return;
    if (getParam(py, 0, vs23.lastLine(), I_COMMA)) return;
    // XXX: valid range depends on tile size
    if (getParam(pw, 0, sc0.getGWidth(), I_NONE)) return;
    vs23.setBgPattern(m, px, py, pw);
    break;
  case I_SIZE:
    if (getParam(tx, 8, 32, I_COMMA)) return;
    if (getParam(ty, 8, 32, I_NONE)) return;
    vs23.setBgTileSize(m, tx, ty);
    break;
  case I_WINDOW:
    if (getParam(wx, 0, sc0.getGWidth() - 1, I_COMMA)) return;
    if (getParam(wy, 0, sc0.getGHeight() - 1, I_COMMA)) return;
    if (getParam(ww, 0, sc0.getGWidth(), I_COMMA)) return;
    if (getParam(wh, 0, sc0.getGHeight(), I_NONE)) return;
    vs23.setBgWin(m, wx, wy, ww, wh);
    break;
  case I_ON:
    // XXX: do sanity check before enabling
    vs23.enableBg(m);
    break;
  case I_OFF:
    vs23.disableBg(m);
    break;
  default:
    cip--;
    if (!end_of_statement())
      err = ERR_SYNTAX;
    return;
  }
}

void iloadbg() {
  int32_t bg;
  uint8_t w, h, tsx, tsy;
  BString filename;

  cip++;

  if (!(filename = getParamFname()))
    return;
  if (getParam(bg, 0, VS23_MAX_BG, I_TO))
    return;
  
  Unifile f = Unifile::open(filename, FILE_READ);
  if (!f) {
    err = ERR_FILE_OPEN;
    return;
  }
  
  w = f.read();
  h = f.read();
  tsx = f.read();
  tsy = f.read();
  
  if (!w || !h || !tsx || !tsy) {
    err = ERR_VALUE;
    goto out;
  }
  
  vs23.setBgTileSize(bg, tsx, tsy);
  if (vs23.setBgSize(bg, w, h)) {
    err = ERR_OOM;
    goto out;
  }

  if (f.read((char *)vs23.bgTiles(bg), w*h) != w*h) {
    err = ERR_FILE_READ;
    goto out;
  }

out:
  f.close();
}

void isavebg() {
  int32_t bg;
  uint8_t w, h;
  BString filename;

  cip++;

  if (!(filename = getParamFname()))
    return;
  if (getParam(bg, 0, VS23_MAX_BG, I_TO))
    return;
  
  Unifile f = Unifile::open(filename, FILE_OVERWRITE);
  if (!f) {
    err = ERR_FILE_OPEN;
    return;
  }

  w = vs23.bgWidth(bg); h = vs23.bgHeight(bg);
  f.write(w); f.write(h);
  f.write(vs23.bgTileSizeX(bg));
  f.write(vs23.bgTileSizeY(bg));
  
  f.write((char *)vs23.bgTiles(bg), w*h);

  f.close();
}

void imovebg() {
  int32_t bg, x, y;
  if (getParam(bg, 0, VS23_MAX_BG, I_TO)) return;
  // XXX: arbitrary limitation?
  if (getParam(x, INT32_MIN, INT32_MAX, I_COMMA)) return;
  if (getParam(y, INT32_MIN, INT32_MAX, I_NONE)) return;
  
  vs23.scroll(bg, x, y);
}

void isprite() {
  int32_t num, pat_x, pat_y, w, h, frame_x, frame_y, flags;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetSprites();
    return;
  }

  if (getParam(num, 0, VS23_MAX_SPRITES, I_NONE)) return;
  for (;;) switch (*cip++) {
  case I_PATTERN:
    if (getParam(pat_x, 0, sc0.getGWidth(), I_COMMA)) return;
    if (getParam(pat_y, 0, 1023, I_NONE)) return;
    vs23.setSpritePattern(num, pat_x, pat_y);
    break;
  case I_SIZE:
    if (getParam(w, 1, VS23_MAX_SPRITE_W, I_COMMA)) return;
    if (getParam(h, 1, VS23_MAX_SPRITE_H, I_NONE)) return;
    vs23.resizeSprite(num, w, h);
    break;
  case I_FRAME:
    if (getParam(frame_x, 0, sc0.getGWidth(), I_NONE)) return;
    if (*cip == I_COMMA) {
      ++cip;
      if (getParam(frame_y, 0, sc0.getGHeight(), I_NONE)) return;
    } else
      frame_y = 0;
    vs23.setSpriteFrame(num, frame_x, frame_y);
    break;
  case I_ON:
    vs23.enableSprite(num);
    break;
  case I_OFF:
    vs23.disableSprite(num);
    break;
  case I_FLAGS:
    if (getParam(flags, 0, 1, I_NONE)) return;
    // Bit 0: sprite opaque
    vs23.setSpriteOpaque(num, flags & 1);
    break;
  default:
    // XXX: throw an error if nothing has been done
    cip--;
    if (!end_of_statement())
      err = ERR_SYNTAX;
    return;
  }
}

void imovesprite() {
  int32_t num, pos_x, pos_y;
  if (getParam(num, 0, VS23_MAX_SPRITES, I_TO)) return;
  if (getParam(pos_x, INT32_MIN, INT32_MAX, I_COMMA)) return;
  if (getParam(pos_y, INT32_MIN, INT32_MAX, I_NONE)) return;
  vs23.moveSprite(num, pos_x, pos_y);
}

void imove()
{
  if (*cip == I_SPRITE) {
    ++cip;
    imovesprite();
  } else if (*cip == I_BG) {
    ++cip;
    imovebg();
  } else
    err = ERR_SYNTAX;
}

void iplot() {
  int32_t bg, x, y, t;
  if (getParam(bg, 0, VS23_MAX_BG, I_COMMA)) return;
  if (!vs23.bgTiles(bg)) {
    // BG not defined
    err = ERR_RANGE;
    return;
  }
  if (getParam(x, 0, INT_MAX, I_COMMA)) return;
  if (getParam(y, 0, INT_MAX, I_COMMA)) return;
  if (getParam(t, 0, 255, I_NONE)) return;
  vs23.setBgTile(bg, x, y, t);
}

#include "Psx.h"
Psx psx;

static int cursor_pad_state()
{
  return kb.state(PS2KEY_L_Arrow) << psxLeftShift |
         kb.state(PS2KEY_R_Arrow) << psxRightShift |
         kb.state(PS2KEY_Down_Arrow) << psxDownShift |
         kb.state(PS2KEY_Up_Arrow) << psxUpShift |
         kb.state(PS2KEY_X) << psxXShift |
         kb.state(PS2KEY_A) << psxTriShift |
         kb.state(PS2KEY_S) << psxOShift |
         kb.state(PS2KEY_Z) << psxSquShift;
}

int32_t ipad() {
  int32_t num;
  if (checkOpen()) return 0;
  if (getParam(num, 0, 2, I_CLOSE)) return 0;
  switch (num) {
  case 0:	return psx.read() | cursor_pad_state();
  case 1:	return cursor_pad_state();
  case 2:	return psx.read();
  default:	return 0;
  }
}

int te_main(char argc, const char **argv);
void iedit() {
  BString fn;
  const char *argv[2] = { NULL, NULL };
  int argc = 1;
  if ((fn = getParamFname())) {
    ++argc;
    argv[1] = fn.c_str();
  }
  te_main(argc, argv);
}

//
// プログラムのロード・実行 LRUN/LOAD
// LRUN プログラム番号
// LRUN プログラム番号,行番号
// LRUN プログラム番号,"ラベル"
// LRUN "ファイル名"
// LRUN "ファイル名",行番号
// LRUN "ファイル名","ラベル"
// LOAD プログラム番号
// LOAD "ファイル名"

// 戻り値
//  1:正常 0:異常
//
uint8_t SMALL ilrun() {
  int32_t prgno;
  uint32_t lineno = (uint32_t)-1;
  uint8_t *lp;
  //char fname[SD_PATH_LEN];  // ファイル名
  uint8_t label[34];
  uint8_t len;
  uint8_t mode = 0;        // ロードモード 0:フラッシュメモリ 1:SDカード
  int8_t fg;               // ファイル形式 0:バイナリ形式  1:テキスト形式
  uint8_t rc;
  uint8_t islrun = 1;
  uint8_t newmode = 1;
  BString fname;
  int32_t flgMerge = 0;    // マージモード
  uint8_t flgPrm2 = 0;    // 第2引数の有無

  // コマンド識別
  if (*(cip-1) == I_LOAD) {
    islrun  = 0;
    lineno  = 0;
    newmode = 0;
  }

  // ファイル名またはプログラム番号の取得
  if (is_strexp()) {
    if(!(fname = getParamFname())) {
      return 0;
    }
    mode = 1;
  } else {
    err = ERR_SYNTAX;
    return 0;
  }

  if (islrun) {
    // LRUN
    // 第2引数 行番号またはラベルの取得
    if(*cip == I_COMMA) {  // 第2引数の処理
      cip++;
      if (*cip == I_STR) { // ラベルの場合
	cip++;
	len = *cip <= 32 ? *cip : 32;
	label[0] = I_STR;
	label[1] = len;
	strncpy((char*)label+2, (char*)cip+1, len);
	cip+=*cip+1;
      } else {             // 行番号の場合
	if ( getParam(lineno, I_NONE) ) return 0;
      }
    } else {
      lineno = 0;
    }
  } else {
    // LOAD
    if(*cip == I_COMMA) {  // 第2引数の処理
      cip++;
      if ( getParam(flgMerge, 0, 1, I_NONE) ) return 0;
      flgPrm2 = 1;
      if (flgMerge == 0) {
	newmode = 0; // 上書きモード
      } else {
	newmode = 2; // 追記モード
      }
    }
  }

  // プログラムのロード処理
  if (mode == 0) {
    // フラッシュメモリからプログラムのロード
    if (!islrun && flgPrm2) {
      // LOADコマンド時、フラッシュメモリからのロードで第2引数があった場合はエラーとする
      err = ERR_SYNTAX;
      return 0;
    }
    if ( loadPrg(prgno,newmode) ) {
      return 0;
    }
  } else {
#if USE_SD_CARD == 1
    // SDカードからプログラムのロード
    // SDカードからのロード
    fg = bfs.IsText((char *)fname.c_str()); // 形式チェック
    if (fg < 0) {
      // Abnormal form (形式異常)
      rc = -fg;
      if( rc == SD_ERR_INIT ) {
	err = ERR_SD_NOT_READY;
      } else if (rc == SD_ERR_OPEN_FILE) {
	err = ERR_FILE_OPEN;
      } else if (rc == SD_ERR_READ_FILE) {
	err = ERR_FILE_READ;
      } else if (rc == SD_ERR_NOT_FILE) {
	err = ERR_BAD_FNAME;
      }
    } else if (fg == 0) {
      // Binary format load from SD card
      err = ERR_NOT_SUPPORTED;
    } else if (fg == 1) {
      // Text format load from SD card
      if( loadPrgText((char *)fname.c_str(),newmode)) {
	err = ERR_FILE_READ;
      }
    }
    if (err)
      return 0;
#endif
  }

  // 行番号・ラベル指定の処理
  if (lineno == 0) {
    clp = listbuf; // 行ポインタをプログラム保存領域の先頭に設定
  } else if (lineno == (uint32_t)-1) {
    lp = getlpByLabel(label);
    if (lp == NULL) {
      err = ERR_ULN;
      return 0;
    }
    clp = lp;     // 行ポインタをプログラム保存領域の指定行先頭に設定
  } else {
    // 指定行にジャンプする
    lp = getlp(lineno);   // 分岐先のポインタを取得
    if (lineno != getlineno(lp)) {
      err = ERR_ULN;
      return 0;
    }
    clp = lp;     // 行ポインタをプログラム保存領域の指定ラベル先頭に設定
  }
  if (!err) {
    if (islrun || (cip >= listbuf && cip < listbuf+size_list)) {
      cip = clp+sizeof(num_t)+1;        // XXX: really? was 3
    }
  }
  return 1;
}

// output error message
// Arguments:
// flgCmd: set to false at program execution, true at command line
void SMALL error(uint8_t flgCmd = false) {
  char msg[40];
  if (err) {
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + size_list && *clp && !flgCmd) {
      // エラーメッセージを表示
      strcpy_P(msg, errmsg[err]);
      c_puts(msg);
      c_puts(" in ");
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      newline();

      // リストの該当行を表示
      putnum(getlineno(clp), 0);
      c_puts(" ");
      putlist(clp + sizeof(num_t) + 1);
      newline();
      //err = 0;
      //return;
    } else {                   // 指示の実行中なら
      strcpy_P(msg, errmsg[err]);
      c_puts(msg);     // エラーメッセージを表示
      newline();               // 改行
      //err = 0;               // エラー番号をクリア
      //return;
    }
  }
  strcpy_P(msg, errmsg[0]);
  c_puts(msg);           //「OK」を表示
  newline();                   // 改行
  err = 0;                     // エラー番号をクリア
}

num_t irgb() {
  int32_t r, g, b;
  if (checkOpen() ||
      getParam(r, 0, 255, I_COMMA) ||
      getParam(g, 0, 255, I_COMMA) ||
      getParam(b, 0, 255, I_CLOSE)) {
    err = ERR_SYNTAX;
    return 0;
  }
  return vs23.colorFromRgb(r, g, b);
}

// Get value
num_t GROUP(basic_core) ivalue() {
  num_t value, value2; // 値
  uint8_t i;   // 文字数
  int dims;
  int idxs[MAX_ARRAY_DIMS];
  int32_t a, b, c;

  switch (*cip++) { //中間コードで分岐

  //定数の取得
  case I_NUM:    // 定数
    os_memcpy(&value, cip, sizeof(num_t));
    cip += sizeof(num_t);
    break;

  case I_HEXNUM: // 16進定数
    value = cip[0] | (cip[1] << 8) | (cip[2] << 16) | (cip[3] << 24); //定数を取得
    cip += 4;
    break;

  //+付きの値の取得
  case I_PLUS: //「+」
    value = ivalue(); //値を取得
    break;

  //負の値の取得
  case I_MINUS: //「-」
    value = 0 - ivalue(); //値を取得して負の値に変換
    break;

  case I_LNOT: //「!」
    value = !ivalue(); //値を取得してNOT演算
    break;

  case I_BITREV: // 「~」 ビット反転
    //cip++; //中間コードポインタを次へ進める
    value = ~((uint32_t)ivalue()); //値を取得してNOT演算
    break;

  //変数の値の取得
  case I_VAR: //変数
    value = var.var(*cip++);
    break;
    
  case I_VARARR:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != num_arr.var(i).dims()) {
      err = ERR_SOR;
    } else {
      value = num_arr.var(i).var(idxs);
    }
    break;

  case I_SVAR:
    // String character accessor 
    i = *cip++;
    if (*cip++ != I_SQOPEN) {
      // XXX: Can we actually get here?
      err = ERR_SYNTAX;
      return 0;
    }
    if (getParam(a, 0, svar.var(i).length(), I_SQCLOSE))
      return 0;
    value = svar.var(i)[a];
    break;
    
  //括弧の値の取得
  case I_OPEN: //「(」
    cip--;
    value = getparam(); //括弧の値を取得
    break;

  case I_RND: //関数RND
    value = getparam(); //括弧の値を取得
    if (!err) {
      if (value < 0 )
	err = ERR_VALUE;
      else
	value = getrnd(value);  //乱数を取得
    }
    break;

  case I_ABS: //関数ABS
    value = getparam(); //括弧の値を取得
    if (value == INT32_MIN)
      err = ERR_VOF;
    if (err)
      break;
    if (value < 0)
      value *= -1;  //正負を反転
    break;

  case I_SIN:	value = sin(getparam()); break;
  case I_COS:	value = cos(getparam()); break;
  case I_EXP:	value = exp(getparam()); break;
  case I_ATN:	value = atan(getparam()); break;
  case I_ATN2:
    if (checkOpen() || getParam(value, I_COMMA) || getParam(value2, I_CLOSE))
      return 0;
    value = atan2(value, value2);
    break;
  case I_SQR:
    value = getparam();
    if (value < 0)
      err = ERR_FP;
    else
      value = sqrt(value);
    break;
  case I_TAN:
    // XXX: check for +/-inf
    value = tan(getparam()); break;
  case I_LOG:
    value = getparam();
    if (value <= 0)
      err = ERR_FP;
    else
      value = log(value);
    break;
      
  case I_FREE: //関数FREE
    if (checkOpen()||checkClose()) break;
#ifdef ESP8266
    value = umm_free_heap_size();
#else
    value = -1;
#endif
    break;

  case I_INKEY: //関数INKEY
    if (checkOpen()||checkClose()) break;
    value = iinkey(); // キー入力値の取得
    break;

  case I_VPEEK: value = ivpeek();  break; //関数VPEEK
  case I_GPEEK: value = igpeek();  break; //関数GPEEK(X,Y)
  case I_GINP:  value = iginp();   break; //関数GINP(X,Y,W,H,C)
  case I_MAP:   value = imap();    break; //関数MAP(V,L1,H1,L2,H2)
  case I_ASC:   value = iasc();    break; // 関数ASC(文字列)

  case I_LEN:  // 関数LEN(変数)
    if (checkOpen()) break;
    value = istrexp().length();
    break;

  case I_TICK: // 関数TICK()
    if ((*cip == I_OPEN) && (*(cip + 1) == I_CLOSE)) {
      // 引数無し
      value = 0;
      cip+=2;
    } else {
      value = getparam(); // 括弧の値を取得
      if (err)
	break;
    }
    if(value == 0) {
      value = millis();              // 0～INT32_MAX ms
    } else if (value == 1) {
      value = millis()/1000;         // 0～INT32_MAX s
    } else {
      value = 0;                                // 引数が正しくない
      err = ERR_VALUE;
    }
    break;

  case I_PEEK: value = ipeek();   break;     // PEEK()関数
  case I_I2CW:  value = ii2cw();   break;    // I2CW()関数
  case I_I2CR:  value = ii2cr();   break;    // I2CR()関数

  case I_RET:   value = iret(); break;
  case I_ARG:	value = iarg(); break;

  // 定数
  case I_HIGH:  value = CONST_HIGH; break;
  case I_LOW:   value = CONST_LOW;  break;
  case I_LSB:   value = CONST_LSB;  break;
  case I_MSB:   value = CONST_MSB;  break;

  case I_MPRG:  value = (unsigned int)listbuf;   break;
  case I_MFNT:  value = (unsigned int)sc0.getfontadr();   break;

  case I_DIN: // DIN(ピン番号)
    if (checkOpen()) break;
    if (getParam(a, 0, 15, I_NONE)) break;
    if (checkClose()) break;
    while (!blockFinished()) {}
    if (Wire.requestFrom(0x20, 2) != 2)
      err = ERR_IO;
    else {
      uint16_t state = Wire.read();
      state |= Wire.read() << 8;
      value = !!(state & (1 << a));
    }
    break;

  case I_ANA: // ANA(ピン番号)
#ifdef ESP8266_NOWIFI
    err = ERR_NOT_SUPPORTED;
#else
    if (checkOpen()) break;
    if (checkClose()) break;
    value = analogRead(A0);    // 入力値取得
#endif
    break;

  case I_EEPREAD: // EEPREAD(アドレス)の場合
    value = getparam();
    if (err) break;
    value = ieepread(value);   // 入力値取得
    break;

  case I_SREAD: // SREAD() シリアルデータ1バイト受信
    if (checkOpen()||checkClose()) break;
    value = Serial.read();
    break; //ここで打ち切る

  case I_SREADY: // SREADY() シリアルデータデータチェック
    if (checkOpen()||checkClose()) break;
    value = Serial.available();
    break; //ここで打ち切る

  // 画面サイズ定数の参照
  case I_CW: value = sc0.getWidth(); break;
  case I_CH: value = sc0.getHeight(); break;
#if USE_NTSC == 1
  case I_GW: value = scmode ? sc0.getGWidth() : 0; break;
  case I_GH: value = scmode ? sc0.getGHeight() : 0; break;
#else
  case I_GW: value = 0; break;
  case I_GH: value = 0; break;
#endif
  // カーソル・スクロール等の方向
  case I_UP:    value = psxUp; break;
  case I_DOWN:  value = psxDown; break;
  case I_RIGHT: value = psxRight; break;
  case I_LEFT:  value = psxLeft; break;

  case I_PAD:	value = ipad(); break;
  case I_RGB:	value = irgb(); break;

  case I_TILECOLL:
    if (checkOpen()) return 0;
    if (getParam(a, 0, VS23_MAX_SPRITES, I_COMMA)) return 0;
    if (getParam(b, 0, VS23_MAX_BG, I_COMMA)) return 0;
    if (getParam(c, 0, 255, I_NONE)) return 0;
    if (checkClose()) return 0;
    value = vs23.spriteTileCollision(a, b, c);
    break;

  case I_SPRCOLL:
    if (checkOpen()) return 0;
    if (getParam(a, 0, VS23_MAX_SPRITES, I_COMMA)) return 0;
    if (getParam(b, 0, VS23_MAX_SPRITES, I_NONE)) return 0;
    if (checkClose()) return 0;
    value = vs23.spriteCollision(a, b);
    break;

  default:
    cip--;
    if (is_strexp())
      err = ERR_TYPE;
    else
      err = ERR_SYNTAX;
    break;
  }
  return value; //取得した値を持ち帰る
}

// multiply or divide calculation
num_t GROUP(basic_core) imul() {
  num_t value, tmp; //値と演算値

  value = ivalue(); //値を取得
  if (err)
    return -1;

  while (1) //無限に繰り返す
    switch(*cip) { //中間コードで分岐

    case I_MUL: //掛け算の場合
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value *= tmp; //掛け算を実行
      break;

    case I_DIV: //割り算の場合
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      if (tmp == 0) { //もし演算値が0なら
	err = ERR_DIVBY0; //エラー番号をセット
	return -1;
      }
      value /= tmp; //割り算を実行
      break;

    case I_DIVR: //剰余の場合
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      if (tmp == 0) { //もし演算値が0なら
	err = ERR_DIVBY0; //エラー番号をセット
	return -1; //終了
      }
      value = (int32_t)value % (int32_t)tmp; //割り算を実行
      break;

    case I_LSHIFT: // シフト演算 "<<" の場合
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value =((uint32_t)value)<<(uint32_t)tmp;
      break;

    case I_RSHIFT: // シフト演算 ">>" の場合
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value =((uint32_t)value)>>(uint32_t)tmp;
      break;

    case I_AND: // 算術積(ビット演算)
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value =((uint32_t)value)&((uint32_t)tmp);
      break; //ここで打ち切る

    case I_OR:  //算術和(ビット演算)
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value =((uint32_t)value)|((uint32_t)tmp);
      break;

    case I_XOR: //非排他OR(ビット演算)
      cip++; //中間コードポインタを次へ進める
      tmp = ivalue(); //演算値を取得
      value =((uint32_t)value)^((uint32_t)tmp);

    default: //以上のいずれにも該当しなかった場合
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

// add or subtract calculation
num_t GROUP(basic_core) iplus() {
  num_t value, tmp; //値と演算値
  value = imul(); //値を取得
  if (err)
    return -1;

  while (1)
    switch(*cip) {
    case I_PLUS: //足し算の場合
      cip++; //中間コードポインタを次へ進める
      tmp = imul(); //演算値を取得
      value += tmp; //足し算を実行
      break;

    case I_MINUS: //引き算の場合
      cip++; //中間コードポインタを次へ進める
      tmp = imul(); //演算値を取得
      value -= tmp; //引き算を実行
      break;

    default: //以上のいずれにも該当しなかった場合
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

BString ilrstr(bool right) {
  BString value;
  int len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    err = ERR_SYNTAX;
    goto out;
  }

  if (getParam(len, I_CLOSE)) goto out;

  if (right)
    value = value.substring(value.length() - len, value.length());
  else
    value = value.substring(0, len);

out:
  return value;
}

BString imidstr() {
  BString value;
  int32_t start;
  int32_t len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    err = ERR_SYNTAX;
    goto out;
  }

  if (getParam(start, I_COMMA)) goto out;
  if (getParam(len, I_CLOSE)) goto out;

  value = value.substring(start, start + len);

out:
  return value;
}

BString istrvalue()
{
  BString value;
  BString *bp;
  int len, dims;
  char c;
  uint8_t i;
  int idxs[MAX_ARRAY_DIMS];

  switch (*cip++) {
  case I_STR:
    len = value.fromBasic(cip);
    cip += len;
    if (!len)
      err = ERR_OOM;
    break;
  case I_SVAR:
    value = svar.var(*cip++);
    break;

  case I_STRARR:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != str_arr.var(i).dims()) {
      err = ERR_SOR;
    } else {
      value = str_arr.var(i).var(idxs);
    }
    break;

  case I_ARGSTR:
    bp = iargstr();
    if (!err)
      value = *bp;
    break;
  case I_STRSTR:
    if (checkOpen()) return value;
    // The BString ctor for doubles is not helpful because it uses dtostrf()
    // which can only do a fixed number of decimal places. That is not
    // the BASIC Way(tm).
    sprintf(lbuf, "%0g", iexp());
    value = lbuf;
    if (checkClose()) return value;
    break;
  case I_LEFTSTR:
    value = ilrstr(false);
    break;
  case I_RIGHTSTR:
    value = ilrstr(true);
    break;
  case I_MIDSTR:
    value = imidstr();
    break;
  case I_HEX:
    value = ihex();
    break;
  case I_BIN:
    value = ibin();
    break;
  case I_CWD:
    value = Unifile::cwd();
    break;
  case I_INKEYSTR:
    c = iinkey();
    if (c)
      value = BString(c);
    else
      value = "";
    break;
  default:
    cip--;
    // Check if a numeric expression follows, so we can give a more
    // helpful error message.
    err = 0;
    iexp();
    if (!err)
      err = ERR_TYPE;
    else
      err = ERR_SYNTAX;
    break;
  }
  if (err)
    return BString();
  else
    return value;
}

BString istrexp()
{
  BString value, tmp;
  
  value = istrvalue();

  for (;;) switch(*cip) {
  case I_PLUS:
    cip++;
    tmp = istrvalue();
    if (err)
      return BString();
    value += tmp;
    break;
  default:
    return value;
  }
}

// Numeric expression parser
num_t GROUP(basic_core) iexp() {
  num_t value, tmp;

  if (is_strexp()) {
    // string comparison (or error)
    BString lhs = istrexp();
    BString rhs;
    switch (*cip++) {
    case I_EQ:
      rhs = istrexp();
      return lhs == rhs;
    case I_NEQ:
    case I_NEQ2:
      rhs = istrexp();
      return lhs != rhs;
    case I_LT:
      rhs = istrexp();
      return lhs < rhs;
    case I_GT:
      rhs = istrexp();
      return lhs > rhs;
    default:
      err = ERR_TYPE;
      return -1;
    }
  }

  value = iplus();
  if (err)
    return -1;

  // conditional expression
  while (1)
    switch(*cip++) {
    case I_EQ:
      tmp = iplus();
      value = (value == tmp);
      break;
    case I_NEQ:
    case I_NEQ2:
      tmp = iplus();
      value = (value != tmp);
      break;
    case I_LT:
      tmp = iplus();
      value = (value < tmp);
      break;
    case I_LTE:
      tmp = iplus();
      value = (value <= tmp);
      break;
    case I_GT:
      tmp = iplus();
      value = (value > tmp);
      break;
    case I_GTE:
      tmp = iplus();
      value = (value >= tmp);
      break;
    case I_LAND:
      tmp = iplus();
      value = (value && tmp);
      break;
    case I_LOR:
      tmp = iplus();
      value = (value || tmp);
      break;
    default:
      cip--;
      return value;
    }
}

// Get number of line at top left of the screen
uint32_t getTopLineNum() {
  uint8_t* ptr = sc0.getScreen();
  uint32_t n = 0;
  int rc = -1;
  while (isDigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
    if (n>INT32_MAX) {
      n = 0;
      break;
    }
    ptr++;
  }
  if (!n)
    rc = -1;
  else
    rc = n;
  return rc;
}

// Get number of line at the bottom left of the screen
uint32_t getBottomLineNum() {
  uint8_t* ptr = sc0.getScreen()+sc0.getWidth()*(sc0.getHeight()-1);
  uint32_t n = 0;
  int rc = -1;
  while (isDigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
    if (n>INT32_MAX) {
      n = 0;
      break;
    }
    ptr++;
  }
  if (!n)
    rc = -1;
  else
    rc = n;
  return rc;
}

// Get the number of the line preceding the specified line
uint32_t getPrevLineNo(uint32_t lineno) {
  uint8_t* lp, *prv_lp = NULL;
  int32_t rc = -1;
  for ( lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) {
    prv_lp = lp;
  }
  if (prv_lp)
    rc = getlineno(prv_lp);
  return rc;
}

// Get the number of the line succeeding the specified line
uint32_t getNextLineNo(uint32_t lineno) {
  uint8_t* lp;
  int32_t rc = -1;

  lp = getlp(lineno);
  if (lineno == getlineno(lp)) {
    // Move to the next line
    lp+=*lp;
    rc = getlineno(lp);
  }
  return rc;
}

// Get the program text of the specified line
char* getLineStr(uint32_t lineno) {
  uint8_t* lp = getlp(lineno);
  if (lineno != getlineno(lp))
    return NULL;

  // Output of specified line text to line buffer
  cleartbuf();
  putnum(lineno, 0, 3);
  c_putch(' ', 3);
  putlist(lp+sizeof(num_t)+1, 3);
  c_putch(0,3);        // zero-terminate tbuf
  return tbuf;
}

// システム情報の表示
void SMALL iinfo() {
  char top = 't';
  uint32_t adr = (uint32_t)&top;
  uint8_t* tmp = (uint8_t*)malloc(1);
  uint32_t hadr = (uint32_t)tmp;
  free(tmp);

  // スタック領域先頭アドレスの表示
  c_puts("stack top:");
  putHexnum(adr, 8);
  newline();

  // ヒープ領域先頭アドレスの表示
  c_puts("heap top :");
  putHexnum(hadr, 8);
  newline();

  // SRAM未使用領域の表示
  c_puts("SRAM Free:");
  putnum(adr-hadr, 0);
  newline();
}

static void do_goto(uint32_t line)
{
  uint8_t *lp = getlp(line);
  if (line != getlineno(lp)) {            // もし分岐先が存在しなければ
    err = ERR_ULN;                          // エラー番号をセット
    return;
  }

  clp = lp;        // 行ポインタを分岐先へ更新
  cip = clp + sizeof(num_t) + 1; // 中間コードポインタを先頭の中間コードに更新
}

// GOTO
void igoto() {
  uint32_t lineno;    // 行番号

  // 引数の行番号取得
  lineno = iexp();
  if (err) return;
  do_goto(lineno);
}

static void do_gosub(uint32_t lineno)
{
  uint8_t *lp = getlp(lineno);                       // 分岐先のポインタを取得
  if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
    err = ERR_ULN;                          // エラー番号をセット
    return;
  }

  //ポインタを退避
  if (gstki > SIZE_GSTK - 3) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
    return;
  }
  gstk[gstki++] = clp;                      // 行ポインタを退避
  gstk[gstki++] = cip;                      // 中間コードポインタを退避
  gstk[gstki++] = 0;

  clp = lp;                                 // 行ポインタを分岐先へ更新
  cip = clp + sizeof(num_t) + 1;            // 中間コードポインタを先頭の中間コードに更新
}

// GOSUB
void igosub() {
  uint32_t lineno;    // 行番号

  // 引数の行番号取得
  lineno = iexp();
  if (err)
    return;
  do_gosub(lineno);
}

// ON ... <GOTO|GOSUB> ...
static void on_go(bool is_gosub, int cas)
{
  uint32_t line;
  for (;;) {
    line = iexp();
    if (err)
      return;

    if (!cas)
      break;

    if (*cip != I_COMMA) {
      err = ERR_ULN;
      return;
    }
    ++cip;
    --cas;
  }

  if (is_gosub)
    do_gosub(line);
  else
    do_goto(line);
}

void ion()
{
  uint32_t cas = iexp();
  if (*cip == I_GOTO) {
    ++cip;
    on_go(false, cas);
  } else if (*cip == I_GOSUB) {
    ++cip;
    on_go(true, cas);
  } else {
    err = ERR_SYNTAX;
  }
}

void icall() {
  num_t n;
  uint8_t proc_idx = *cip++;

  struct basic_location proc_loc = proc.proc(proc_idx);  
  dbg_var("call got %p/%p for %d\n", proc_loc.lp, proc_loc.ip, proc_idx);

  if (!proc_loc.lp || !proc_loc.ip) {
    err = ERR_UNDEFPROC;
    return;
  }
  
  if (gstki > SIZE_GSTK - 3) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
    return;
  }

  if (checkOpen())
    return;

  int num_args = 0;
  int str_args = 0;
  if (*cip != I_CLOSE) for(;;) {
    if (is_strexp()) {
      BString b = istrexp();
      if (err)
        return;
      if (astk_str_i >= SIZE_ASTK)
        goto overflow;
      astk_str[astk_str_i++] = b;
      ++str_args;
    } else {
      n = iexp();
      if (err)
        return;
      if (astk_num_i >= SIZE_ASTK)
        goto overflow;
      astk_num[astk_num_i++] = n;
      ++num_args;
    }
    if (*cip != I_COMMA)
      break;
    ++cip;
  }

  if (checkClose())
    return;

  gstk[gstki++] = clp;                      // 行ポインタを退避
  gstk[gstki++] = cip;                      // 中間コードポインタを退避
  gstk[gstki++] = (unsigned char *)(num_args | (str_args << 16));

  clp = proc_loc.lp;
  cip = proc_loc.ip;
  return;
overflow:
  err = ERR_ASTKOF;
  return;
}

void iproc() {
  ++cip;
}

// RETURN
void ireturn() {
  if (gstki < 3) {    // もしGOSUBスタックが空なら
    err = ERR_GSTKUF; // エラー番号をセット
    return;
  }

  // Set return values, if any.
  if (!end_of_statement()) {
    int rcnt = 0;
    do {
      // XXX: implement return strings
      retval[rcnt++] = iexp();
    } while (*cip++ == I_COMMA && rcnt < MAX_RETVALS);
  }

  uint32_t a = (uint32_t)gstk[--gstki];
  astk_num_i -= a & 0xffff;
  astk_str_i -= a >> 16;
  cip = gstk[--gstki]; //行ポインタを復帰
  clp = gstk[--gstki]; //中間コードポインタを復帰
  return;
}

void GROUP(basic_core) ido() {
  if (lstki > SIZE_LSTK - 5) {
    err = ERR_LSTKOF;
    return;
  }
  lstk[lstki++] = clp;
  lstk[lstki++] = cip;
  lstk[lstki++] = (unsigned char*)-1;
  lstk[lstki++] = (unsigned char*)-1;
  lstk[lstki++] = (unsigned char*)-1;
}

// FOR
void GROUP(basic_core) ifor() {
  int index, vto, vstep; // FOR文の変数番号、終了値、増分

  // 変数名を取得して開始値を代入（例I=1）
  if (*cip++ != I_VAR) { // もし変数がなかったら
    err = ERR_FORWOV;    // エラー番号をセット
    return;
  }
  index = *cip; // 変数名を取得
  ivar();       // 代入文を実行
  if (err)      // もしエラーが生じたら
    return;

  // 終了値を取得（例TO 5）
  if (*cip == I_TO) { // もしTOだったら
    cip++;             // 中間コードポインタを次へ進める
    vto = iexp();      // 終了値を取得
  } else {             // TOではなかったら
    err = ERR_FORWOTO; //エラー番号をセット
    return;
  }

  // 増分を取得（例STEP 1）
  if (*cip == I_STEP) { // もしSTEPだったら
    cip++;              // 中間コードポインタを次へ進める
    vstep = iexp();     // 増分を取得
  } else                // STEPではなかったら
    vstep = 1;          // 増分を1に設定

  // もし変数がオーバーフローする見込みなら
  if (((vstep < 0) && (-INT32_MAX - vstep > vto)) ||
      ((vstep > 0) && (INT32_MAX - vstep < vto))) {
    err = ERR_VOF; //エラー番号をセット
    return;
  }

  // 繰り返し条件を退避
  if (lstki > SIZE_LSTK - 5) { // もしFORスタックがいっぱいなら
    err = ERR_LSTKOF;          // エラー番号をセット
    return;
  }
  lstk[lstki++] = clp; // 行ポインタを退避
  lstk[lstki++] = cip; // 中間コードポインタを退避

  // FORスタックに終了値、増分、変数名を退避
  // Special thanks hardyboy
  lstk[lstki++] = (unsigned char*)(uintptr_t)vto;
  lstk[lstki++] = (unsigned char*)(uintptr_t)vstep;
  lstk[lstki++] = (unsigned char*)(uintptr_t)index;
}

void GROUP(basic_core) iloop() {
  uint8_t cond;

  if (lstki < 5) {
    err = ERR_LSTKUF;
    return;
  }

  cond = *cip++;
  if (cond != I_WHILE && cond != I_UNTIL) {
    err = ERR_LOOPWOC;
    return ;
  }

  // Look for nearest DO.
  while (lstki) {
    if ((intptr_t)lstk[lstki - 1] == -1)
      break;
    lstki -= 5;
  }
  
  if (!lstki) {
    err = ERR_LSTKUF;
    return;
  }

  num_t exp = iexp();
  
  if ((cond == I_WHILE && exp != 0) ||
      (cond == I_UNTIL && exp == 0)) {
    // Condition met, loop.
    cip = lstk[lstki - 4];
    clp = lstk[lstki - 5];
  } else {
    // Pop loop off stack.
    lstki -= 5;
  }
}

// NEXT
void GROUP(basic_core) inext() {
  int want_index;	// variable we want to NEXT
  int index;		// variable index
  int vto;		// end of loop value
  int vstep;		// increment value

  if (lstki < 5) {    // FOR stack is empty
    err = ERR_LSTKUF;
    return;
  }

  if (*cip != I_VAR)
    want_index = -1;		// just use whatever is TOS
  else {
    ++cip;
    want_index = *cip++;	// NEXT a specific iterator
  }

  while (lstki) {
    // Get index of iterator on top of stack.
    index = (int)(uintptr_t)lstk[lstki - 1];

    // Done if it's the one we want (or if none is specified).
    if (want_index < 0 || want_index == index)
      break;

    // If it is not the specified variable, we assume we
    // want to NEXT to a loop higher up the stack.
    lstki -= 5;
  }

  if (!lstki) {
    // Didn't find anything that matches the NEXT.
    err = ERR_LSTKUF;	// XXX: have something more descriptive
    return;
  }

  vstep = (int)(uintptr_t)lstk[lstki - 2];
  var.var(index) += vstep;
  vto = (int)(uintptr_t)lstk[lstki - 3];

  // Is this loop finished?
  if (((vstep < 0) && (var.var(index) < vto)) ||
      ((vstep > 0) && (var.var(index) > vto))) {
    lstki -= 5;  // drop it from FOR stack
    return;
  }

  // Jump to the start of the loop.
  cip = lstk[lstki - 4];
  clp = lstk[lstki - 5];
}

// IF
void iif() {
  num_t condition;    // IF文の条件値
  uint8_t* newip;       // ELSE文以降の処理対象ポインタ

  condition = iexp(); // 真偽を取得
  if (err) {          // もしエラーが生じたら
    err = ERR_IFWOC;  // エラー番号をセット
    return;
  }

  ifstk[ifstki] = false;	// assume it's a single-line IF
  if (*cip == I_THEN) {
    ++cip;
    if (*cip == I_EOL) {
      // THEN followed by EOL indicates a multi-line IF.
      ifstk[ifstki] = true;
    }
  }
  inc_ifstk();

  if (condition) {    // もし真なら
    return;
  } else {
    // 偽の場合の処理
    // ELSEがあるかチェックする
    // もしELSEより先にIFが見つかったらELSE無しとする
    // ELSE文が無い場合の処理はREMと同じ
    newip = getELSEptr(cip);
    if (newip != NULL) {
      cip = newip;
      return;
    }
    while (*cip != I_EOL) // I_EOLに達するまで繰り返す
      cip++;              // 中間コードポインタを次へ進める
  }
}

void iendif()
{
  if (ifstki > 0) {
    dec_ifstk();
  }
  // XXX: Do we want to consider it an error if we encounter an ENDIF
  // outside an IF block?
}

void ielse()
{
  // When we encounter an ELSE in our normal instruction stream, we have
  // to skip what follows.
  dec_ifstk();
  if (ifstk[ifstki]) {
    // Multi-line IF, find ENDIF.
    for (;;) {
      while (*cip != I_EOL && *cip != I_ENDIF)
        cip++;
      if (*cip == I_EOL) {
        clp += *clp;
        cip = clp + sizeof(num_t) + 1;
      } else
        break;
    }
  } else {
    // Single-line IF, skip to EOL.
    while (*cip != I_EOL) // I_EOLに達するまで繰り返す
      cip++;              // 中間コードポインタを次へ進める
  }
}

// スキップ
void iskip() {
  while (*cip != I_EOL) // I_EOLに達するまで繰り返す
    cip++;              // 中間コードポインタを次へ進める
}

// END
void iend() {
  while (*clp)    // 行の終端まで繰り返す
    clp += *clp;  // 行ポインタを次へ進める
}

void ecom() {
  err = ERR_COM;
}

void esyntax() {
  cip--;
  err = ERR_SYNTAX;
}

void iprint_() {
  iprint();
}

void irefresh() {
  sc0.refresh();
}

void isprint() {
  iprint(1);
}

void inew_() {
  inew();
}

void iclv() {
  inew(2);
}

void inil() {
}

void eunimp() {
  err = ERR_NOT_SUPPORTED;
}

void ilist_() {
  ilist();
}

void ilrun_() {
  if (*cip == I_PCX) {
    cip++;
    ildbmp();
  } else if (*cip == I_BG) {
    iloadbg();
  } else
    ilrun();
}

void istop() {
  err = ERR_CTR_C;
}

void ichdir() {
  BString new_cwd;
  if(!(new_cwd = getParamFname())) {
    return;
  }
  if (!Unifile::chDir(new_cwd.c_str()))
    err = ERR_FILE_OPEN;
}

void iopen() {
  BString filename;
  uint8_t flags = FILE_READ;
  int32_t filenum;

  if (!(filename = getParamFname()))
    return;
  
  if (*cip == I_FOR) {
    ++cip;
    switch (*cip++) {
    case I_OUTPUT:	flags = FILE_OVERWRITE; break;
    case I_INPUT:	flags = FILE_READ; break;
    case I_APPEND:	flags = FILE_WRITE; break;
    default:		err = ERR_SYNTAX; return;
    }
  }
  
  if (*cip++ != I_AS) {
    err = ERR_SYNTAX;
    return;
  }
  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;
  
  user_files[filenum] = Unifile::open(filename, flags);
  if (!user_files[filenum])
    err = ERR_FILE_OPEN;
}

void iclose() {
  int32_t filenum;

  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;

  if (!user_files[filenum])
    err = ERR_FILE_NOT_OPEN;
  else
    user_files[filenum].close();
}

typedef void (*cmd_t)();
#include "funtbl.h"

// 中間コードの実行
// 戻り値      : 次のプログラム実行位置(行の先頭)
unsigned char* GROUP(basic_core) iexe() {
  uint8_t c;               // 入力キー
  err = 0;

  while (*cip != I_EOL) { //行末まで繰り返す

    pump_events();
    //強制的な中断の判定
    c = c_kbhit();
    if (c) { // もし未読文字があったら
      if (c == SC_KEY_CTRL_C || c==27 ) { // 読み込んでもし[ESC],［CTRL_C］キーだったら
	err = ERR_CTR_C;                  // エラー番号をセット
	prevPressKey = 0;
	break;
      } else {
	prevPressKey = c;
      }
    }

    //中間コードを実行
    if (*cip < sizeof(funtbl)/sizeof(funtbl[0])) {
      funtbl[*cip++]();
    } else
      err = ERR_SYNTAX;

    if (err)
      return NULL;
  }

  // Last IF is single-line, so it ends here, at I_EOL.
  if (ifstki > 0 && ifstk[ifstki-1] == false) {
    dec_ifstk(NULL);
  }

  return clp + *clp;
}

void iflash();

void SMALL resize_windows()
{
  int x, y, w, h;
  sc0.getWindow(x, y, w, h);
  restore_bgs = false;
  restore_text_window = false;
  // Default text window?
  if (x == 0 && w == sc0.getScreenWidth() &&
      y == 0 && h == sc0.getScreenHeight()) {
    // If there are backgrounds enabled, they partially or fully obscure
    // the text window. We adjust it to cover the bottom 5 lines only,
    // and clip or disable any backgrounds that overlap the new text
    // window.
    bool obscured = false;
    int top_y = (y + (sc0.getScreenHeight() - 5)) * sc0.getFontHeight();
    for (int i = 0; i < VS23_MAX_BG; ++i) {
      if (vs23.bgWinY(i) >= top_y) {
        vs23.disableBg(i);
        turn_bg_back_on[i] = true;
        restore_bgs = true;
      }
      else if (vs23.bgWinY(i) + vs23.bgWinHeight(i) >= top_y) {
        original_bg_height[i] = vs23.bgWinHeight(i);
        vs23.setBgWin(i, vs23.bgWinX(i), vs23.bgWinY(i), vs23.bgWinWidth(i),
                      vs23.bgWinHeight(i) - vs23.bgWinY(i) - vs23.bgWinHeight(i) + top_y);
        obscured = true;
        turn_bg_back_on[i] = false;
        restore_bgs = true;
      } else {
        original_bg_height[i] = -1;
        turn_bg_back_on[i] = false;
      }
    }
    if (obscured) {
      original_text_pos[0] = sc0.c_x();
      original_text_pos[1] = sc0.c_y();
      sc0.setWindow(x, y + sc0.getScreenHeight() - 5, w, 5);
      sc0.locate(0,0);
      sc0.cls();
      restore_text_window = true;
    }
  }
}

void SMALL restore_windows()
{
  if (restore_text_window) {
    restore_text_window = false;
    sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
    sc0.locate(original_text_pos[0], original_text_pos[1]);
  }
  if (restore_bgs) {
    restore_bgs = false;
    for (int i = 0; i < VS23_MAX_BG; ++i) {
      if (turn_bg_back_on[i]) {
        vs23.enableBg(i);
      } else if (original_bg_height[i] >= 0) {
        vs23.setBgWin(i, vs23.bgWinX(i), vs23.bgWinY(i), vs23.bgWinWidth(i),
                      original_bg_height[i]);
      }
    }
  }
}

//Command precessor
uint8_t SMALL icom() {
  uint8_t rc = 1;
  cip = ibuf;          // 中間コードポインタを中間コードバッファの先頭に設定

  switch (*cip++) {    // 中間コードポインタが指し示す中間コードによって分岐
  case I_LOAD:  ilrun_(); break;

  case I_LRUN:  if(ilrun()) {
      sc0.show_curs(0); irun(clp);  sc0.show_curs(1);
  }
    break;
  case I_RUN:
    sc0.show_curs(0);
    irun();
    sc0.show_curs(1);
    break;
  case I_CONT:
    if (!cont_cip || !cont_clp) {
      err = ERR_CONT;
    } else {
      restore_windows();
      sc0.show_curs(0);
      irun(NULL, true);
      sc0.show_curs(1);
    }
    break;
  case I_RENUM: // I_RENUMの場合
    //if (*cip == I_EOL || *(cip + 3) == I_EOL || *(cip + 7) == I_EOL)
    irenum();
    //else
    //  err = ERR_SYNTAX;
    break;
  case I_DELETE:     idelete();  break;
  case I_FORMAT:     iformat(); break;
  case I_FLASH:      iflash(); break;

  default:            // どれにも該当しない場合
    cip--;
    sc0.show_curs(0);
    iexe();           // 中間コードを実行
    sc0.show_curs(1);
    break;
  }
  
  if (err)
    resize_windows();

  return rc;
}

#include "basic_engine_pcx.h"
#include "dr_pcx.h"

static size_t read_image_bytes(void *user_data, void *buf, size_t bytesToRead) {
  unsigned char **ldp = (unsigned char **)user_data;

  if (buf == (void *)-1)
    return (size_t)*ldp;
  else if (buf == (void *)-2)
    return (size_t)(*ldp = (unsigned char *)bytesToRead);
  else {
    memcpy_P(buf, *ldp, bytesToRead);
    *ldp += bytesToRead;
    return bytesToRead;
  }
}

static void show_logo() {
  const unsigned char *ld = basic_engine_pcx;
  drpcx_load(read_image_bytes, &ld, false, NULL, NULL, NULL, 0,
             sc0.getGWidth() - 160 - 0, 0, 0, 0, 160, 64);
}

/*
   TOYOSHIKI Tiny BASIC
   The BASIC entry point
 */
void SMALL basic() {
  unsigned char len; // Length of intermediate code
  char* textline;    // input line
  uint8_t rc;

  vs23.begin();

  psx.setupPins(0, 1, 2, 3, 3);

  size_list = 0;
  listbuf = NULL;

  loadConfig();

  // Initialize execution environment
  inew();

  sc0.init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, SC_DEFAULT);

  sound_init();

  Wire.begin(2, 0);
  // ESP8266 Wire code assumes that SCL and SDA pins are set low, instead
  // of taking care of that itself. WTF?!?
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  icls();
  sc0.show_curs(0);

  // Want to make sure we get the right hue.
  vs23.setColorSpace(1);
  vs23.setColorConversion(1, 7, 3, 3, true);
  show_logo();
  vs23.setColorSpace(1);	// reset color conversion

  // Startup screen
  // Epigram
  sc0.setFont(fonts[1]);
  sc0.setColor(vs23.colorFromRgb(72,72,72), 0);
  {
    char epi[80];
    strcpy_P(epi, epigrams[random(sizeof(epigrams)/sizeof(*epigrams))]);
    c_puts(epi);
  }
  newline();

  // Banner
  sc0.setColor(vs23.colorFromRgb(192,0,0), 0);
  static const char engine_basic[] PROGMEM = "Engine BASIC";
  c_puts_P(engine_basic);
  sc0.setFont(fonts[0]);

  // Platform/version
  sc0.setColor(vs23.colorFromRgb(64,64,64), 0);
  sc0.locate(sc0.getWidth() - 14, 8);
  c_puts(STR_EDITION);
  c_puts(" " STR_VARSION);

  // Initialize file systems
  SPIFFS.begin();
#if USE_SD_CARD == 1
  // Initialize SD card file system
  bfs.init(16);		// CS on GPIO16
#endif

  // Free memory
  sc0.setColor(vs23.colorFromRgb(255,255,255), 0);
  sc0.locate(0,2);
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
  static const char bytes_free[] PROGMEM = " bytes free";
  c_puts_P(bytes_free); newline();
#endif

  sc0.show_curs(1);
  error();          // "OK" or display an error message and clear the error number

  // Program auto-start
  if (CONFIG.STARTPRG >=0  && loadPrg(CONFIG.STARTPRG) == 0) {
    sc0.show_curs(0);
    irun();
    sc0.show_curs(1);
    newline();
    c_puts("Autorun No."); putnum(CONFIG.STARTPRG,0); c_puts(" stopped.");
    newline();
    err = 0;
  }

  // Enter one line from the terminal and execute
  while (1) {
    rc = sc0.edit();
    if (rc) {
      textline = (char*)sc0.getText();
      if (!strlen(textline) ) {
	newline();
	continue;
      }
      if (strlen(textline) >= SIZE_LINE) {
	err = ERR_LONG;
	newline();
	error();
	continue;
      }

      strcpy(lbuf, textline);
      tlimR((char*)lbuf);
      newline();
    } else {
      continue;
    }

    // Convert one line of text to a sequence of intermediate code
    len = toktoi();
    if (err) {
      error(true);  // display direct mode error message
      continue;
    }

    // If the intermediate code is a program line
    if (*ibuf == I_NUM) { // the beginning of the code buffer is a line number
      *ibuf = len;        // overwrite the token with the length
      inslist();          // Insert one line of intermediate code into the list
      if (err)
	error();          // display program mode error message
      continue;
    }

    // If the intermediate code is a direct mode command
    if (icom())		// execute
      error(false);	// display direct mode error message
  }
}

// システム環境設定のロード
void loadConfig() {
  CONFIG.NTSC      =  0;
  CONFIG.KEYBOARD  =  1;
  CONFIG.STARTPRG  = -1;
  
  Unifile f = Unifile::open("/flash/.config", FILE_READ);
  if (!f)
    return;
  f.read((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}

// システム環境設定の保存
void isaveconfig() {
  Unifile f = Unifile::open("/flash/.config", FILE_OVERWRITE);
  if (!f) {
    err = ERR_FILE_OPEN;
  }
  f.write((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}
