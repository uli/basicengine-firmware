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
#include "sound.h"

#include "epigrams.h"

#ifdef ESP8266
#ifdef ESP8266_NOWIFI
#define STR_EDITION "ESP8266"
#else
#define STR_EDITION "ESP8266 WiFi"
#endif
#else
#define STR_EDITION "unknown"
#endif
#include "version.h"

#include "basic.h"
#include "net.h"

struct unaligned_num_t {
  num_t n;
} __attribute__((packed));

#define UNALIGNED_NUM_T(ip) (reinterpret_cast<struct unaligned_num_t *>(ip)->n)

// TOYOSHIKI TinyBASIC プログラム利用域に関する定義
int size_list;
#define MAX_VAR_NAME 32  // maximum length of variable names
#define SIZE_GSTK 10     // GOSUB stack size
#define SIZE_LSTK 10     // FOR stack size
#define SIZE_ASTK 16	// argument stack

// *** フォント参照 ***************
const uint8_t* ttbasic_font = TV_DISPLAY_FONT;

#define MIN_FONT_SIZE_X 6
#define MIN_FONT_SIZE_Y 8
#define NUM_FONTS 4
static const uint8_t *fonts[] = {
  console_font_6x8,
  console_font_8x8,
  cbm_ascii_font_8x8,
  font6x8tt,
};

// **** スクリーン管理 *************
uint8_t scmode = USE_SCREEN_MODE;

#include "tTVscreen.h"
tTVscreen   sc0; 

// Saved background and text window dimensions.
// These values are set when a program is interrupted and the text window is
// forced to be visible at the bottom of the screen. That way, the original
// geometries can be restored when the program is CONTinued.
#ifdef VS23_BG_ENGINE
static int original_bg_height[VS23_MAX_BG];
static bool turn_bg_back_on[VS23_MAX_BG];
bool restore_bgs = false;
static uint16_t original_text_pos[2];
bool restore_text_window = false;
#endif

#define KEY_ENTER 13

#include <Wire.h>

// *** SDカード管理 ****************
sdfiles bfs;

#define MAX_USER_FILES 16
Unifile *user_files[MAX_USER_FILES];

SystemConfig CONFIG;

// プロトタイプ宣言
void loadConfig();
void isaveconfig();
void mem_putch(uint8_t c);
unsigned char* BASIC_FP iexe(bool until_return = false);
num_t BASIC_FP iexp(void);
void error(uint8_t flgCmd);
// **** RTC用宣言 ********************
#ifdef USE_INNERRTC
  #include <Time.h>
#endif

// 定数
#define CONST_HIGH   1
#define CONST_LOW    0
#define CONST_LSB    LSBFIRST
#define CONST_MSB    MSBFIRST

// Terminal control(文字の表示・入力は下記の3関数のみ利用)

#include <TKeyboard.h>
extern TKeyboard kb;

// 1桁16進数文字を整数に変換する
uint16_t BASIC_INT hex2value(char c) {
  if (c <= '9' && c >= '0')
    return c - '0';
  else if (c <= 'f' && c >= 'a')
    return c - 'a' + 10;
  else if (c <= 'F' && c >= 'A')
    return c - 'A' + 10;
  return 0;
}

void BASIC_INT screen_putch(uint8_t c, bool lazy) {
  static bool escape = false;
  static uint8_t col_digit = 0, color, is_bg;
  static bool reverse = false;
  if (c == '\\') {
    if (!escape) {
      escape = true;
      return;
    }
    sc0.putch('\\');
  } else {
    if (col_digit > 0) {
      if (col_digit == 1) {
        color = hex2value(c);
        col_digit++;
      } else {
        color = (color << 4) | hex2value(c);
        if (is_bg)
          sc0.setColor(sc0.getFgColor(), color);
        else
          sc0.setColor(color, sc0.getBgColor());

        col_digit = 0;
      }
    } else if (escape) {
      switch (c) {
      case 'R':
        if (!reverse) {
          reverse = true;
          sc0.flipColors();
        }
        break;
      case 'N':
        if (reverse) { 
          reverse = false;
          sc0.flipColors();
        }
        break;
      case 'l':
        if (sc0.c_x())
          sc0.locate(sc0.c_x() - 1, sc0.c_y());
        else if (sc0.c_y() > 0)
          sc0.locate(sc0.getWidth() - 1, sc0.c_y() - 1);
        break;
      case 'r':
        if (sc0.c_x() < sc0.getWidth() - 1)
          sc0.locate(sc0.c_x() + 1, sc0.c_y());
        else if (sc0.c_y() < sc0.getHeight() - 1)
          sc0.locate(0, sc0.c_y() + 1);
        break;
      case 'u':
        if (sc0.c_y() > 0)
          sc0.locate(sc0.c_x(), sc0.c_y() - 1);
        break;
      case 'd':
        if (sc0.c_y() < sc0.getHeight() - 1)
          sc0.locate(sc0.c_x(), sc0.c_y() + 1);
        break;
      case 'c':	sc0.cls();	// fallthrough
      case 'h': sc0.locate(0, 0);
      case 'f':
        col_digit = 1;
        is_bg = false;
        break;
      case 'b':
        col_digit = 1;
        is_bg = true;
        break;
      default:	break;
      }
    } else {
      switch (c) {
      case '\n':newline(); break;
      case '\r':sc0.locate(0, sc0.c_y()); break;
      default:	sc0.putch(c, lazy); break;
      }
    }
  }
  escape = false;
}

// 文字の出力
inline void c_putch(uint8_t c, uint8_t devno) {
  if (devno == 0)
    screen_putch(c);
  else if (devno == 1)
    Serial.write(c);
  else if (devno == 2)
    sc0.gputch(c);
  else if (devno == 3)
    mem_putch(c);
  else if (devno == 4)
    bfs.putch(c);
}

void newline(uint8_t devno) {
  if (devno==0) {
    if (sc0.peekKey() == SC_KEY_CTRL_C) {
      c_getch();
      err = ERR_CTR_C;
    } else if (kb.state(PS2KEY_L_Shift)) {
      uint32_t m = millis() + 200;
      while (millis() < m) {
        sc0.peekKey();
        if (!kb.state(PS2KEY_L_Shift))
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
    bfs.putch('\x0a');
  }
}

// tick用支援関数
void iclt() {
//  systick_uptime_millis = 0;
}

// 乱数
num_t BASIC_FP getrnd(int value) {
  if (value <= 1)
    return ((num_t)random(RAND_MAX)/(num_t)RAND_MAX);
  else
    return random(value);
}

#ifdef ESP8266
#define __FLASH__ ICACHE_RODATA_ATTR
#endif

// List formatting condition
// Intermediate code without trailing blank
const uint8_t i_nsa[] BASIC_DAT = {
  I_END,
  I_CLS,I_CLT,
  I_HIGH, I_LOW, I_CSIZE, I_PSIZE,
  I_INKEY,I_CHAR, I_CHR, I_ASC, I_HEX, I_BIN,I_LEN, I_STRSTR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT,
  I_RND, I_ABS, I_FREE, I_TICK, I_PEEK, I_PEEKW, I_PEEKD, I_I2CW, I_I2CR,
  I_SIN, I_COS, I_EXP, I_ATN, I_ATN2, I_SQR, I_TAN, I_LOG, I_INT,
  I_OUTPUT, I_INPUT_ANALOG,
  I_DIN, I_ANA, I_MAP,
  I_LSB, I_MSB, I_MPRG, I_MFNT,
  I_SREAD, I_SREADY, I_POINT,
  I_RET, I_ARG, I_ARGSTR, I_ARGC,
  I_PAD, I_SPRCOLL, I_TILECOLL,
};

// Intermediate code which eliminates previous space when former is constant or variable
const uint8_t i_nsb[] BASIC_DAT = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2,I_LT,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL
};

// insert a blank before intermediate code
const uint8_t i_sf[] BASIC_DAT  = {
  I_CLS, I_COLOR, I_DATE, I_END, I_FILES, I_TO, I_STEP,I_QUEST,I_AND, I_OR, I_XOR,
  I_GETDATE,I_GETTIME,I_GOSUB,I_GOTO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,I_THEN,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_SETDATE,I_WAIT,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BLIT, I_SWRITE, I_SPRINT,I_SMODE,
  I_BEEP, I_CSCROLL, I_GSCROLL, I_MOD,
};

// tokens that can be functions (no space before paren) or something else
const uint8_t i_dual[] BASIC_DAT = {
  I_FRAME, I_PLAY, I_VREG, I_POS,
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
#define dual(c) sstyle(c, i_dual, sizeof(i_dual))

// エラーメッセージ定義
uint8_t err; // Error message index
const char *err_expected;

#define ESTR(n,s) static const char _errmsg_ ## n[] PROGMEM = s;
#include "errdef.h"

#undef ESTR
#define ESTR(n,s) _errmsg_ ## n,
static const char* const errmsg[] PROGMEM = {
#include "errdef.h"
};

#undef ESTR

#include "error.h"

void SMALL E_VALUE(int32_t from, int32_t to) {
  static const char __fmt_ft[] PROGMEM = "from %d to %d";
  static const char __fmt_t[] PROGMEM = "max %d";
  static const char __fmt_f[] PROGMEM = "min %d";
  err = ERR_VALUE;
  if (from == INT32_MIN)
    sprintf_P(tbuf, __fmt_t, to);
  else if (to == INT32_MAX)
    sprintf_P(tbuf, __fmt_f, from);
  else
    sprintf_P(tbuf, __fmt_ft, from, to);
  err_expected = tbuf;
}

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
BasicListVariables<BString> str_lst;
VarNames str_lst_names;
BasicListVariables<num_t> num_lst;
VarNames num_lst_names;

VarNames proc_names;
Procedures proc;
VarNames label_names;
Labels labels;

unsigned char *listbuf; // Pointer to program list area

// BASIC line number descriptor.
// NB: A lot of code relies on this being of size "sizeof(num_t)+1".
typedef struct {
  uint8_t next;
  union {
    num_t raw_line;
    struct {
      uint32_t line;
      int8_t indent;
    };
  };
} __attribute__((packed)) line_desc_t;

unsigned char* clp;               // Pointer current line
unsigned char* cip;               // Pointer current Intermediate code
struct {
  uint8_t *lp;
  uint8_t *ip;
  uint8_t num_args;
  uint8_t str_args;
  uint8_t proc_idx;
} gstk[SIZE_GSTK];   // GOSUB stack
unsigned char gstki;              // GOSUB stack index

// Arguments/locals stack
num_t astk_num[SIZE_ASTK];
unsigned char astk_num_i;
BString astk_str[SIZE_ASTK];
unsigned char astk_str_i;

struct {
  uint8_t *lp;
  uint8_t *ip;
  num_t vto;
  num_t vstep;
  int16_t index;
  bool local;
} lstk[SIZE_LSTK];   // loop stack
unsigned char lstki;              // loop stack index

uint8_t *cont_clp = NULL;
uint8_t *cont_cip = NULL;

num_t retval[MAX_RETVALS];        // multi-value returns (numeric)

// メモリへの文字出力
inline void mem_putch(uint8_t c) {
  if (tbuf_pos < SIZE_LINE) {
    tbuf[tbuf_pos] = c;
    tbuf_pos++;
  }
}

void* BASIC_INT sanitize_addr(uint32_t vadr, int type) {
  // Unmapped memory, causes exception
  if (vadr < 0x20000000UL) {
    E_ERR(VALUE, "mapped address");
    return NULL;
  }
  // IRAM, flash: 32-bit only
  if ((vadr >= 0x40100000UL && vadr < 0x40300000UL) && type != 2) {
    E_ERR(VALUE, "32-bit access");
    return NULL;
  }
  if ((type == 1 && (vadr & 1)) ||
      (type == 2 && (vadr & 3))) {
    E_ERR(VALUE, "aligned address");
    return NULL;
  }
  return (void *)vadr;
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
static inline uint8_t BASIC_FP getParam(num_t& prm, num_t v_min,  num_t v_max, token_t next_token) {
  prm = iexp();
  if (!err &&  (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

static inline uint32_t BASIC_FP getParam(uint32_t& prm, uint32_t v_min, uint32_t v_max, token_t next_token) {
  prm = iexp();
  if (!err &&  (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

// コマンド引数取得(int32_t,引数チェックなし)
static inline uint8_t BASIC_FP getParam(uint32_t& prm, token_t next_token) {
  prm = iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

// コマンド引数取得(uint32_t,引数チェックなし)
static inline uint8_t BASIC_FP getParam(num_t& prm, token_t next_token) {
  prm = iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    if (next_token == I_OPEN || next_token == I_CLOSE)
      err = ERR_PAREN;
    else
      E_SYNTAX(next_token);
  }
  return err;
}

static inline bool BASIC_FP is_strexp() {
  // XXX: does not detect string comparisons (numeric)
  return (*cip == I_STR ||
          ((*cip == I_SVAR || *cip == I_LSVAR) && cip[2] != I_SQOPEN) ||
          *cip == I_STRARR ||
          *cip == I_STRLST ||
          *cip == I_ARGSTR ||
          *cip == I_STRSTR ||
          *cip == I_CHR ||
          *cip == I_HEX ||
          *cip == I_BIN ||
          *cip == I_LEFTSTR ||
          *cip == I_RIGHTSTR ||
          *cip == I_MIDSTR ||
          *cip == I_CWD ||
          *cip == I_DIRSTR ||
          *cip == I_INSTSTR ||
          *cip == I_INPUTSTR ||
          *cip == I_POPFSTR ||
          *cip == I_POPBSTR ||
          (*cip == I_NET && (*cip == I_INPUTSTR || *cip == I_GETSTR)) ||
          *cip == I_INKEYSTR
         );
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
#ifdef FLOAT_NUMS
static const char num_prec_fmt[] PROGMEM = "%%%s%d.%dg";
#else
static const char num_fmt[] PROGMEM = "%%%s%dd";
#endif
void putnum(num_t value, int8_t d, uint8_t devno) {
  char f[6];
  const char *l;

  if (d < 0) {
    d = -d;
    l = "0";
  } else
    l = "";

#ifdef FLOAT_NUMS
  if (d == 0)
    sprintf_P(f, num_prec_fmt, l, d, 9);
  else
    sprintf_P(f, num_prec_fmt, l, d, d);
#else
  sprintf_P(f, num_fmt, l, d);
#endif
  sprintf(lbuf, f, value);
  c_puts(lbuf, devno);
}

#ifdef FLOAT_NUMS
void putint(int value, int8_t d, uint8_t devno) {
  char f[] = "%.d";
  f[1] = '0' + d;
  sprintf(lbuf, f, value);
  c_puts(lbuf, devno);
}
#else
#define putint putnum
#endif

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
void putHexnum(uint32_t value, uint8_t d, uint8_t devno) {
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
  } while ((isAlpha(v) || (var_len > 1 && (isAlphaNumeric(v) || v == '_'))) &&
           var_len < MAX_VAR_NAME-1);
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
  bool implicit_endif = false;

  bool is_prg_text = false;

  while (*s) {                  //文字列1行分の終端まで繰り返す
    while (c_isspace(*s)) s++;  //空白を読み飛ばす

    key = lookup(s);
    if (key >= 0) {
      // 該当キーワードあり
      if (len >= SIZE_IBUF - 2) {      // もし中間コードが長すぎたら
	err = ERR_IBUFOF;              // エラー番号をセット
	return 0;                      // 0を持ち帰る
      }
      ibuf[len++] = key;                 // 中間コードを記録
      s+= strlen_P(kwtbl[key]);
      if (key == I_THEN) {
        while (c_isspace(*s)) s++;
        if (*s)
          implicit_endif = true;
      }
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

	if (len >= SIZE_IBUF - 5) { // もし中間コードが長すぎたら
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
      if (len >= SIZE_IBUF - 3 - i) {    // もし中間コードが長すぎたら
	err = ERR_IBUFOF;                // エラー番号をセット
	return 0;                        // 0を持ち帰る
      }

      ibuf[len++] = i;                   // コメントの文字数を記録
      while (i--) {                      // コメントの文字数だけ繰り返す
	ibuf[len++] = *s++;              // コメントを記録
      }
      break;                             // 文字列の処理を打ち切る（終端の処理へ進む）
    } else if (key == I_PROC) {
      if (!is_prg_text) {
        err = ERR_COM;
        return 0;
      }
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF;
	return 0;
      }

      while (c_isspace(*s)) s++;
      s += parse_identifier(s, vname);

      int idx = proc_names.assign(vname, true);
      ibuf[len++] = idx;
      if (proc.reserve(proc_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_LABEL) {
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF;
	return 0;
      }

      while (c_isspace(*s)) s++;
      s += parse_identifier(s, vname);

      int idx = label_names.assign(vname, true);
      ibuf[len++] = idx;
      if (labels.reserve(label_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_CALL || key == I_FN) {
      while (c_isspace(*s)) s++;
      s += parse_identifier(s, vname);
      int idx = proc_names.assign(vname, is_prg_text);
      if (proc.reserve(proc_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
      if (len >= SIZE_IBUF - 2) {
	err = ERR_IBUFOF;
	return 0;
      }
      ibuf[len++] = idx;
    }
    
    // done if keyword parsed
    if (key >= 0) {
      find_prg_text = false;
      continue;
    }

    // Attempt to convert to constant
    ptok = s;                            // Points to the beginning of a word
    if (isDigit(*ptok) || *ptok == '.') {
      if (len >= SIZE_IBUF - sizeof(num_t) - 2) { // If the intermediate code is too long
	err = ERR_IBUFOF;
	return 0;
      }
      value = strtonum(ptok, &ptok);
      s = ptok;			// Stuff the processed part of the character string
      ibuf[len++] = I_NUM;	// Record intermediate code
      UNALIGNED_NUM_T(ibuf+len) = value;
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
      for (i = 0; *ptok && (*ptok != c); i++)
	ptok++;

      if (len >= SIZE_IBUF - 3 - i) { // if the intermediate code is too long
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
    } else if (*ptok == '@' || *ptok == '~' || isAlpha(*ptok)) {
      // Try converting to variable
      bool is_local = false;
      bool is_list = false;
      if (*ptok == '@') {
        is_local = true;
        ++ptok; ++s;
      } else if (*ptok == '~') {
        is_list = true;
        ++ptok; ++s;
      }

      var_len = parse_identifier(ptok, vname);
      if (len >= SIZE_IBUF - 3) { // if the intermediate code is too long
	err = ERR_IBUFOF;
	return 0;
      }

      char *p = ptok + var_len;
      if (*p == '$' && p[1] == '(') {
        if (is_local) {
          err = ERR_NOT_SUPPORTED;
          return 0;
        }
        int idx;
        if (is_list) {
          ibuf[len++] = I_STRLST;
          idx = str_lst_names.assign(vname, is_prg_text);
        } else {
          ibuf[len++] = I_STRARR;
          idx = str_arr_names.assign(vname, is_prg_text);
        }
        if (idx < 0)
          goto oom;
        ibuf[len++] = idx;
        if ((!is_list && str_arr.reserve(str_arr_names.varTop())) ||
            ( is_list && str_lst.reserve(str_lst_names.varTop())))
          goto oom;
        s += var_len + 2;
        ptok += 2;
      } else if (*p == '$') {
        uint8_t tok;
        if (is_local)
          tok = I_LSVAR;
        else if (is_list)
          tok = I_STRLSTREF;
        else
          tok = I_SVAR;
        ibuf[len++] = tok;
	int idx;
	if (is_list) {
	  idx = str_lst_names.assign(vname, is_prg_text);
	} else {
	  idx = svar_names.assign(vname, is_prg_text);
        }
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if ((!is_list && svar.reserve(svar_names.varTop())) ||
	    ( is_list && str_lst.reserve(str_lst_names.varTop())))
	  goto oom;
	s += var_len + 1;
	ptok++;
      } else if (*p == '(') {
        if (is_local) {
          err = ERR_NOT_SUPPORTED;
          return 0;
        }
        int idx;
        if (is_list) {
          ibuf[len++] = I_NUMLST;
          idx = num_lst_names.assign(vname, is_prg_text);
        } else {
          ibuf[len++] = I_VARARR;
          idx = num_arr_names.assign(vname, is_prg_text);
        }
        if (idx < 0)
          goto oom;
        ibuf[len++] = idx;
        if ((!is_list && num_arr.reserve(num_arr_names.varTop())) ||
            ( is_list && num_lst.reserve(num_lst_names.varTop())))
          goto oom;
        s += var_len + 1;
        ptok++;
      } else {
	// Convert to intermediate code
        uint8_t tok;
        if (is_local)
          tok = I_LVAR;
        else if (is_list)
          tok = I_NUMLSTREF;
        else
          tok = I_VAR;
	ibuf[len++] = tok;
	int idx;
	if (is_list) {
	  idx = num_lst_names.assign(vname, is_prg_text);
	} else {
	  idx = var_names.assign(vname, is_prg_text);
        }
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if ((!is_list && var.reserve(var_names.varTop())) ||
	    ( is_list && num_lst.reserve(num_lst_names.varTop())))
	  goto oom;
	s += var_len;
      }
    } else { // if none apply
      err = ERR_SYNTAX;
      return 0;
    }
  }

  if (implicit_endif)
    ibuf[len++] = I_IMPLICITENDIF;
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
uint32_t BASIC_FP getlineno(unsigned char *lp) {
  line_desc_t *ld = (line_desc_t *)lp;
  if(ld->next == 0) //もし末尾だったら
    return (uint32_t)-1;
  return ld->line;
}

// Search line by line number
unsigned char* BASIC_FP getlp(uint32_t lineno) {
  unsigned char *lp; //ポインタ

  for (lp = listbuf; *lp; lp += *lp) //先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) //もし指定の行番号以上なら
      break;  //繰り返しを打ち切る

  return lp; //ポインタを持ち帰る
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
uint8_t* getELSEptr(uint8_t* p, bool endif_only = false) {
  uint8_t* rc = NULL;
  uint8_t* lp;
  unsigned char lifstki = 1;

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (lp = p; ; ) {
    switch(*lp) {
    case I_IF:    // IF命令
      lp++;
      lifstki++;
      break;
    case I_ELSE:  // ELSE命令
      if (lifstki == 1 && !endif_only) {
        // Found the highest-level ELSE, we're done.
        rc = lp+1;
        goto DONE;
      }
      lp++;
      break;
    case I_EOL:
    case I_REM:
    case I_SQUOT:
      // Continue at next line.
      clp += *clp;
      if (!*clp) {
        err = ERR_NOENDIF;
        return NULL;
      }
      lp = cip = clp + sizeof(line_desc_t);
      break;
    case I_ENDIF:
    case I_IMPLICITENDIF:
      lifstki--;
      lp++;
      if (!lifstki) {
        // End of the last IF, no ELSE found -> done.
        rc = lp;
        goto DONE;
      }
      break;
    default:        // その他
      lp += token_size(lp);
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
  labels.reset();

  // Empty check (If this is the case, it may be impossible to delete lines
  // when only line numbers are entered when there is insufficient space ..)
  // @Tamakichi)
  if (list_free() < *ibuf) { // If the vacancy is insufficient
    listbuf = (unsigned char *)realloc(listbuf, size_list + *ibuf);
    if (!listbuf) {
      err = ERR_OOM;
      size_list = 0;
      return;
    }
    size_list += *ibuf;
  }

  // Convert I_NUM literal to line number descriptor.
  line_desc_t *ld = (line_desc_t *)ibuf;
  num_t lin = ld->raw_line;
  ld->line = lin;
  ld->indent = 0;

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

#define MAX_INDENT 10
#define INDENT_STEP 2

static int8_t indent_level;

// tokens that increase indentation
inline bool is_indent(uint8_t c) {
  return c == I_IF || c == I_FOR || c == I_DO;
}
// tokens that reduce indentation
inline bool is_unindent(uint8_t c) {
  return c == I_ENDIF || c == I_IMPLICITENDIF || c == I_NEXT || c == I_LOOP;
}
// tokens that temporarily reduce indentation
inline bool is_reindent(uint8_t c) {
  return c == I_ELSE;
}

void SMALL recalc_indent_line(unsigned char *lp) {
  bool re_indent = false;
  bool skip_indent = false;
  line_desc_t *ld = (line_desc_t *)lp;
  unsigned char *ip = lp + sizeof(line_desc_t);

  re_indent = is_reindent(*ip);	// must be reverted at the end of the line
  if (is_unindent(*ip) || re_indent) {
    skip_indent = true;	// don't do this again in the main loop
    indent_level -= INDENT_STEP;
  }

  if (indent_level < 0)
    indent_level = 0;

  ld->indent = indent_level;
  
  while (*ip != I_EOL) {
    if (skip_indent)
      skip_indent = false;
    else {
      if (is_indent(*ip))
        indent_level += INDENT_STEP;
      else if (is_unindent(*ip))
        indent_level -= INDENT_STEP;
    }
    int ts = token_size(ip);
    if (ts < 0)
      break;
    else
      ip += ts;
  }

  if (re_indent)
    indent_level += INDENT_STEP;
}

void SMALL recalc_indent() {
  unsigned char *lp = listbuf;
  indent_level = 0;

  while (*lp) {               // 行ポインタが末尾を指すまで繰り返す
    recalc_indent_line(lp);    // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    lp += *lp;               // 行ポインタを次の行へ進める
  }
}

//指定中間コード行レコードのテキスト出力
void SMALL putlist(unsigned char* ip, uint8_t devno) {
  unsigned char i;  // ループカウンタ
  uint8_t var_code; // 変数コード
  line_desc_t *ld = (line_desc_t *)ip;
  ip += sizeof(line_desc_t);

  for (i = 0; i < ld->indent && i < MAX_INDENT; ++i)
    c_putch(' ', devno);

  while (*ip != I_EOL) { //行末でなければ繰り返す
    //キーワードの処理
    if (*ip < SIZE_KWTBL && kwtbl[*ip]) { //もしキーワードなら
      char kw[MAX_KW_LEN+1];
      strcpy_P(kw, kwtbl[*ip]);

      if (isAlpha(kw[0]))
        sc0.setColor(COL(KEYWORD), COL(BG));
      else if (*ip == I_LABEL)
        sc0.setColor(COL(PROC), COL(BG));
      else
        sc0.setColor(COL(OP), COL(BG));
      c_puts(kw, devno); //キーワードテーブルの文字列を表示
      sc0.setColor(COL(FG), COL(BG));

      if (*(ip+1) != I_COLON && (*(ip+1) != I_OPEN || !dual(*ip)))
	if ( (!nospacea(*ip) || spacef(*(ip+1))) &&
	     *ip != I_COLON && *ip != I_SQUOT && *ip != I_LABEL) //もし例外にあたらなければ
	  c_putch(' ',devno);  //空白を表示

      if (*ip == I_REM||*ip == I_SQUOT) { //もし中間コードがI_REMなら
	ip++; //ポインタを文字数へ進める
	i = *ip++; //文字数を取得してポインタをコメントへ進める
	sc0.setColor(COL(COMMENT), COL(BG));
	while (i--) //文字数だけ繰り返す
	  c_putch(*ip++,devno);  //ポインタを進めながら文字を表示
        sc0.setColor(COL(FG), COL(BG));
	return;
      } else if (*ip == I_PROC || *ip == I_CALL || *ip == I_FN) {
        ip++;
        sc0.setColor(COL(PROC), COL(BG));
        c_puts(proc_names.name(*ip), devno);
        sc0.setColor(COL(FG), COL(BG));
      } else if (*ip == I_LABEL) {
        ip++;
        sc0.setColor(COL(PROC), COL(BG));
        c_puts(label_names.name(*ip), devno);
        sc0.setColor(COL(FG), COL(BG));
      }

      ip++; //ポインタを次の中間コードへ進める
    }
    else

    //定数の処理
    if (*ip == I_NUM) { //もし定数なら
      ip++; //ポインタを値へ進める
      num_t n = UNALIGNED_NUM_T(ip);
      sc0.setColor(COL(NUM), COL(BG));
      putnum(n, 0,devno); //値を取得して表示
      sc0.setColor(COL(FG), COL(BG));
      ip += sizeof(num_t); //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    }
    else

    //16進定数の処理
    if (*ip == I_HEXNUM) { //もし16進定数なら
      ip++; //ポインタを値へ進める
      sc0.setColor(COL(NUM), COL(BG));
      c_putch('$',devno); //空白を表示
      putHexnum(ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24), 8,devno); //値を取得して表示
      sc0.setColor(COL(FG), COL(BG));
      ip += 4; //ポインタを次の中間コードへ進める
      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_VAR || *ip == I_LVAR) { //もし定数なら
      if (*ip == I_LVAR) {
        sc0.setColor(COL(LVAR), COL(BG));
        c_putch('@', devno);
      } else
        sc0.setColor(COL(VAR), COL(BG));
      ip++; //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(var_names.name(var_code), devno);
      sc0.setColor(COL(FG), COL(BG));

      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_VARARR || *ip == I_NUMLST) {
      ip++;
      var_code = *ip++;
      sc0.setColor(COL(VAR), COL(BG));
      if (ip[-2] == I_NUMLST) {
        c_putch('~', devno);
        c_puts(num_lst_names.name(var_code), devno);
      } else {
        c_puts(num_arr_names.name(var_code), devno);
      }
      sc0.setColor(COL(FG), COL(BG));
      c_putch('(', devno);
    } else if (*ip == I_NUMLSTREF) {
      ip++;
      var_code = *ip++;
      sc0.setColor(COL(VAR), COL(BG));
      c_putch('~', devno);
      c_puts(num_lst_names.name(var_code), devno);
      sc0.setColor(COL(FG), COL(BG));
    } else if (*ip == I_SVAR || *ip == I_LSVAR) {
      if (*ip == I_LSVAR) {
        sc0.setColor(COL(LVAR), COL(BG));
        c_putch('@', devno);
      } else
        sc0.setColor(COL(VAR), COL(BG));
      ip++; //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(svar_names.name(var_code), devno);
      c_putch('$', devno);
      sc0.setColor(COL(FG), COL(BG));

      if (!nospaceb(*ip)) //もし例外にあたらなければ
	c_putch(' ',devno);  //空白を表示
    } else if (*ip == I_STRARR || *ip == I_STRLST) {
      ip++;
      var_code = *ip++;
      sc0.setColor(COL(VAR), COL(BG));
      if (ip[-2] == I_STRLST) {
        c_putch('~', devno);
        c_puts(str_lst_names.name(var_code), devno);
      } else {
        c_puts(str_arr_names.name(var_code), devno);
      }
      c_putch('$', devno);
      sc0.setColor(COL(FG), COL(BG));
      c_putch('(', devno);
    } else if (*ip == I_STRLSTREF) {
      ip++;
      var_code = *ip++;
      sc0.setColor(COL(VAR), COL(BG));
      c_putch('~', devno);
      c_puts(str_lst_names.name(var_code), devno);
      c_putch('$', devno);
      sc0.setColor(COL(FG), COL(BG));
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

      sc0.setColor(COL(STR), COL(BG));
      //文字列を表示する
      c_putch(c,devno); //文字列の括りを表示
      i = *ip++; //文字数を取得してポインタを文字列へ進める
      while (i--) //文字数だけ繰り返す
	c_putch(*ip++,devno);  //ポインタを進めながら文字を表示
      c_putch(c,devno); //文字列の括りを表示
      sc0.setColor(COL(FG), COL(BG));
      if (*ip == I_VAR || *ip ==I_ELSE || *ip == I_AS || *ip == I_TO)
	c_putch(' ',devno);
    } else if (*ip == I_IMPLICITENDIF) {
      // invisible
      ip++;
    } else { //どれにも当てはまらなかった場合
      err = ERR_SYS; //エラー番号をセット
      return;
    }
  }
}

int BASIC_FP get_array_dims(int *idxs);

// Get argument in parenthesis
num_t BASIC_FP getparam() {
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
  int32_t filenum = -1;

  if (*cip == I_SHARP) {
    cip++;
    if (getParam(filenum, 0, MAX_USER_FILES, I_COMMA))
      return;
    if (!user_files[filenum] || !*user_files[filenum]) {
      err = ERR_FILE_NOT_OPEN;
      return;
    }
  } else if(is_strexp() && *cip != I_SVAR) {
    // We have to exclude string variables here because they may be lvalues.
    c_puts(istrexp().c_str());

    if (*cip != I_SEMI) {
      E_SYNTAX(I_SEMI);
      goto DONE;
    }
    cip++;
  }

  sc0.show_curs(1);
  for(;;) {
    // Processing input values
    switch (*cip++) {
    case I_VAR:
    case I_VARARR:
    case I_NUMLST:
      index = *cip;

      if (cip[-1] == I_VARARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
      } else if (cip[-1] == I_NUMLST) {
        if (get_array_dims(idxs) != 1) {
          SYNTAX_T("single dimension");
          return;
        }
        dims = -1;
      }
 
      cip++;

      if (filenum >= 0) {
        int c;
        str_value = "";
        for (;;) {
          c = user_files[filenum]->peek();
          if (isdigit(c) || c == '.')
            str_value += (char)user_files[filenum]->read();
          else if (isspace(c))
            user_files[filenum]->read();
          else
            break;
        }
        value = str_value.toFloat();
      } else
        value = getnum();

      if (err)
        return;

      if (dims > 0)
        num_arr.var(index).var(idxs) = value;
      else if (dims < 0)
        num_lst.var(index).var(idxs[0]) = value;
      else
        var.var(index) = value;

      break;

    case I_SVAR:
    case I_STRARR:
    case I_STRLST:
      index = *cip;
      
      if (cip[-1] == I_STRARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
      } else if (cip[-1] == I_STRLST) {
        if (get_array_dims(idxs) != 1) {
          SYNTAX_T("single dimension");
          return;
        }
        dims = -1;
      }
 
      cip++;

      if (filenum >= 0) {
        int c;
        str_value = "";
        while ((c = user_files[filenum]->read()) >= 0 && c != '\n') {
          if (c != '\r')
            str_value += c;
        }
      } else {      
        str_value = getstr();
      }
      if (err)
        return;
      
      if (dims > 0)
        str_arr.var(index).var(idxs) = str_value;
      else if (dims < 0)
        str_lst.var(index).var(idxs[0]) = str_value;
      else
        svar.var(index) = str_value;

      break;

    default: // 以上のいずれにも該当しなかった場合
      SYNTAX_T("variable");
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
      SYNTAX_T("separator");
      //return;           // 終了
      goto DONE;
    } // 中間コードで分岐の末尾
  }   // 無限に繰り返すの末尾

DONE:
  sc0.show_curs(0);
}

// Variable assignment handler
void BASIC_FP ivar() {
  num_t value; //値
  short index; //変数番号

  index = *cip++; //変数番号を取得して次へ進む

  if (*cip != I_EQ) { //もし「=」でなければ
    err = ERR_VWOEQ; //エラー番号をセット
    return;
  }
  cip++;
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  var.var(index) = value;
}

// Local variables are handled by encoding them with global variable indices.
//
// When a local variable is created (either through a procedure signature or
// by referencing it), a global variable entry of the same name is created
// (unless it already exists), and its index is used to encode the local
// variable in the program text.
//
// When a local variable is dereferenced, the global index is translated to
// an argument stack offset depending on which procedure is currently running.
// So global and local variables of the same name share entries in the variable
// table, but the value in the table is only used by the global variable. The
// local's value resides on the argument stack.
//
// The reason for doing this in such a convoluted way is that we don't know
// what a procedure's stack frame looks like at compile time because we may
// have to compile code out-of-order.

static int BASIC_FP get_num_local_offset(uint8_t arg, bool &is_local)
{
  is_local = false;
  if (!gstki) {
    // not in a subroutine
    err = ERR_GLOBAL;
    return 0;
  }
  uint8_t proc_idx = gstk[gstki-1].proc_idx;
  int local_offset = proc.getNumArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = proc.getNumLoc(proc_idx, arg);
    if (local_offset < 0) {
      err = ERR_ASTKOF;
      return 0;
    }
    is_local = true;
  }
  return local_offset;
}

static num_t& BASIC_FP get_lvar(uint8_t arg)
{
  bool is_local;
  int local_offset = get_num_local_offset(arg, is_local);
  if (err)
    return astk_num[0];	// dummy
  if (is_local) {
    if (astk_num_i + local_offset >= SIZE_ASTK) {
      err = ERR_ASTKOF;
      return astk_num[0];	// dummy
    }
    return astk_num[astk_num_i + local_offset];
  } else {
    uint16_t argc = gstk[gstki-1].num_args;
    return astk_num[astk_num_i - argc + local_offset];
  }
}

void BASIC_FP ilvar() {
  num_t value;
  short index;	// variable index

  index = *cip++;

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;
  value = iexp();
  if (err)
    return;
  num_t &var = get_lvar(index);
  if (err)
    return;
  var = value;
}

int BASIC_FP get_array_dims(int *idxs) {
  int dims = 0;
  while (dims < MAX_ARRAY_DIMS) {
    if (getParam(idxs[dims], 0, 255, I_NONE))
      return -1;
    dims++;
    if (*cip == I_CLOSE)
      break;
    if (*cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
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
  bool is_string;
  uint8_t index;

  if (*cip != I_VARARR && *cip != I_STRARR) {
    SYNTAX_T("array variable");
    return;
  }
  is_string = *cip == I_STRARR;
  ++cip;

  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims < 0)
    return;
  if (dims == 0) {
    SYNTAX_T("at least one dimension");
    return;
  }

  // BASIC convention: reserve one more element than specified
  for (int i = 0; i < dims; ++i)
    idxs[i]++;

  if ((!is_string && num_arr.var(index).reserve(dims, idxs)) ||
      (is_string  && str_arr.var(index).reserve(dims, idxs))) {
    err = ERR_OOM;
    return;
  }

  if (*cip == I_EQ) {
    // Initializers
    if (dims > 1) {
      err = ERR_NOT_SUPPORTED;
    } else {
      if (is_string) {
        BString svalue;
        int cnt = 0;
        do {
          cip++;
          svalue = istrexp();
          if (err)
            return;
          BString &s = str_arr.var(index).var(&cnt);
          if (err)
            return;
          s = svalue;
          cnt++;
        } while(*cip == I_COMMA);
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
  }

  return;
}

// Numeric array variable assignment handler
void BASIC_FP ivararr() {
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
  cip++;
  //値の取得と代入
  value = iexp(); //式の値を取得
  if (err) //もしエラーが生じたら
    return;  //終了
  num_t &n = num_arr.var(index).var(idxs);
  if (err)
    return;
  n = value;
}

static int get_str_local_offset(uint8_t arg, bool &is_local)
{
  is_local = false;
  if (!gstki) {
    // not in a subroutine
    err = ERR_GLOBAL;
    return 0;
  }
  uint8_t proc_idx = gstk[gstki-1].proc_idx;
  int local_offset = proc.getStrArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = proc.getStrLoc(proc_idx, arg);
    if (local_offset < 0) {
      err = ERR_ASTKOF;
      return 0;
    }
    is_local = true;
  }
  return local_offset;
}

static BString& get_lsvar(uint8_t arg)
{
  bool is_local;
  int local_offset = get_str_local_offset(arg, is_local);
  if (err)
    return astk_str[0];	// dummy
  if (is_local) {
    if (astk_str_i + local_offset >= SIZE_ASTK) {
      err = ERR_ASTKOF;
      return astk_str[0];	// dummy
    }
    return astk_str[astk_str_i + local_offset];
  } else {
    uint16_t argc = gstk[gstki-1].str_args;
    return astk_str[astk_str_i - argc + local_offset];
  }
}

void set_svar(bool is_lsvar) {
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
    if (is_lsvar) {
      BString &str = get_lsvar(index);
      if (err)
        return;
      str[offset] = sval;
    } else {
      svar.var(index)[offset] = sval;
    }

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
  if (is_lsvar) {
    BString &str = get_lsvar(index);
    if (err)
      return;
    str = value;
  } else {
    svar.var(index) = value;
  }
}

void isvar() {
  set_svar(false);
}

void ilsvar() {
  set_svar(true);
}

// String array variable assignment handler
void istrarr() {
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

// String list variable assignment handler
void istrlst() {
  BString value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims != 1) {
    SYNTAX_T("one dimension");
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

  BString &s = str_lst.var(index).var(idxs[0]);
  if (err)
    return;
  s = value;
}

// Numeric list variable assignment handler
void inumlst() {
  num_t value;
  int idxs[MAX_ARRAY_DIMS];
  int dims = 0;
  uint8_t index;
  
  index = *cip++;

  dims = get_array_dims(idxs);
  if (dims != 1) {
    SYNTAX_T("one dimension");
    return;
  }

  if (*cip != I_EQ) {
    err = ERR_VWOEQ;
    return;
  }
  cip++;

  value = iexp();
  if (err)
    return;

  num_t &s = num_lst.var(index).var(idxs[0]);
  if (err)
    return;
  s = value;
}

void iappend() {
  uint8_t index;
  if (*cip == I_STRLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    BString value = istrexp();
    if (err)
      return;
    str_lst.var(index).append(value);
  } else if (*cip == I_NUMLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    num_t value = iexp();
    if (err)
      return;
    num_lst.var(index).append(value);
  } else {
    SYNTAX_T("list reference");
  }
  return;
}

void iprepend() {
  uint8_t index;
  if (*cip == I_STRLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    BString value = istrexp();
    if (err)
      return;
    str_lst.var(index).prepend(value);
  } else if (*cip == I_NUMLSTREF) {
    index = *++cip;
    if (*++cip != I_COMMA) {
      E_SYNTAX(I_COMMA);
      return;
    }
    ++cip;
    num_t value = iexp();
    if (err)
      return;
    num_lst.var(index).prepend(value);
  } else {
    SYNTAX_T("list reference");
  }
  return;
}

static inline bool end_of_statement()
{
  return *cip == I_EOL || *cip == I_COLON || *cip == I_ELSE || *cip == I_IMPLICITENDIF;
}

int BASIC_FP token_size(uint8_t *code) {
  switch (*code) {
  case I_STR:
    return code[1] + 2;
  case I_NUM:
    return sizeof(num_t) + 1;
  case I_HEXNUM:
    return 5;
  case I_LVAR:
  case I_VAR:
  case I_LSVAR:
  case I_SVAR:
  case I_VARARR:
  case I_NUMLST:
  case I_NUMLSTREF:
  case I_STRARR:
  case I_STRLST:
  case I_STRLSTREF:
  case I_CALL:
  case I_FN:
  case I_PROC:
  case I_LABEL:
    return 2;
  case I_EOL:
  case I_REM:
  case I_SQUOT:
    return -1;
  case I_GOTO:
  case I_GOSUB:
  case I_COMMA:
    if (code[1] == I_LABEL)
      return 3;
    else
      return 1;
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
    scip = sclp + sizeof(line_desc_t);

  while (*scip != tok) {
    next = token_size(scip);
    if (next < 0) {
      sclp += *sclp;
      if (!*sclp)
        return;
      scip = sclp + sizeof(line_desc_t);
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

  for (int i = 0; i < proc.size(); ++i) {
    proc.proc(i).lp = NULL;
  }

  for (;;) {
    find_next_token(&lp, &ip, I_PROC);
    if (!lp)
      return;

    uint8_t proc_id = ip[1];
    ip += 2;

    proc_t &pr = proc.proc(proc_id);

    if (pr.lp) {
      err = ERR_DUPPROC;
      clp = lp; cip = ip;
      return;
    }

    pr.argc_num = 0;
    pr.argc_str = 0;
    pr.profile_total = 0;

    if (*ip == I_OPEN) {
      ++ip;
      do {
        switch (*ip++) {
        case I_VAR:
          if (pr.argc_num >= MAX_PROC_ARGS) {
            err = ERR_ASTKOF;
            clp = lp; cip = ip;
            return;
          }
          pr.args_num[pr.argc_num] = *ip++;
          pr.argc_num++;
          break;
        case I_SVAR:
          if (pr.argc_str >= MAX_PROC_ARGS) {
            err = ERR_ASTKOF;
            clp = lp; cip = ip;
            return;
          }
          pr.args_str[pr.argc_str] = *ip++;
          pr.argc_str++;
          break;
        default:
          SYNTAX_T("variable");
          clp = lp; cip = ip;
          return;
        }
      } while (*ip++ == I_COMMA);

      if (ip[-1] != I_CLOSE) {
        if (ip[-1] == I_COMMA)
          err = ERR_UNDEFARG;
        else
          E_SYNTAX(I_CLOSE);
        clp = lp; cip = ip;
        return;
      }
    }

    pr.lp = lp;
    pr.ip = ip;
  }
}

void initialize_label_pointers(void)
{
  unsigned char *lp, *ip;

  lp = listbuf; ip = NULL;

  for (int i = 0; i < labels.size(); ++i) {
    labels.label(i).lp = NULL;
  }

  for (;;) {
    find_next_token(&lp, &ip, I_LABEL);
    if (!lp)
      return;

    uint8_t label_id = ip[1];
    ip += 2;

    label_t &lb = labels.label(label_id);
    if (lb.lp) {
      err = ERR_DUPLABEL;
      clp = lp; cip = ip;
      return;
    }

    lb.lp = lp;
    lb.ip = ip;
    printf("lbas %d <- %p %p\r\n", label_id, lp, ip);
  }
}

unsigned char *data_ip;
unsigned char *data_lp;
bool in_data = false;

bool find_next_data() {
  int next;

  if (!data_lp) {
    if (*listbuf)
      data_lp = listbuf;
    else
      return false;
  }
  if (!data_ip) {
    data_ip = data_lp + sizeof(line_desc_t);
  }
  
  while (*data_ip != I_DATA && (!in_data || *data_ip != I_COMMA)) {
    in_data = false;
    next = token_size(data_ip);
    if (next < 0) {
      data_lp += *data_lp;
      if (!*data_lp)
        return false;
      data_ip = data_lp + sizeof(line_desc_t);
    } else
      data_ip += next;
  }
  in_data = true;
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
      cip = clp + sizeof(line_desc_t);
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

  switch (*cip++) {
  case I_VAR:
    cip_save = cip;
    cip = data_ip + 1;
    value = iexp();
    if (err) {
      clp = data_lp;
      return;
    }
    data_ip = cip;
    cip = cip_save;
    var.var(*cip++) = value;
    break;
    
  case I_VARARR:
  case I_NUMLST:
    {
    bool is_list = cip[-1] == I_NUMLST;
    int idxs[MAX_ARRAY_DIMS];
    int dims = 0;
    
    index = *cip++;
    dims = get_array_dims(idxs);
    if (dims < 0 || (is_list && dims != 1))
      return;

    cip_save = cip;
    cip = data_ip + 1;
    value = iexp();
    if (err) {
      clp = data_lp;
      return;
    }
    data_ip = cip;
    cip = cip_save;

    num_t &n = is_list ?
                  num_lst.var(index).var(idxs[0]) :
                  num_arr.var(index).var(idxs);
    if (err)
      return;
    n = value;
    break;
    }

  case I_SVAR:
    cip_save = cip;
    cip = data_ip + 1;
    svalue = istrexp();
    if (err) {
      clp = data_lp;
      return;
    }
    data_ip = cip;
    cip = cip_save;
    svar.var(*cip++) = svalue;
    break;
    
  case I_STRARR:
  case I_STRLST:
    {
    bool is_list = cip[-1] == I_STRLST;
    int idxs[MAX_ARRAY_DIMS];
    int dims = 0;
    
    index = *cip++;
    dims = get_array_dims(idxs);
    if (dims < 0 || (is_list && dims != 1))
      return;

    cip_save = cip;
    cip = data_ip + 1;
    svalue = istrexp();
    if (err) {
      clp = data_lp;
      return;
    }
    data_ip = cip;
    cip = cip_save;

    BString &s = is_list ?
                    str_lst.var(index).var(idxs[0]) :
                    str_arr.var(index).var(idxs);
    if (err)
      return;
    s = svalue;
    break;
    }

  default:
    SYNTAX_T("variable");
    break;
  }
}

void irestore() {
  data_lp = NULL;
}

// LET handler
void ilet() {
  switch (*cip) { //中間コードで分岐
  case I_VAR: // 変数の場合
    cip++;     // 中間コードポインタを次へ進める
    ivar();    // 変数への代入を実行
    break;

  case I_LVAR:
    cip++;
    ilvar();
    break;

  case I_VARARR:
    cip++;
    ivararr();
    break;

  case I_SVAR:
    cip++;
    isvar();
    break;

  case I_LSVAR:
    cip++;
    ilsvar();
    break;

  case I_STRARR:
    cip++;
    istrarr();
    break;

  case I_STRLST:
    cip++;
    istrlst();
    break;

  case I_NUMLST:
    cip++;
    inumlst();
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

bool event_sprite_enabled;
uint8_t event_sprite_proc_idx;

bool event_error_enabled;
uint32_t event_error_line;
unsigned char *event_error_resume_lp;
unsigned char *event_error_resume_ip;

bool event_play_enabled;
uint8_t event_play_proc_idx[SOUND_CHANNELS];

#define MAX_PADS 3
bool event_pad_enabled;
bool event_pad_proc_idx[MAX_PADS];
int event_pad_last[MAX_PADS];

void inew(uint8_t mode = NEW_ALL);
static void BASIC_FP do_goto(uint32_t line);

// RUN command handler
void BASIC_FP irun(uint8_t* start_clp = NULL, bool cont = false) {
  uint8_t*   lp;     // 行ポインタの一時的な記憶場所
  if (cont) {
    if (!start_clp) {
      clp = cont_clp;
      cip = cont_cip;
    } else {
      clp = start_clp;
      cip = clp + sizeof(line_desc_t);
    }
    goto resume;
  }
  initialize_proc_pointers();
  initialize_label_pointers();
  event_sprite_enabled = false;
  event_error_enabled = false;
  event_error_resume_lp = NULL;
  event_play_enabled = false;
  event_pad_enabled = false;
  memset(event_pad_proc_idx, 0, sizeof(event_pad_proc_idx));

  if (err)
    return;

  gstki = 0;         // GOSUBスタックインデクスを0に初期化
  lstki = 0;         // FORスタックインデクスを0に初期化
  astk_num_i = 0;
  astk_str_i = 0;
  data_lp = data_ip = NULL;
  in_data = false;
  inew(NEW_VAR);

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

    cip = clp + sizeof(line_desc_t);   // 中間コードポインタを行番号の後ろに設定

resume:
    lp = iexe();     // 中間コードを実行して次の行の位置を得る
    if (err) {         // もしエラーを生じたら
      event_error_resume_lp = NULL;
      if (event_error_enabled) {
        retval[0] = err;
        retval[1] = getlineno(clp);
        event_error_enabled = false;
        event_error_resume_lp = clp;
        event_error_resume_ip = cip;
        do_goto(event_error_line);
        // XXX: What about clp = lp below?
      } else if (err == ERR_CTR_C) {
        cont_cip = cip;
        cont_clp = clp;
        return;
      } else {
        cont_cip = cont_clp = NULL;
        return;
      }
    }
    clp = lp;         // 行ポインタを次の行の位置へ移動
  }
}

num_t BASIC_FP imul();

static bool get_range(uint32_t &start, uint32_t &end)
{
  start = 0;
  end = UINT32_MAX;
  if (!end_of_statement()) {
    if (*cip == I_MINUS) {
      // -<num> -> from start to line <num>
      cip++;
      if (getParam(end, I_NONE)) return false;
    } else {
      // <num>, <num>-, <num>-<num>

      // Slight hack: We don't know how to disambiguate between range and
      // minus operator. We therefore skip the +/- part of the expression
      // parser.
      // It is still possible to use an expression containing +/- by
      // enclosing it in parentheses.
      start = imul();
      if (err)
        return false;

      if (*cip == I_MINUS) {
        // <num>-, <num>-<num>
        cip++;
        if (!end_of_statement()) {
          // <num>-<num> -> list specified range
          if (getParam(end, start, UINT32_MAX, I_NONE)) return false;
        } else {
          // <num>- -> list from line <num> to the end
        }
      } else {
        // <num> -> only one line
        end = start;
      }
    }
  }
  return true;
}

// LIST command
void SMALL ilist(uint8_t devno=0, BString *search = NULL) {
  uint32_t lineno;			// start line number
  uint32_t endlineno;	// end line number
  uint32_t prnlineno;			// output line number
  unsigned char *lp;

  if (!get_range(lineno, endlineno))
    return;

  // Skip until we reach the start line.
  for ( lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) ;

  //リストを表示する
  while (*lp) {               // 行ポインタが末尾を指すまで繰り返す
    prnlineno = getlineno(lp); // 行番号取得
    if (prnlineno > endlineno) // 表示終了行番号に達したら抜ける
      break;
    if (search) {
      char *l = getLineStr(prnlineno);
      if (!strstr(l, search->c_str())) {
        lp += *lp;
        continue;
      }
    }
    sc0.setColor(COL(LINENUM), COL(BG));
    putnum(prnlineno, 0,devno); // 行番号を表示
    sc0.setColor(COL(FG), COL(BG));
    c_putch(' ',devno);        // 空白を入れる
    putlist(lp,devno);    // 行番号より後ろを文字列に変換して表示
    if (err)                   // もしエラーが生じたら
      break;                   // 繰り返しを打ち切る
    newline(devno);            // 改行
    lp += *lp;               // 行ポインタを次の行へ進める
  }
}

void isearch() {
  BString needle = istrexp();
  if (!err)
    ilist(0, &needle);
}

// Argument 0: all erase, 1: erase only program, 2: erase variable area only
void inew(uint8_t mode) {
  data_ip = data_lp = NULL;
  in_data = false;

  if (mode != NEW_PROG) {
    var.reset();
    svar.reset();
    num_arr.reset();
    num_lst.reset();
    str_arr.reset();
    str_lst.reset();
  }

  // Initialization of variables and arrays
  if (mode == NEW_ALL|| mode == NEW_VAR) {
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
    str_lst_names.deleteDirect();
    str_lst.reserve(str_lst_names.varTop());
  }

  // Initialization for execution control
  if (mode != NEW_VAR) {
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
    str_lst_names.deleteAll();
    str_lst.reserve(0);
    proc_names.deleteAll();
    proc.reserve(0);
    label_names.deleteAll();
    labels.reserve(0);

    gstki = 0; //GOSUBスタックインデクスを0に初期化
    lstki = 0; //FORスタックインデクスを0に初期化
    astk_num_i = 0;
    astk_str_i = 0;

    if (listbuf)
      free(listbuf);
    // XXX: Should we be more generous here to avoid fragmentation?
    listbuf = (unsigned char *)malloc(1);
    if (!listbuf) {
      err = ERR_OOM;
      size_list = 0;
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
    cip += sizeof(line_desc_t);
    if (*cip == I_COMMA) {
      cip++;                          // カンマをスキップ
      if (*cip == I_NUM) {            // 増分指定があったら
	increase = getlineno(cip);    // 引数を読み取って増分とする
      } else {
        SYNTAX_T("line number");
	return;
      }
    }
  }

  // 引数の有効性チェック
  cnt = countLines()-1;
  if (startLineNo <= 0 || increase <= 0) {
    err = ERR_RANGE;
    return;
  }
  if (startLineNo + increase * cnt > INT32_MAX) {
    err = ERR_RANGE;
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
	if (ptr[i] == I_NUM) {		// XXX: I_HEXNUM? :)
	  num = getlineno(&ptr[i]);      // 現在の行番号を取得する
	  index = getlineIndex(num);     // 行番号の行インデックスを取得する
	  if (index == INT32_MAX) {
	    // 該当する行が見つからないため、変更は行わない
	    i += sizeof(line_desc_t);
	    continue;
	  } else {
	    // とび先行番号を付け替える
	    newnum = startLineNo + increase*index;
	    line_desc_t *ld = (line_desc_t *)(ptr + i);
	    ld->line = newnum;
	    i += sizeof(line_desc_t);
	    continue;
	  }
	} else if (ptr[i] == I_LABEL) {
	  ++i;
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
    line_desc_t *ld = (line_desc_t *)clp;
    ld->line = newnum;
    index++;
  }
}

void SMALL config_color()
{
  int32_t idx, r, g, b;
  if (getParam(idx, 0, CONFIG_COLS - 1, I_COMMA)) return;
  if (getParam(r, 0, 255, I_COMMA)) return;
  if (getParam(g, 0, 255, I_COMMA)) return;
  if (getParam(b, 0, 255, I_NONE)) return;
  CONFIG.color_scheme[idx][0] = r;
  CONFIG.color_scheme[idx][1] = g;
  CONFIG.color_scheme[idx][2] = b;
}

// CONFIGコマンド
// CONFIG 項目番号,設定値
void SMALL iconfig() {
  int32_t itemNo;
  int32_t value;

  if (*cip == I_COLOR) {
    ++cip;
    config_color();
    return;
  }

  if ( getParam(itemNo, I_COMMA) ) return;
  if ( getParam(value, I_NONE) ) return;
  switch(itemNo) {
  case 0: // NTSC, PAL, PAL60 (XXX: unimplemented)
    if (value <0 || value >2)  {
      E_VALUE(0, 2);
    } else {
      sc0.adjustNTSC(value);
      CONFIG.NTSC = value;
    }
    break;
  case 1: // キーボード補正
    if (value < 0 || value > 2)  {
      E_VALUE(0, 2);
    } else {
      kb.setLayout(value);
      CONFIG.KEYBOARD = value;
    }
    break;
  case 2:
    CONFIG.interlace = value != 0;
    vs23.setInterlace(CONFIG.interlace);
    break;
  case 3:
    CONFIG.lowpass = value != 0;
    vs23.setLowpass(CONFIG.lowpass);
    break;
  case 4:
    if (value < 1 || value >= vs23.numModes())
      E_VALUE(1, vs23.numModes() - 1);
    else
      CONFIG.mode = value;
    break;
  case 5:
    if (value < 0 || value >= NUM_FONTS)
      E_VALUE(0, NUM_FONTS - 1);
    else
      CONFIG.font = value;
    break;
  case 6:
    if (value < 0 || value > 255)
      E_VALUE(0, 255);
    else
      CONFIG.cursor_color = value;
    break;
  case 7:
    if (value < 0 || value > 15)
      E_VALUE(0, 15);
    else
      CONFIG.beep_volume = value;
    break;
  default:
    E_VALUE(0, 7);
    break;
  }
}

void iloadconfig() {
  loadConfig();
}

void isavebg();
void isavepcx();

// "SAVE <file name>" or "SAVE BG ..."
void isave() {
  BString fname;
  int8_t rc;

  if (*cip == I_BG) {
    isavebg();
    return;
  } else if (*cip == I_PCX) {
    ++cip;
    isavepcx();
    return;
  } else if (*cip == I_CONFIG) {
    ++cip;
    isaveconfig();
    return;
  }

  if(!(fname = getParamFname())) {
    return;
  }

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
}

// テキスト形式のプログラムのロード
// 引数
//   fname  :  ファイル名
//   newmode:  初期化モード 0:初期化する 1:変数を初期化しない 2:追記モード
// 戻り値
//   0:正常終了
//   1:異常終了
uint8_t SMALL loadPrgText(char* fname, uint8_t newmode = NEW_ALL) {
  int32_t len;
  uint8_t rc = 0;
  
  cont_clp = cont_cip = NULL;
  proc.reset();
  labels.reset();

  err = bfs.tmpOpen(fname,0);
  if (err)
    return 1;

  if (newmode != NEW_VAR)
    inew(newmode);
  while(bfs.readLine(lbuf)) {
    tlimR(lbuf);  // 2017/07/31 追記
    len = toktoi();
    if (err) {
      c_puts(lbuf);
      newline();
      rc = 1;
      break;
    }
    if (*ibuf == I_NUM) {
      *ibuf = len;
      inslist();
      if (err) {
	error(true);
	rc = 1;
	break;
      }
    }
  }
  recalc_indent();
  bfs.tmpClose();
  return rc;
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
  labels.reset();

  uint32_t current_line = getlineno(clp);

  if (!get_range(sNo, eNo))
    return;
  if (!end_of_statement()) {
    SYNTAX_T("end of statement");
    return;
  }

  if (eNo < sNo) {
    err = ERR_RANGE;
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
  
  // continue on the next line, in the likely case the DELETE command didn't
  // delete itself
  clp = getlp(current_line + 1);
  cip = clp + sizeof(line_desc_t);
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

void SdFormat();
void iformat() {
#ifdef UNIFILE_USE_SPIFFS
  BString target = getParamFname();
  if (err)
    return;

  if (target == BString(F("/flash"))) {
    PRINT_P("This will ERASE ALL DATA on the internal flash file system!\n");
    PRINT_P("ARE YOU SURE? (Y/N) ");
    BString answer = getstr();
    if (answer != "Y" && answer != "y") {
      PRINT_P("Aborted\n");
      return;
    }
    PRINT_P("Formatting... ");
    if (SPIFFS.format())
      PRINT_P("Success!\n");
    else
      PRINT_P("Failed.");
  } else if (target == "/sd") {
    SdFormat();
  } else {
    err = ERR_NOT_SUPPORTED;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// 画面クリア
void icls() {
  sc0.cls();
  sc0.locate(0,0);
}

static bool profile_enabled;

void BASIC_FP init_stack_frame()
{
  if (gstki > 0) {
    struct proc_t &p = proc.proc(gstk[gstki-1].proc_idx);
    astk_num_i += p.locc_num;
    astk_str_i += p.locc_str;
  }
  gstk[gstki].num_args = 0;
  gstk[gstki].str_args = 0;
}

void BASIC_FP push_num_arg(num_t n)
{
  if (astk_num_i >= SIZE_ASTK) {
    err = ERR_ASTKOF;
    return;
  }
  astk_num[astk_num_i++] = n;
  gstk[gstki].num_args++;
}

void BASIC_FP do_call(uint8_t proc_idx)
{
  struct proc_t &proc_loc = proc.proc(proc_idx);

  if (!proc_loc.lp || !proc_loc.ip) {
    err = ERR_UNDEFPROC;
    return;
  }
  
  if (gstki >= SIZE_GSTK) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
    return;
  }

  gstk[gstki].lp = clp;
  gstk[gstki].ip = cip;
  gstk[gstki++].proc_idx = proc_idx;

  clp = proc_loc.lp;
  cip = proc_loc.ip;
  
  if (profile_enabled) {
    proc_loc.profile_current = ESP.getCycleCount();
  }
  return;
}

#ifdef VS23_BG_ENGINE
void BASIC_INT event_handle_sprite()
{
  uint8_t dir;
  for (int i = 0; i < VS23_MAX_SPRITES; ++i) {
    if (!vs23.spriteEnabled(i))
      continue;
    for (int j = i+1; j < VS23_MAX_SPRITES; ++j) {
      if (!vs23.spriteEnabled(j))
        continue;
      if ((dir = vs23.spriteCollision(i, j))) {
        init_stack_frame();
        push_num_arg(i);
        push_num_arg(j);
        push_num_arg(dir);
        do_call(event_sprite_proc_idx);
        event_sprite_enabled = false;	// prevent interrupt storms
        return;
      }
    }
  }
}
#endif

void BASIC_INT event_handle_play(int ch)
{
  init_stack_frame();
  push_num_arg(ch);
  do_call(event_play_proc_idx[ch]);
  event_play_enabled = false;
}

void event_handle_pad();

#define EVENT_PROFILE_SAMPLES 7
uint32_t event_profile[EVENT_PROFILE_SAMPLES];

void BASIC_INT draw_profile(void)
{
  int x = 0;
  int bw = vs23.borderWidth();
  int scale = 1000000/60/bw + 1;

  for (int i = 1; i < EVENT_PROFILE_SAMPLES; ++i) {
    int pixels = (event_profile[i] - event_profile[i-1]) / scale;
    if (x + pixels > bw)
      pixels = bw - x;
    if (pixels > 0)
      vs23.setBorder(0x70, (i * 0x4c) % 256, x, pixels);
    x += pixels;
  }

  if (x < bw)
    vs23.setBorder(0x20, 0, x, bw - x);
}


void BASIC_FP pump_events(void)
{
  static uint32_t last_frame;

  if (vs23.frame() == last_frame) {
#ifdef HAVE_TSF
    if (sound.needSamples())
      sound.render();
#endif
    return;
  }

  last_frame = vs23.frame();

  event_profile[0] = micros();
#ifdef VS23_BG_ENGINE
  vs23.updateBg();
#endif

  event_profile[1] = micros();
#ifdef HAVE_TSF
  if (sound.needSamples())
    sound.render();
#endif
  sound.pumpEvents();
  event_profile[2] = micros();
#ifdef HAVE_MML
  if (event_play_enabled) {
    for (int i = 0; i < SOUND_CHANNELS; ++i) {
      if (sound.isFinished(i))
        event_handle_play(i);
    }
  }
#endif
  event_profile[3] = micros();

  sc0.updateCursor();
  event_profile[4] = micros();
  
#ifdef VS23_BG_ENGINE
  if (event_sprite_enabled)
    event_handle_sprite();
#endif
  event_profile[5] = micros();
  if (event_pad_enabled)
    event_handle_pad();
  event_profile[6] = micros();
  
  if (profile_enabled)
    draw_profile();

#ifndef ESP8266_NOWIFI
  yield();
#endif
}

// 時間待ち
void iwait() {
  int32_t tm;
  if ( getParam(tm, 0, INT32_MAX, I_NONE) ) return;
  uint32_t end = tm + millis();
  while (millis() < end) {
    pump_events();
    if (sc0.peekKey() == SC_KEY_CTRL_C) {
      err = ERR_CTR_C;
      break;
    }
  }
}

void ivsync() {
  uint32_t tm;
  if (end_of_statement())
    tm = vs23.frame() + 1;
  else
    if ( getParam(tm, 0, INT32_MAX, I_NONE) )
      return;

  while (vs23.frame() < tm) {
    pump_events();
    if (sc0.peekKey() == SC_KEY_CTRL_C) {
      err = ERR_CTR_C;
      break;
    }
  }
}

static const uint8_t vs23_write_regs[] PROGMEM = {
  0x01, 0x82, 0xb8, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x34, 0x35
};

void ivreg() {
  int32_t opcode;
  int vals;

  if (getParam(opcode, 0, 255, I_COMMA)) return;

  bool good = false;
  for (uint32_t i = 0; i < sizeof(vs23_write_regs); ++i) {
    if (pgm_read_byte(&vs23_write_regs[i]) == opcode) {
      good = true;
      break;
    }
  }
  if (!good) {
    err = ERR_VALUE;
    return;
  }

  switch (opcode) {
  case BLOCKMVC1:	vals = 3; break;
  case 0x35:		vals = 3; break;
  case PROGRAM:		vals = 2; break;
  default:		vals = 1; break;
  }

  int32_t values[vals];
  for (int i = 0; i < vals; ++i) {
    if (getParam(values[i], 0, 65535, i == vals - 1 ? I_NONE : I_COMMA))
      return;
  }

  switch (opcode) {
  case BLOCKMVC1:
    SpiRamWriteBMCtrl(BLOCKMVC1, values[0], values[1], values[2]);
    break;
  case 0x35:
    SpiRamWriteBM2Ctrl(values[0], values[1], values[2]);
    break;
  case PROGRAM:
    SpiRamWriteProgram(PROGRAM, values[0], values[1]);
    break;
  case WRITE_MULTIIC:
  case WRITE_STATUS:
    SpiRamWriteByteRegister(opcode, values[0]);
    break;
  default:
    SpiRamWriteRegister(opcode, values[0]);
    break;
  }
}

void ivpoke() {
  int32_t addr, value;
  if (getParam(addr, 0, 131071, I_COMMA)) return;
  if (getParam(value, 0, 255, I_NONE)) return;
  SpiRamWriteByte(addr, value);
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

// 文字色の指定 COLOR fc,bc
void icolor() {
  int32_t fc,  bc = 0;
  if ( getParam(fc, 0, 255, I_NONE) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(bc, 0, 255, I_NONE) ) return;
    if (*cip == I_COMMA) {
      ++cip;
      int cc;
      if (getParam(cc, 0, 255, I_NONE)) return;
      sc0.setCursorColor(cc);
    }
  }
  // 文字色の設定
  sc0.setColor((uint16_t)fc, (uint16_t)bc);
}

// キー入力文字コードの取得 INKEY()関数
int32_t BASIC_FP iinkey() {
  int32_t rc = 0;

  if (c_kbhit()) {
    rc = sc0.tryGetChar();
    if (rc == SC_KEY_CTRL_C)
      err = ERR_CTR_C;
  }

  return rc;
}

// メモリ参照　PEEK(adr[,bnk])
int32_t ipeek(int type) {
  int32_t value = 0, vadr;
  void* radr;

  if (checkOpen()) return 0;
  if ( getParam(vadr, I_NONE) ) return 0;
  if (checkClose()) return 0;
  radr = sanitize_addr(vadr, type);
  if (radr) {
    switch (type) {
    case 0: value = *(uint8_t*)radr; break;
    case 1: value = *(uint16_t*)radr; break;
    case 2: value = *(uint32_t*)radr; break;
    default: err = ERR_SYS; break;
    }
  }
  else
    err = ERR_RANGE;
  return value;
}

num_t BASIC_FP nret() {
  int32_t r;

  if (checkOpen()) return 0;
  if ( getParam(r, 0, MAX_RETVALS-1, I_NONE) ) return 0;
  if (checkClose()) return 0;

  return retval[r];
}

num_t BASIC_FP narg() {
  int32_t a;
  if (astk_num_i == 0) {
    err = ERR_UNDEFARG;
    return 0;
  }
  uint16_t argc = gstk[gstki-1].num_args;

  if (checkOpen()) return 0;
  if ( getParam(a, 0, argc-1, I_NONE) ) return 0;
  if (checkClose()) return 0;

  return astk_num[astk_num_i-argc+a];
}

BString sarg() {
  int32_t a;
  if (astk_str_i == 0) {
    err = ERR_UNDEFARG;
    return BString();
  }
  uint16_t argc = gstk[gstki-1].str_args;

  if (checkOpen()) return BString();
  if ( getParam(a, 0, argc-1, I_NONE) ) return BString();
  if (checkClose()) return BString();

  return BString(astk_str[astk_str_i-argc+a]);
}

num_t BASIC_FP nargc() {
  int32_t type = getparam();
  if (!gstki)
    return 0;
  if (type == 0)
    return gstk[gstki-1].num_args;
  else
    return gstk[gstki-1].str_args;
}

// スクリーン座標の文字コードの取得 'CHAR(X,Y)'
int32_t BASIC_FP ncharfun() {
  int32_t value; // 値
  int32_t x, y;  // 座標

  if (checkOpen()) return 0;
  if ( getParam(x, I_COMMA) ) return 0;
  if ( getParam(y, I_NONE) ) return 0;
  if (checkClose()) return 0;
  value = (x < 0 || y < 0 || x >=sc0.getWidth() || y >=sc0.getHeight()) ? 0 : sc0.vpeek(x, y);
  return value;
}

void BASIC_FP ichar() {
  int32_t x, y, c;
  if ( getParam(x, I_COMMA) ) return;
  if ( getParam(y, I_COMMA) ) return;
  if ( getParam(c, I_NONE) ) return;
  sc0.write(x, y, c);
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

#ifdef DEBUG_GPIO
  int ret = 
#endif
  Wire.endTransmission();
#ifdef DEBUG_GPIO
  Serial.printf("wire st %d pcf 0x%x\n", ret, pcf_state);
#endif
}

BString shex() {
  int value; // 値

  if (checkOpen()) goto out;
  if (getParam(value, I_NONE)) goto out;
  if (checkClose()) goto out;
  return BString(value, 16);
out:
  return BString();
}

// 2進数出力 'BIN$(数値, 桁数)' or 'BIN$(数値)'
BString sbin() {
  int32_t value; // 値

  if (checkOpen()) goto out;
  if (getParam(value, I_NONE)) goto out;
  if (checkClose()) goto out;
  return BString(value, 2);
out:
  return BString();
}

// POKEコマンド POKE ADR,データ[,データ,..データ]
void BASIC_FP do_poke(int type) {
  void* adr;
  int32_t value;
  int32_t vadr;

  // アドレスの指定
  vadr = iexp(); if(err) return;
  if(*cip != I_COMMA) { E_SYNTAX(I_COMMA); return; }

  // 例: 1,2,3,4,5 の連続設定処理
  do {
    adr = sanitize_addr(vadr, type);
    if (!adr) {
      err = ERR_RANGE;
      break;
    }
    cip++;          // 中間コードポインタを次へ進める
    if (getParam(value, I_NONE)) return;
    switch (type) {
    case 0: *((uint8_t*)adr) = value; break;
    case 1: *((uint16_t *)adr) = value; break;
    case 2: *((uint32_t *)adr) = value; break;
    default: err = ERR_SYS; break;
    }
    vadr++;
  } while(*cip == I_COMMA);
}

void BASIC_FP ipoke() {
  do_poke(0);
}
void BASIC_FP ipokew() {
  do_poke(1);
}
void BASIC_FP ipoked() {
  do_poke(2);
}

void isys() {
  void (*sys)() = (void (*)())(uintptr_t)iexp();
  sys();
}

num_t BASIC_INT ni2cw() {
  int32_t i2cAdr;
  BString out;

  if (checkOpen()) return 0;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA)) return 0;
  out = istrexp();
  if (checkClose()) return 0;

  // I2Cデータ送信
  Wire.beginTransmission(i2cAdr);
  if (out.length()) {
    for (uint32_t i = 0; i < out.length(); i++)
      Wire.write(out[i]);
  }
  return Wire.endTransmission();
}

BString si2cr() {
  int32_t i2cAdr, rdlen;
  BString in, out;

  if (checkOpen()) goto out;
  if (getParam(i2cAdr, 0, 0x7f, I_COMMA)) goto out;
  out = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }
  if (getParam(rdlen, 0, INT32_MAX, I_CLOSE)) goto out;

  // I2Cデータ送受信
  Wire.beginTransmission(i2cAdr);

  // 送信
  if (out.length()) {
    Wire.write(out.c_str(), out.length());
  }
  if ((retval[0] = Wire.endTransmission()))
    goto out;
  Wire.requestFrom(i2cAdr, rdlen);
  while (Wire.available()) {
    in += Wire.read();
  }
out:
  return in;
}

// SETDATEコマンド  SETDATE 年,月,日,時,分,秒
void isetDate() {
#ifdef USE_INNERRTC
  int32_t p_year, p_mon, p_day;
  int32_t p_hour, p_min, p_sec;

  if ( getParam(p_year, 1900,2036, I_COMMA) ) return;  // 年
  if ( getParam(p_mon,     1,  12, I_COMMA) ) return;  // 月
  if ( getParam(p_day,     1,  31, I_COMMA) ) return;  // 日
  if ( getParam(p_hour,    0,  23, I_COMMA) ) return;  // 時
  if ( getParam(p_min,     0,  59, I_COMMA) ) return;  // 分
  if ( getParam(p_sec,     0,  61, I_NONE)) return;  // 秒

  setTime(p_hour, p_min, p_sec, p_day, p_mon, p_year);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// GETDATEコマンド  SETDATE 年格納変数,月格納変数, 日格納変数, 曜日格納変数
void igetDate() {
#ifdef USE_INNERRTC
  int16_t index;
  time_t tt = now();

  int v[] = {
    year(tt),
    month(tt),
    day(tt),
    weekday(tt),
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
  err = ERR_NOT_SUPPORTED;
#endif
}

// GETDATEコマンド  SETDATE 時格納変数,分格納変数, 秒格納変数
void igetTime() {
#ifdef USE_INNERRTC
  int16_t index;
  time_t tt = now();

  int v[] = {
    hour(tt),
    minute(tt),
    second(tt),
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
  err = ERR_NOT_SUPPORTED;
#endif
}

// DATEコマンド
void idate() {
#ifdef USE_INNERRTC
  time_t tt = now();

  putnum(year(tt), -4);
  c_putch('/');
  putnum(month(tt), -2);
  c_putch('/');
  putnum(day(tt), -2);
  PRINT_P(" [");
  switch (weekday(tt)) {
  case 1: PRINT_P("Sun"); break;
  case 2: PRINT_P("Mon"); break;
  case 3: PRINT_P("Tue"); break;
  case 4: PRINT_P("Wed"); break;
  case 5: PRINT_P("Thu"); break;
  case 6: PRINT_P("Fri"); break;
  case 7: PRINT_P("Sat"); break;
  };
  PRINT_P("] ");
  putnum(hour(tt), -2);
  c_putch(':');
  putnum(minute(tt), -2);
  c_putch(':');
  putnum(second(tt), -2);
  newline();
#else
  err = ERR_NOT_SUPPORTED;
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
  if (y >= vs23.lastLine()) y = vs23.lastLine()-1;
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
  if (y1 >= vs23.lastLine()) y1 = vs23.lastLine()-1;
  if (x2 >= sc0.getGWidth()) x2 = sc0.getGWidth()-1;
  if (y2 >= vs23.lastLine()) y2 = vs23.lastLine()-1;
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
  if (y >= vs23.lastLine()) y = vs23.lastLine()-1;
  if (r < 0) r = -r;
  sc0.circle(x, y, r, c, f);
}

// 四角の描画 RECT X1,Y1,X2,Y2,C,F
void irect() {
  int32_t x1,y1,x2,y2,c,f;
  if (getParam(x1, I_COMMA)||getParam(y1, I_COMMA)||getParam(x2, I_COMMA)||getParam(y2, I_COMMA)||getParam(c, I_COMMA)||getParam(f, I_NONE))
    return;
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 || x2 >= sc0.getGWidth() || y2 >= vs23.lastLine())  {
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
      x2 >= sc0.getGWidth() || y2 >= vs23.lastLine()) {
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

void ibeep() {
  int32_t period;
  int32_t vol = CONFIG.beep_volume;

  if ( getParam(period, 0, I2S_BUFLEN, I_NONE) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(vol, 0, 15, I_NONE) ) return;
  }
  if (period == 0)
    sound.noBeep();
  else
    sound.beep(period, vol);
}

void isound() {
#ifdef HAVE_TSF
  if (*cip == I_FONT) {
    ++cip;
    BString font = getParamFname();
    sound.setFontName(font);
  } else {
    int32_t ch, inst, note, len = 1000;
    num_t vel = 1;
    if (getParam(ch, 0, SOUND_CHANNELS - 1, I_COMMA)) return;
    if (getParam(inst, 0, sound.instCount() - 1, I_COMMA)) return;
    if (getParam(note, 0, INT32_MAX, I_NONE)) return;
    if (*cip == I_COMMA) {
      ++cip;
      if (getParam(len, 0, INT32_MAX, I_NONE)) return;
      if (*cip == I_COMMA) {
        ++cip;
        if (getParam(vel, 0, 1.0, I_NONE)) return;
      }
    }
    sound.noteOn(ch, inst, note, vel, len);
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

#ifdef HAVE_MML
BString mml_text;
#endif
void iplay() {
#ifdef HAVE_MML
  sound.stopMml(0);
  mml_text = istrexp();
  sound.playMml(0, mml_text.c_str());
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

// POINT(X,Y)関数の処理
num_t BASIC_FP npoint() {
  int x, y;  // 座標
  if (checkOpen()) return 0;
  if ( getParam(x, 0, sc0.getGWidth()-1, I_COMMA)) return 0;
  if ( getParam(y, 0, vs23.lastLine()-1, I_NONE) ) return 0;
  if (checkClose()) return 0;
  return vs23.getPixel(x,y);
}

// MAP(V,L1,H1,L2,H2)関数の処理
num_t BASIC_FP SMALL nmap() {
  int32_t value,l1,h1,l2,h2,rc;
  if (checkOpen()) return 0;
  if ( getParam(value, I_COMMA)||getParam(l1, I_COMMA)||getParam(h1, I_COMMA)||getParam(l2, I_COMMA)||getParam(h2, I_NONE) )
    return 0;
  if (checkClose()) return 0;
  if (l1 >= h1 || l2 >= h2) {
    err = ERR_RANGE;
    return 0;
  } else if (value < l1 || value > h1) {
    E_VALUE(l1, h1);
  }
  rc = (value-l1)*(h2-l2)/(h1-l1)+l2;
  return rc;
}

// ASC(文字列)
num_t BASIC_FP nasc() {
  int32_t value;

  if (checkOpen()) return 0;
  value = istrexp()[0];
  checkClose();

  return value;
}

// PRINT handler
void iprint(uint8_t devno=0,uint8_t nonewln=0) {
  num_t value;     //値
  int32_t filenum;
  BString str;
  BString format = F("%0.9g");	// default format without USING

  while (!end_of_statement()) {
    if (is_strexp()) {
      str = istrexp();
      c_puts(str.c_str(), devno);
    } else  switch (*cip) { //中間コードで分岐
    case I_USING: { //「#
      cip++;
      str = istrexp();
      bool had_point = false;	// encountered a decimal point
      int leading = 0;
      int trailing = 0;		// decimal places
      BString prefix, suffix;	// random literal characters
      for (unsigned int i = 0; i < str.length(); ++i) {
        switch (str[i]) {
        case '#': if (!had_point) leading++; else trailing++; break;
#ifdef FLOAT_NUMS
        case '.':
          if (had_point) {
            E_ERR(USING, "single period");
            return;
          } else
            had_point = true;
          break;
#endif
        case '%': if (!had_point) prefix += F("%%"); else suffix += F("%%"); break;
        default:  if (!had_point) prefix += str[i]; else suffix += str[i]; break;
        }
      }
#ifdef FLOAT_NUMS
      format = prefix + '%' + (trailing + leading + (trailing ? 1 : 0)) + '.' + trailing + 'f' + suffix;
#else
      format = prefix + '%' + leading + 'd' + suffix;
#endif
      if (err || *cip != I_SEMI)
	return;
      break;
    }
    case I_SHARP:
      cip++;
      if (getParam(filenum, 0, MAX_USER_FILES, I_COMMA))
        return;
      if (!user_files[filenum] || !*user_files[filenum]) {
        err = ERR_FILE_NOT_OPEN;
        return;
      }
      bfs.setTempFile(*user_files[filenum]);
      devno = 4;
      break;
      
    default:	// anything else is assumed to be a numeric expression
      value = iexp();
      if (err) {
	newline();
	return;
      }
      sprintf(lbuf, format.c_str(), value);
      c_puts(lbuf, devno);
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
    } else if (*cip == I_COMMA) {
      cip++;
      while (sc0.c_x() % 8)
        c_putch(' ');
      if (end_of_statement())
        return;
    } else if (*cip == I_SEMI) {
      cip++;
      if (end_of_statement())
	return;
    } else {
      if (!end_of_statement()) {
        SYNTAX_T("separator");
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
    if ( getParam(y, 0, vs23.lastLine(), I_COMMA) ) return;
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

void SMALL isavepcx() {
  BString fname;
  int32_t x = 0,y = 0;
  int32_t w = sc0.getGWidth();
  int32_t h = sc0.getGHeight();

  if(!(fname = getParamFname())) {
    return;
  }

  for (;;) {
    if (*cip == I_POS) {
      if (getParam(x, 0, sc0.getGWidth() - 1, I_COMMA)) return;
      if (getParam(y, 0, vs23.lastLine() - 1, I_NONE)) return;
    } else if (*cip == I_SIZE) {
      if (getParam(w, 0, sc0.getGWidth() - x - 1, I_COMMA)) return;
      if (getParam(h, 0, vs23.lastLine() - y - 1, I_NONE)) return;
    }
  }
  
  err = bfs.saveBitmap((char *)fname.c_str(), x, y, w, h);
  return;
}

void SMALL ildbmp() {
  BString fname;
  int32_t dx = -1, dy = -1;
  int32_t x = 0,y = 0,w = -1, h = -1;
#ifdef VS23_BG_ENGINE
  uint32_t spr_from, spr_to;
  bool define_bg = false, define_spr = false;
  int bg;
#endif
  int32_t key = -1;	// no keying

  if(!(fname = getParamFname())) {
    return;
  }

  for (;; ) {
    if (*cip == I_AS) {
#ifdef VS23_BG_ENGINE
      cip++;
      if (*cip == I_BG) {		// AS BG ...
        ++cip;
        dx = dy = -1;
        if (getParam(bg,  0, VS23_MAX_BG-1, I_NONE)) return;
        define_bg = true;
      } else if (*cip == I_SPRITE) {	// AS SPRITE ...
        ++cip;
        dx = dy = -1;
        define_spr = true;
        if (!get_range(spr_from, spr_to)) return;
        if (spr_to == UINT32_MAX)
          spr_to = VS23_MAX_SPRITES - 1;
        if (spr_to > VS23_MAX_SPRITES-1 || spr_from > spr_to) {
          err = ERR_RANGE;
          return;
        }
      } else {
        SYNTAX_T("BG or SPRITE");
	return;
      }
#else
      err = ERR_NOT_SUPPORTED;
      return;
#endif
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
    } else if (*cip == I_KEY) {
      // KEY c
      cip++;
      if (getParam(key, 0, 255, I_NONE)) return;
    } else {
      break;
    }
  }

  // 画像のロード
  err = bfs.loadBitmap((char *)fname.c_str(), dx, dy, x, y, w, h, key);
  if (!err) {
#ifdef VS23_BG_ENGINE
    if (define_bg)
      vs23.setBgPattern(bg, dx, dy, w / vs23.bgTileSizeX(bg));
    if (define_spr) {
      for (uint32_t i = spr_from; i < spr_to + 1; ++i) {
        vs23.setSpritePattern(i, dx, dy);
      }
    }
#endif
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

Unifile user_dir;
void iopendir()
{
  BString fname = Unifile::cwd();

  if (is_strexp() && !(fname = getParamFname())) {
    return;
  }

  user_dir.close();
  user_dir = Unifile::openDir(fname.c_str());
  if (!user_dir)
    err = ERR_FILE_OPEN;
}

// RENAME <old$> TO <new$>
void irename() {
  uint8_t rc;

  BString old_fname = getParamFname();
  if (err)
    return;
  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
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

void icopy() {
  uint8_t rc;

  BString old_fname = getParamFname();
  if (err)
    return;
  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
    return;
  }
  cip++;
  BString new_fname = getParamFname();
  if (err)
    return;

  rc = bfs.fcopy(old_fname.c_str(), new_fname.c_str());
  if (rc)
    err = ERR_FILE_WRITE;
}

// BSAVE "ファイル名", アドレス
void SMALL ibsave() {
  uint32_t vadr, len;
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }
  cip++;
  if ( getParam(vadr, I_COMMA) ) return;  // アドレスの取得
  if ( getParam(len, I_NONE) ) return;             // データ長の取得

  // アドレスの範囲チェック
  if ( !sanitize_addr(vadr, 0) || !sanitize_addr(vadr + len, 0) ) {
    err = ERR_RANGE;
    return;
  }

  // ファイルオープン
  rc = bfs.tmpOpen((char *)fname.c_str(),1);
  if (rc) {
    err = rc;
    return;
  }

  if ((vadr & 3) == 0 && (len & 3) == 0) {
    // データの書込み
    for (uint32_t i = 0; i < len; i += 4) {
      uint32_t c = *(uint32_t *)vadr;
      if(bfs.tmpWrite((char *)&c, 4)) {
        err = ERR_FILE_WRITE;
        goto DONE;
      }
      vadr += 4;
    }
  } else {
    // データの書込み
    for (uint32_t i = 0; i < len; i++) {
      if(bfs.putch(*(uint8_t *)vadr)) {
        err = ERR_FILE_WRITE;
        goto DONE;
      }
      vadr++;
    }
  }

DONE:
  bfs.tmpClose();
}

void SMALL ibload() {
  uint32_t vadr;
  ssize_t len = -1;
  int32_t c;
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

  if (*cip != I_TO) {
    E_SYNTAX(I_TO);
    return;
  }
  cip++;
  if ( getParam(vadr, I_NONE) ) return;  // アドレスの取得
  if (*cip == I_COMMA) {
    cip++;
    if ( getParam(len, I_NONE) ) return;              // データ長の取得
  }

  // ファイルオープン
  rc = bfs.tmpOpen((char *)fname.c_str(),0);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err =  ERR_FILE_OPEN;
    return;
  }
  
  if (len == -1)
    len = bfs.tmpSize();

  // アドレスの範囲チェック
  if ( !sanitize_addr(vadr, 0) || !sanitize_addr(vadr + len, 0) ) {
    err = ERR_RANGE;
    goto DONE;
  }

  if ((vadr & 3) == 0 && (len & 3) == 0) {
    for (ssize_t i = 0; i < len; i += 4) {
      uint32_t c;
      if (bfs.tmpRead((char *)&c, 4) < 0) {
        err = ERR_FILE_READ;
        goto DONE;
      }
      *((uint32_t *)vadr) = c;
      vadr += 4;
    }
  } else {
    // データの読込み
    for (ssize_t i = 0; i < len; i++) {
      c = bfs.read();
      if (c <0 ) {
        err = ERR_FILE_READ;
        goto DONE;
      }
      *((uint8_t *)vadr++) = c;
    }
  }

DONE:
  bfs.tmpClose();
  return;
}

// TYPE "ファイル名"
void  itype() {
  //char fname[SD_PATH_LEN];
  //uint8_t rc;
  int32_t line = 0;
  uint8_t c;

  BString fname;
  int8_t rc;

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

#ifdef VS23_BG_ENGINE
  // Discard the dimensions saved for CONTing.
  restore_text_window = false;
#endif

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
  sc0.show_curs(false);
}

void ifont() {
  int32_t idx;
  if (getParam(idx, 0, NUM_FONTS - 1, I_NONE))
    return;
  sc0.setFont(fonts[idx]);
  sc0.forget();
}

// スクリーンモード指定 SCREEN M
void SMALL iscreen() {
  int32_t m;

  if ( getParam(m,  1, vs23.numModes(), I_NONE) ) return;   // m

#ifdef VS23_BG_ENGINE
  // Discard dimensions saved for CONTing.
  restore_bgs = false;
  restore_text_window = false;
#endif

  vs23.reset();

  sc0.setFont(fonts[CONFIG.font]);

  if (scmode == m) {
    sc0.reset();
    sc0.locate(0,0);
    sc0.cls();
    sc0.show_curs(false);
    return;
  }

  sc0.end();
  scmode = m;

  // NTSCスクリーン設定
  sc0.init(SIZE_LINE,CONFIG.NTSC, NULL, m - 1);

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
  int32_t y, uv, x, w;
  if (getParam(uv, 0, 255, I_COMMA)) return;
  if (getParam(y, 0, 255 - 0x66, I_NONE)) return;
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(x, 0, vs23.borderWidth(), I_COMMA)) return;
    if (getParam(w, 0, vs23.borderWidth() - x, I_NONE)) return;
    vs23.setBorder(y, uv, x, w);
  } else
    vs23.setBorder(y, uv);
}

void ibg() {
#ifdef VS23_BG_ENGINE
  int32_t m;
  int32_t w, h, px, py, pw, tx, ty, wx, wy, ww, wh, prio;

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
    if (getParam(ww, 0, sc0.getGWidth() - wx, I_COMMA)) return;
    if (getParam(wh, 0, sc0.getGHeight() - wy, I_NONE)) return;
    vs23.setBgWin(m, wx, wy, ww, wh);
    break;
  case I_ON:
    // XXX: do sanity check before enabling
    vs23.enableBg(m);
    break;
  case I_OFF:
    vs23.disableBg(m);
    break;
  case I_PRIO:
    if (getParam(prio, 0, VS23_MAX_PRIO, I_NONE)) return;
    vs23.setBgPriority(m, prio);
    break;
  default:
    cip--;
    if (!end_of_statement())
      SYNTAX_T("BG parameter");
    return;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void iloadbg() {
#ifdef VS23_BG_ENGINE
  int32_t bg;
  uint8_t w, h, tsx, tsy;
  BString filename;

  cip++;

  if (getParam(bg, 0, VS23_MAX_BG, I_COMMA))
    return;
  if (!(filename = getParamFname()))
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
    err = ERR_FORMAT;
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
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void isavebg() {
#ifdef VS23_BG_ENGINE
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
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void BASIC_FP imovebg() {
#ifdef VS23_BG_ENGINE
  int32_t bg, x, y;
  if (getParam(bg, 0, VS23_MAX_BG, I_TO)) return;
  // XXX: arbitrary limitation?
  if (getParam(x, INT32_MIN, INT32_MAX, I_COMMA)) return;
  if (getParam(y, INT32_MIN, INT32_MAX, I_NONE)) return;
  
  vs23.scroll(bg, x, y);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void BASIC_INT isprite() {
#ifdef VS23_BG_ENGINE
  int32_t num, pat_x, pat_y, w, h, frame_x, frame_y, flags, key, prio;
  bool set_frame = false, set_opacity = false;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetSprites();
    return;
  }

  if (getParam(num, 0, VS23_MAX_SPRITES, I_NONE)) return;
  
  frame_x = vs23.spriteFrameX(num);
  frame_y = vs23.spriteFrameY(num);
  flags = (vs23.spriteOpaque(num) << 0) |
          (vs23.spriteFlipX(num) << 1) |
          (vs23.spriteFlipY(num) << 2);

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
    if (getParam(frame_x, 0, 255, I_NONE)) return;
    if (*cip == I_COMMA) {
      ++cip;
      if (getParam(frame_y, 0, 255, I_NONE)) return;
    } else
      frame_y = 0;
    set_frame = true;
    break;
  case I_ON:
    vs23.enableSprite(num);
    break;
  case I_OFF:
    vs23.disableSprite(num);
    break;
  case I_FLAGS: {
      int32_t new_flags;
      if (getParam(new_flags, 0, 7, I_NONE)) return;
      if (new_flags != flags) {
        if ((new_flags & 1) != (flags & 1))
          set_opacity = true;
        if ((new_flags & 6) != (flags & 6))
          set_frame = true;
        flags = new_flags;
      }
    }
    break;
  case I_KEY:
    if (getParam(key, 0, 255, I_NONE)) return;
    vs23.setSpriteKey(num, key);
    break;
  case I_PRIO:
    if (getParam(prio, 0, VS23_MAX_PRIO, I_NONE)) return;
    vs23.setSpritePriority(num, prio);
    break;
  default:
    // XXX: throw an error if nothing has been done
    cip--;
    if (!end_of_statement())
      SYNTAX_T("sprite parameter");
    if (set_frame)
      vs23.setSpriteFrame(num, frame_x, frame_y, flags & 2, flags & 4);
    if (set_opacity || (set_frame && (flags & 1)))
      vs23.setSpriteOpaque(num, flags & 1);
    if (!vs23.spriteReload(num))
      err = ERR_OOM;
    return;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void BASIC_FP imovesprite() {
#ifdef VS23_BG_ENGINE
  int32_t num, pos_x, pos_y;
  if (getParam(num, 0, VS23_MAX_SPRITES, I_TO)) return;
  if (getParam(pos_x, INT32_MIN, INT32_MAX, I_COMMA)) return;
  if (getParam(pos_y, INT32_MIN, INT32_MAX, I_NONE)) return;
  vs23.moveSprite(num, pos_x, pos_y);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void BASIC_FP imove()
{
  if (*cip == I_SPRITE) {
    ++cip;
    imovesprite();
  } else if (*cip == I_BG) {
    ++cip;
    imovebg();
  } else
    SYNTAX_T("BG or SPRITE");
}

void iplot() {
#ifdef VS23_BG_ENGINE
  int32_t bg, x, y, t;
  if (getParam(bg, 0, VS23_MAX_BG, I_COMMA)) return;
  if (!vs23.bgTiles(bg)) {
    // BG not defined
    err = ERR_RANGE;
    return;
  }
  if (getParam(x, 0, INT_MAX, I_COMMA)) return;
  if (getParam(y, 0, INT_MAX, I_COMMA)) return;
  if (is_strexp()) {
    BString dat = istrexp();
    vs23.setBgTiles(bg, x, y, (const uint8_t *)dat.c_str(), dat.length());
  } else {
    if (getParam(t, 0, 255, I_NONE)) return;
    vs23.setBgTile(bg, x, y, t);
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void iframeskip() {
#ifdef VS23_BG_ENGINE
  int32_t skip;
  if (getParam(skip, 0, 60, I_NONE)) return;
  vs23.setFrameskip(skip);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

#include "Psx.h"
Psx psx;

static int BASIC_INT cursor_pad_state()
{
  // The state is kept up-to-date by the interpreter polling for Ctrl-C.
  return kb.state(PS2KEY_L_Arrow) << psxLeftShift |
         kb.state(PS2KEY_R_Arrow) << psxRightShift |
         kb.state(PS2KEY_Down_Arrow) << psxDownShift |
         kb.state(PS2KEY_Up_Arrow) << psxUpShift |
         kb.state(PS2KEY_X) << psxXShift |
         kb.state(PS2KEY_A) << psxTriShift |
         kb.state(PS2KEY_S) << psxOShift |
         kb.state(PS2KEY_Z) << psxSquShift;
}

int BASIC_INT pad_state(int num)
{
  switch (num) {
  case 0:	return (psx.read() & 0xffff) | cursor_pad_state();
  case 1:	return cursor_pad_state();
  case 2:	return psx.read() & 0xffff;
  }
  return 0;
}

num_t BASIC_INT npad() {
  int32_t num;
  if (checkOpen()) return 0;
  if (getParam(num, 0, 2, I_CLOSE)) return 0;
  return pad_state(num);
}

void BASIC_INT event_handle_pad()
{
  for (int i = 0; i < MAX_PADS; ++i) {
    if (!event_pad_proc_idx[i])
      continue;
    int new_state = pad_state(i);
    int old_state = event_pad_last[i];
    event_pad_last[i] = new_state;
    if (new_state != old_state) {
      init_stack_frame();
      push_num_arg(i);
      push_num_arg(new_state ^ old_state);
      do_call(event_pad_proc_idx[i]);
      // Have to end here so the handler can be executed.
      // Events on other pads will be processed next time.
      return;
    }
  }
}

int e_main(int argc, char **argv);

void iedit() {
  BString fn;
  const char *argv[2] = { NULL, NULL };
  int argc = 1;
  if ((fn = getParamFname())) {
    ++argc;
    argv[1] = fn.c_str();
  }
  sc0.show_curs(1);
  e_main(argc, (char **)argv);
  sc0.show_curs(0);
}

//
// load / execute the program LRUN / LOAD
// LRUN "file name"
// LRUN "file name", line number
// LOAD "file name"
// MERGE "file name", line number

// Return value
// 1: normal 0: abnormal
//
uint8_t SMALL ilrun() {
  uint32_t lineno = (uint32_t)-1;
  uint8_t *lp;
  int8_t fg;               // File format 0: Binary format 1: Text format
  bool islrun = true;
  bool ismerge = false;
  uint8_t newmode = NEW_PROG;
  BString fname;

  // Command identification
  if (*(cip-1) == I_LOAD) {
    islrun  = false;
    lineno  = 0;
    newmode = NEW_ALL;
  } else if (cip[-1] == I_MERGE) {
    islrun = false;
    ismerge = true;
    newmode = NEW_VAR;
  }

  // Get file name
  if (is_strexp()) {
    if(!(fname = getParamFname())) {
      return 0;
    }
  } else {
    SYNTAX_T("file name");
    return 0;
  }

  if (islrun || ismerge) {
    // LRUN or MERGE
    // Obtain the second argument line number
    if(*cip == I_COMMA) {
      islrun = true;	// MERGE + line number => run!
      cip++;
      if ( getParam(lineno, I_NONE) ) return 0;
    } else {
      lineno = 0;
    }
  }

  // Load program from storage
  fg = bfs.IsText((char *)fname.c_str()); // Format check
  if (fg < 0) {
    // Abnormal form (形式異常)
    err = -fg;
  } else if (fg == 0) {
    // Binary format load from SD card
    err = ERR_NOT_SUPPORTED;
  } else if (fg == 1) {
    // Text format load from SD card
    loadPrgText((char *)fname.c_str(),newmode);
  }
  if (err)
    return 0;

  // Processing of line number
  if (lineno == 0) {
    clp = listbuf; // Set the line pointer to start of program buffer
  } else {
    // Jump to specified line
    lp = getlp(lineno);
    if (lineno != getlineno(lp)) {
      err = ERR_ULN;
      return 0;
    }
    clp = lp;
  }
  if (!err) {
    if (islrun || (cip >= listbuf && cip < listbuf+size_list)) {
      cip = clp+sizeof(line_desc_t);
    }
  }
  return 1;
}

// output error message
// Arguments:
// flgCmd: set to false at program execution, true at command line
void SMALL error(uint8_t flgCmd = false) {
  if (err) {
    if (err == ERR_OOM) {
      // free as much as possible first
#ifdef HAVE_TSF
      sound.unloadFont();
#endif
#ifdef VS23_BG_ENGINE
      vs23.resetSprites();
      vs23.resetBgs();
#endif
      inew(NEW_VAR);
    }
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + size_list && *clp && !flgCmd) {
      // エラーメッセージを表示
      c_puts_P(errmsg[err]);
      PRINT_P(" in ");
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      if (err_expected) {
        PRINT_P(" (exp ");
        c_puts_P(err_expected);
        PRINT_P(")");
      }
      newline();

      // リストの該当行を表示
      putnum(getlineno(clp), 0);
      c_putch(' ');
      putlist(clp);
      newline();
      //err = 0;
      //return;
    } else {                   // 指示の実行中なら
      c_puts_P(errmsg[err]);     // エラーメッセージを表示
      if (err_expected) {
        PRINT_P(" (exp ");
        c_puts_P(err_expected);
        PRINT_P(")");
      }
      newline();               // 改行
      //err = 0;               // エラー番号をクリア
      //return;
    }
  }
  c_puts_P(errmsg[0]);           //「OK」を表示
  newline();                   // 改行
  err = 0;                     // エラー番号をクリア
  err_expected = NULL;
}

num_t BASIC_FP nrgb() {
  int32_t r, g, b;
  if (checkOpen() ||
      getParam(r, 0, 255, I_COMMA) ||
      getParam(g, 0, 255, I_COMMA) ||
      getParam(b, 0, 255, I_CLOSE)) {
    return 0;
  }
  return vs23.colorFromRgb(r, g, b);
}

static inline bool is_var(unsigned char tok)
{
  return tok >= I_VAR && tok <= I_STRLSTREF;
}

void BASIC_FP icall();

static const uint8_t vs23_read_regs[] PROGMEM = {
  0x01, 0x9f, 0x84, 0x86, 0xb7, 0x53
};

int get_filenum_param() {
  int32_t f = getparam();
  if (f < 0 || f >= MAX_USER_FILES) {
    E_VALUE(0, MAX_USER_FILES - 1);
    return -1;
  } else
    return f;
}

BString ilrstr(bool right) {
  BString value;
  int len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
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

static BString sleft() {
  return ilrstr(false);
}
static BString sright() {
  return ilrstr(true);
}

BString smid() {
  BString value;
  int32_t start;
  int32_t len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(start, I_COMMA)) goto out;
  if (getParam(len, I_CLOSE)) goto out;

  value = value.substring(start, start + len);

out:
  return value;
}

BString sdir()
{
  auto dir_entry = user_dir.next();
  if (!dir_entry)
    user_dir.close();
  retval[0] = dir_entry.size;
  retval[1] = dir_entry.is_directory;
  return dir_entry.name;
}

BString sinput()
{
  int32_t len, fnum;
  BString value;
  ssize_t rd;

  if (checkOpen()) goto out;
  if (getParam(len, I_COMMA)) goto out;
  if (*cip == I_SHARP)
    ++cip;
  if (getParam(fnum, 0, MAX_USER_FILES - 1, I_CLOSE)) goto out;
  if (!user_files[fnum] || !*user_files[fnum]) {
    err = ERR_FILE_NOT_OPEN;
    goto out;
  }
  if (!value.reserve(len)) {
    err = ERR_OOM;
    goto out;
  }
  rd = user_files[fnum]->read(value.begin(), len);
  if (rd < 0) {
    err = ERR_FILE_READ;
    goto out;
  }
  value.resetLength(rd);
out:
  return value;
}

static BString schr() {
  int32_t nv;
  BString value;
  if (checkOpen()) return value;
  if (getParam(nv, 0,255, I_NONE)) return value; 
  value = BString((char)nv);
  checkClose();
  return value;
}

static BString sstr() {
  BString value;
  if (checkOpen()) return value;
  // The BString ctor for doubles is not helpful because it uses dtostrf()
  // which can only do a fixed number of decimal places. That is not
  // the BASIC Way(tm).
  sprintf(lbuf, "%0g", iexp());
  value = lbuf;
  checkClose();
  return value;
}

static BString scwd() {
  return Unifile::cwd();
}

static BString sinkey() {
  int32_t c = iinkey();
  if (c)
    return BString((char)c);
  else
    return BString();
}

static BString spopf() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).front();
    str_lst.var(*cip++).pop_front();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("string list reference");
    return value;
  }
  checkClose();
  return value;
}

static BString spopb() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).back();
    str_lst.var(*cip++).pop_back();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("string list reference");
    return value;
  }
  checkClose();
  return value;
}

static BString sinst() {
#ifdef HAVE_TSF
  return sound.instName(getparam());
#else
  err = ERR_NOT_SUPPORTED;
  return BString();
#endif
}

typedef BString (*strfun_t)();
#include "strfuntbl.h"

BString istrvalue()
{
  BString value;
  int len, dims;
  uint8_t i;
  int idxs[MAX_ARRAY_DIMS];

  if (*cip >= STRFUN_FIRST && *cip < STRFUN_LAST) {
    return strfuntbl[*cip++ - STRFUN_FIRST]();
  } else switch (*cip++) {
  case I_STR:
    len = value.fromBasic(cip);
    cip += len;
    if (!len)
      err = ERR_OOM;
    break;

  case I_SVAR:
    value = svar.var(*cip++);
    break;

  case I_LSVAR:
    value = get_lsvar(*cip++);
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

  case I_STRLST:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != 1) {
      SYNTAX_T("one dimension");
    } else {
      value = str_lst.var(i).var(idxs[0]);
    }
    break;

  case I_INPUTSTR:	value = sinput(); break;
  case I_NET:
#ifdef ESP8266_NOWIFI
    err = ERR_NOT_SUPPORTED;
#else
    if (*cip == I_INPUTSTR) {
      ++cip;
      value = snetinput();
    } else if (*cip == I_GETSTR) {
      ++cip;
      value = snetget();
    } else
      SYNTAX_T("network function");
#endif
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
      SYNTAX_T("string expr");
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

num_t BASIC_INT nlen() {
  int32_t value;
  if (checkOpen()) return 0;
  if (*cip == I_STRLSTREF) {
    ++cip;
    value = str_lst.var(*cip++).size();
  } else if (*cip == I_NUMLSTREF) {
    ++cip;
    value = num_lst.var(*cip++).size();
  } else {
    value = istrexp().length();
  }
  checkClose();
  return value;
}

num_t BASIC_FP nrnd() {
  num_t value = getparam(); //括弧の値を取得
  if (!err) {
    if (value < 0 )
      E_VALUE(0, INT32_MAX);
    else
      value = getrnd(value);  //乱数を取得
  }
  return value;
}

num_t BASIC_FP nabs() {
  num_t value = getparam(); //括弧の値を取得
  if (value == INT32_MIN) {
    err = ERR_VOF;
    return 0;
  }
  if (value < 0)
    value *= -1;  //正負を反転
  return value;
}

num_t BASIC_FP nsin() {
  return sin(getparam());
}
num_t BASIC_FP ncos() {
  return cos(getparam());
}
num_t BASIC_FP nexp() {
  return exp(getparam());
}
num_t BASIC_FP natn() {
  return atan(getparam());
}

num_t BASIC_FP natn2() {
  num_t value, value2;
  if (checkOpen() || getParam(value, I_COMMA) || getParam(value2, I_CLOSE))
    return 0;
  return atan2(value, value2);
}

num_t BASIC_FP nsqr() {
  num_t value = getparam();
  if (value < 0)
    err = ERR_FP;
  else
    value = sqrt(value);
  return value;
}

num_t BASIC_FP ntan() {
  // XXX: check for +/-inf
  return tan(getparam());
}

num_t BASIC_FP nlog() {
  num_t value = getparam();
  if (value <= 0)
    err = ERR_FP;
  else
    value = log(value);
  return value;
}

num_t BASIC_FP nint() {
  return floor(getparam());
}

num_t BASIC_FP nsgn() {
  num_t value = getparam();
  return (0 < value) - (value < 0);
}

num_t BASIC_FP nhigh() {
  return CONST_HIGH;
}
num_t BASIC_FP nlow() {
  return CONST_LOW;
}
num_t BASIC_FP nlsb() {
  return CONST_LSB;
}
num_t BASIC_FP nmsb() {
  return CONST_MSB;
}

num_t BASIC_INT nmprg() {
  return (unsigned int)listbuf;
}
num_t BASIC_INT nmfnt() {
  return (unsigned int)sc0.getfontadr();
}

num_t BASIC_INT ngpin() {
  int32_t a;
  if (checkOpen()) return 0;
  if (getParam(a, 0, 15, I_NONE)) return 0;
  if (checkClose()) return 0;
  while (!blockFinished()) {}
  if (Wire.requestFrom(0x20, 2) != 2) {
    err = ERR_IO;
    return 0;
  } else {
    uint16_t state = Wire.read();
    state |= Wire.read() << 8;
    return !!(state & (1 << a));
  }
}

num_t BASIC_FP nana() {
#ifdef ESP8266_NOWIFI
  err = ERR_NOT_SUPPORTED;
  return 0;
#else
  if (checkOpen()) return 0;
  if (checkClose()) return 0;
  return analogRead(A0);    // 入力値取得
#endif
}

num_t BASIC_FP nsread() {
  if (checkOpen()||checkClose()) return 0;
  return Serial.read();
}

num_t BASIC_FP nsready() {
  if (checkOpen()||checkClose()) return 0;
  return Serial.available();
}

num_t BASIC_FP ncsize() {
  // 画面サイズ定数の参照
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.getWidth();
  case 1:	return sc0.getHeight();
  default:	E_VALUE(0, 1); return 0;
  }
}

num_t BASIC_FP npsize() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.getGWidth();
  case 1:	return sc0.getGHeight();
  case 2:	return vs23.lastLine();
  default:	E_VALUE(0, 2); return 0;
  }
}

num_t BASIC_FP npos() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.c_x();
  case 1:	return sc0.c_y();
  default:	E_VALUE(0, 1); return 0;
  }
}

num_t BASIC_FP nup() {
  // カーソル・スクロール等の方向
  return psxUp;
}
num_t BASIC_FP nright() {
  return psxRight;
}
num_t BASIC_FP nleft() {
  return psxLeft;
}

num_t BASIC_FP ntilecoll() {
#ifdef VS23_BG_ENGINE
  int32_t a, b, c;
  if (checkOpen()) return 0;
  if (getParam(a, 0, VS23_MAX_SPRITES, I_COMMA)) return 0;
  if (getParam(b, 0, VS23_MAX_BG, I_COMMA)) return 0;
  if (getParam(c, 0, 255, I_NONE)) return 0;
  if (checkClose()) return 0;
  return vs23.spriteTileCollision(a, b, c);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

num_t BASIC_FP nsprcoll() {
#ifdef VS23_BG_ENGINE
  int32_t a, b;
  if (checkOpen()) return 0;
  if (getParam(a, 0, VS23_MAX_SPRITES, I_COMMA)) return 0;
  if (getParam(b, 0, VS23_MAX_SPRITES, I_NONE)) return 0;
  if (checkClose()) return 0;
  if (vs23.spriteEnabled(a) && vs23.spriteEnabled(b))
    return vs23.spriteCollision(a, b);
  else
    return 0;
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

num_t BASIC_FP nplay() {
#ifdef HAVE_MML
  int32_t a, b;
  if (checkOpen()) return 0;
  if (getParam(a, 0, SOUND_CHANNELS, I_CLOSE)) return 0;
  if (a == 0) {
    b = 0;
    for (int i = 0; i < SOUND_CHANNELS; ++i) {
      b |= sound.isPlaying(i);
    }
    return b;
  } else
    return sound.isPlaying(a - 1);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

num_t BASIC_FP nfree() {
  if (checkOpen()||checkClose()) return 0;
#ifdef ESP8266
  return umm_free_heap_size();
#else
  err = ERR_NOT_SUPPORTED;
  return -1;
#endif
}

num_t BASIC_FP ninkey() {
  if (checkOpen()||checkClose()) return 0;
  return iinkey(); // キー入力値の取得
}

num_t BASIC_FP ntick() {
  num_t value;
  if ((*cip == I_OPEN) && (*(cip + 1) == I_CLOSE)) {
    // 引数無し
    value = 0;
    cip+=2;
  } else {
    value = getparam(); // 括弧の値を取得
    if (err)
      return 0;
  }
  if(value == 0) {
    value = millis();              // 0～INT32_MAX ms
  } else if (value == 1) {
    value = millis()/1000;         // 0～INT32_MAX s
  } else {
    E_VALUE(0, 1);
  }
  return value;
}

num_t BASIC_FP npeek() {
  return ipeek(0);
}
num_t BASIC_FP npeekw() {
  return ipeek(1);
}
num_t BASIC_FP npeekd() {
  return ipeek(2);
}

num_t BASIC_FP nframe() {
  if (checkOpen()||checkClose()) return 0;
  return vs23.frame();
}

num_t BASIC_FP nvreg() {
  int32_t a = getparam();
  bool good = false;
  for (uint32_t i = 0; i < sizeof(vs23_read_regs); ++i) {
    if (pgm_read_byte(&vs23_read_regs[i]) == a) {
      good = true;
      break;
    }
  }
  if (!good) {
    err = ERR_VALUE;
    return 0;
  } else
    return SpiRamReadRegister(a);
}
  
num_t BASIC_FP nvpeek() {
  num_t value = getparam();
  if (value < 0 || value > 131071) {
    E_VALUE(0, 131071);
    return 0;
  } else
    return SpiRamReadByte(value);
}

num_t BASIC_FP neof() {
  int32_t a = get_filenum_param();
  if (!err)
    return !user_files[a]->available();
  else
    return 0;
}

num_t BASIC_INT nlof() {
  int32_t a = get_filenum_param();
  if (!err)
    return user_files[a]->fileSize();
  else
    return 0;
}

num_t BASIC_INT nloc() {
  int32_t a = get_filenum_param();
  if (!err)
    return user_files[a]->position();
  else
    return 0;
}
  
num_t BASIC_FP npopf() {
  num_t value;
  if (checkOpen()) return 0;
  if (*cip++ == I_NUMLSTREF) {
    value = num_lst.var(*cip).front();
    num_lst.var(*cip++).pop_front();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

num_t BASIC_FP npopb() {
  num_t value;
  if (checkOpen()) return 0;
  if (*cip++ == I_NUMLSTREF) {
    value = num_lst.var(*cip).back();
    num_lst.var(*cip++).pop_back();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

num_t BASIC_INT nval() {
  if (checkOpen()) return 0;
  num_t value = strtonum(istrexp().c_str(), NULL);
  checkClose();
  return value;
}

typedef num_t (*numfun_t)();
#include "numfuntbl.h"

// Get value
num_t BASIC_FP ivalue() {
  num_t value = 0; // 値
  uint8_t i;   // 文字数
  int dims;
  static int idxs[MAX_ARRAY_DIMS];
  int32_t a;

  if (*cip >= NUMFUN_FIRST && *cip < NUMFUN_LAST) {
    value = numfuntbl[*cip++ - NUMFUN_FIRST]();
  } else switch (*cip++) {
  //定数の取得
  case I_NUM:    // 定数
    value = UNALIGNED_NUM_T(cip);
    cip += sizeof(num_t);
    break;

  case I_HEXNUM: // 16進定数
    value = cip[0] | (cip[1] << 8) | (cip[2] << 16) | (cip[3] << 24); //定数を取得
    cip += 4;
    break;

  //変数の値の取得
  case I_VAR: //変数
    value = var.var(*cip++);
    break;

  case I_LVAR:
    value = get_lvar(*cip++);
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
      E_SYNTAX(I_SQOPEN);
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

  case I_CHAR: value = ncharfun(); break; //関数CHAR

  case I_DOWN:  value = psxDown; break;

  case I_PLAY:	value = nplay(); break;

  case I_NUMLST:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != 1) {
      SYNTAX_T("one dimension");
    } else {
      value = num_lst.var(i).var(idxs[0]);
    }
    break;

  case I_FN: {
    unsigned char *lp;
    icall();
    for (;;) {
      lp = iexe(true);
      if (!lp || err)
        break;
      clp = lp;
      cip = clp + sizeof(line_desc_t);
    }
    value = retval[0];
    break;
  }

  case I_VREG:	value = nvreg(); break;

  default:
    cip--;
    if (is_strexp())
      err = ERR_TYPE;
    else
      SYNTAX_T("numeric expr");
    break;
  }

  while (1)
    switch (*cip) {
    case I_POW:
      cip++;
      value = pow(value, ivalue());
      break;
    default:
      return value;
    }
}

// multiply or divide calculation
num_t BASIC_FP imul() {
  num_t value, tmp; //値と演算値

  while(1) {
    switch (*cip++) {
    case I_MINUS: //「-」
      return 0 - imul(); //値を取得して負の値に変換
      break;
    case I_PLUS:
      return imul();
      break;
    default:
      cip--;
      value = ivalue();
      goto out;
    }
  }
out:

  if (err)
    return -1;

  while (1) //無限に繰り返す
    switch(*cip) { //中間コードで分岐

    case I_MUL: //掛け算の場合
      cip++;
      tmp = ivalue();
      value *= tmp; //掛け算を実行
      break;

    case I_DIV: //割り算の場合
      cip++;
      tmp = ivalue();
      if (tmp == 0) { //もし演算値が0なら
	err = ERR_DIVBY0; //エラー番号をセット
	return -1;
      }
      value /= tmp; //割り算を実行
      break;

    case I_MOD: //剰余の場合
      cip++;
      tmp = ivalue();
      if (tmp == 0) { //もし演算値が0なら
	err = ERR_DIVBY0; //エラー番号をセット
	return -1; //終了
      }
      value = (int32_t)value % (int32_t)tmp; //割り算を実行
      break;

    case I_LSHIFT: // シフト演算 "<<" の場合
      cip++;
      tmp = ivalue();
      value =((uint32_t)value)<<(uint32_t)tmp;
      break;

    case I_RSHIFT: // シフト演算 ">>" の場合
      cip++;
      tmp = ivalue();
      value =((uint32_t)value)>>(uint32_t)tmp;
      break;

    default:
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

// add or subtract calculation
num_t BASIC_FP iplus() {
  num_t value, tmp; //値と演算値
  value = imul(); //値を取得
  if (err)
    return -1;

  while (1)
    switch(*cip) {
    case I_PLUS: //足し算の場合
      cip++;
      tmp = imul();
      value += tmp; //足し算を実行
      break;

    case I_MINUS: //引き算の場合
      cip++;
      tmp = imul();
      value -= tmp; //引き算を実行
      break;

    default:
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

#define basic_bool(x) ((x) ? -1 : 0)

num_t BASIC_FP irel() {
  num_t value, tmp;

  value = iplus();
  if (err)
    return -1;

  // conditional expression
  while (1)
    switch(*cip++) {
    case I_EQ:
      tmp = iplus();
      value = basic_bool(value == tmp);
      break;
    case I_NEQ:
    case I_NEQ2:
      tmp = iplus();
      value = basic_bool(value != tmp);
      break;
    case I_LT:
      tmp = iplus();
      value = basic_bool(value < tmp);
      break;
    case I_LTE:
      tmp = iplus();
      value = basic_bool(value <= tmp);
      break;
    case I_GT:
      tmp = iplus();
      value = basic_bool(value > tmp);
      break;
    case I_GTE:
      tmp = iplus();
      value = basic_bool(value >= tmp);
      break;
    default:
      cip--;
      return value;
    }
}

num_t BASIC_FP iand() {
  num_t value, tmp;

  switch(*cip++) {
  case I_BITREV: // NOT
    return ~((int32_t)irel());
  default:
    cip--;
    value = irel();
  }

  if (err)
    return -1;

  while (1)
    switch(*cip++) {
    case I_AND:
      tmp = irel();
      value = ((uint32_t)value)&((uint32_t)tmp);
      break;
    default:
      cip--;
      return value;
    }
}

// Numeric expression parser
num_t BASIC_FP iexp() {
  num_t value, tmp;

  if (is_strexp()) {
    // string comparison (or error)
    BString lhs = istrexp();
    BString rhs;
    switch (*cip++) {
    case I_EQ:
      rhs = istrexp();
      return basic_bool(lhs == rhs);
    case I_NEQ:
    case I_NEQ2:
      rhs = istrexp();
      return basic_bool(lhs != rhs);
    case I_LT:
      rhs = istrexp();
      return basic_bool(lhs < rhs);
    case I_GT:
      rhs = istrexp();
      return basic_bool(lhs > rhs);
    default:
      err = ERR_TYPE;
      return -1;
    }
  }

  value = iand();
  if (err)
    return -1;

  while (1)
    switch(*cip++) {
    case I_OR:
      tmp = iand();
      value = ((uint32_t)value) | ((uint32_t)tmp);
      break;
    case I_XOR:
      tmp = iand();
      value = ((uint32_t)value) ^ ((uint32_t)tmp);
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
char* getLineStr(uint32_t lineno, uint8_t devno) {
  uint8_t* lp = getlp(lineno);
  if (lineno != getlineno(lp))
    return NULL;

  // Output of specified line text to line buffer
  if (devno == 3)
    cleartbuf();
  if (devno == 0)
    sc0.setColor(COL(LINENUM), COL(BG));
  putnum(lineno, 0, devno);
  if (devno == 0)
    sc0.setColor(COL(FG), COL(BG));
  c_putch(' ', devno);
  putlist(lp, devno);
  if (devno == 3)
    c_putch(0,devno);        // zero-terminate tbuf
  return tbuf;
}

// システム情報の表示
void SMALL isysinfo() {
  char top = 't';
  uint32_t adr = (uint32_t)&top;

  PRINT_P("Program size: ");
  putnum(size_list, 0);
  newline();

  newline();
  PRINT_P("Variables:\n");
  
  PRINT_P(" Numerical: ");
  putnum(var.size(), 0);
  PRINT_P(", ");
  putnum(num_arr.size(), 0);
  PRINT_P(" arrays, ");
  putnum(num_lst.size(), 0);
  PRINT_P(" lists\n");

  PRINT_P(" Strings:   ");
  putnum(svar.size(), 0);
  PRINT_P(", ");
  putnum(str_arr.size(), 0);
  PRINT_P(" arrays, ");
  putnum(str_lst.size(), 0);
  PRINT_P(" lists\n");
  
  newline();
  PRINT_P("Loop stack:   ");
  putnum(lstki, 0);
  newline();
  PRINT_P("Return stack: ");
  putnum(gstki, 0);
  newline();

  // スタック領域先頭アドレスの表示
  newline();
  PRINT_P("CPU stack: ");
  putHexnum(adr, 8);
  newline();

  // SRAM未使用領域の表示
  PRINT_P("SRAM Free: ");
  putnum(umm_free_heap_size(), 0);
  newline();

  newline();
  PRINT_P("Video timing: ");
  putint(vs23.cyclesPerFrame(), 0);
  PRINT_P(" cpf (");
  putint(vs23.cyclesPerFrameCalculated(), 0);
  PRINT_P(" nominal)\n");
}

static void BASIC_FP do_goto(uint32_t line)
{
  uint8_t *lp = getlp(line);
  if (line != getlineno(lp)) {            // もし分岐先が存在しなければ
    err = ERR_ULN;                          // エラー番号をセット
    return;
  }

  clp = lp;        // 行ポインタを分岐先へ更新
  cip = clp + sizeof(line_desc_t); // 中間コードポインタを先頭の中間コードに更新
}

// GOTO
void BASIC_FP igoto() {
  uint32_t lineno;    // 行番号

  if (*cip == I_LABEL) {
    ++cip;
    label_t &lb = labels.label(*cip++);
    if (!lb.lp || !lb.ip) {
      err = ERR_UNDEFLABEL;
      return;
    }
    clp = lb.lp;
    cip = lb.ip;
  } else {
    // 引数の行番号取得
    lineno = iexp();
    if (err) return;
    do_goto(lineno);
  }
}

static void BASIC_FP do_gosub_p(unsigned char *lp, unsigned char *ip)
{
  //ポインタを退避
  if (gstki >= SIZE_GSTK) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
    return;
  }
  gstk[gstki].lp = clp;                      // 行ポインタを退避
  gstk[gstki].ip = cip;                      // 中間コードポインタを退避
  gstk[gstki].num_args = 0;
  gstk[gstki].str_args = 0;
  gstk[gstki++].proc_idx = 0;

  clp = lp;                                 // 行ポインタを分岐先へ更新
  cip = ip;
}

static void BASIC_FP do_gosub(uint32_t lineno) {
  uint8_t *lp = getlp(lineno);
  if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
    err = ERR_ULN;                          // エラー番号をセット
    return;
  }
  do_gosub_p(lp, lp + sizeof(line_desc_t));
}

// GOSUB
void BASIC_FP igosub() {
  uint32_t lineno;    // 行番号

  if (*cip == I_LABEL) {
    ++cip;
    label_t &lb = labels.label(*cip++);
    if (!lb.lp || !lb.ip) {
      err = ERR_UNDEFLABEL;
      return;
    }
    do_gosub_p(lb.lp, lb.ip);
  } else {
    // 引数の行番号取得
    lineno = iexp();
    if (err)
      return;
    do_gosub(lineno);
  }
}

// ON ... <GOTO|GOSUB> ...
static void BASIC_FP on_go(bool is_gosub, int cas)
{
  unsigned char *lp, *ip;
  for (;;) {
    if (*cip == I_LABEL) {
      ++cip;
      label_t &lb = labels.label(*cip++);
      lp = lb.lp;
      ip = lb.ip;
    } else {
      uint32_t line = iexp();
      lp = getlp(line);
      ip = lp + sizeof(line_desc_t);
    }

    if (err)
      return;
    if (!cas)
      break;
    if (*cip != I_COMMA)
      return;

    ++cip;
    --cas;
  }

  if (is_gosub)
    do_gosub_p(lp, ip);
  else {
    clp = lp;
    cip = ip;
  }
}

void BASIC_FP ion()
{
  if (*cip == I_SPRITE) {
    ++cip;
    if (*cip == I_OFF) {
      event_sprite_enabled = false;
      ++cip;
    } else {
      if (*cip++ != I_CALL) {
        E_SYNTAX(I_CALL);
        return;
      }
      event_sprite_enabled = true;
      event_sprite_proc_idx = *cip++;
    }
  } else if (*cip == I_PLAY) {
    ++cip;
    int ch = getparam();
    if (ch < 0 || ch >= SOUND_CHANNELS) {
      E_VALUE(0, SOUND_CHANNELS - 1);
      return;
    }
    if (*cip == I_OFF) {
      event_play_enabled = false;
      ++cip;
    } else {
      if (*cip++ != I_CALL) {
        E_SYNTAX(I_CALL);
        return;
      }
      event_play_enabled = true;
      event_play_proc_idx[ch] = *cip++;
    }
  } else if (*cip == I_PAD) {
    ++cip;
    int pad = getparam();
    if (pad < 0 || pad >= MAX_PADS) {
      E_VALUE(0, MAX_PADS - 1);
      return;
    }
    if (*cip == I_OFF) {
      event_pad_enabled = false;
      memset(event_pad_proc_idx, 0, sizeof(event_pad_proc_idx));
      ++cip;
    } else {
      if (*cip++ != I_CALL) {
        E_SYNTAX(I_CALL);
        return;
      }
      event_pad_enabled = true;
      event_pad_proc_idx[pad] = *cip++;
    }
  } else if (*cip == I_ERROR) {
    ++cip;
    if (*cip++ != I_GOTO) {
      E_SYNTAX(I_GOTO);
      return;
    }
    event_error_enabled = true;
    event_error_line = iexp();
  } else {
    uint32_t cas = iexp();
    if (*cip == I_GOTO) {
      ++cip;
      on_go(false, cas);
    } else if (*cip == I_GOSUB) {
      ++cip;
      on_go(true, cas);
    } else {
      SYNTAX_T("GOTO or GOSUB");
    }
  }
}

void iresume()
{
  if (!event_error_resume_lp) {
    err = ERR_CONT;
  } else {
    clp = event_error_resume_lp;
    cip = event_error_resume_ip;
    event_error_resume_lp = NULL;
    // XXX: To work reliably, this requires a lot of discipline in the
    // error-generating code, which none of it currently has.
    // And even then, it needs to do a better crawl job, similar
    // to find_next_token().
    while (!end_of_statement()) {
      ++cip;
    }
  }
}

void BASIC_FP icall() {
  num_t n;
  uint8_t proc_idx = *cip++;

  struct proc_t &proc_loc = proc.proc(proc_idx);

  if (!proc_loc.lp || !proc_loc.ip) {
    err = ERR_UNDEFPROC;
    return;
  }
  
  if (gstki >= SIZE_GSTK) {              // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;                       // エラー番号をセット
    return;
  }

  int num_args = 0;
  int str_args = 0;
  if (gstki > 0) {
    struct proc_t &p = proc.proc(gstk[gstki-1].proc_idx);
    astk_num_i += p.locc_num;
    astk_str_i += p.locc_str;
  }
  if (*cip == I_OPEN) {
    ++cip;
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
  }
  if (num_args < proc_loc.argc_num ||
      str_args < proc_loc.argc_str) {
    err = ERR_ARGS;
    return;
  }

  gstk[gstki].lp = clp;
  gstk[gstki].ip = cip;
  gstk[gstki].num_args = num_args;
  gstk[gstki].str_args = str_args;
  gstk[gstki++].proc_idx = proc_idx;

  clp = proc_loc.lp;
  cip = proc_loc.ip;
  
  if (profile_enabled) {
    proc_loc.profile_current = ESP.getCycleCount();
  }
  return;
overflow:
  err = ERR_ASTKOF;
  return;
}

void iproc() {
  err = ERR_ULN;	// XXX: come up with something better
}

// RETURN
void BASIC_FP ireturn() {
  if (!gstki) {    // もしGOSUBスタックが空なら
    err = ERR_GSTKUF; // エラー番号をセット
    return;
  }

  // Set return values, if any.
  if (!end_of_statement()) {
    int rcnt = 0;
    num_t my_retval[MAX_RETVALS];
    do {
      // XXX: implement return strings
      my_retval[rcnt++] = iexp();
    } while (*cip++ == I_COMMA && rcnt < MAX_RETVALS);
    for (int i = 0; i < rcnt; ++i)
      retval[i] = my_retval[i];
  }

  astk_num_i -= gstk[--gstki].num_args;
  astk_str_i -= gstk[gstki].str_args;
  if (gstki > 0) {
    // XXX: This can change if the parent procedure was called by this one
    // (directly or indirectly)!
    struct proc_t &p = proc.proc(gstk[gstki-1].proc_idx);
    astk_num_i -= p.locc_num;
    astk_str_i -= p.locc_str;
  }
  if (profile_enabled) {
    struct proc_t &p = proc.proc(gstk[gstki].proc_idx);
    p.profile_total += ESP.getCycleCount() - p.profile_current;
  }
  clp = gstk[gstki].lp; //中間コードポインタを復帰
  cip = gstk[gstki].ip; //行ポインタを復帰
  return;
}

void BASIC_FP ido() {
  if (lstki >= SIZE_LSTK) {
    err = ERR_LSTKOF;
    return;
  }
  lstk[lstki].lp = clp;
  lstk[lstki].ip = cip;
  lstk[lstki].vto = -1;
  lstk[lstki].vstep = -1;
  lstk[lstki].index = -1;
  lstk[lstki++].local = false;
}

// FOR
void BASIC_FP ifor() {
  int index;
  num_t vto, vstep; // FOR文の変数番号、終了値、増分

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

  // 繰り返し条件を退避
  if (lstki >= SIZE_LSTK) { // もしFORスタックがいっぱいなら
    err = ERR_LSTKOF;          // エラー番号をセット
    return;
  }
  lstk[lstki].lp = clp; // 行ポインタを退避
  lstk[lstki].ip = cip; // 中間コードポインタを退避

  // FORスタックに終了値、増分、変数名を退避
  // Special thanks hardyboy
  lstk[lstki].vto = vto;
  lstk[lstki].vstep = vstep;
  lstk[lstki].index = index;
  lstk[lstki++].local = false;
}

void BASIC_FP iloop() {
  uint8_t cond;

  if (!lstki) {
    err = ERR_LSTKUF;
    return;
  }

  // Look for nearest DO.
  while (lstki) {
    if (lstk[lstki - 1].index == -1)
      break;
    lstki--;
  }
  
  if (!lstki) {
    err = ERR_LSTKUF;
    return;
  }

  cond = *cip;
  if (cond == I_WHILE || cond == I_UNTIL) {
    ++cip;
    num_t exp = iexp();
  
    if ((cond == I_WHILE && exp != 0) ||
        (cond == I_UNTIL && exp == 0)) {
      // Condition met, loop.
      cip = lstk[lstki - 1].ip;
      clp = lstk[lstki - 1].lp;
    } else {
      // Pop loop off stack.
      lstki--;
    }
  } else {
    // Infinite loop.
    cip = lstk[lstki - 1].ip;
    clp = lstk[lstki - 1].lp;
  }
}

// NEXT
void BASIC_FP inext() {
  int want_index;	// variable we want to NEXT
  int index;		// variable index
  num_t vto;		// end of loop value
  num_t vstep;		// increment value

  if (!lstki) {    // FOR stack is empty
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
    index = lstk[lstki - 1].index;

    // Done if it's the one we want (or if none is specified).
    if (want_index < 0 || want_index == index)
      break;

    // If it is not the specified variable, we assume we
    // want to NEXT to a loop higher up the stack.
    lstki--;
  }

  if (!lstki) {
    // Didn't find anything that matches the NEXT.
    err = ERR_LSTKUF;	// XXX: have something more descriptive
    return;
  }

  vstep = lstk[lstki - 1].vstep;
  var.var(index) += vstep;
  vto = lstk[lstki - 1].vto;

  // Is this loop finished?
  if (((vstep < 0) && (var.var(index) < vto)) ||
      ((vstep > 0) && (var.var(index) > vto))) {
    lstki--;  // drop it from FOR stack
    return;
  }

  // Jump to the start of the loop.
  cip = lstk[lstki - 1].ip;
  clp = lstk[lstki - 1].lp;
}

// IF
void BASIC_FP iif() {
  num_t condition;    // IF文の条件値
  uint8_t* newip;       // ELSE文以降の処理対象ポインタ

  condition = iexp(); // 真偽を取得
  if (err) {          // もしエラーが生じたら
    err = ERR_IFWOC;  // エラー番号をセット
    return;
  }

  if (*cip == I_THEN) {
    ++cip;
  }

  if (condition) {    // もし真なら
    return;
  } else {
    newip = getELSEptr(cip);
    if (newip) {
      cip = newip;
    }
  }
}

void BASIC_FP iendif()
{
}

void BASIC_FP ielse()
{
  uint8_t *newip = getELSEptr(cip, true);
  if (newip)
    cip = newip;
}

// スキップ
void BASIC_FP iskip() {
  while (*cip != I_EOL) // I_EOLに達するまで繰り返す
    cip++;              // 中間コードポインタを次へ進める
}

void BASIC_FP ilabel() {
  ++cip;
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
#define esyntax_workaround esyntax

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

void iclear() {
  inew(NEW_VAR);
}

void BASIC_FP inil() {
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
  } else if (*cip == I_CONFIG) {
    iloadconfig();
  } else
    ilrun();
}

void imerge() {
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
    default:		SYNTAX_T("file mode"); return;
    }
  }
  
  if (*cip++ != I_AS) {
    E_SYNTAX(I_AS);
    return;
  }
  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;
  
  Unifile f = Unifile::open(filename, flags);
  if (!f)
    err = ERR_FILE_OPEN;
  else {
    user_files[filenum] = new Unifile();
    if (!user_files[filenum]) {
      err = ERR_OOM;
      f.close();
      return;
    }
    *user_files[filenum] = f;
  }
}

void iclose() {
  int32_t filenum;

  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_NONE))
    return;

  if (!user_files[filenum] || !*user_files[filenum])
    err = ERR_FILE_NOT_OPEN;
  else {
    user_files[filenum]->close();
    delete user_files[filenum];
    user_files[filenum] = NULL;
  }
}

void iprofile() {
  switch (*cip++) {
  case I_ON:
    profile_enabled = true;
    break;
  case I_OFF:
    profile_enabled = false;
    break;
  case I_LIST:
    for (int i = 0; i < proc.size(); ++i) {
      struct proc_t &p = proc.proc(i);
      sprintf(lbuf, "%10d %s", p.profile_total, proc_names.name(i));
      c_puts(lbuf); newline();
    }
    break;
  default:
    SYNTAX_T("ON, OFF or LIST");
    break;
  }
}

#include <eboot_command.h>
#include <spi_flash.h>
void iboot() {
  int32_t sector;
  if (getParam(sector, 0, 1048576 / SPI_FLASH_SEC_SIZE - 1, I_NONE))
    return;
  eboot_command ebcmd;
  ebcmd.action = ACTION_LOAD_APP;
  ebcmd.args[0] = sector * SPI_FLASH_SEC_SIZE;
  eboot_command_write(&ebcmd);
#ifdef ESP8266_NOWIFI
  // SDKnoWiFi does not have system_restart*(). The only working
  // alternative I could find is triggering the WDT.
  ets_wdt_enable(2,1,1);
  for(;;);
#else
  ESP.reset();        // UNTESTED!
#endif
}

typedef void (*cmd_t)();
#include "funtbl.h"

// 中間コードの実行
// 戻り値      : 次のプログラム実行位置(行の先頭)
unsigned char* BASIC_FP iexe(bool until_return) {
  uint8_t c;               // 入力キー
  err = 0;
  uint8_t stk = gstki;

  while (*cip != I_EOL) { //行末まで繰り返す
    //強制的な中断の判定
    if ((c = sc0.peekKey())) { // もし未読文字があったら
      if (c == SC_KEY_CTRL_C || c==27 ) { // 読み込んでもし[ESC],［CTRL_C］キーだったら
        c_getch();
	err = ERR_CTR_C;                  // エラー番号をセット
	err_expected = NULL;
	break;
      }
    }

    //中間コードを実行
    if (*cip < sizeof(funtbl)/sizeof(funtbl[0])) {
      funtbl[*cip++]();
    } else
      SYNTAX_T("command");

    pump_events();

    if (err || (until_return && gstki < stk))
      return NULL;
  }

  return clp + *clp;
}

void iflash();

void SMALL resize_windows()
{
#ifdef VS23_BG_ENGINE
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
      if (!vs23.bgEnabled(i))
        continue;
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
#endif
}

void SMALL restore_windows()
{
#ifdef VS23_BG_ENGINE
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
#endif
}

//Command precessor
uint8_t SMALL icom() {
  uint8_t rc = 1;
  cip = ibuf;          // 中間コードポインタを中間コードバッファの先頭に設定

  switch (*cip++) {    // 中間コードポインタが指し示す中間コードによって分岐
  case I_LOAD:
  case I_MERGE:
    ilrun_(); break;

  case I_LRUN:  if(ilrun()) {
      sc0.show_curs(0); irun(clp);
    }
    break;
  case I_RUN:
    sc0.show_curs(0);
    irun();
    break;
  case I_CONT:
    if (!cont_cip || !cont_clp) {
      err = ERR_CONT;
    } else {
      restore_windows();
      sc0.show_curs(0);
      irun(NULL, true);
    }
    break;
  case I_GOTO:
    igoto();
    if (!err) {
      restore_windows();
      sc0.show_curs(0);
      irun(clp, true);
    }
    break;
  case I_RESUME:
    iresume();
    if (!err) {
      restore_windows();
      sc0.show_curs(0);
      irun(clp, true);
    }
    break;
  case I_RENUM:
    irenum();
    break;

  case I_DELETE:     idelete();  break;
  case I_FORMAT:     iformat(); break;
  case I_FLASH:      iflash(); break;

  default: {
    cip--;
    sc0.show_curs(0);
    unsigned char gstki_save = gstki;
    unsigned char lstki_save = lstki;
    unsigned char astk_num_i_save = astk_num_i;
    unsigned char astk_str_i_save = astk_str_i;
    iexe();
    gstki = gstki_save;
    lstki = lstki_save;
    astk_num_i = astk_num_i_save;
    astk_str_i = astk_str_i_save;
    break;
  }
  }
  
  if (err) {
    while (sc0.tryGetChar()) {}
    resize_windows();
  }

  sc0.show_curs(true);

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
             sc0.getGWidth() - 160 - 0, 0, 0, 2, 160, 62, 0);
}

/*
   TOYOSHIKI Tiny BASIC
   The BASIC entry point
 */
void SMALL basic() {
  unsigned char len; // Length of intermediate code
  char* textline;    // input line
  uint8_t rc;

#ifdef UNIFILE_USE_SPIFFS
#ifdef ESP8266_NOWIFI
  SPIFFS.begin(false);
#else
  SPIFFS.begin();
#endif
#else
  fs.mount();
#endif
  loadConfig();

  vs23.begin(CONFIG.interlace, CONFIG.lowpass, CONFIG.NTSC != 0);
  vs23.setColorSpace(1);

  psx.setupPins(0, 1, 2, 3, 1);

  size_list = 0;
  listbuf = NULL;
  memset(user_files, 0, sizeof(user_files));

  // Initialize execution environment
  inew();

  sc0.setCursorColor(CONFIG.cursor_color);
  sc0.init(SIZE_LINE, CONFIG.NTSC, NULL, CONFIG.mode - 1);
  sc0.reset_kbd(CONFIG.KEYBOARD);

  Wire.begin(2, 0);
  // ESP8266 Wire code assumes that SCL and SDA pins are set low, instead
  // of taking care of that itself. WTF?!?
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  icls();
  sc0.show_curs(0);

  // Want to make sure we get the right hue.
  vs23.setColorConversion(1, 7, 2, 4, true);
  show_logo();
  vs23.setColorSpace(1);	// reset color conversion

  // Startup screen
  // Epigram
  sc0.setFont(fonts[1]);
  sc0.setColor(vs23.colorFromRgb(72,72,72), COL(BG));
  srand(ESP.getCycleCount());
  c_puts_P(epigrams[random(sizeof(epigrams)/sizeof(*epigrams))]);
  newline();

  // Banner
  sc0.setColor(vs23.colorFromRgb(192,0,0), COL(BG));
  static const char engine_basic[] PROGMEM = "Engine BASIC";
  c_puts_P(engine_basic);
  sc0.setFont(fonts[CONFIG.font]);

  // Platform/version
  sc0.setColor(vs23.colorFromRgb(64,64,64), COL(BG));
  static const char __e[] PROGMEM = STR_EDITION;
  sc0.locate(sc0.getWidth() - strlen_P(__e), 7);
  c_puts_P(__e);
  static const char __v[] PROGMEM = STR_VARSION;
  sc0.locate(sc0.getWidth() - strlen_P(__v), 8);
  c_puts_P(__v);

  // Initialize file systems
#ifdef UNIFILE_USE_SPIFFS
  SPIFFS.begin();
#else
  if (!fs.mount()) {
    fs.mkfs();
    fs.mount();
  }
#endif
#if USE_SD_CARD == 1
  // Initialize SD card file system
  bfs.init(16);		// CS on GPIO16
#endif

  // Free memory
  sc0.setColor(COL(FG), COL(BG));
  sc0.locate(0,2);
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
  PRINT_P(" bytes free\n");
#endif

  if (!Unifile::chDir(SD_PREFIX))
    Unifile::chDir(FLASH_PREFIX);
  else
    bfs.fakeTime();

  PRINT_P("Directory ");
  c_puts(Unifile::cwd()); newline();

  // XXX: make sound font configurable
  sound.begin();

  if (CONFIG.beep_volume > 0) {
    static const uint8_t startup_env[] PROGMEM = {
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
    };
    sound.beep(30, CONFIG.beep_volume, startup_env);
    delay(150);
    sound.beep(15, CONFIG.beep_volume, startup_env);
  }

  sc0.show_curs(1);
  err_expected = NULL;
  error();          // "OK" or display an error message and clear the error number

  sc0.forget();

  // Enter one line from the terminal and execute
  while (1) {
    rc = sc0.edit();
    if (rc) {
      textline = (char*)sc0.getText();
      int textlen = strlen(textline);
      if (!textlen) {
	newline();
	continue;
      }
      if (textlen >= SIZE_LINE) {
	err = ERR_LONG;
	newline();
	error();
	continue;
      }

      strcpy(lbuf, textline);
      tlimR((char*)lbuf);
      while (--rc)
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
      recalc_indent();
      if (err)
	error();          // display program mode error message
      continue;
    }

    // If the intermediate code is a direct mode command
    if (icom())		// execute
      error(false);	// display direct mode error message
  }
}

static const uint8_t default_color_scheme[CONFIG_COLS][3] PROGMEM = {
  {   0,   0,   0 },	// BG
  { 192, 192, 192 },	// FG
  { 255, 255, 255 },	// KEYWORD
  { 128, 128, 128 },	// LINENUM
  {  10, 120, 160 },	// NUM (teal)
  { 140, 140, 140 },	// VAR (light gray)
  { 244, 233, 123 },	// LVAR (beige)
  { 214,  91, 189 },	// OP (pink)
  {  50,  50, 255 },	// STR (blue)
  { 238, 137,  17 },	// PROC (orange)
  {  84, 255,   0 },	// COMMENT (green)
  {   0,   0,   0 },	// BORDER
};

#define CONFIG_FILE "/flash/.config"

// システム環境設定のロード
void loadConfig() {
  CONFIG.NTSC      =  0;
  CONFIG.KEYBOARD  =  1;
  memcpy_P(CONFIG.color_scheme, default_color_scheme, sizeof(CONFIG.color_scheme));
  CONFIG.mode = 1;
  CONFIG.font = 0;
  CONFIG.cursor_color = 0x92;
  CONFIG.beep_volume = 15;
  
  Unifile f = Unifile::open(BString(F(CONFIG_FILE)), FILE_READ);
  if (!f)
    return;
  f.read((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}

// システム環境設定の保存
void isaveconfig() {
  Unifile f = Unifile::open(BString(F(CONFIG_FILE)), FILE_OVERWRITE);
  if (!f) {
    err = ERR_FILE_OPEN;
  }
  f.write((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}
