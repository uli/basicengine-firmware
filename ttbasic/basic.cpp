/*
   TOYOSHIKI Tiny BASIC for Arduino
   (C)2012 Tetsuya Suzuki
   GNU General Public License
   2017/03/22, Modified by Tamakichi、for Arduino STM32
 */

// 2017/07/25 豊四季Tiny BASIC for Arduino STM32 V0.84 NTSC・ターミナル統合バージョン
// 2017/07/26 変数名を英字1文字+数字0～9の2桁対応:例 X0,X1 ... X9
// 2017/07/31 putnum()の値が-32768の場合の不具合対応(オリジナル版の不具合)
// 2017/07/31 toktoi()の定数の変換仕様の変更(-32768はオーバーフローエラーとしない)
// 2017/07/31 SDカードからテキスト形式プログラムロード時の中間コード変換不具合の対応(loadPrgText)
// 2017/08/03 比較演算子 "<>"の追加("!="と同じ)
// 2017/08/04 RENUMで振り直しする行範囲を指定可能機能追加、行番号0,1は除外に修正
// 2017/08/04 LOADコマンドでSDカードからの読み込みで追記機能を追加
// 2017/08/12 RENUMに重大な不具合あり、V0.83版の機能に一時差し換え
// 2017/08/13 TFT(ILI9341)モジュールの暫定対応
// 2017/08/19 SAVE,ERASE,LOAD,LRUN,CONFIGコマンドのプログラム番号範囲チェックミス不具合対応
//

#include <Arduino.h>
#include <unistd.h>
#include <stdlib.h>
//#include <wirish.h>
#include "ttconfig.h"
#include "tTermscreen.h"
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
#define NUM_FONTS 2
static const uint8_t *fonts[] = {
  console_font_6x8,
  console_font_8x8,
  font6x8tt,
};

// **** スクリーン管理 *************
uint8_t workarea[VS23_MAX_X/MIN_FONT_SIZE_X * VS23_MAX_Y/MIN_FONT_SIZE_Y];
uint8_t scmode = USE_SCREEN_MODE;
tscreenBase* sc;
tTermscreen sc1;

#if USE_NTSC == 1 || USE_VS23 == 1
  #include "tTVscreen.h"
  tTVscreen   sc0; 
#endif

uint16_t tv_get_gwidth();
uint16_t tv_get_gheight();

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

// **** GPIOピンに関する定義 **********

// GPIOピンモードの設定
const uint8_t pinType[] = {
  OUTPUT_OPEN_DRAIN, OUTPUT, INPUT_PULLUP, /*INPUT_PULLDOWN, INPUT_ANALOG,*/ INPUT, /*PWM,*/
};

#define FNC_IN_OUT  1  // デジタルIN/OUT
#define FNC_PWM     2  // PWM
#define FNC_ANALOG  4  // アナログIN

#define IsPWM_PIN(N) IsUseablePin(N,FNC_PWM)
#define IsADC_PIN(N) IsUseablePin(N,FNC_ANALOG)
#define IsIO_PIN(N)  IsUseablePin(N,FNC_IN_OUT)

// ピン機能チェックテーブル
const uint8_t pinFunc[]  = {
  5,0,5,5,5,5,7,7,3,3,  //  0 -  9: PA0,PA1,PA2,PA3,PA4,PA5,PA6,PA7,PA8,PA9,
  3,0,0,1,1,1,7,7,1,1,  // 10 - 19: PA10,PA11,PA12,PA13,PA14,PA15,PB0,PB1,PB2,PB3,
  0,0,0,0,1,0,1,1,1,1,  // 20 - 29: PB4,PB5,PB6,PB7,PB8,PB9,PB10,PB11,PB12,PB13,
  1,0,1,0,0,            // 30 - 34: PB14,PB15,PC13,PC14,PC15,
};

// ピン利用可能チェック
inline uint8_t IsUseablePin(uint8_t pinno, uint8_t fnc) {
  return pinFunc[pinno] & fnc;
}

// Terminal control(文字の表示・入力は下記の3関数のみ利用)
#define c_getch( ) sc->get_ch()
#define c_kbhit( ) sc->isKeyIn()

// 文字の出力
inline void c_putch(uint8_t c, uint8_t devno) {
  if (devno == 0)
    sc->putch(c);
  else if (devno == 1)
    sc->Serial_write(c);
#if USE_NTSC == 1
  else if (devno == 2)
    ((tTVscreen*)sc)->gputch(c);
#endif
  else if (devno == 3)
    mem_putch(c);
#if USE_SD_CARD == 1
  else if (devno == 4)
    bfs.putch(c);
#endif
}

// 改行
void newline(uint8_t devno) {
  if (devno==0)
    sc->newLine();
  else if (devno == 1)
    sc->Serial_newLine();
#if USE_NTSC == 1
  else if (devno == 2)
    ((tTVscreen*)sc)->gputch('\n');
#endif
  else if (devno == 3)
    mem_putch('\n');
#if USE_SD_CARD == 1
  else if (devno == 4) {
    bfs.putch('\x0d');
    bfs.putch('\x0a');
  }
#endif
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
  I_ARRAY, I_RND, I_ABS, I_FREE, I_TICK, I_PEEK, I_I2CW, I_I2CR,
  I_OUTPUT_OPEN_DRAIN, I_OUTPUT, I_INPUT_PULLUP, I_INPUT_PULLDOWN, I_INPUT_ANALOG, I_INPUT_F, I_PWM,
  I_DIN, I_ANA, I_SHIFTIN, I_MAP, I_DMP,
  I_PA0, I_PA1, I_PA2, I_PA3, I_PA4, I_PA5, I_PA6, I_PA7, I_PA8,
  I_PA9, I_PA10, I_PA11, I_PA12, I_PA13,I_PA14,I_PA15,
  I_PB0, I_PB1, I_PB2, I_PB3, I_PB4, I_PB5, I_PB6, I_PB7, I_PB8,
  I_PB9, I_PB10, I_PB11, I_PB12, I_PB13,I_PB14,I_PB15,
  I_PC13, I_PC14,I_PC15,
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
  I_GETDATE,I_GETTIME,I_GOSUB,I_GOTO,I_GPIO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_SETDATE,I_SHIFTOUT,I_WAIT,I_EEPFORMAT, I_EEPWRITE,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BLIT, I_SWRITE, I_SPRINT,  I_SOPEN, I_SCLOSE,I_SMODE,
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

VarNames proc_names;
Procedures proc;

num_t arr[SIZE_ARRY];             // 配列領域
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
num_t retval[MAX_RETVALS];        // multi-value returns

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
char c_toupper(char c) {
  return(c <= 'z' && c >= 'a' ? c - 32 : c);
}
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
inline uint8_t getParam(num_t& prm, int32_t v_min,  int32_t v_max, token_t next_token) {
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
  return (*cip == I_STR ||
          *cip == I_SVAR ||
          *cip == I_ARGSTR ||
          *cip == I_CHR ||
          *cip == I_HEX ||
          *cip == I_BIN ||
          *cip == I_DMP ||
          *cip == I_LEFTSTR ||
          *cip == I_RIGHTSTR ||
          *cip == I_MIDSTR
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
      sc->movePosPrevChar();
      sc->delete_char();
    } else
    //行頭の符号および数字が入力された場合の処理（符号込みで6桁を超えないこと）
    if (len < SIZE_LINE - 1 && (!numeric || c == '.' ||
        (len == 0 && (c == '+' || c == '-')) || isDigit(c)) ) {
      lbuf[len++] = c; //バッファへ入れて文字数を1増やす
      c_putch(c); //表示
    } else {
      sc->beep();
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
int16_t lookup(char* str) {
  for (int i = 0; i < SIZE_KWTBL; ++i) {
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

int getlineno(unsigned char *lp);

//
// テキストを中間コードに変換
// [戻り値]
//   0 または 変換中間コードバイト数
//
uint8_t SMALL toktoi() {
  int16_t i;
  int16_t key;
  uint8_t len = 0;  // 中間コードの並びの長さ
  char* pkw = 0;          // ひとつのキーワードの内部を指すポインタ
  char* ptok;             // ひとつの単語の内部を指すポインタ
  char* s = lbuf;         // 文字列バッファの内部を指すポインタ
  char c;                 // 文字列の括りに使われている文字（「"」または「'」）
  num_t value;            // 定数
  uint32_t tmp;              // 変換過程の定数
  uint32_t hex;           // 16進数定数
  uint16_t hcnt;          // 16進数桁数
  uint8_t var_len;        // 変数名長さ
  char vname[MAX_VAR_NAME];       // 変数名

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
    
    if (key >= 0)                           // もしすでにキーワードで変換に成功していたら以降はスキップ
      continue;

    //定数への変換を試みる
    ptok = s;                            // 単語の先頭を指す
    if (isDigit(*ptok)) {              // もし文字が数字なら
      if (len >= SIZE_IBUF - 3) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF; //エラー番号をセット
	return 0; //0を持ち帰る
      }
      value = strtonum(ptok, &ptok);
      s = ptok; //文字列の処理ずみの部分を詰める
      ibuf[len++] = I_NUM; //中間コードを記録
      os_memcpy(ibuf+len, &value, sizeof(num_t));
      len += sizeof(num_t);
      is_prg_text = true;
    }
    else

    //文字列への変換を試みる
    if (*s == '\"' ) { //もし文字が '\"'
      c = *s++; //「"」か「'」を記憶して次の文字へ進む
      ptok = s; //文字列の先頭を指す
      //文字列の文字数を得る
      for (i = 0; (*ptok != c) && c_isprint(*ptok); i++)
	ptok++;
      if (len >= SIZE_IBUF - 1 - i) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF; //エラー番号をセット
	return 0; //0を持ち帰る
      }
      ibuf[len++] = I_STR; //中間コードを記録
      ibuf[len++] = i; //文字列の文字数を記録
      while (i--) { //文字列の文字数だけ繰り返す
	ibuf[len++] = *s++; //文字列を記録
      }
      if (*s == c) s++;  //もし文字が「"」か「'」なら次の文字へ進む
    }
    else

    //変数への変換を試みる(2017/07/26 A～Z9:対応)
    //  1文字目
    if (isAlpha(*ptok)) { //もし文字がアルファベットなら
      var_len = 0;
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
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
      if (*p == '$') {
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
	//もし変数が3個並んだら
	if (len >= 4 && ibuf[len - 2] == I_VAR && ibuf[len - 4] == I_VAR) {
	  err = ERR_SYNTAX;  //エラー番号をセット
	  return 0;  //0を持ち帰る
	}

	// 中間コードに変換
	ibuf[len++] = I_VAR; //中間コードを記録
	int idx = var_names.assign(vname, is_prg_text);
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if (var.reserve(var_names.varTop()))
	  goto oom;
	s+=var_len; //次の文字へ進む
      }
    } else { //どれにも当てはまらなかった場合
      err = ERR_SYNTAX; //エラー番号をセット
      return 0; //0を持ち帰る
    }
  } //文字列1行分の終端まで繰り返すの末尾

  ibuf[len++] = I_EOL; //文字列1行分の終端を記録
  return len; //中間コードの長さを持ち帰る
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
int getlineno(unsigned char *lp) {
  num_t l;
  if(*lp == 0) //もし末尾だったら
    return -1;
  os_memcpy(&l, lp+1, sizeof(num_t));
  return l;
}

// Search line by line number
unsigned char* getlp(int lineno) {
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
uint32_t countLines(int32_t st=0, int32_t ed=INT32_MAX) {
  unsigned char *lp; //ポインタ
  uint32_t cnt = 0;
  uint32_t lineno;
  for (lp = listbuf; *lp; lp += *lp)  {
    lineno = getlineno(lp);
    if (lineno < 0)
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
    }
    else

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
      if (*ip == I_VAR || *ip ==I_ELSE) //もし次の中間コードが変数だったら
	c_putch(' ',devno);  //空白を表示
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
  num_t value;          // 値
  BString str_value;
  short index;          // 配列の添え字or変数番号
  unsigned char i;      // 文字数
  unsigned char prompt; // プロンプト表示フラグ
  num_t ofvalue;        // オーバーフロー時の設定値
  uint8_t flgofset =0;  // オーバーフロ時の設定値指定あり

  sc->show_curs(1);
  for(;; ) {           // 無限に繰り返す
    prompt = 1;       // まだプロンプトを表示していない

    // プロンプトが指定された場合の処理
    if(*cip == I_STR) {   // もし中間コードが文字列なら
      cip++;             // 中間コードポインタを次へ進める
      i = *cip++;        // 文字数を取得
      while (i--)        // 文字数だけ繰り返す
	c_putch(*cip++);  // 文字を表示
      prompt = 0;        // プロンプトを表示した

      if (*cip != I_COMMA) {
	err = ERR_SYNTAX;
	goto DONE;
	//return;
      }
      cip++;
    }

    // 値を入力する処理
    switch (*cip++) {         // 中間コードで分岐
    case I_VAR:             // 変数の場合
    case I_VARARR:
      index = *cip;         // 変数番号の取得

      if (cip[-1] == I_VARARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
        value = num_arr.var(i).var(idxs);
      }
 
      cip++;

      // Setting value at overflow
      // XXX: this is weird syntax; how does this look like in other BASICs?
      // XXX: do we need it at all?
      if (*cip == I_COMMA) {
	cip++;
	ofvalue = iexp();
	if (err) {
	  goto DONE;
	}
	flgofset = 1;
      }

      // XXX: idiosyncrasy, never seen this in other BASICs
      if (prompt) {          // If you are not prompted yet
        if (dims)
          c_puts(num_arr_names.name(index));
        else
	  c_puts(var_names.name(index));
	c_putch(':');
      }

      value = getnum();
      if (err) {            // もしエラーが生じたら
	if (err == ERR_VOF && flgofset) {
	  err = ERR_OK;
	  value = ofvalue;
	} else {
	  return;            // 終了
	}
      }

      if (dims)
        num_arr.var(index).var(idxs) = value;
      else
        var.var(index) = value;
      break;               // 打ち切る

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

    case I_ARRAY: // 配列の場合
      index = getparam();       // 配列の添え字を取得
      if (err)                  // もしエラーが生じたら
	goto DONE;
      //return;                 // 終了

      if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
	err = ERR_SOR;          // エラー番号をセット
	goto DONE;
	//return;                 // 終了
      }

      // オーバーフロー時の設定値
      if (*cip == I_COMMA) {
	cip++;
	ofvalue = iexp();
	if (err) {
	  //return;
	  goto DONE;
	}
	flgofset = 1;
      }

      if (prompt) { // もしまだプロンプトを表示していなければ
	c_puts("@(");     //「@(」を表示
	putnum(index, 0); // 添え字を表示
	c_puts("):");     //「):」を表示
      }
      value = getnum(); // 値を入力
      if (err) {           // もしエラーが生じたら
	if (err == ERR_VOF && flgofset) {
	  err = ERR_OK;
	  value = ofvalue;
	} else {
	  goto DONE;
	  //return;            // 終了
	}
      }
      arr[index] = value; //配列へ代入
      break;              // 打ち切る

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
  sc->show_curs(0);
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
  int len;
  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;
  if (*cip == I_STR) {
    cip++;
    len = svar.var(index).fromBasic(cip);
    if (!len)
      err = ERR_OOM;
    else
      cip += len;
  } else {
    value = istrexp();
    if (err)
      return;
    svar.var(index) = value;
  }
}

// Array assignment handler
void iarray() {
  int value; //値
  short index; //配列の添え字

  index = getparam(); //配列の添え字を取得
  if (err) //もしエラーが生じたら
    return;  //終了

  if (index >= SIZE_ARRY || index < 0 ) { //もし添え字が上下限を超えたら
    err = ERR_SOR; //エラー番号をセット
    return; //終了
  }

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return; //終了
  }

  // 例: @(n)=1,2,3,4,5 の連続設定処理
  do {
    cip++;
    value = iexp(); // 式の値を取得
    if (err)        // もしエラーが生じたら
      return;       // 終了
    if (index >= SIZE_ARRY) { // もし添え字が上限を超えたら
      err = ERR_SOR;          // エラー番号をセット
      return;
    }
    arr[index] = value; //配列へ代入
    index++;
  } while(*cip == I_COMMA);
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
  unsigned char tok;
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
  unsigned char tok;
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

  case I_ARRAY: // 配列の場合
    cip++;      // 中間コードポインタを次へ進める
    iarray();   // 配列への代入を実行
    break;

  default:      // 以上のいずれにも該当しなかった場合
    err = ERR_LETWOV; // エラー番号をセット
    break;            // 打ち切る
  }
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

// LISTコマンド
void SMALL ilist(uint8_t devno=0) {
  int32_t lineno = 0;          // 表示開始行番号
  int32_t endlineno = INT32_MAX;   // 表示終了行番号
  int32_t prnlineno;           // 出力対象行番号

  //表示開始行番号の設定
  if (*cip != I_EOL && *cip != I_COLON) {
    // 引数あり
    if (getParam(lineno,0,INT32_MAX, I_NONE)) return;
    if (*cip == I_COMMA) {
      cip++;                         // カンマをスキップ
      if (getParam(endlineno,lineno,INT32_MAX, I_NONE)) return;
    }
  }

  //行ポインタを表示開始行番号へ進める
  for ( clp = listbuf; *clp && (getlineno(clp) < lineno); clp += *clp) ;

  //リストを表示する
  while (*clp) {               // 行ポインタが末尾を指すまで繰り返す
    prnlineno = getlineno(clp); // 行番号取得
    if (prnlineno > endlineno) // 表示終了行番号に達したら抜ける
      break;
    putnum(prnlineno, 0,devno); // 行番号を表示
    c_putch(' ',devno);        // 空白を入れる
    putlist(clp + sizeof(num_t) + 1,devno);    // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    newline(devno);            // 改行
    clp += *clp;               // 行ポインタを次の行へ進める
  }
}

// Argument 0: all erase, 1: erase only program, 2: erase variable area only
void inew(uint8_t mode) {
  int i; //ループカウンタ

  data_ip = data_lp = NULL;

  if (mode != 1) {
    var.reset();
    svar.reset();
    num_arr.reset();
  }

  //変数と配列の初期化
  if (mode == 0|| mode == 2) {
    // forget variables assigned in direct mode
    for (i = 0; i < SIZE_ARRY; i++) //配列の数だけ繰り返す
      arr[i] = 0;  //配列を0に初期化

    // XXX: These reserve() calls always downsize (or same-size) the
    // variable pools. Can they fail doing so?
    svar_names.deleteDirect();
    svar.reserve(svar_names.varTop());
    var_names.deleteDirect();
    var.reserve(var_names.varTop());
    num_arr_names.deleteDirect();
    num_arr.reserve(num_arr_names.varTop());
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
        num = token_size(ptr+i);
        if (num < 0)
          i = len + 1;	// skip rest of line
        else
          i += num;	// next token
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
      ((tTVscreen*)sc)->adjustNTSC(value);
      CONFIG.NTSC = value;
    }
    break;
  case 1: // キーボード補正
    if (value <0 || value >1)  {
      err = ERR_VALUE;
    } else {
      ((tTVscreen*)sc)->reset_kbd(value);
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

// プログラム保存 SAVE 保存領域番号|"ファイル名"
void isave() {
  int16_t prgno = 0;
  int32_t ascii = 1;
  uint8_t* sram_adr;
  //char fname[64];
  BString fname;
  uint8_t mode = 0;
  int8_t rc;

  if (*cip == I_BG) {
    isavebg();
    return;
  }

  if (*cip == I_EOL) {
    prgno = 0;
  } else
  // ファイル名またはプログラム番号の取得
  if (is_strexp()) {
    if(!(fname = getParamFname())) {
      return;
    }
    mode = 1;
  } //else if ( getParam(prgno, 0, FLASH_SAVE_NUM-1, I_NONE) ) return;
  if (mode == 1) {
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
  } else {
    err = ERR_NOT_SUPPORTED;
  }
}

// フラッシュメモリ上のプログラム消去 ERASE[プログラム番号[,プログラム番号]
void ierase() {
#if 0
  int16_t s_prgno, e_prgno;
  uint8_t* sram_adr;
  uint32_t flash_adr;

  if ( getParam(s_prgno, 0, FLASH_SAVE_NUM-1, I_NONE) ) return;
  e_prgno = s_prgno;
  if (*cip == I_COMMA) {
    cip++;
    if ( getParam(e_prgno, 0, FLASH_SAVE_NUM-1, I_NONE) ) return;
  }

  TFlash.unlock();
  for (uint8_t prgno = s_prgno; prgno <= e_prgno; prgno++) {
    for (uint8_t i=0; i < FLASH_PAGE_PAR_PRG; i++) {
      flash_adr = FLASH_START_ADDRESS + FLASH_PAGE_SIZE*(FLASH_PRG_START_PAGE+ prgno*FLASH_PAGE_PAR_PRG+i);
      TFlash.eracePage(flash_adr);
    }
  }
  TFlash.lock();
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
  int32_t sNo;
  int32_t eNo;
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
  uint32_t flash_adr;
  uint8_t* save_clp;
  BString fname;
  char wildcard[SD_PATH_LEN];
  char* wcard = NULL;
  char* ptr = NULL;
  uint8_t flgwildcard = 0;
  int16_t rc;

  if (!is_strexp())
    fname = "*";
  else
    fname = getParamFname();

  for (int8_t i = 0; i < fname.length(); i++) {
    if (fname[i] >='a' && fname[i] <= 'z') {
      fname[i] = fname[i] - 'a' + 'A';
    }
  }

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
      fname = "/";
    }
  }
#if USE_SD_CARD == 1
  rc = bfs.flist((char *)fname.c_str(), wcard, sc->getWidth()/14);
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
  static const char success[] PROGMEM = "Success!\n";
  static const char failed[] PROGMEM = "Failed\n";

  BString target = getParamFname();
  if (err)
    return;

  if (target == "f:" || target == "F:") {
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
  sc->cls();
  sc->locate(0,0);
}

#include "sound.h"

void ICACHE_RAM_ATTR pump_events(void)
{
  vs23.updateBg();
  sound_pump_events();
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
  if ( x >= sc->getWidth() )   // xの有効範囲チェック
    x = sc->getWidth() - 1;
  else if (x < 0) x = 0;
  if( y >= sc->getHeight() )   // yの有効範囲チェック
    y = sc->getHeight() - 1;
  else if(y < 0) y = 0;

  // カーソル移動
  sc->locate((uint16_t)x, (uint16_t)y);
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
  sc->setColor((uint16_t)fc, (uint16_t)bc);
}

// 文字属性の指定 ATTRコマンド
void iattr() {
  int32_t attr;
  if ( getParam(attr, 0, 4, I_NONE) ) return;
  sc->setAttr(attr);
}

// キー入力文字コードの取得 INKEY()関数
int32_t iinkey() {
  int32_t rc = 0;

  if (prevPressKey) {
    // 一時バッファに入力済キーがあればそれを使う
    rc = prevPressKey;
    prevPressKey = 0;
  } else if (c_kbhit( )) {
    // キー入力
    rc = c_getch();
  }
  return rc;
}

// メモリ参照　PEEK(adr[,bnk])
int32_t ipeek() {
  int32_t value, vadr;
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
  value = (x < 0 || y < 0 || x >=sc->getWidth() || y >=sc->getHeight()) ? 0 : sc->vpeek(x, y);
  return value;
}

// ピンモード設定(タイマー操作回避版)
void Fixed_pinMode(uint8 pin, uint8_t mode) {
#if 0
  gpio_pin_mode outputMode;
  bool pwm = false;

  if (pin >= BOARD_NR_GPIO_PINS) {
    return;
  }

  switch(mode) {
  case OUTPUT:
    outputMode = GPIO_OUTPUT_PP;
    break;
  case OUTPUT_OPEN_DRAIN:
    outputMode = GPIO_OUTPUT_OD;
    break;
  case INPUT:
  case INPUT_FLOATING:
    outputMode = GPIO_INPUT_FLOATING;
    break;
  case INPUT_ANALOG:
    outputMode = GPIO_INPUT_ANALOG;
    break;
  case INPUT_PULLUP:
    outputMode = GPIO_INPUT_PU;
    break;
//    case INPUT_PULLDOWN:
//        outputMode = GPIO_INPUT_PD;
//        break;
//    case PWM:
//        outputMode = GPIO_AF_OUTPUT_PP;
//        pwm = true;
//        break;
  case PWM_OPEN_DRAIN:
    outputMode = GPIO_AF_OUTPUT_OD;
    pwm = true;
    break;
  default:
    return;
  }
  gpio_set_mode(PIN_MAP[pin].gpio_device, PIN_MAP[pin].gpio_bit, outputMode);

  if (PIN_MAP[pin].timer_device != NULL) {
    if ( pwm ) {     // we're switching into PWM, enable timer channels
      timer_set_mode(PIN_MAP[pin].timer_device,
                     PIN_MAP[pin].timer_channel,
                     TIMER_PWM );
    } else {      // disable channel output in non pwm-Mode
      timer_cc_disable(PIN_MAP[pin].timer_device,
                       PIN_MAP[pin].timer_channel);
    }
  }
#else
  pinMode(pin, mode);
#endif
}

// GPIO ピン機能設定
void igpio() {
#if 0
  int32_t pinno;       // ピン番号
  WiringPinMode pmode; // 入出力モード
  uint8_t flgok = false;

  // 入出力ピンの指定
  if ( getParam(pinno, 0, I_PC15-I_PA0, I_COMMA) ) return;  // ピン番号取得
  pmode = (WiringPinMode)iexp();  if(err) return;       // 入出力モードの取得

  // ピンモードの設定
  if (pmode == PWM) {
    // PWMピンとして利用可能かチェック
    if (!IsPWM_PIN(pinno)) {
      err = ERR_GPIO;
      return;
    }

    Fixed_pinMode(pinno, pmode);
    pwmWrite(pinno,0);
  } else if (pmode == INPUT_ANALOG ) {
    // アナログ入力として利用可能かチェック
    if (!IsADC_PIN(pinno)) {
      err = ERR_GPIO;
      return;
    }
    Fixed_pinMode(pinno, pmode);
  } else {
    // デジタル入出力として利用可能かチェック
    if (!IsIO_PIN(pinno)) {
      err = ERR_GPIO;
      return;
    }
    Fixed_pinMode(pinno, pmode);
  }
#endif
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

// shiftOutコマンド SHIFTOUT dataPin, clockPin, bitOrder, value
void ishiftOut() {
  int32_t dataPin, clockPin;
  int32_t bitOrder;
  int32_t data;

  if (getParam(dataPin, 0,I_PC15-I_PA0, I_COMMA)) return;
  if (getParam(clockPin,0,I_PC15-I_PA0, I_COMMA)) return;
  if (getParam(bitOrder,0,1, I_COMMA)) return;
  if (getParam(data, 0,255, I_NONE)) return;

  if ( !IsIO_PIN(dataPin) ||  !IsIO_PIN(clockPin) ) {
    err = ERR_GPIO;
    return;
  }

  shiftOut(dataPin, clockPin, bitOrder, data);
}

// 16進文字出力 'HEX$(数値,桁数)' or 'HEX$(数値)'
void ihex(uint8_t devno=0) {
  int value; // 値
  int d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value, I_NONE)) return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(d,0,4, I_NONE)) return;
  }
  if (checkClose()) return;
  putHexnum(value, d, devno);
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
void ibin(uint8_t devno=0) {
  int32_t value; // 値
  int32_t d = 0; // 桁数(0で桁数指定なし)

  if (checkOpen()) return;
  if (getParam(value, I_NONE)) return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(d,0,16, I_NONE)) return;
  }
  if (checkClose()) return;
  putBinnum(value, d, devno);
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

  for (uint32_t i=0; i<n; i++) {
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
  int32_t i2cAdr, ctop, clen, top, len;
  uint8_t* ptr;
  uint8_t* cptr;
  int16_t rc;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA)) return 0;
  if (getParam(ctop, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(clen, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(top, 0, INT32_MAX, I_COMMA)) return 0;
  if (getParam(len, 0, INT32_MAX, I_NONE)) return 0;
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

uint8_t _shiftIn( uint8_t ulDataPin, uint8_t ulClockPin, uint8_t ulBitOrder, uint8_t lgc){
  uint8_t value = 0;
  uint8_t i;
  for ( i=0; i < 8; ++i ) {
    digitalWrite( ulClockPin, lgc );
    if ( ulBitOrder == LSBFIRST ) value |= digitalRead( ulDataPin ) << i;
    else value |= digitalRead( ulDataPin ) << (7 - i);
    digitalWrite( ulClockPin, !lgc );
  }
  return value;
}

// SHIFTIN関数 SHIFTIN(データピン, クロックピン, オーダ[,ロジック])
int32_t ishiftIn() {
  int16_t rc;
  int32_t dataPin, clockPin;
  int32_t bitOrder;
  int32_t lgc = HIGH;

  if (checkOpen()) return 0;
  if (getParam(dataPin, 0,I_PC15-I_PA0, I_COMMA)) return 0;
  if (getParam(clockPin,0,I_PC15-I_PA0, I_COMMA)) return 0;
  if (getParam(bitOrder,0,1, I_NONE)) return 0;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(lgc,LOW, HIGH, I_NONE)) return 0;
  }
  if (checkClose()) return 0;
  rc = _shiftIn((uint8_t)dataPin, (uint8_t)clockPin, (uint8_t)bitOrder, lgc);
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
    } else if (*cip == I_ARRAY) { // 配列の場合
      cip++;
      index = getparam();         // 添え字の取得
      if (err) return;
      if (index >= SIZE_ARRY || index < 0 ) {
	err = ERR_SOR;
	return;
      }
      arr[index] = v[i];          // 配列に格納
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
    } else if (*cip == I_ARRAY) { // 配列の場合
      cip++;
      index = getparam();         // 添え字の取得
      if (err) return;
      if (index >= SIZE_ARRY || index < 0 ) {
	err = ERR_SOR;
	return;
      }
      arr[index] = v[i];          // 配列に格納
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
#if USE_NTSC == 1
  int32_t x,y,c;
  if (scmode) {
    if (getParam(x, I_COMMA)||getParam(y, I_COMMA)||getParam(c, I_NONE))
      if (x < 0) x =0;
    if (y < 0) y =0;
    if (x >= ((tTVscreen*)sc)->getGWidth()) x = ((tTVscreen*)sc)->getGWidth()-1;
    if (y >= ((tTVscreen*)sc)->getGHeight()) y = ((tTVscreen*)sc)->getGHeight()-1;
    if (c < 0 || c > 2) c = 1;
    ((tTVscreen*)sc)->pset(x,y,c);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// 直線の描画 LINE X1,Y1,X2,Y2,C
void iline() {
#if USE_NTSC == 1
  int32_t x1,x2,y1,y2,c;
  if (scmode) {
    if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(c, I_NONE))
      if (x1 < 0) x1 =0;
    if (y1 < 0) y1 =0;
    if (x2 < 0) x1 =0;
    if (y2 < 0) y1 =0;
    if (x1 >= ((tTVscreen*)sc)->getGWidth()) x1 = ((tTVscreen*)sc)->getGWidth()-1;
    if (y1 >= ((tTVscreen*)sc)->getGHeight()) y1 = ((tTVscreen*)sc)->getGHeight()-1;
    if (x2 >= ((tTVscreen*)sc)->getGWidth()) x2 = ((tTVscreen*)sc)->getGWidth()-1;
    if (y2 >= ((tTVscreen*)sc)->getGHeight()) y2 = ((tTVscreen*)sc)->getGHeight()-1;
    if (c < 0 || c > 2) c = 1;
    ((tTVscreen*)sc)->line(x1, y1, x2, y2, c);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// 円の描画 CIRCLE X,Y,R,C,F
void icircle() {
#if USE_NTSC == 1
  int32_t x,y,r,c,f;
  if (scmode) {
    if (getParam(x, I_COMMA)||getParam(y, I_COMMA)||getParam(r, I_COMMA)||getParam(c, I_COMMA)||getParam(f, I_NONE))
      if (x < 0) x =0;
    if (y < 0) y =0;
    if (x >= ((tTVscreen*)sc)->getGWidth()) x = ((tTVscreen*)sc)->getGWidth()-1;
    if (y >= ((tTVscreen*)sc)->getGHeight()) y = ((tTVscreen*)sc)->getGHeight()-1;
    if (c < 0 || c > 2) c = 1;
    if (r < 0) r = 1;
    ((tTVscreen*)sc)->circle(x, y, r, c, f);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// 四角の描画 RECT X1,Y1,X2,Y2,C,F
void irect() {
#if USE_NTSC == 1
  int32_t x1,y1,x2,y2,c,f;
  if (scmode) {
    if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(c, I_COMMA)||getParam(f, I_NONE))
      return;
    if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= ((tTVscreen*)sc)->getGWidth() || y2 >= ((tTVscreen*)sc)->getGHeight())  {
      err = ERR_VALUE;
      return;
    }
    if (c < 0 || c > 2) c = 1;
    ((tTVscreen*)sc)->rect(x1, y1, x2-x1+1, y2-y1+1, c, f);
  }else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
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
    if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc->getWidth() || y2 >= sc->getHeight())  {
      err = ERR_VALUE;
      return;
    }
    if (d < 0 || d > 3) d = 0;
    ((tTVscreen*)sc)->cscroll(x1, y1, x2-x1+1, y2-y1+1, d);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// グラフィックスクロール GSCROLL X1,Y1,X2,Y2,方向
void igscroll() {
#if USE_NTSC == 1
  int32_t x1,y1,x2,y2,d;
  if (scmode) {
    if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(d, I_NONE))
      return;
    if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= ((tTVscreen*)sc)->getGWidth() || y2 >= ((tTVscreen*)sc)->getGHeight()) {
      err = ERR_VALUE;
      return;
    }
    if (d < 0 || d > 3) d = 0;
    ((tTVscreen*)sc)->gscroll(x1,y1,x2-x1+1, y2-y1+1, d);
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// シリアル1バイト出力 : SWRITE データ
void iswrite() {
  int32_t c;
  if ( getParam(c, I_NONE) ) return;
  sc->Serial_write((uint8_t)c);
}

// シリアルモード設定: SMODE MODE [,"通信速度"]
void SMALL ismode() {
  int32_t c,flg;
  uint16_t ln;
  uint32_t baud = 0;

  if ( getParam(c, 0, 3, I_NONE) ) return;
  if (c == 1) {
    if(*cip != I_COMMA) {
      err = ERR_SYNTAX;
      return;
    }
    cip++;
    if (*cip != I_STR) {
      err = ERR_VALUE;
      return;
    }

    cip++;        //中間コードポインタを次へ進める
    ln = *cip++;  //文字数を取得

    // 引数のチェック
    for (uint32_t i=0; i < ln; i++) {
      if (*cip >='0' && *cip <= '9') {
	baud = baud*10 + *cip - '0';
      } else {
	err = ERR_VALUE;
	return;
      }
      cip++;
    }
  }
  else if (c == 3) {
    // シリアルからの制御入力許可設定
    cip++;
    if ( getParam(flg, 0, 1, I_NONE) ) return;
    sc->set_allowCtrl(flg);
    return;
  }
  sc->Serial_mode((uint8_t)c, baud);
}

// シリアル1クローズ
void isclose() {
  delay(500);
  if(lfgSerial1Opened == true)
    //Serial1.end();
    sc->Serial_close();
  lfgSerial1Opened = false;
}

// シリアル1オープン
void SMALL isopen() {
  uint16_t ln;
  uint32_t baud = 0;

  if(lfgSerial1Opened) {
    isclose();
  }

  if (*cip != I_STR) {
    err = ERR_VALUE;
    return;
  }

  cip++;        //中間コードポインタを次へ進める
  ln = *cip++;  //文字数を取得

  // 引数のチェック
  for (uint32_t i=0; i < ln; i++) {
    if (*cip >='0' && *cip <= '9') {
      baud = baud*10 + *cip - '0';
    } else {
      err = ERR_VALUE;
      return;
    }
    cip++;
  }
  sc->Serial_open(baud);
  lfgSerial1Opened = true;
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
void imusic() {
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
  int value; // 値
  int x, y;  // 座標
  if (scmode) {
    if (checkOpen()) return 0;
    if ( getParam(x, I_COMMA) || getParam(y, I_NONE) ) return 0;
    if (checkClose()) return 0;
    if (x < 0 || y < 0 || x >= ((tTVscreen*)sc)->getGWidth()-1 || y >= ((tTVscreen*)sc)->getGHeight()-1) return 0;
    return ((tTVscreen*)sc)->gpeek(x,y);
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
    if (x < 0 || y < 0 || x >= ((tTVscreen*)sc)->getGWidth() || y >= ((tTVscreen*)sc)->getGHeight() || h < 0 || w < 0) return 0;
    if (x+w >= ((tTVscreen*)sc)->getGWidth() || y+h >= ((tTVscreen*)sc)->getGHeight() ) return 0;
    return ((tTVscreen*)sc)->ginp(x, y, w, h, c);
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
// ASC(文字列,文字位置)
// ASC(変数,文字位置)
int32_t iasc() {
  int32_t value =0;
  int32_t len;     // 文字列長
  int32_t pos = 0;  // 文字位置
  BString str;    // 文字列先頭位置

  if (checkOpen()) return 0;

  str = istrexp();

  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(pos, 0, str.length() - 1, I_NONE)) return 0;
  }

  if (pos < str.length()) {
    value = str[pos];
  } else {
    err = ERR_RANGE;
  }

  checkClose();

  return value;
}

// PRINT handler
void iprint(uint8_t devno=0,uint8_t nonewln=0) {
  num_t value;     //値
  int len;       //桁数
  unsigned char i; //文字数
  BString str;

  len = 0; //桁数を初期化
  while (*cip != I_COLON && *cip != I_EOL) { //文末まで繰り返す
    if (is_strexp()) {
      str = istrexp();
      c_puts(str.c_str(), devno);
    } else  switch (*cip) { //中間コードで分岐
    case I_SHARP: //「#
      cip++;
      len = iexp(); //桁数を取得
      if (err)
	return;
      break;

    case I_CHR: // CHR$()関数
      cip++;
      if (getParam(value, 0,255, I_NONE)) break;   // 括弧の値を取得
//     if (!err)
      c_putch(value, devno);
      break;

    case I_HEX:  cip++; ihex(devno); break; // HEX$()関数
    case I_BIN:  cip++; ibin(devno); break; // BIN$()関数
    case I_DMP:  cip++; idmp(devno); break; // DMP$()関数
    case I_ELSE:        // ELSE文がある場合は打ち切る
      newline(devno);
      return;
      break;

    default: //以上のいずれにも該当しなかった場合（式とみなす）
      value = iexp();   // 値を取得
      if (err) {
	newline();
	return;
      }
      putnum(value, len,devno); // 値を表示
      break;
    } //中間コードで分岐の末尾
    if (err)  {
      newline(devno);
      return;
    }
    if (nonewln && *cip == I_COMMA) // 文字列引数流用時はここで終了
      return;

    if (*cip == I_ELSE) {
      newline(devno);
      return;
    } else if (*cip == I_COMMA|*cip == I_SEMI) { // もし',' ';'があったら
      cip++;
      if (*cip == I_COLON || *cip == I_EOL || *cip == I_ELSE) //もし文末なら
	return;
    } else {    //',' ';'がなければ
      if (*cip != I_COLON && *cip != I_EOL) { //もし文末でなければ
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
    if ( getParam(x, 0, ((tTVscreen*)sc)->getGWidth(), I_COMMA) ) return;
    if ( getParam(y, 0, ((tTVscreen*)sc)->getGHeight(), I_COMMA) ) return;
    ((tTVscreen*)sc)->set_gcursor(x,y);
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
  uint8_t rc;

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
  if (x + w > ((tTVscreen*)sc)->getGWidth() || y + h > vs23.lastLine()) {
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

// RENAME "現在のファイル名","新しいファイル名"
void irename() {
  char old_fname[SD_PATH_LEN];
  char new_fname[SD_PATH_LEN];
  uint8_t rc;

  if (*cip != I_STR) {
    err = ERR_SYNTAX;
    return;
  }

  cip++;

  // ファイル名指定
  if (*cip >= SD_PATH_LEN) {
    err = ERR_LONGPATH;
    return;
  }

  // 現在のファイル名の取得
  strncpy(old_fname, (char*)(cip+1), *cip);
  old_fname[*cip]=0;
  cip+=*cip;
  cip++;

  if (*cip != I_COMMA) {
    err = ERR_SYNTAX;
    return;
  }
  cip++;
  if (*cip!= I_STR) {
    err = ERR_SYNTAX;
    return;
  }

  cip++;

  // ファイル名指定
  if (*cip >= SD_PATH_LEN) {
    err = ERR_LONGPATH;
    return;
  }

  // 新しいのファイル名の取得
  strncpy(new_fname, (char*)(cip+1), *cip);
  new_fname[*cip]=0;
  cip+=*cip;
  cip++;

  rc = bfs.rename(old_fname,new_fname);
  if (rc) {
    err = ERR_FILE_WRITE;
    return;
  }
}

// REMOVE "ファイル名"
void iremove() {
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

#if USE_SD_CARD == 1
  rc = bfs.remove((char *)fname.c_str());
  if (rc) {
    err = ERR_FILE_WRITE;
    return;
  }
#endif
}

// BSAVE "ファイル名", アドレス
void SMALL ibsave() {
  //char fname[SD_PATH_LEN];
  uint8_t*radr;
  int32_t vadr, len;
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
  if ( getParam(vadr,  0, UINT32_MAX, I_COMMA) ) return;  // アドレスの取得
  if ( getParam(len,  0, INT32_MAX, I_NONE) ) return;             // データ長の取得

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
  int32_t vadr, len,c;
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
  if ( getParam(vadr,  0, UINT32_MAX, I_COMMA) ) return;  // アドレスの取得
  if ( getParam(len,  0, INT32_MAX, I_NONE) ) return;              // データ長の取得

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
    rc = bfs.textOut((char *)fname.c_str(), line, sc->getHeight());
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
    line += sc->getHeight();
  }
#endif
}

// ターミナルスクリーンの画面サイズ指定 WINDOW X,Y,W,H
void iwindow() {
  int32_t x, y, w, h;

  if ( getParam(x,  0, sc0.getScreenWidth() - 8, I_COMMA) ) return;   // x
  if ( getParam(y,  0, sc0.getScreenHeight() - 2, I_COMMA) ) return;        // y
  if ( getParam(w,  8, sc0.getScreenWidth() - x, I_COMMA) ) return;   // w
  if ( getParam(h,  2, sc0.getScreenHeight() - y, I_NONE) ) return;        // h

  if (scmode == 0) {
    // 現在、ターミナルモードの場合は画面をクリアして、再設定する
    sc->cls();
    sc->locate(0,0);
    sc->end();
    sc->init(w, h, SIZE_LINE, workarea); // スクリーン初期設定
  } else if (scmode > 0) {
    sc0.setWindow(x, y, w, h);
    sc->locate(0,0);
    sc->init(w, h, SIZE_LINE, workarea);
    sc->cls();
  }
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
  int8_t prv_m;
#if USE_NTSC == 1
  // 引数チェック
#if USE_VS23 == 1
  if ( getParam(m,  0, vs23.numModes, I_NONE) ) return;   // m
#else
  if ( getParam(m,  0, 3, I_NONE) ) return;   // m
#endif
  if (scmode == m)
    return;

  for (int i=0; i < VS23_MAX_BG; ++i)
    vs23.freeBg(i);

  // 現在、ターミナルモードの場合は画面をクリア、終了、資源開放
  prv_m = sc->getSerialMode();
  sc->cls();
  sc->show_curs(true);
  sc->locate(0,0);
  sc->end();
  scmode = m;
  if (m == 0) {
    // USB-シリアルターミナルスクリーン設定
    sc = &sc1;
    ((tTermscreen*)sc)->init(TERM_W,TERM_H,SIZE_LINE, workarea); // スクリーン初期設定
  } else {
    // NTSCスクリーン設定
    sc = &sc0;
#if USE_VS23 == 1
    ((tTVscreen*)sc)->init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, m - 1);
#else
    if (m == 1)
      ((tTVscreen*)sc)->init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, SC_DEFAULT);
    else if (m == 2)
      ((tTVscreen*)sc)->init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, SC_224x108);
    else if (m == 3)
      ((tTVscreen*)sc)->init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, SC_112x108);
#endif
  }
  sc->Serial_mode(prv_m, GPIO_S1_BAUD);
  sc->cls();
  sc->show_curs(false);
  sc->draw_cls_curs();
  sc->locate(0,0);
  sc->refresh();
#else
  err = ERR_NOT_SUPPORTED;
#endif
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

void ibg() {
  int32_t m;
  int32_t w, h, px, py, pw, tx, ty, wx, wy, ww, wh;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetBgs();
    return;
  }

  if (getParam(m, 0, VS23_MAX_BG, I_NONE)) return;

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
    if (*cip != I_EOL && *cip != I_COLON)
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
  uint8_t w, h, tsx, tsy;
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
  int32_t num, pat_x, pat_y, w, h, frame;

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
    if (getParam(frame, 0, sc0.getGWidth(), I_NONE)) return;
    vs23.setSpriteFrame(num, frame);
    break;
  case I_ON:
    vs23.enableSprite(num);
    break;
  case I_OFF:
    vs23.disableSprite(num);
    break;
  default:
    // XXX: throw an error if nothing has been done
    cip--;
    if (*cip != I_EOL && *cip != I_COLON)
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
  // XXX: check actual bg dimensions!
  if (getParam(x, 0, 1023, I_COMMA)) return;
  if (getParam(y, 0, 1023, I_COMMA)) return;
  if (getParam(t, 0, 255, I_NONE)) return;
  vs23.setBgTile(bg, x, y, t);
}

#include "Psx.h"
Psx psx;
int32_t ipad() {
  int32_t num;
  if (checkOpen()) return 0;
  if (getParam(num, 0, 1, I_CLOSE)) return 0;
  return psx.read();
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
  int32_t prgno, lineno = -1;
  uint8_t* tmpcip, *lp;
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
  uint8_t* ptr;
  uint32_t sz;
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
	if ( getParam(lineno, 0, INT32_MAX, I_NONE) ) return 0;
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
  } else if (lineno == -1) {
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

// エラーメッセージ出力
// 引数: dlfCmd プログラム実行時 false、コマンド実行時 true
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
    // XXX: check if dims matches array
    value = num_arr.var(i).var(idxs);
    break;

  //括弧の値の取得
  case I_OPEN: //「(」
    cip--;
    value = getparam(); //括弧の値を取得
    break;

  //配列の値の取得
  case I_ARRAY: //配列
    value = getparam(); //括弧の値を取得
    if (!err) {
      if (value >= SIZE_ARRY)
	err = ERR_SOR;          // 添え字が範囲を超えた
      else
	value = arr[(int)value];     // 配列の値を取得
    }
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
  case I_SHIFTIN: value = ishiftIn(); break; // SHIFTIN()関数

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
    if (checkOpen()) break;
    if (getParam(value,0,I_PC15 - I_PA0, I_NONE)) break;
    if (checkClose()) break;
    value = -1; //analogRead(value);    // 入力値取得
    break;

  case I_EEPREAD: // EEPREAD(アドレス)の場合
    value = getparam();
    if (err) break;
    value = ieepread(value);   // 入力値取得
    break;

  case I_SREAD: // SREAD() シリアルデータ1バイト受信
    if (checkOpen()||checkClose()) break;
    value =sc->Serial_read();
    break; //ここで打ち切る

  case I_SREADY: // SREADY() シリアルデータデータチェック
    if (checkOpen()||checkClose()) break;
    value =sc->Serial_available();
    break; //ここで打ち切る

  // 画面サイズ定数の参照
  case I_CW: value = sc->getWidth(); break;
  case I_CH: value = sc->getHeight(); break;
#if USE_NTSC == 1
  case I_GW: value = scmode ? ((tTVscreen*)sc)->getGWidth() : 0; break;
  case I_GH: value = scmode ? ((tTVscreen*)sc)->getGHeight() : 0; break;
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

  default: //以上のいずれにも該当しなかった場合
    // 定数ピン番号
    cip--;
    if (*cip >= I_PA0 && *cip <= I_PC15) {
      value = *cip - I_PA0;
      cip++;
      return value;
      // 定数GPIOモード
    } else if (*cip >= I_OUTPUT_OPEN_DRAIN && *cip <= I_PWM) {
      value = pinType[*cip - I_OUTPUT_OPEN_DRAIN];
      cip++;
      return value;
    }
    err = ERR_SYNTAX; //エラー番号をセット
    break; //ここで打ち切る
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
  int len;

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
  case I_ARGSTR:
    bp = iargstr();
    if (!err)
      value = *bp;
    break;
  case I_STRSTR:
    svar.var(*cip++) = String(iexp());
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
  default:
    cip--;
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

// The parser
num_t GROUP(basic_core) iexp() {
  num_t value, tmp; //値と演算値

  value = iplus(); //値を取得
  if (err) //もしエラーが生じたら
    return -1;  //終了

  // conditional expression
  while (1) //無限に繰り返す
    switch(*cip++) { //中間コードで分岐

    case I_EQ: //「=」の場合
      tmp = iplus(); //演算値を取得
      value = (value == tmp); //真偽を判定
      break;
    case I_NEQ: //「!=」の場合
    case I_NEQ2: //「<>」の場合
    case I_SHARP: //「#」の場合
      tmp = iplus(); //演算値を取得
      value = (value != tmp); //真偽を判定
      break;
    case I_LT: //「<」の場合
      tmp = iplus(); //演算値を取得
      value = (value < tmp); //真偽を判定
      break;
    case I_LTE: //「<=」の場合
      tmp = iplus(); //演算値を取得
      value = (value <= tmp); //真偽を判定
      break;
    case I_GT: //「>」の場合
      tmp = iplus(); //演算値を取得
      value = (value > tmp); //真偽を判定
      break;
    case I_GTE: //「>=」の場合
      tmp = iplus(); //演算値を取得
      value = (value >= tmp); //真偽を判定
      break;
    case I_LAND: // AND (論理積)
      tmp = iplus(); //演算値を取得
      value = (value && tmp); //真偽を判定
      break;
    case I_LOR: // OR (論理和)
      tmp = iplus(); //演算値を取得
      value = (value || tmp); //真偽を判定
      break;
    default: //以上のいずれにも該当しなかった場合
      cip--;
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

// 左上の行番号の取得
int32_t getTopLineNum() {
  uint8_t* ptr = sc->getScreen();
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

// 左下の行番号の取得
int32_t getBottomLineNum() {
  uint8_t* ptr = sc->getScreen()+sc->getWidth()*(sc->getHeight()-1);
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

// 指定した行の前の行番号を取得する
int32_t getPrevLineNo(int32_t lineno) {
  uint8_t* lp, *prv_lp = NULL;
  int32_t rc = -1;
  for ( lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) {
    prv_lp = lp;
  }
  if (prv_lp)
    rc = getlineno(prv_lp);
  return rc;
}

// 指定した行の次の行番号を取得する
int32_t getNextLineNo(int32_t lineno) {
  uint8_t* lp, *prv_lp = NULL;
  int32_t rc = -1;

  lp = getlp(lineno);
  if (lineno == getlineno(lp)) {
    // 次の行に移動
    lp+=*lp;
    rc = getlineno(lp);
  }
  return rc;
}

// 指定した行のプログラムテキストを取得する
char* getLineStr(int32_t lineno) {
  uint8_t* lp = getlp(lineno);
  if (lineno != getlineno(lp))
    return NULL;

  // 行バッファへの指定行テキストの出力
  cleartbuf();
  putnum(lineno, 0,3);   // 行番号を表示
  c_putch(' ',3);      // 空白を入れる
  putlist(lp+sizeof(num_t)+1,3);   // XXX: really lp+5? was lp+3.   // 行番号より後ろを文字列に変換して表示
  c_putch(0,3);        // \0を入れる
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

// ラベル
void ilabel() {
  cip+= *cip+1;
}

// GOTO
void igoto() {
  uint8_t* lp;       // 飛び先行ポインタ
  int32_t lineno;    // 行番号

  if (*cip == I_STR) {
    // ラベル参照による分岐先取得
    lp = getlpByLabel(cip);
    if (lp == NULL) {
      err = ERR_ULN;                          // エラー番号をセット
      return;
    }
  } else {
    // 引数の行番号取得
    lineno = iexp();
    if (err) return;
    lp = getlp(lineno);                       // 分岐先のポインタを取得
    if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
      err = ERR_ULN;                          // エラー番号をセット
      return;
    }
  }
  clp = lp;        // 行ポインタを分岐先へ更新
  cip = clp + sizeof(num_t) + 1; // XXX: really? was 3.   // 中間コードポインタを先頭の中間コードに更新
}

// GOSUB
void igosub() {
  uint8_t* lp;       // 飛び先行ポインタ
  int32_t lineno;    // 行番号

  if (*cip == I_STR) {
    // ラベル参照による分岐先取得
    lp = getlpByLabel(cip);
    if (lp == NULL) {
      err = ERR_ULN;  // エラー番号をセット
      return;
    }
  } else {
    // 引数の行番号取得
    lineno = iexp();
    if (err)
      return;

    lp = getlp(lineno);                       // 分岐先のポインタを取得
    if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
      err = ERR_ULN;                          // エラー番号をセット
      return;
    }
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
  cip = clp + sizeof(num_t) + 1; // XXX: really? was 3.                            // 中間コードポインタを先頭の中間コードに更新
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
    if ((uintptr_t)lstk[lstki - 1] == -1)
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
  int want_index, index, vto, vstep; // FOR文の変数番号、終了値、増分

  if (lstki < 5) {    // もしFORスタックが空なら
    err = ERR_LSTKUF; // エラー番号をセット
    return;
  }

  if (*cip++ != I_VAR) {                     // もしNEXTの後ろに変数がなかったら
    err = ERR_NEXTWOV;                       // エラー番号をセット
    return;
  }
  
  want_index = *cip++;

  while (lstki) {
    // 変数名を復帰
    index = (int)(uintptr_t)lstk[lstki - 1]; // 変数名を復帰
    if (want_index == index)
      break;
    // If it does not match the returned variable name
    // Assume we want to NEXT to a loop higher up the stack.
    lstki -= 5;
  }

  if (!lstki) {
    // Didn't find anything that matches the NEXT.
    err = ERR_LSTKUF;	// XXX: have something more descriptive
    return;
  }

  vstep = (int)(uintptr_t)lstk[lstki - 2]; // 増分を復帰
  var.var(index) += vstep;
  vto = (int)(uintptr_t)lstk[lstki - 3];   // 終了値を復帰

  // もし変数の値が終了値を超えていたら
  if (((vstep < 0) && (var.var(index) < vto)) ||
      ((vstep > 0) && (var.var(index) > vto))) {
    lstki -= 5;  // FORスタックを1ネスト分戻す
    return;
  }

  // 開始値が終了値を超えていなかった場合
  cip = lstk[lstki - 4]; //行ポインタを復帰
  clp = lstk[lstki - 5]; //中間コードポインタを復帰
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
  sc->refresh();
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
    if (*cip >= I_PA0 && *cip <= I_PC15) {
      igpio();
    } else {
      // 以上のいずれにも該当しない場合
      err = ERR_SYNTAX; //エラー番号をセット
    } //中間コードで分岐の末尾

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

//Command precessor
uint8_t SMALL icom() {
  uint8_t rc = 1;
  uint8_t v;
  cip = ibuf;          // 中間コードポインタを中間コードバッファの先頭に設定

  switch (*cip++) {    // 中間コードポインタが指し示す中間コードによって分岐
  case I_LOAD:  ilrun_(); break;

  case I_LRUN:  if(ilrun()) {
      sc->show_curs(0); irun(clp);  sc->show_curs(1);
  }
    break;
  case I_RUN:
    sc->show_curs(0);
    irun();
    sc->show_curs(1);
    break;
  case I_CONT:
    if (!cont_cip || !cont_clp) {
      err = ERR_CONT;
    } else {
      sc->show_curs(0);
      irun(NULL, true);
      sc->show_curs(1);
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
    sc->show_curs(0);
    iexe();           // 中間コードを実行
    sc->show_curs(1);
    break;
  }
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
  uint8_t serialMode = DEF_SMODE;
  unsigned char len; // 中間コードの長さ
  uint8_t rc;

  vs23.begin();
  psx.setupPins(0, 1, 2, 3, 3);

  SPIFFS.begin();

  size_list = 0;
  listbuf = NULL;

  // 環境設定
  loadConfig();

  // 実行環境を初期化
  inew();

  if (scmode == 0) {
    sc = &sc1;
    tv_fontInit(); // フォントデータ利用のための設定
    ((tTermscreen*)sc)->init(TERM_W,TERM_H,SIZE_LINE, workarea); // スクリーン初期設定
  }
#if USE_NTSC == 1
  else {
    // NTSCスクリーン設定
    sc = &sc0;
    ((tTVscreen*)sc)->init(SIZE_LINE, CONFIG.KEYBOARD,CONFIG.NTSC, workarea, SC_DEFAULT);
  }
#endif
  if (serialMode == 1) {
    sc->Serial_mode(1, GPIO_S1_BAUD);
  }

  // PWM単音出力初期化
  tv_toneInit();

  sound_init();

#if USE_SD_CARD == 1
  // SDカード利用
  bfs.init(16); // この処理ではGPIOの操作なし
#endif

  Wire.begin(2, 0);
  // ESP8266 Wire code assumes that SCL and SDA pins are set low, instead
  // of taking care of that itself. WTF?!?
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  icls();
  sc->show_curs(0);

  // Want to make sure we get the right hue.
  vs23.setColorSpace(1);
  vs23.setColorConversion(1, 7, 3, 3, true);
  show_logo();
  vs23.setColorSpace(1);	// reset color conversion

  char* textline;    // 入力行

  // Startup screen
  // Epigram
  sc0.setFont(fonts[1]);
  sc->setColor(vs23.colorFromRgb(72,72,72), 0);
  {
    char epi[80];
    strcpy_P(epi, epigrams[random(sizeof(epigrams)/sizeof(*epigrams))]);
    c_puts(epi);
  }
  newline();

  // Banner
  sc->setColor(vs23.colorFromRgb(192,0,0), 0);
  static const char engine_basic[] PROGMEM = "Engine BASIC";
  c_puts_P(engine_basic);
  sc0.setFont(fonts[0]);

  // Platform/version
  sc->setColor(vs23.colorFromRgb(64,64,64), 0);
  sc->locate(sc->getWidth() - 14, 8);
  c_puts(STR_EDITION);          // 版を区別する文字列「EDITION」を表示
  c_puts(" " STR_VARSION);      // バージョンの表示

  // Free memory
  sc->setColor(vs23.colorFromRgb(255,255,255), 0);
  sc->locate(0,2);
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
  static const char bytes_free[] PROGMEM = " bytes free";
  c_puts_P(bytes_free); newline();
#endif

  sc->show_curs(1);
  error();          // "OK" or display an error message and clear the error number

  // プログラム自動起動
  if (CONFIG.STARTPRG >=0  && loadPrg(CONFIG.STARTPRG) == 0) {
    sc->show_curs(0);
    irun();        // RUN命令を実行
    sc->show_curs(1);
    newline();     // 改行
    c_puts("Autorun No."); putnum(CONFIG.STARTPRG,0); c_puts(" stopped.");
    newline();     // 改行
    err = 0;
  }

  // 端末から1行を入力して実行
  while (1) { //無限ループ
    rc = sc->edit();
    if (rc) {
      textline = (char*)sc->getText();
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

    // 1行の文字列を中間コードの並びに変換
    len = toktoi(); // 文字列を中間コードに変換して長さを取得
    if (err) {      // もしエラーが発生したら
      error(true);  // エラーメッセージを表示してエラー番号をクリア
      continue;     // 繰り返しの先頭へ戻ってやり直し
    }

    //中間コードの並びがプログラムと判断される場合
    if (*ibuf == I_NUM) { // もし中間コードバッファの先頭が行番号なら
      *ibuf = len;        // 中間コードバッファの先頭を長さに書き換える
      inslist();          // 中間コードの1行をリストへ挿入
      if (err)            // もしエラーが発生したら
	error();          // エラーメッセージを表示してエラー番号をクリア
      continue;           // 繰り返しの先頭へ戻ってやり直し
    }

    // 中間コードの並びが命令と判断される場合
    if (icom())  // 実行する
      error(false);   // エラーメッセージを表示してエラー番号をクリア
  } // 無限ループの末尾
}

// システム環境設定のロード
void loadConfig() {
  CONFIG.NTSC      =  0;
  CONFIG.KEYBOARD  =  1;
  CONFIG.STARTPRG  = -1;
  
  Unifile f = Unifile::open("f:.config", FILE_READ);
  if (!f)
    return;
  f.read((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}

// システム環境設定の保存
void isaveconfig() {
  Unifile f = Unifile::open("f:.config", FILE_OVERWRITE);
  if (!f) {
    err = ERR_FILE_OPEN;
  }
  f.write((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}
