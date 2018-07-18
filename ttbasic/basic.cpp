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
#include "video.h"
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
#include "credits.h"

struct unaligned_num_t {
  num_t n;
} __attribute__((packed));

#define UNALIGNED_NUM_T(ip) (reinterpret_cast<struct unaligned_num_t *>(ip)->n)

// TOYOSHIKI TinyBASIC プログラム利用域に関する定義

// *** フォント参照 ***************
const uint8_t* ttbasic_font = TV_DISPLAY_FONT;

static const uint8_t *fonts[NUM_FONTS] = {
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
#ifdef USE_BG_ENGINE
static int original_bg_height[MAX_BG];
static bool turn_bg_back_on[MAX_BG];
bool restore_bgs = false;
static uint16_t original_text_pos[2];
bool restore_text_window = false;
#endif

#include <Wire.h>

// *** SDカード管理 ****************
sdfiles bfs;

Unifile *user_files[MAX_USER_FILES];

SystemConfig CONFIG;

// プロトタイプ宣言
void loadConfig();
void isaveconfig();
void mem_putch(uint8_t c);
unsigned char* BASIC_FP iexe(int stk = -1);
num_t BASIC_FP iexp(void);
void error(uint8_t flgCmd);
// **** RTC用宣言 ********************
#ifdef USE_INNERRTC
  #include <Time.h>
#endif

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

bool screen_putch_disable_escape_codes = false;

void BASIC_INT screen_putch(uint8_t c, bool lazy) {
  static bool escape = false;
  static uint8_t col_digit = 0, color, is_bg;
  static bool reverse = false;

  if (screen_putch_disable_escape_codes) {
    sc0.putch(c, lazy);
    return;
  }

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
      case 'h': sc0.locate(0, 0); break;
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
      case '\b':
        if (sc0.c_x() > 0) {
          sc0.locate(sc0.c_x() - 1);
          sc0.putch(' ');
          sc0.locate(sc0.c_x() - 1);
        }
        break;
      default:	sc0.putch(c, lazy); break;
      }
    }
  }
  escape = false;
}

int redirect_output_file = -1;
int redirect_input_file = -1;

// 文字の出力
inline void c_putch(uint8_t c, uint8_t devno) {
  if (devno == 0) {
    if (redirect_output_file >= 0)
      user_files[redirect_output_file]->write(c);
    else
      screen_putch(c);
  } else if (devno == 1)
    Serial.write(c);
  else if (devno == 2)
    sc0.gputch(c);
  else if (devno == 3)
    mem_putch(c);
  else if (devno == 4)
    bfs.putch(c);
}

uint8_t BASIC_FP process_hotkeys(uint16_t c, bool dont_dump = false) {
  if (c == SC_KEY_CTRL_C) {
    err = ERR_CTR_C;
  } else if (c == KEY_PRINT) {
    sc0.saveScreenshot();
  } else
    return 0;

  if (!dont_dump)
    sc0.get_ch();
  return err;
}

void BASIC_INT newline(uint8_t devno) {
  if (devno==0) {
    if (redirect_output_file >= 0) {
      user_files[redirect_output_file]->write('\n');
      return;
    }
    uint16_t c = sc0.peekKey();
    if (!process_hotkeys(c) && kb.state(PS2KEY_L_Shift)) {
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

/***bf m RND
Returns a random number.
\usage r = RND(mod)
\args
@mod	modifier determining the type of random number to generate
\ret A random number of the selected type.
\sec TYPES
The following types of random numbers are generated depending on the value
of `mod`:
\table
| `mod` > 1 | a random integer number between 0 and 2147483647
| `mod` between 0 and 1 | a random fractional number between 0 and 1
| `mod` < 0 | a random fractional number between 0 and 1; a specific
              value of `mod` will always yield the same random number
\endtable
***/
num_t BASIC_FP getrnd(int value) {
  if (value < 0)
    randomSeed(value);
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
  I_CSIZE, I_PSIZE,
  I_INKEY,I_CHAR, I_CHR, I_ASC, I_HEX, I_BIN,I_LEN, I_STRSTR,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT,
  I_RND, I_ABS, I_FREE, I_TICK, I_PEEK, I_PEEKW, I_PEEKD, I_I2CW, I_I2CR,
  I_SIN, I_COS, I_EXP, I_ATN, I_ATN2, I_SQR, I_TAN, I_LOG, I_INT,
  I_DIN, I_ANA,
  I_SREAD, I_SREADY, I_POINT,
  I_RET, I_RETSTR, I_ARG, I_ARGSTR, I_ARGC,
  I_PAD, I_SPRCOLL, I_SPRX, I_SPRY, I_TILECOLL,
  I_DIRSTR, I_INSTR, I_ERRORSTR, I_COMPARE,
  I_SQOPEN, I_SQCLOSE,
};

// Intermediate code which eliminates previous space when former is constant or variable
const uint8_t i_nsb[] BASIC_DAT = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2,I_LT,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL, I_SQOPEN, I_SQCLOSE,
};

// insert a blank before intermediate code
const uint8_t i_sf[] BASIC_DAT  = {
  I_CLS, I_COLOR, I_DATE, I_END, I_FILES, I_TO, I_STEP,I_QUEST,I_AND, I_OR, I_XOR,
  I_GET,I_TIME,I_GOSUB,I_GOTO,I_INKEY,I_INPUT,I_LET,I_LIST,I_ELSE,I_THEN,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_SET,I_WAIT,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BLIT, I_SWRITE, I_SPRINT,I_SMODE,
  I_BEEP, I_CSCROLL, I_GSCROLL, I_MOD,
};

// tokens that can be functions (no space before paren) or something else
const uint8_t i_dual[] BASIC_DAT = {
  I_FRAME, I_PLAY, I_VREG, I_POS, I_CONNECT, I_SYS, I_MAP,
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

void SMALL E_SYNTAX(unsigned char token) {
  err = ERR_SYNTAX;
  strcpy_P(tbuf, PSTR("expected \""));
  strcat_P(tbuf, kwtbl[token]);
  strcat_P(tbuf, PSTR("\""));
  err_expected = tbuf;
}
// RAM mapping
char lbuf[SIZE_LINE];          // コマンド入力バッファ
char tbuf[SIZE_LINE];          // テキスト表示用バッファ
int32_t tbuf_pos = 0;

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

basic_ctx_t *bc = NULL;

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
    E_ERR(VALUE, "unmapped address");
    return NULL;
  }
  // IRAM, flash: 32-bit only
  if ((vadr >= 0x40100000UL && vadr < 0x40300000UL) && type != 2) {
    E_ERR(VALUE, "non-32-bit access");
    return NULL;
  }
  if ((type == 1 && (vadr & 1)) ||
      (type == 2 && (vadr & 3))) {
    E_ERR(VALUE, "misaligned address");
    return NULL;
  }
  return (void *)vadr;
}

// Standard C library (about) same functions
// XXX: We pull in ctype anyway, can do away with these.
char c_isprint(char c) {
  //return(c >= 32 && c <= 126);
  return(c >= 32 && c!=127 );
}

// Delete whitespace on the right side of the string
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
  char f[8];
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

void get_input(bool numeric, uint8_t eoi) {
  char c; //文字
  uint8_t len; //文字数

  len = 0; //文字数をクリア
  while(1) {
    c = c_getch();
    if (c == eoi || (eoi == '\r' && redirect_input_file >= 0 && c == '\n')) {
      break;
    } else if (process_hotkeys(c, true)) {
      break;
    } else if (c == 8 || c == 127) {
      // Processing when the [BackSpace] key is pressed (not at the
      // beginning of the line)
      if (len > 0) {
        len--;
        if (redirect_output_file >= 0)
          c_putch('\x08');
        else {
          sc0.movePosPrevChar();
          sc0.delete_char();
        }
      }
    } else if (len < SIZE_LINE - 1 && (!numeric || c == '.' ||
        c == '+' || c == '-' || isDigit(c) || c == 'e' || c == 'E') ) {
      lbuf[len++] = c;
      c_putch(c);
    } else {
      if (redirect_output_file < 0)
        sc0.beep();
    }
  }
  if (eoi == '\r' || eoi == '\n')
    newline();
  else
    c_putch(eoi);
  lbuf[len] = 0; //終端を置く
}

// 数値の入力
num_t getnum(uint8_t eoi = '\r') {
  num_t value;

  get_input(true, eoi);
  value = strtonum(lbuf, NULL);
  // XXX: say "EXTRA IGNORED" if there is unused input?

  return value;
}

BString getstr(uint8_t eoi = '\r') {
  get_input(false, eoi);
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
int lookup_ext(char* str) {
  for (uint8_t i = 0; i < SIZE_KWTBL_EXT; ++i) {
    if (kwtbl_ext[i] && !strncasecmp_P(str, kwtbl_ext[i], strlen_P(kwtbl_ext[i])))
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
uint8_t BASIC_INT SMALL toktoi(bool find_prg_text) {
  int16_t i;
  int key;
  int len = 0;	// length of sequence of intermediate code
  char *ptok;		// pointer to the inside of one word
  char *s = lbuf;	// pointer to the inside of the string buffer
  char c;		// Character used to enclose string (")
  num_t value;		// constant
  uint32_t hex;		// hexadecimal constant
  uint16_t hcnt;	// hexadecimal digit count
  uint8_t var_len;	// variable name length
  char vname [MAX_VAR_NAME];	// variable name
  bool had_if = false;
  int implicit_endif = 0;

  bool is_prg_text = false;

  while (*s) {                  //文字列1行分の終端まで繰り返す
    while (isspace(*s)) s++;  //空白を読み飛ばす

    bool isext = false;
    key = lookup(s);
    if (key < 0) {
      key = lookup_ext(s);
      if (key >= 0) {
        isext = true;
        ibuf[len++] = I_EXTEND;
        s += strlen_P(kwtbl_ext[key]);
      }
    } else {
      s += strlen_P(kwtbl[key]);
    }
    if (key >= 0) {
      // 該当キーワードあり
      if (len >= SIZE_IBUF - 2) {      // もし中間コードが長すぎたら
	err = ERR_IBUFOF;              // エラー番号をセット
	return 0;                      // 0を持ち帰る
      }
      // translate "END IF" to "ENDIF"
      if (key == I_IF && len > 0 && ibuf[len-1] == I_END)
        ibuf[len-1] = I_ENDIF;
      else {
        if (key == I_IF)
          had_if = true;
        if ((key == I_REM || key == I_SQUOT) && implicit_endif) {
          // execution skips the rest of the line when encountering
          // a comment, so implicit ENDIF must be inserted before that
          while (implicit_endif) {
            ibuf[len++] = I_IMPLICITENDIF;
            --implicit_endif;
          }
        }
        ibuf[len++] = key;                 // 中間コードを記録
      }
      if (had_if && !isext && (key == I_THEN || key == I_GOTO)) {
        had_if = false;	// prevent multiple implicit endifs for "THEN GOTO"
        while (isspace(*s)) s++;
        if (*s) {
          // Handle "IF ... THEN ' comment" properly
          // XXX: Should "IF ... THEN : REM comment" also be considered?
          int nk = lookup(s);
          if (nk != I_SQUOT)
            implicit_endif++;
        }
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

    if (isext) {
      find_prg_text = false;
      continue;
    }

    //コメントへの変換を試みる
    if(key == I_REM|| key == I_SQUOT) {       // もし中間コードがI_REMなら
      while (isspace(*s)) s++;         // 空白を読み飛ばす
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
// XXX: No better place to put this...
/***bc bas PROC
Define a procedure or function.
\usage PROC name[(<num_arg|str_arg$>[, <num_arg|str_arg$> ...])]
\args
@name		procedure name
@num_arg	name of a numeric argument variable
@str_arg$	name of a string argument variable
\desc
`PROC` defines a procedure or function that can be called using `CALL` or
`FN`.  If a procedure or function has arguments, they can be referenced as
local variables using the `@` sigil.

Within a procedure or function, any numeric or string variable prefixed with
the `@` sigil will be treated as a local variable that is valid only within
the scope of the procedure. Setting such a variable will not affect global
variables or local variables in other procedures.
\error
Procedures can only be called using `CALL` or `FN`. If a `PROC` statement
is encountered during normal program flow, an error is generated.
\example
====
----
PROC foo(x, y)
  MOVE SPRITE 0 TO @x, @y
  RETURN
----
----
CALL foo(20, 30)
----
====

====
----
PROC sum(eins, zwei)
  RETURN @eins+@zwei
----
----
s = FN sum(1, 1)
----
====
\bugs
It is not currently possible to use complex data types (arrays and lists)
as local variables or arguments.
\ref CALL FN RETURN
***/
      if (!is_prg_text) {
        err = ERR_COM;
        return 0;
      }
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF;
	return 0;
      }

      while (isspace(*s)) s++;
      s += parse_identifier(s, vname);

      int idx = proc_names.assign(vname, true);
      ibuf[len++] = idx;
      if (procs.reserve(proc_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_LABEL) {
      if (len >= SIZE_IBUF - 2) { //もし中間コードが長すぎたら
	err = ERR_IBUFOF;
	return 0;
      }

      while (isspace(*s)) s++;
      s += parse_identifier(s, vname);

      int idx = label_names.assign(vname, true);
      ibuf[len++] = idx;
      if (labels.reserve(label_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_CALL || key == I_FN) {
      while (isspace(*s)) s++;
      s += parse_identifier(s, vname);
      int idx = proc_names.assign(vname, is_prg_text);
      if (procs.reserve(proc_names.varTop())) {
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
      if (s == ptok) {
        // nothing parsed, most likely a random single period
        SYNTAX_T("invalid number");
        return 0;
      }
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
        // XXX: Does not parse "A$ (" correctly.
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
	  idx = nvar_names.assign(vname, is_prg_text);
        }
	if (idx < 0)
	  goto oom;
	ibuf[len++] = idx;
	if ((!is_list && nvar.reserve(nvar_names.varTop())) ||
	    ( is_list && num_lst.reserve(num_lst_names.varTop())))
	  goto oom;
	s += var_len;
      }
    } else { // if none apply
      err = ERR_SYNTAX;
      return 0;
    }
  }

  while (implicit_endif) {
    ibuf[len++] = I_IMPLICITENDIF;
    --implicit_endif;
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

// Get line index from line number
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
uint8_t* BASIC_INT getELSEptr(uint8_t* p, bool endif_only = false, int adjust = 0) {
  uint8_t* rc = NULL;
  uint8_t* lp;
  unsigned char lifstki = 1 + adjust;
  uint8_t *stlp = clp; uint8_t *stip = cip;

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
      if (*lp == I_IF)
        lp++;
      break;
    case I_EOL:
    case I_REM:
    case I_SQUOT:
      // Continue at next line.
      clp += *clp;
      if (!*clp) {
        clp = stlp; cip = stip;
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

uint8_t* BASIC_INT getWENDptr(uint8_t* p) {
  uint8_t* rc = NULL;
  uint8_t* lp;
  unsigned char lifstki = 1;

  for (lp = p; ; ) {
    switch(*lp) {
    case I_WHILE:
      if (lp[-1] != I_LOOP)
        lifstki++;
      lp++;
      break;
    case I_WEND:
      if (lifstki == 1) {
        // Found the highest-level WEND, we're done.
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
        err = ERR_WHILEWOW;
        return NULL;
      }
      lp = cip = clp + sizeof(line_desc_t);
      break;
    default:
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
  procs.reset();
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
inline bool is_indent(uint8_t *c) {
  return (*c == I_IF && c[-1] != I_ELSE) || *c == I_DO || *c == I_WHILE ||
        (*c == I_FOR && c[1] != I_OUTPUT && c[1] != I_INPUT && c[1] != I_APPEND && c[1] != I_DIRECTORY);
}
// tokens that reduce indentation
inline bool is_unindent(uint8_t c) {
  return c == I_ENDIF || c == I_IMPLICITENDIF || c == I_NEXT || c == I_LOOP || c == I_WEND;
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
      if (is_indent(ip))
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

  while (*lp) {
    recalc_indent_line(lp);
    if (err)
      break;
    lp += *lp;
  }
}

// Text output of specified intermediate code line record
int SMALL putlist(unsigned char* ip, uint8_t devno) {
  int mark = -1;
  unsigned char i;
  uint8_t var_code;
  line_desc_t *ld = (line_desc_t *)ip;
  ip += sizeof(line_desc_t);

  for (i = 0; i < ld->indent && i < MAX_INDENT; ++i)
    c_putch(' ', devno);

  while (*ip != I_EOL) {
    // keyword processing
    if ((*ip < SIZE_KWTBL && kwtbl[*ip]) || *ip == I_EXTEND) {
      // if it is a keyword
      const char *kw;
      if (*ip == I_EXTEND) {
        ip++;
        if (*ip >= SIZE_KWTBL_EXT) {
          err = ERR_SYS;
          return 0;
        }
        kw = kwtbl_ext[*ip];
      } else { 
        kw = kwtbl[*ip];
      }

      if (isAlpha(pgm_read_byte(&kw[0])))
        sc0.setColor(COL(KEYWORD), COL(BG));
      else if (*ip == I_LABEL)
        sc0.setColor(COL(PROC), COL(BG));
      else
        sc0.setColor(COL(OP), COL(BG));
      if (*ip == I_SQUOT)
        PRINT_P("  ");
      c_puts_P(kw, devno);
      sc0.setColor(COL(FG), COL(BG));

      if (*(ip+1) != I_COLON && (*(ip+1) != I_OPEN || !dual(*ip)))
	if ( (!nospacea(*ip) || spacef(*(ip+1))) &&
	     *ip != I_COLON && *ip != I_SQUOT && *ip != I_LABEL)
	  c_putch(' ',devno);

      if (*ip == I_REM||*ip == I_SQUOT) { //もし中間コードがI_REMなら
	ip++; //ポインタを文字数へ進める
	i = *ip++; //文字数を取得してポインタをコメントへ進める
	sc0.setColor(COL(COMMENT), COL(BG));
	while (i--) //文字数だけ繰り返す
	  c_putch(*ip++,devno);  //ポインタを進めながら文字を表示
        sc0.setColor(COL(FG), COL(BG));
	return mark;
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
      c_puts(nvar_names.name(var_code), devno);
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
      // XXX: Why I_VAR? Such code wouldn't make sense anyway.
      if (*ip == I_VAR || *ip ==I_ELSE || *ip == I_AS || *ip == I_TO || *ip == I_THEN || *ip == I_FOR)
	c_putch(' ',devno);
    } else if (*ip == I_IMPLICITENDIF) {
      // invisible
      ip++;
    } else { //どれにも当てはまらなかった場合
      err = ERR_SYS; //エラー番号をセット
      return mark;
    }
    if (ip <= cip)
      mark = sc0.c_x();
  }
  return mark;
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

static inline bool end_of_statement()
{
  return *cip == I_EOL || *cip == I_COLON || *cip == I_ELSE || *cip == I_IMPLICITENDIF || *cip == I_SQUOT;
}

static inline bool BASIC_FP is_strexp();

// INPUT handler
void SMALL iinput() {
  int dims = 0;
  int idxs[MAX_ARRAY_DIMS];
  num_t value;
  BString str_value;
  short index;          // Array subscript or variable number
  int32_t filenum = -1;
  uint8_t eoi;	// end-of-input character

  if (*cip == I_SHARP) {
    cip++;
    if (getParam(filenum, 0, MAX_USER_FILES, I_COMMA))
      return;
    if (!user_files[filenum] || !*user_files[filenum]) {
      err = ERR_FILE_NOT_OPEN;
      return;
    }
  } else if(is_strexp() && *cip != I_SVAR && *cip != I_STRARR) {
    // We have to exclude string variables here because they may be lvalues.
    c_puts(istrexp().c_str());

    if (*cip != I_SEMI) {
      E_SYNTAX(I_SEMI);
      goto DONE;
    }
    cip++;
  } else if (*cip == I_SEMI)
    ++cip;

  sc0.show_curs(1);
  for(;;) {
    // Processing input values
    switch (*cip++) {
    case I_VAR:
    case I_VARARR:
    case I_NUMLST:
      index = *cip++;

      if (cip[-2] == I_VARARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
      } else if (cip[-2] == I_NUMLST) {
        if (get_array_dims(idxs) != 1) {
          SYNTAX_T("invalid list index");
          return;
        }
        dims = -1;
      }
 
      if (*cip == I_COMMA)
        eoi = ',';
      else
        eoi = '\r';

      if (filenum >= 0) {
        int c;
        str_value = "";
        if (eoi == '\r')
          eoi = '\n';
        for (;;) {
          c = user_files[filenum]->peek();
          if (isdigit(c) || c == '.')
            str_value += (char)user_files[filenum]->read();
          else if (isspace(c))
            user_files[filenum]->read();
          else if (c == eoi) {
            user_files[filenum]->read();
            break;
          } else
            break;
        }
        value = str_value.toFloat();
      } else {
        value = getnum(eoi);
      }

      if (err)
        return;

      if (dims > 0)
        num_arr.var(index).var(dims, idxs) = value;
      else if (dims < 0)
        num_lst.var(index).var(idxs[0]) = value;
      else
        nvar.var(index) = value;

      break;

    case I_SVAR:
    case I_STRARR:
    case I_STRLST:
      index = *cip++;
      
      if (cip[-2] == I_STRARR) {
        dims = get_array_dims(idxs);
        // XXX: check if dims matches array
      } else if (cip[-2] == I_STRLST) {
        if (get_array_dims(idxs) != 1) {
          SYNTAX_T("invalid list index");
          return;
        }
        dims = -1;
      }
 
      if (*cip == I_COMMA)
        eoi = ',';
      else
        eoi = '\r';

      if (filenum >= 0) {
        int c;
        str_value = "";
        if (eoi == '\r')
          eoi = '\n';
        while ((c = user_files[filenum]->read()) >= 0 && c != eoi) {
          if (c != '\r')
            str_value += (char)c;
        }
      } else {
        str_value = getstr(eoi);
      }
      if (err)
        return;
      
      if (dims > 0)
        str_arr.var(index).var(dims, idxs) = str_value;
      else if (dims < 0)
        str_lst.var(index).var(idxs[0]) = str_value;
      else
        svar.var(index) = str_value;

      break;

    default: // 以上のいずれにも該当しなかった場合
      SYNTAX_T("exp variable");
      //return;            // 終了
      goto DONE;
    } // 中間コードで分岐の末尾

    //値の入力を連続するかどうか判定する処理
    if (end_of_statement())
      goto DONE;
    else {
      switch (*cip) { // 中間コードで分岐
      case I_COMMA:    // コンマの場合
        cip++;         // 中間コードポインタを次へ進める
        break;         // 打ち切る
      default:      // 以上のいずれにも該当しなかった場合
        SYNTAX_T("exp separator");
        //return;           // 終了
        goto DONE;
      } // 中間コードで分岐の末尾
    }
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
  nvar.var(index) = value;
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
  if (proc_idx == NO_PROC) {
    err = ERR_GLOBAL;
    return 0;
  }
  int local_offset = procs.getNumArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = procs.getNumLoc(proc_idx, arg);
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
    if (getParam(idxs[dims], I_NONE))
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

  for (;;) {
    if (*cip != I_VARARR && *cip != I_STRARR) {
      SYNTAX_T("not an array variable");
      return;
    }
    is_string = *cip == I_STRARR;
    ++cip;

    index = *cip++;

    dims = get_array_dims(idxs);
    if (dims < 0)
      return;
    if (dims == 0) {
      SYNTAX_T("missing dimensions");
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
            BString &s = str_arr.var(index).var(1, &cnt);
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
            num_t &n = num_arr.var(index).var(1, &cnt);
            if (err)
              return;
            n = value;
            cnt++;
          } while(*cip == I_COMMA);
        }
      }
      break;
    }
    if (*cip != I_COMMA)
      break;
    ++cip;
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
  num_t &n = num_arr.var(index).var(dims, idxs);
  if (err)
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
  if (proc_idx == NO_PROC) {
    err = ERR_GLOBAL;
    return 0;
  }
  int local_offset = procs.getStrArg(proc_idx, arg);
  if (local_offset < 0) {
    local_offset = procs.getStrLoc(proc_idx, arg);
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
  BString &s = str_arr.var(index).var(dims, idxs);
  if (err)
    return;

  if (*cip == I_SQOPEN) {
    // String character accessor
    ++cip;

    int32_t offset;
    if (getParam(offset, 0, s.length(), I_SQCLOSE))
      return;

    if (*cip != I_EQ) {
      err = ERR_VWOEQ;
      return;
    }
    cip++;
    
    uint8_t sval = iexp();
    if (err)
      return;

    s[offset] = sval;

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
    SYNTAX_T("invalid list index");
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
    SYNTAX_T("invalid list index");
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
    SYNTAX_T("exp list reference");
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
    SYNTAX_T("exp list reference");
  }
  return;
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
  case I_RESTORE:
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

  if (!*sclp)
    return;

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

  for (int i = 0; i < procs.size(); ++i) {
    procs.proc(i).lp = NULL;
  }

  for (;;) {
    find_next_token(&lp, &ip, I_PROC);
    if (!lp)
      return;

    uint8_t proc_id = ip[1];
    ip += 2;

    proc_t &pr = procs.proc(proc_id);

    if (pr.lp) {
      err = ERR_DUPPROC;
      clp = lp; cip = ip;
      return;
    }

    pr.argc_num = 0;
    pr.argc_str = 0;
    pr.locc_num = 0;
    pr.locc_str = 0;
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
          SYNTAX_T("exp variable");
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

bool BASIC_INT find_next_data() {
  int next;

  if (!data_lp) {
    in_data = false;
    data_ip = NULL;
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

static unsigned char *data_cip_save;
void BASIC_INT data_push() {
  data_cip_save = cip;
  cip = data_ip + 1;
}

void BASIC_INT data_pop() {
  if (err) {
    // XXX: You would actually want to know both locations (READ and DATA).
    clp = data_lp;
    return;
  }
  if (!end_of_statement() && *cip != I_COMMA) {
    clp = data_lp;
    SYNTAX_T("malformed DATA");
    return;
  }
  data_ip = cip;
  cip = data_cip_save;
}

void BASIC_INT iread() {
  num_t value;
  BString svalue;
  uint8_t index;

  if (!find_next_data()) {
    err = ERR_OOD;
    return;
  }

  for (;;) switch (*cip++) {
  case I_VAR:
    data_push();
    value = iexp();
    data_pop();
    if (err)
      return;
    nvar.var(*cip++) = value;
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

    data_push();
    value = iexp();
    data_pop();
    if (err)
      return;

    num_t &n = is_list ?
                  num_lst.var(index).var(idxs[0]) :
                  num_arr.var(index).var(dims, idxs);
    if (err)
      return;
    n = value;
    break;
    }

  case I_SVAR:
    data_push();
    svalue = istrexp();
    data_pop();
    if (err)
      return;
    svar.var(*cip++) = svalue;
    break;
    
  case I_STRARR:
  case I_STRLST: {
    bool is_list = cip[-1] == I_STRLST;
    int idxs[MAX_ARRAY_DIMS];
    int dims = 0;
    
    index = *cip++;
    dims = get_array_dims(idxs);
    if (dims < 0 || (is_list && dims != 1))
      return;

    data_push();
    svalue = istrexp();
    data_pop();
    if (err)
      return;

    BString &s = is_list ?
                    str_lst.var(index).var(idxs[0]) :
                    str_arr.var(index).var(dims, idxs);
    if (err)
      return;
    s = svalue;
    break;
  }

  case I_COMMA:
    if (!find_next_data()) {
      err = ERR_OOD;
      return;
    }
    break;

  default:
    --cip;
    if (!end_of_statement()) 
      SYNTAX_T("exp variable");
    return;
  }
}

void irestore() {
  if (end_of_statement())
    data_lp = NULL;
  else if (*cip == I_LABEL) {
    ++cip;
    label_t &lb = labels.label(*cip++);
    if (!lb.lp || !lb.ip) {
      err = ERR_UNDEFLABEL;
      return;
    }
    data_lp = lb.lp;
    data_ip = lb.ip;
  } else {
    uint32_t line = iexp();
    if (err)
      return;
    data_lp = getlp(line);
    data_ip = data_lp + sizeof(line_desc_t);
  }
}

// LET handler
void BASIC_INT ilet() {
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

static void do_trace() {
  putnum(getlineno(clp), 0);
  c_putch(' ');
}
#define TRACE do { if (trace_enabled) do_trace(); } while(0)

void itron() {
  trace_enabled = true;
}
void itroff() {
  trace_enabled = false;
}

uint8_t event_sprite_proc_idx;

bool event_play_enabled;
uint8_t event_play_proc_idx[SOUND_CHANNELS];

#define MAX_PADS 3
bool event_pad_enabled;
uint8_t event_pad_proc_idx[MAX_PADS];
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
  event_sprite_proc_idx = NO_PROC;
  event_error_enabled = false;
  event_error_resume_lp = NULL;
  event_play_enabled = false;
  event_pad_enabled = false;
  math_exceptions_disabled = false;
  memset(event_pad_proc_idx, NO_PROC, sizeof(event_pad_proc_idx));
  memset(event_play_proc_idx, NO_PROC, sizeof(event_play_proc_idx));

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
    TRACE;
    cip = clp + sizeof(line_desc_t);   // 中間コードポインタを行番号の後ろに設定

resume:
    lp = iexe();     // 中間コードを実行して次の行の位置を得る
    if (err) {         // もしエラーを生じたら
      event_error_resume_lp = NULL;
      if (event_error_enabled) {
        retval[0] = err;
        retval[1] = getlineno(clp);
        retval[2] = -1;
        err = 0;
        err_expected = NULL;	// prevent stale "expected" messages
        event_error_enabled = false;
        event_error_resume_lp = clp;
        event_error_resume_ip = cip;
        clp = event_error_lp;
        cip = event_error_ip;
      } else if (err == ERR_CTR_C) {
        cont_cip = cip;
        cont_clp = clp;
        return;
      } else {
        cont_cip = cont_clp = NULL;
        return;
      }
    } else
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

  screen_putch_disable_escape_codes = true;
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
  screen_putch_disable_escape_codes = false;
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
    nvar.reset();
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
    nvar_names.deleteDirect();
    nvar.reserve(nvar_names.varTop());
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
    if (mode == NEW_ALL) {
      // forget all variables
      nvar_names.deleteAll();
      nvar.reserve(0);
      svar_names.deleteAll();
      svar.reserve(0);
      num_arr_names.deleteAll();
      num_arr.reserve(0);
      str_arr_names.deleteAll();
      str_arr.reserve(0);
      str_lst_names.deleteAll();
      str_lst.reserve(0);
      proc_names.deleteAll();
      procs.reserve(0);
      label_names.deleteAll();
      labels.reserve(0);
    }

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

/***bc bas RENUM
Renumber BASIC program in memory.
\usage RENUM [<start>[, <step>]]
\args
@start	new line number to start at [min `0`, default: `10`]
@step	line number increment [min `0`, default: `10`]
\note
Computed branches (`GOTO` and `GOSUB` commands with variable arguments)
cannot be renumbered correctly.
***/
void SMALL irenum() {
  int32_t startLineNo = 10;  // 開始行番号
  int32_t increase = 10;     // 増分
  uint8_t* ptr;               // プログラム領域参照ポインタ
  uint32_t len;               // 行長さ
  uint32_t i;                 // 中間コード参照位置
  num_t newnum;            // 新しい行番号
  uint32_t num;               // 現在の行番号
  uint32_t index;             // 行インデックス
  uint32_t cnt;               // プログラム行数
  int toksize;

  if (!end_of_statement()) {
    startLineNo = iexp();
    if (*cip == I_COMMA) {
      cip++;
      increase = iexp();
    }
  }

  // 引数の有効性チェック
  cnt = countLines()-1;
  if (startLineNo < 0 || increase <= 0) {
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
      case I_GOTO:
      case I_GOSUB:
      case I_THEN:
	i++;
	if (ptr[i] == I_NUM) {		// XXX: I_HEXNUM? :)
	  num = getlineno(&ptr[i]);
	  index = getlineIndex(num);
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

/***bc sys CONFIG COLOR
Changes the color scheme.
\desc
The color scheme is a set of colors that are used to print system messages
and BASIC program listings. It also contains the default foreground and
background colors.
\usage CONFIG COLOR col, red, green, blue
\args
@col	color code [`0` to `{CONFIG_COLS_m1}`]
@red	red component [0 to 255]
@green	green component [0 to 255]
@blue	blue component [0 to 255]
\sec COLOR CODES
\table
| 0 | Default background
| 1 | Default foreground
| 2 | Syntax: BASIC keywords
| 3 | Syntax: line numbers
| 4 | Syntax: numbers
| 5 | Syntax: global variables
| 6 | Syntax: local variables and arguments
| 7 | Syntax: operands
| 8 | Syntax: string constants
| 9 | Syntax: procedure names
| 10 | Syntax: comments
| 11 | Default border color
\endtable
\note
* Unlike the `COLOR` command, `CONFIG COLOR` requires the colors to be given
  in RGB format; this is to ensure that the color scheme works with all YUV
  colorspaces.
* To set the current color scheme as the default, use `SAVE CONFIG`.
\bugs
The default border color is not used.
\ref COLOR CONFIG SAVE_CONFIG
***/
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

/***bc sys CONFIG
Changes configuration options.
\desc
The options will be reset to their defaults on system startup unless they
have been saved using <<SAVE CONFIG>>. Changing power-on default options does not
affect the system until the options are saved and the system is restarted.
\usage CONFIG option, value
\args
@option	configuration option to be set [`0` to `8`]
@value	option value
\sec OPTIONS
The following options exist:

* `0`: TV norm +
  Sets the TV norm to NTSC (`0`), PAL (`1`) or PAL60 (`2`).
+
WARNING: This configuration option does not make much sense; the available
TV norm depends on the installed color burst crystal and is automatically
detected; PAL60 mode is not currently implemented. +
The semantics of this option are therefore likely to change in the future.

* `1`: Keyboard layout +
  Three different keyboard layouts are supported: +
  `0` (Japanese), `1` (US English, default) and `2` (German).

* `2`: Interlacing +
  Sets the video output to progressive (`0`) or interlaced (`1`). A change
  in this option will become effective on the next screen mode change.
+
WARNING: The intention of this option is to provide an interlaced signal
to TVs that do not handle a progressive signal well. So far, no displays
have turned up that require it, so it may be removed and/or replaced
with a different option in future releases.

* `3`: Luminance low-pass filter +
  This option enables (`1`) or disables (`0`, default) the video luminance
  low-pass filter. The recommended setting depends on the properties of the
  display device used.
+
Many recent TVs are able to handle high resolutions well; with such
devices, it is recommended to turn the low-pass filter off to achieve
a more crisp display.
+
On other (usually older) TVs, high-resolution images may cause excessive
color artifacting (a "rainbow" effect) or flicker; with such devices,
it is recommended to turn the low-pass filter on to reduce these effects,
at the expense of sharpness.

* `4`: Power-on screen mode [`1` (default) to `10`] +
  Sets the screen mode the system defaults to at power-on.

* `5`: Power-on screen font [`0` (default) to `{NUM_FONTS_m1}`] +
  Sets the screen font the system defaults to at power-on.

* `6`: Power-on cursor color [`0` to `255`] +
  Sets the cursor color the system defaults to at power-on.

* `7`: Beeper sound volume [`0` to `15` (default)] +
  Sets the volume of the "beeper" sound engine, which applies, among
  other things, to the start-up jingle.

* `8`: Screen line adjustment [`-128` to `128`, default: `0`]
  Adjusts the number of screen lines. A positive value adds lines, a negative
  value subtracts them. +
  This option may be useful to mitigate issues with color artifacting and
  flicker on some devices.
+
WARNING: It is not clear if this option is useful in practice, and it may be
removed in future releases.

\note
To restore the default configuration, run the command `REMOVE
"/flash/.config"` and restart the system.
\bugs
* Changing the low-pass filter option (`3`) only takes effect at the time
  of the next block move, which happens for instance when scrolling the
  screen.
* There is no way to override a saved configuration, which may make it
  difficult or even impossible to fix an system configured to an unusable
  state.
\ref BEEP FONT SAVE_CONFIG SCREEN
***/
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
#ifndef HOSTED
      kb.setLayout(value);
#endif
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
    if (value < 1 || value > vs23.numModes())
      E_VALUE(1, vs23.numModes());
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
  case 8:
    if (value < -128 || value > 127)
      E_VALUE(-128, 127);
    else {
      CONFIG.line_adjust = value;
      vs23.setLineAdjust(CONFIG.line_adjust);
    }
    break;
  default:
    E_VALUE(0, 8);
    break;
  }
}

void iloadconfig() {
  loadConfig();
}

void isavebg();
void isavepcx();

/***bc bas SAVE
Saves the BASIC program in memory to storage.
\usage SAVE file$
\args
@file$	name of file to be saved
\note
BASIC programs are saved in plain text (ASCII) format.
\ref SAVE_BG SAVE_CONFIG SAVE_PCX
***/
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
  uint32_t last_line = 0;
  
  cont_clp = cont_cip = NULL;
  procs.reset();
  labels.reset();

  err = bfs.tmpOpen(fname,0);
  if (err)
    return 1;

  if (newmode != NEW_VAR)
    inew(newmode);
  while(bfs.readLine(lbuf)) {
    char *sbuf = lbuf;
    while (isspace(*sbuf)) sbuf++;
    if (!isDigit(*sbuf)) {
      // Insert a line number before tokenizing.
      if (strlen(lbuf) > SIZE_LINE - 12) {
        err = ERR_LONG;
        error(true);
        rc = 1;
        break;
      }
      memmove(lbuf + 11, lbuf, strlen(lbuf) + 1);
      memset(lbuf,' ', 11);
      last_line += 1;
      int lnum_size = sprintf(lbuf, "%d", last_line);
      lbuf[lnum_size] = ' ';
    }
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
      last_line = ((line_desc_t *)ibuf)->line;
    } else {
      SYNTAX_T("invalid program line");
      error(true);
      rc = 1;
      break;
    }
  }
  recalc_indent();
  bfs.tmpClose();
  return rc;
}

/***bc bas DELETE
Delete specified line(s) from the BASIC program in memory.

WARNING: Do not confuse with `REMOVE`, which deletes files from storage.
\usage DELETE range
\args
@range	a range of BASIC program lines
\note
* Using `DELETE` does not affect variables.
* When called from a running program, execution will continue at the next
  program line, i.e. statements following `DELETE` on the same line are
  disregarded.
\ref REMOVE
***/
void SMALL idelete() {
  uint32_t sNo, eNo;
  uint8_t  *lp;      // 削除位置ポインタ
  uint8_t *p1, *p2;  // 移動先と移動元ポインタ
  int32_t len;       // 移動の長さ

  cont_clp = cont_cip = NULL;
  procs.reset();
  labels.reset();

  uint32_t current_line = getlineno(clp);

  if (!get_range(sNo, eNo))
    return;
  if (!end_of_statement()) {
    SYNTAX_T("exp end of statement");
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
      if (!*lp)
        break;
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
  
  initialize_proc_pointers();
  initialize_label_pointers();
  // continue on the next line, in the likely case the DELETE command didn't
  // delete itself
  clp = getlp(current_line + 1);
  cip = clp + sizeof(line_desc_t);
  TRACE;
}

/***bc fs FILES
Displays the contents of the current or a specified directory.
\usage FILES [filespec$]
\args
@filespec$	a filename or path, may include wildcard characters +
                [default: all files in the current directory]
\ref CHDIR CWD$()
***/
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

  rc = bfs.flist((char *)fname.c_str(), wcard, sc0.getWidth()/14);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_FILE_OPEN;
  }
}

/***bc fs FORMAT
Formats the specified file system.

WARNING: This command will erase any files stored on the formatted file
system, including system files and the saved configuration, if any.

IMPORTANT: Formatting the SD card file system does not currently work.
\usage FORMAT <"/flash"|"/sd">
\bugs
SD card formatting is currently broken; use another device to format SD
cards for use with the BASIC Engine.
***/
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

/***bc scr CLS
Clears the current text window.
\usage CLS
\ref WINDOW
***/
void icls() {
  if (redirect_output_file < 0) {
    sc0.cls();
    sc0.locate(0,0);
  }
}

static bool profile_enabled;

void BASIC_FP init_stack_frame()
{
  if (gstki > 0 && gstk[gstki-1].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki-1].proc_idx);
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
  struct proc_t &proc_loc = procs.proc(proc_idx);

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
  TRACE;
  
  if (profile_enabled) {
    proc_loc.profile_current = ESP.getCycleCount();
  }
  return;
}

#ifdef USE_BG_ENGINE
void BASIC_INT event_handle_sprite()
{
  uint8_t dir;
  for (int i = 0; i < MAX_SPRITES; ++i) {
    if (!vs23.spriteEnabled(i))
      continue;
    for (int j = i+1; j < MAX_SPRITES; ++j) {
      if (!vs23.spriteEnabled(j))
        continue;
      if ((dir = vs23.spriteCollision(i, j))) {
        init_stack_frame();
        push_num_arg(i);
        push_num_arg(j);
        push_num_arg(dir);
        do_call(event_sprite_proc_idx);
        event_sprite_proc_idx = NO_PROC;	// prevent interrupt storms
        return;
      }
    }
  }
}
#endif

void BASIC_INT event_handle_play(int ch)
{
  if (event_play_proc_idx[ch] == NO_PROC)
    return;
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
#ifdef HOSTED
  hosted_pump_events();
#endif
  if (vs23.frame() == last_frame) {
#if defined(HAVE_TSF) && !defined(HOSTED)
    // Wasn't able to get this to work without underruns in the hosted build.
    // Doing the rendering in the SDL callback instead.
    if (sound.needSamples())
      sound.render();
#endif
    return;
  }

  last_frame = vs23.frame();

  event_profile[0] = micros();
#ifdef USE_BG_ENGINE
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
  
#ifdef USE_BG_ENGINE
  if (event_sprite_proc_idx != NO_PROC)
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

/***bc sys WAIT
Pause for a specific amount of time.
\usage WAIT ms
\args
@ms	time in milliseconds
\ref TICK()
***/
void iwait() {
  int32_t tm;
  if ( getParam(tm, 0, INT32_MAX, I_NONE) ) return;
  uint32_t end = tm + millis();
  while (millis() < end) {
    pump_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      break;
    }
  }
}

/***bc scr VSYNC
Synchronize with video output.
\desc
Waits until the video frame counter has reached or exceeded a given
value, or until the next frame if none is specified.
\usage VSYNC [frame]
\args
@frame	frame number to be waited for
\note
Specifying a frame number helps avoid slowdowns by continuing immediately
if the program is "late", and not wait for another frame.
====
----
f = FRAME()
DO
  ' stuff that may take longer than expected
  VSYNC f+1	// <1>
  f = f + 1
LOOP
----
<1> This command will not wait if frame `f+1` has already passed.
====
\ref FRAME()
***/
void ivsync() {
  uint32_t tm;
  if (end_of_statement())
    tm = vs23.frame() + 1;
  else
    if ( getParam(tm, 0, INT32_MAX, I_NONE) )
      return;

  while (vs23.frame() < tm) {
    pump_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      break;
    }
  }
}

static const uint8_t vs23_write_regs[] PROGMEM = {
  0x01, 0x82, 0xb8, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x34, 0x35
};

/***bc scr VREG
Writes to a video controller register.

WARNING: Incorrect use of this command can configure the video controller to
produce invalid output, with may cause damage to older CRT displays.
\usage VREG reg, val[, val ...]
\args
@reg	video controller register number
@val	value [`0` to `65535`]
\note
Valid registers numbers are: `$01`, `$28` to `$30`, `$34`, `$35`, `$82` and `$b8`

The number of values required depends on the register accessed:
\table
| `$34`, `$35` | 3 values
| `$30` | 2 values
| all others | 1 value
\endtable
\ref VREG()
***/
void BASIC_INT ivreg() {
#ifdef USE_VS23
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
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc scr VPOKE
Write one byte of data to video memory.

WARNING: Incorrect use of this command can configure the video controller to
produce invalid output, with may cause damage to older CRT displays.
\usage VPOKE addr, val
\args
@addr	video memory address [`0` to `131071`]
@val	value [`0` to `255`]
\ref VPEEK()
***/
void BASIC_INT ivpoke() {
#ifdef USE_VS23
  int32_t addr, value;
  if (getParam(addr, 0, 131071, I_COMMA)) return;
  if (getParam(value, 0, 255, I_NONE)) return;
  SpiRamWriteByte(addr, value);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}
  
/***bc scr LOCATE
Moves the text cursor to the specified position.
\usage LOCATE x_pos, y_pos
\args
@x_pos	cursor X coordinate +
        [`0` to `CSIZE(0)-1`]
@y_pos	cursor Y coordinate +
        [`0` to `CSIZE(1)-1`]
\ref CSIZE()
***/
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

/***bc scr COLOR
Changes the foreground and background color for text output.
\usage COLOR fg_color[, bg_color]
\args
@fg_color	foreground color [`0` to `255`]
@bg_color	background color [`0` to `255`, default: `0`]
\ref RGB()
***/
void icolor() {
  int32_t fc,  bgc = 0;
  if ( getParam(fc, 0, 255, I_NONE) ) return;
  if(*cip == I_COMMA) {
    cip++;
    if ( getParam(bgc, 0, 255, I_NONE) ) return;
    if (*cip == I_COMMA) {
      ++cip;
      int cc;
      if (getParam(cc, 0, 255, I_NONE)) return;
      sc0.setCursorColor(cc);
    }
  }
  // 文字色の設定
  sc0.setColor((uint16_t)fc, (uint16_t)bgc);
}

int32_t BASIC_FP iinkey() {
  int32_t rc = 0;

  if (c_kbhit()) {
    rc = sc0.tryGetChar();
    process_hotkeys(rc);
  }

  return rc;
}

/***bf sys PEEK
Read a byte of data from an address in memory.
\usage v = PEEK(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
Memory at `addr` must allow byte-wise access.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEKD() PEEKW()
***/
/***bf sys PEEKW
Read a half-word (16 bits) of data from an address in memory.
\usage v = PEEKW(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
Memory at `addr` must allow byte-wise access, and `addr` must be 2-byte
aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEK() PEEKD()
***/
/***bf sys PEEKD
Read a word (32 bits) of data from an address in memory.
\usage v = PEEKD(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
`addr` must be 4-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEK() PEEKW()
***/
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

/***bf bas RET
Returns one of the numeric return values of the last function call.
\usage rval = RET(num)
\args
@num	number of the numeric return value [`0` to `{MAX_RETVALS_m1}`]
\ret Requested return value.
\ref RETURN RET$()
***/
num_t BASIC_FP nret() {
  int32_t r;

  if (checkOpen()) return 0;
  if ( getParam(r, 0, MAX_RETVALS-1, I_NONE) ) return 0;
  if (checkClose()) return 0;

  return retval[r];
}

/***bf bas ARG
Returns a numeric argument passed to a procedure.
\usage a = ARG(num)
\args
@num	number of the numeric argument
\ret Argument value.
\error
An error is generated if no numeric arguments have been passed,
`num` is equal to or larger than `ARGC(0)`, or the function is
evaluated outside a procedure.
\ref ARG$() ARGC()
***/
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

/***bf bas ARG$
Returns a string argument passed to a procedure.
\usage a$ = ARG$(num)
\args
@num	number of the string argument
\ret Argument value.
\error
An error is generated if no string arguments have been passed,
`num` is equal to or larger than `ARGC(1)`, or the function is
evaluated outside a procedure.
\ref ARG() ARGC()
***/
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

/***bf bas ARGC
Returns the argument count for numeric and string variables passed
to a procedure.
\usage cnt = ARGC(typ)
\args
@typ	type of argument [`0` for numeric, `1` for string]
\ret Argument count.
\note
Returns `0` if called outside a procedure.
\ref CALL FN
***/
num_t BASIC_FP nargc() {
  int32_t type = getparam();
  if (!gstki)
    return 0;
  if (type == 0)
    return gstk[gstki-1].num_args;
  else
    return gstk[gstki-1].str_args;
}

/***bf scr CHAR
Returns the text character on screen at a given position.
\usage c = CHAR(x, y)
\args
@x	X coordinate, characters
@y	Y coordinate, characters
\ret Character code.
\note
* If the coordinates exceed the dimensions of the text screen, `0` will be returned.
* The value returned represents the current character in text screen memory. It
  may not represent what is actually visible on screen if it has been manipulated
  using pixel graphics functions, such as `GPRINT`.
\ref CHAR GPRINT
***/
int32_t BASIC_INT ncharfun() {
  int32_t value; // 値
  int32_t x, y;  // 座標

  if (checkOpen()) return 0;
  if ( getParam(x, I_COMMA) ) return 0;
  if ( getParam(y, I_NONE) ) return 0;
  if (checkClose()) return 0;
  value = (x < 0 || y < 0 || x >=sc0.getWidth() || y >=sc0.getHeight()) ? 0 : sc0.vpeek(x, y);
  return value;
}

/***bc scr CHAR
Prints a single character to the screen.
\usage CHAR x, y, c
\args
@x	X coordinate, characters [`0` to `CSIZE(0)`]
@y	Y coordinate, characters [`0` to `CSIZE(1)`]
@c	character [`0` to `255`]
\bugs
No sanity checks are performed on arguments.
***/
void BASIC_FP ichar() {
  int32_t x, y, c;
  if ( getParam(x, I_COMMA) ) return;
  if ( getParam(y, I_COMMA) ) return;
  if ( getParam(c, I_NONE) ) return;
  sc0.write(x, y, c);
}

uint16_t pcf_state = 0xffff;

/***bc io GPOUT
Sets the state of a general-purpose I/O pin.
\usage GPOUT pin, value
\args
@pin	pin number [`0` to `15`]
@value	pin state [`0` for "off", anything else for "on"]
\note
`GPOUT` allows access to pins on the I2C I/O extender only.
\ref GPIN()
***/
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

/***bf bas HEX$
Returns a string containing the hexadecimal representation of a number.
\usage h$ = HEX$(num)
\args
@num	numeric expression
\ret Hexadecimal number as text.
\ref BIN$()
***/
BString shex() {
  int value; // 値
  if (checkOpen() || getParam(value, I_CLOSE))
    return BString();
  BString hex((unsigned int)value, 16);
  hex.toUpperCase();
  return hex;
}

/***bf bas BIN$
Returns a string containing the binary representation of a number.
\usage b$ = BIN$(num)
\args
@num	numeric expression
\ret Binary number as text.
\ref HEX$()
***/
BString sbin() {
  int32_t value; // 値

  if (checkOpen()) goto out;
  if (getParam(value, I_NONE)) goto out;
  if (checkClose()) goto out;
  return BString(value, 2);
out:
  return BString();
}

/***bc sys POKE
Write a byte to an address in memory.
\usage POKE addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow byte-wise access.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKED POKEW
***/
/***bc sys POKEW
Write a half-word (16 bits) to an address in memory.
\usage POKEW addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow half-word-wise access.
It must be 2-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKE POKED
***/
/***bc sys POKED
Write a word (32 bits) to an address in memory.
\usage POKED addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow half-word-wise access.
It must be 4-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKE POKEW
***/
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

/***bc sys SYS
Call a machine language routine.

WARNING: Using this command incorrectly may crash the system, or worse.
\usage SYS addr
\args
@addr	a memory address mapped as executable
\bugs
No sanity checks are performed on the address.
***/
void isys() {
  void (*sys)() = (void (*)())(uintptr_t)iexp();
  sys();
}

/***bf io I2CW
Sends data to an I2C device.
\usage res = I2CW(i2c_addr, out_data)
\args
@i2c_addr	I2C address [`0` to `$7F`]
@out_data	data to be transmitted
\ret
Status code of the transmission.
\ref I2CR()
***/
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

/***bf io I2CR
Request data from I2C device.
\usage in$ = I2CR(i2c_addr, out_data, read_length)
\args
@i2c_addr	I2C address [`0` to `$7F`]
@out_data	data to be transmitted
@read_length	number of bytes to be received
\ret
Returns the received data as the value of the function call.

Also returns the status code of the outward transmission in `RET(0)`.
\note
If `out_data` is an empty string, no data is sent before the read request.
\ref I2CW()
***/
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
    Wire.write((const uint8_t *)out.c_str(), out.length());
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

/***bc sys SET DATE
Sets the current date and time.
\usage SET DATE year, month, day, hour, minute, second
\args
@year	numeric expression [`1900` to `2036`]
@month	numeric expression [`1` to `12`]
@day	numeric expression [`1` to `31`]
@hour	numeric expression [`0` to `23`]
@minute	numeric expression [`0` to `59`]
@second	numeric expression [`0` to `61`]
\bugs
It is unclear why the maximum value for `second` is 61 and not 59.
\ref DATE GET_DATE
***/
void isetDate() {
#ifdef USE_INNERRTC
  int32_t p_year, p_mon, p_day;
  int32_t p_hour, p_min, p_sec;

  if ( getParam(p_year, 1900,2036, I_COMMA) ) return;  // 年
  if ( getParam(p_mon,     1,  12, I_COMMA) ) return;  // 月
  if ( getParam(p_day,     1,  31, I_COMMA) ) return;  // 日
  if ( getParam(p_hour,    0,  23, I_COMMA) ) return;  // 時
  if ( getParam(p_min,     0,  59, I_COMMA) ) return;  // 分
  // 61? WTF?
  if ( getParam(p_sec,     0,  61, I_NONE)) return;  // 秒

  setTime(p_hour, p_min, p_sec, p_day, p_mon, p_year);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void iset() {
  int32_t flag, val;

  if (*cip == I_DATE) {
    ++cip;
    isetDate();
    return;
  } else if (*cip == I_FLAGS) {
    ++cip;
    if (getParam(flag, 0, 0, I_COMMA)) return;
    if (getParam(val, 0, 1, I_NONE)) return;

    switch (flag) {
    case 0:	math_exceptions_disabled = val; break;
    default:	break;
    }
  } else {
    SYNTAX_T("exp DATE or FLAGS");
  }
}

/***bc sys GET DATE
Get the current date.

\usage GET DATE year, month, day, weekday
\args
@year		numeric variable
@month		numeric variable
@day		numeric variable
@weekday	numeric variable
\ret The current date in the given numeric variables.
\bugs
Only supports scalar variables, not array or list members.

WARNING: The syntax and semantics of this command are not consistent with
other BASIC implementations and may be changed in future releases.
\ref DATE GET_TIME SET_DATE
***/
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
      nvar.var(index) = v[i];
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

/***bc sys GET TIME
Get the current time.

\usage GET TIME hour, minute, second
\args
@hour	numeric variable
@minute	numeric variable
@second	numeric variable
\ret The current time in the given numeric variables.
\bugs
Only supports scalar variables, not array or list members.

WARNING: The syntax and semantics of this command are not consistent with
other BASIC implementations and may be changed in future releases.
\ref SET_DATE
***/
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
      nvar.var(index) = v[i];
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

void iget() {
  if (*cip == I_DATE) {
    ++cip;
    igetDate();
  } else if (*cip == I_TIME) {
    ++cip;
    igetTime();
  } else {
    SYNTAX_T("exp DATE or TIME");
  }
}

/***bc sys DATE
Prints the current date and time.

WARNING: The semantics of this command are not consistent with other BASIC
implementations and may be changed in future releases.
\usage DATE
\ref GET_DATE SET_DATE
***/
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

/***bc pix PSET
Draws a pixel.
\usage PSET x_coord, y_coord, color
\args
@x_coord X coordinate of the pixel
@y_coord Y coordinate of the pixel
\ref RGB()
***/
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

/***bc pix LINE
Draws a line.
\usage
LINE x1_coord, y1_coord, x2_coord, y2_coord, color
\args
@x1_coord X coordinate of the line's starting point +
          [`0` to `PSIZE(0)-1`]
@y1_coord Y coordinate of the line's starting point +
          [`0` to `PSIZE(2)-1`]
@x2_coord X coordinate of the line's end point +
          [`0` to `PSIZE(0)-1`]
@y2_coord Y coordinate of the line's end point +
          [`0` to `PSIZE(2)-1`]
@color	  color of the line
\note
Coordinates that exceed the valid pixel memory area will be clamped.
\ref PSIZE() RGB()
***/
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

/***bc pix CIRCLE
Draws a circle.

\usage CIRCLE x_coord, y_coord, radius, color, fill_color

\args
@x_coord	X coordinate of the circle's center +
                [`0` to `PSIZE(0)-1`]
@y_coord	Y coordinate of the circle's center +
                [`0` to `PSIZE(2)-1`]
@radius		circle's radius
@color		color of the circle's outline [`0` to `255`]
@fill_color	color of the circle's body +
                [`0` to `255`, or `-1` for an unfilled circle]
\note
Coordinates that exceed the valid pixel memory area will be clamped.
\bugs
`fill_color` cannot be omitted.
\ref PSIZE() RGB()
***/
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

/***bc pix RECT
Draws a rectangle.
\usage
RECT x1_coord, y1_coord, x2_coord, y2_coord, color, fill_color
\args
@x1_coord X coordinate of the rectangle's top/left corner +
          [`0` to `PSIZE(0)-1`]
@y1_coord Y coordinate of the rectangle's top/left corner +
          [`0` to `PSIZE(2)-1`]
@x2_coord X coordinate of the rectangle's bottom/right corner +
          [`0` to `PSIZE(0)-1`]
@y2_coord Y coordinate of the rectangle's bottom/right corner +
          [`0` to `PSIZE(2)-1`]
@color	  color of the rectangle's outline
@fill_color color of the rectangle's body +
            [`0` to `255`, or `-1` for an unfilled rectangle]
\bugs
`fill_color` cannot be omitted.
\ref PSIZE() RGB()
***/
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

/***bc pix BLIT
Copies a rectangular area of pixel memory to another area.
\usage BLIT x, y TO dest_x, dest_y SIZE width, height [<UP|DOWN>]
\args
@x	source area, X coordinate [`0` to `PSIZE(0)-1`]
@y	source area, Y coordinate [`0` to `PSIZE(2)-1`]
@dest_x	destination area, X coordinate [`0` to `PSIZE(0)-1`]
@dest_y	destination area, Y coordinate [`0` to `PSIZE(2)-1`]
@width	area width [`0` to `PSIZE(0)-x`]
@height	area height [`0` to `PSIZE(2)-y`]
\note
The default transfer direction is down.
\bugs
* Transfers only work up to sizes of 255 in each dimension.
* The semantics are very much tied to the hardware and not
  at all obvious. They are likely to be changed in future
  releases.
\ref GSCROLL
***/
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

/***bc scr CSCROLL
Scrolls the text screen contents in the given direction.
\usage CSCROLL x1, y1, x2, y2, direction
\args
@x1	start of the screen region, X coordinate, characters [`0` to `CSIZE(0)-1`]
@y1	start of the screen region, Y coordinate, characters [`0` to `CSIZE(1)-1`]
@x2	end of the screen region, X coordinate, characters [`x1+1` to `CSIZE(0)-1`]
@y2	end of the screen region, Y coordinate, characters [`y1+1` to `CSIZE(1)-1`]
@direction	direction
\sec DIRECTION
Valid values for `direction` are:
\table
| `0` | up
| `1` | down
| `2` | left
| `3` | right
\endtable
\bugs
Colors are lost when scrolling.
\ref BLIT GSCROLL CSIZE()
***/
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

/***bc pix GSCROLL
Scroll a portion screen contents in the given direction.
\usage GSCROLL x1, y1, x2, y2, direction
\args
@x1	start of the screen region, X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y1	start of the screen region, Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@x2	end of the screen region, X coordinate, pixels [`x1+1` to `PSIZE(0)-1`]
@y2	end of the screen region, Y coordinate, pixels [`y1+1` to `PSIZE(2)-1`]
@direction	direction
\sec DIRECTION
Valid values for `direction` are:
\table
| `0` | up
| `1` | down
| `2` | left
| `3` | right
\endtable
\ref BLIT CSCROLL PSIZE()
***/
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

/***bc io SWRITE
Writes a byte to the serial port.

WARNING: This command may be renamed in the future to reduce namespace
pollution.
\usage SWRITE c
\args
@c	byte to be written
\ref SMODE
***/
void iswrite() {
  int32_t c;
  if ( getParam(c, I_NONE) ) return;
  Serial.write(c);
}

/***bc io SMODE
Changes the serial port configuration.

WARNING: This command is likely to be renamed in the future to reduce
namespace pollution.
\usage SMODE baud[, flags]
\args
@baud	baud rate [`0` to `921600`]
@flags	serial port flags
\bugs
The meaning of `flags` is determined by `enum` values in the Arduino core
and cannot be relied on to remain stable.
\ref SWRITE
***/
void SMALL ismode() {
  int32_t baud, flags = SERIAL_8N1;
  if ( getParam(baud, 0, 921600, I_NONE) ) return;
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(flags, 0, 0x3f, I_NONE)) return;
  }
  Serial.begin(baud,
#ifdef ESP8266
    (SerialConfig)
#endif
    flags);
}

/***bc snd BEEP
Produces a sound using the "beeper" sound engine.
\usage BEEP period[, volume]
\args
@period	- tone cycle period in samples [`0` (off) to `320`/`160`]
@volume	- tone volume +
          [`0` to `15`, default: as set in system configuration]
\note
The maximum value for `period` depends on the flavor of Engine BASIC;
it's 320 for the gaming build, and 160 for the network build.
\ref CONFIG
***/
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

/***bc snd SOUND FONT
Sets the sound font to be used by the wavetable synthesizer.
\usage SOUND FONT file$
\args
@file$	name of the SF2 sound font file
\note
If a relative path is specified, Engine BASIC will first attempt
to load the sound font from `/sd`, then from `/flash`.

The default sound font name is `1mgm.sf2`.
\bugs
No sanity checks are performed before setting the sound font.
\ref INST$()
***/

/***bc snd SOUND
Generates a sound using the wavetable synthesizer.
\usage SOUND ch, inst, note[, len[, vel]]
\args
@ch	sound channel [`0` to `{SOUND_CHANNELS_m1}`]
@inst	instrument number
@note	note pitch
@len	note duration, milliseconds [default: 1000]
@vel	note velocity [`0` to `1` (default)]
\ref INST$() SOUND_FONT
***/
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
    int instcnt = sound.instCount();
    if (!instcnt) {
      err = ERR_TSF;
      return;
    }
    if (getParam(inst, 0, instcnt - 1, I_COMMA)) return;
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
/***bc snd PLAY
Plays a piece of music in Music Macro Language (MML) using the
wavetable synthesizer.
\usage PLAY mml$
\args
@mml$	score in MML format
\bugs
* Only supports one sound channel.
* MML syntax undocumented.
* Fails silently if the synthesizer could not be initialized.
***/
void iplay() {
#ifdef HAVE_MML
  sound.stopMml(0);
  mml_text = istrexp();
  sound.playMml(0, mml_text.c_str());
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc snd SAY
Speaks the given text.
\usage SAY text$
\args
@text$	text to be spoken
\note
`text$` is assumed to be in English.

`SAY` cannot be used concurrently with the wavetable synthesizer
because they use different sample rates.
\bugs
* Does not support phonetic input.
* No possibility to change synthesizer parameters.
***/
void isay() {
  BString text = istrexp();
  if (err)
    return;
  ESP8266SAM *sam = sound.sam();
  if (!sam) {
    err = ERR_OOM;
    return;
  }
  sam->Say(text.c_str());
}

/***bf pix POINT
Returns the color of the pixel at the given coordinates.
\usage px = POINT(x, y)
\args
@x	X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y	Y coordinate, pixels [`0` to `PSIZE(2)-1`]
\ret Pixel color [`0` to `255`]
***/
num_t BASIC_INT npoint() {
  int x, y;  // 座標
  if (checkOpen()) return 0;
  if ( getParam(x, 0, sc0.getGWidth()-1, I_COMMA)) return 0;
  if ( getParam(y, 0, vs23.lastLine()-1, I_NONE) ) return 0;
  if (checkClose()) return 0;
  return vs23.getPixel(x,y);
}

/***bf bas MAP
Re-maps a value from one range to another
\usage mval = MAP(val, l1, h1, l2, h2)
\args
@val	value to be re-mapped
@l1	lower bound of the value's current range
@h1	upper bound of the value's current range
@l2	lower bound of the value's target range
@h2	upper bound of the value's target range
\ret Re-mapped value.
\bugs
Restricts `value` to the range `l1`-`h1`, which is arbitrary.
***/
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

/***bf bas ASC
Returns the ASCII code for the first character in a string expression.
\usage val = ASC(s$)
\args
@s$	string expression
\ret ASCII code of the first character.
\error
Generates an error if `s$` is empty.
\ref CHR$()
***/
num_t BASIC_INT nasc() {
  int32_t value;

  if (checkOpen()) return 0;
  BString a = istrexp();
  if (a.length() < 1) {
    E_ERR(VALUE, "empty string");
    return 0;
  }
  value = a[0];
  checkClose();

  return value;
}

/***bc bas PRINT
Prints data to the current output device (usually the screen) or
to a given file.
\usage
PRINT [#file_num, ][*expressions*][<;|,>]
\args
@file_num	file number to be printed to [`0` to `{MAX_USER_FILES_m1}`,
                default: current output device]
@expressions	list of expressions specifying what to print
\sec EXPRESSIONS
The following types of expressions can be used in a `PRINT` command
expressions list:

* Numeric expressions
* String expressions
* `TAB(num)` (inserts spaces until the cursor is at or beyond the
  column `num`)

Expressions have to be separated by either a semicolon (`;`) or
a comma (`,`). The former concatenates expressions directly,
while the later inserts spaces until the next tabulator stop
is reached.
\bugs
`TAB()` does not work when output is redirected to a file using
`CMD`.
\ref CMD GPRINT
***/
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
      bool had_digit = false;	// encountered a #
      int leading = 0;
      int trailing = 0;		// decimal places
      BString prefix, suffix;	// random literal characters
      for (unsigned int i = 0; i < str.length(); ++i) {
        switch (str[i]) {
        case '#':
          if (!had_point) leading++; else trailing++;
          had_digit = true;
          break;
#ifdef FLOAT_NUMS
        case '.':
          if (had_point) {
            E_ERR(USING, "multiple periods");
            return;
          } else
            had_point = true;
          break;
#endif
        case '%': if (!had_point && !had_digit) prefix += F("%%"); else suffix += F("%%"); break;
        default:  if (!had_point && !had_digit) prefix += str[i]; else suffix += str[i]; break;
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
      continue;
      
    case I_TAB: {
      ++cip;
      int32_t col = getparam();
      if (col < 0)
        col = 0;
      col = col % sc0.getWidth();
      if (redirect_output_file < 0 && sc0.c_x() < col)
        sc0.locate(col, sc0.c_y());
      break;
    }

    case I_COMMA:
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
      while (*cip == I_COMMA) {
        cip++;
        if (devno == 0 && redirect_output_file < 0)
          do {
            c_putch(' ');
          } while (sc0.c_x() % 8);
        else
          c_putch('\t', devno);
        if (end_of_statement())
          return;
      }
    } else if (*cip == I_SEMI) {
      cip++;
      if (end_of_statement())
	return;
    } else {
      if (!end_of_statement()) {
        SYNTAX_T("exp separator");
	newline(devno);
	return;
      }
    }
  }
  if (!nonewln)
    newline(devno);
}

/***bc pix GPRINT
Prints a string of characters at the specified pixel position.
\desc
`GPRINT` allows printing characters outside the fixed text grid, including
off-screen pixel memory.
\usage GPRINT x, y, *expressions*
\args
@x	start point, X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y	start point, Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@expressions	`PRINT`-format list of expressions specifying what to draw
\note
* The expression list functions as it does in the `PRINT` command.
* The "new line" character that is generally appended at the end of a `PRINT`
  command is drawn literally by `GPRINT`, showing up as a funny character
  at the end of the drawn text. This can be avoided by appending a semicolon
  (`;`) to the print parameters.
\ref PRINT
***/
void igprint() {
#if USE_NTSC == 1
  int32_t x,y;
  if (scmode) {
    if ( getParam(x, 0, sc0.getGWidth()-1, I_COMMA) ) return;
    if ( getParam(y, 0, vs23.lastLine()-1, I_COMMA) ) return;
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

/***bc scr SAVE PCX
Saves a portion of pixel memory to storage as a PCX file.
\usage SAVE PCX image$ [POS x, y] [SIZE w, h]
\args
@image$	name of image file to be created
@x	left of pixel memory section, pixels +
        [`0` (default) to `PSIZE(0)-1`]
@y	top of pixel memory section, pixels +
        [`0` (default) to `PSIZE(2)-1`]
@w	width of pixel memory section, pixels +
        [`0` to `PSIZE(0)-x-1`, default: `PSIZE(0)`]
@h	height of pixel memory section, pixels +
        [`0` to `PSIZE(2)-y-1`, default: `PSIZE(1)`]
\ref LOAD_PCX
***/
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
    } else
      break;
  }
  
  err = bfs.saveBitmap((char *)fname.c_str(), x, y, w, h);
  return;
}

/***bc scr LOAD PCX
Loads a PCX image file in whole or in parts from storage to pixel memory.
\usage
LOAD PCX image$ [AS <BG bg|SPRITE *range*>] [TO dest_x, dest_y] [OFF x, y]
         [SIZE width, height] [KEY col]
\args
@bg		background number [`0` to `{MAX_BG_m1}`]
@range		sprite range [limits between `0` and `{MAX_SPRITES_m1}`]
@dest_x		destination X coordinate, pixels +
                [`0` (default) to `PSIZE(0)-w`]
@dest_y		destination Y coordinate, pixels +
                [`0` (default) to `PSIZE(2)-h`]
@x		offset within image file, X axis, pixels [default: `0`]
@y		offset within image file, Y axis, pixels [default: `0`]
@width		width of image portion to be loaded, pixels [default: image width]
@height		height of image portion to be loaded, pixels [default: image height]
@col		color key for transparency [default: no transparency]
\ret
Returns the destination coordinates in `RET(0)` and `RET(1)`, as well as width and
height in `RET(2)` and `RET(3)`, respectively.
\note
If no destination is specified, an area of off-screen memory will be allocated
automatically.

If `AS BG` is used, the loaded image portion will be assigned to the specified background
as its tile set.

If `AS SPRITE` is used, the loaded image portion will be assigned to the specified
range of sprites as their sprite patterns.

IMPORTANT: `AS BG` and `AS SPRITE` are not available in the network build.

If a color key is specified, pixels of the given color will not be drawn.
\ref BG SAVE_PCX SPRITE
***/
void SMALL ildbmp() {
  BString fname;
  int32_t dx = -1, dy = -1;
  int32_t x = 0,y = 0,w = -1, h = -1;
#ifdef USE_BG_ENGINE
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
#ifdef USE_BG_ENGINE
      cip++;
      if (*cip == I_BG) {		// AS BG ...
        ++cip;
        dx = dy = -1;
        if (getParam(bg,  0, MAX_BG-1, I_NONE)) return;
        define_bg = true;
      } else if (*cip == I_SPRITE) {	// AS SPRITE ...
        ++cip;
        dx = dy = -1;
        define_spr = true;
        if (!get_range(spr_from, spr_to)) return;
        if (spr_to == UINT32_MAX)
          spr_to = MAX_SPRITES - 1;
        if (spr_to > MAX_SPRITES-1 || spr_from > spr_to) {
          err = ERR_RANGE;
          return;
        }
      } else {
        SYNTAX_T("exp BG or SPRITE");
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
#ifdef USE_BG_ENGINE
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

/***bc fs MKDIR
Creates a directory.
\usage MKDIR directory$
\args
@directory$	path to the new directory
***/
void imkdir() {
  uint8_t rc;
  BString fname;

  if(!(fname = getParamFname())) {
    return;
  }

  rc = bfs.mkdir((char *)fname.c_str());
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
}

/***bc fs RMDIR
Deletes a directory.
\usage RMDIR directory$
\args
@directory$	name of directory to be deleted
\bugs
Does not support wildcard patterns.
\ref MKDIR
***/
void irmdir() {
  BString fname;
  uint8_t rc;

  if(!(fname = getParamFname())) {
    return;
  }

  rc = bfs.rmdir((char *)fname.c_str());
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_BAD_FNAME;
  }
}

/***bc fs RENAME
Changes the name of a file or directory.
\usage RENAME old$ TO new$
\args
@old$	current file name
@new$	new file name
\bugs
Does not support wildcard patterns.
***/
void irename() {
  bool rc;

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
  if (!rc)
    err = ERR_FILE_WRITE;
}

/***bc fs REMOVE
Deletes a file from storage.

WARNING: `REMOVE` does not ask for confirmation before deleting the
         specified file.

WARNING: Do not confuse with `DELETE`, which deletes lines from the
program in memory.
\usage REMOVE file$
\args
@file$	file to be deleted
\bugs
Does not support wildcard patterns.
\ref DELETE
***/
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

/***bc fs COPY
Copies a file.
\usage COPY file$ TO new_file$
\args
@file$		file to be copied
@new_file$	copy of the file to be created
\bugs
Does not support wildcard patterns.
***/
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

/***bc fs BSAVE
Saves an area of memory to a file.
\usage BSAVE file$, addr, len
\args
@file$	name of binary file
@addr	memory start address
@len	number of bytes to be saved
\note
`BSAVE` will use 32-bit memory accesses if `addr` and `len` are
multiples of 4, and 8-bit accesses otherwise.

While the `BSAVE` command does a basic sanity check of the given
parameters, it cannot predict any system malfunctions resulting
from its use. Use with caution.
\ref BLOAD
***/
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

/***bc fs BLOAD
Loads a binary file to memory.
\usage BLOAD file$ TO addr[, len]
\args
@file$	name of binary file
@addr	memory address to be loaded to
@len	number of bytes to be loaded [default: whole file]
\note
`BLOAD` will use 32-bit memory accesses if `addr` and `len` are
multiples of 4, and 8-bit accesses otherwise.

While the `BLOAD` command does a basic sanity check of the given
parameters, it cannot predict any system malfunctions resulting
from its use. Use with caution.
\ref BSAVE
***/
void SMALL ibload() {
  uint32_t vadr;
  int32_t len = -1;
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

/***bc fs TYPE
Writes the contents of a file to the screen. When a screen's worth of text
has been printed, it pauses and waits for a key press.
\usage TYPE file$
\args
@file$	name of the file
***/
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

  while(1) {
    rc = bfs.textOut((char *)fname.c_str(), line, sc0.getHeight());
    if (rc < 0) {
      err = -rc;
      return;
    } else if (rc == 0) {
      break;
    }

    c_puts("== More?('Y' or SPACE key) =="); newline();
    c = c_getch();
    if (c != 'y' && c!= 'Y' && c != 32)
      break;
    line += sc0.getHeight();
  }
}

/***bc scr WINDOW
Sets the text screen window.

`WINDOW OFF` resets the text window to cover the entire screen.
\usage
WINDOW x, y, w, h

WINDOW OFF
\args
@x	left boundary of the text window, in characters +
        [`0` to `CSIZE(0)-1`]
@y	top boundary of the text window, in characters +
        [`0` to `CSIZE(1)-1`]
@w	width of the text window, in characters +
        [`1` to `CSIZE(0)-x`]
@h	height of the text window, in characters +
        [`1` to `CSIZE(1)-y`]
\ref CSIZE()
***/
void BASIC_INT iwindow() {
  int32_t x, y, w, h;

#ifdef USE_BG_ENGINE
  // Discard the dimensions saved for CONTing.
  restore_text_window = false;
#endif

  if (*cip == I_OFF) {
    ++cip;
    sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
    sc0.setScroll(true);
    return;
  }

  if ( getParam(x,  0, sc0.getScreenWidth() - 1, I_COMMA) ) return;   // x
  if ( getParam(y,  0, sc0.getScreenHeight() - 1, I_COMMA) ) return;        // y
  if ( getParam(w,  1, sc0.getScreenWidth() - x, I_COMMA) ) return;   // w
  if ( getParam(h,  1, sc0.getScreenHeight() - y, I_NONE) ) return;        // h

  sc0.setWindow(x, y, w, h);
  sc0.setScroll(h > 1 ? true : false);
  sc0.locate(0,0);
  sc0.show_curs(false);
}

/***bc scr FONT
Sets the current text font.
\usage FONT font_num
\args
@font_num	font number [`0` to `{NUM_FONTS_m1}`]
\sec FONTS
The following fonts are available:
\table
| 0 | ATI console font, 6x8 pixels (default)
| 1 | CPC font, 8x8 pixels
| 2 | PETSCII font, 8x8 pixels
| 3 | Japanese font, 6x8 pixels
\endtable
\note
The font set at power-on can be set using the `CONFIG` command.
\ref CONFIG
***/
void ifont() {
  int32_t idx;
  if (getParam(idx, 0, NUM_FONTS - 1, I_NONE))
    return;
  sc0.setFont(fonts[idx]);
  sc0.forget();
}

/***bc scr SCREEN
Change the screen resolution.
\usage SCREEN mode
\args
@mode screen mode [`1` to `10`]
// XXX: No convenient way to auto-update the number of modes.
\sec MODES
The following modes are available:
\table header
| Mode | Resolution | Comment
| 1 | 460x224 | Maximum usable resolution in NTSC mode.
| 2 | 436x216 | Slightly smaller than mode 1; helpful if
                the display does not show the entirety of mode 1.
| 3 | 320x216 |
| 4 | 320x200 | Common resolution on many home computer systems.
| 5 | 256x224 | Compatible with the SNES system.
| 6 | 256x192 | Common resolution used by MSX, ZX Spectrum and others.
| 7 | 160x200 | Common low-resolution mode on several home computer systems.
| 8 | 352x240 | PC Engine-compatible overscan mode
| 9 | 282x240 | PC Engine-compatible overscan mode
| 10 | 508x240 | Maximum usable resolution in PAL mode. (Overscan on NTSC
                 systems.)
\endtable
\note
* While the resolutions are the same for NTSC and PAL configurations, the
  actual sizes and aspect ratios vary depending on the TV system.

* Resolutions have been chosen to match those used in other systems to
  facilitate porting of existing programs.

* Apart from changing the mode, `SCREEN` clears the screen, resets the font
  and color space (palette) to their defaults, and disables sprites and
  tiled backgrounds.
\ref BG CLS FONT PALETTE SPRITE
***/
void SMALL iscreen() {
  int32_t m;

  if ( getParam(m,  1, vs23.numModes(), I_NONE) ) return;   // m

#ifdef USE_BG_ENGINE
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
  sc0.init(SIZE_LINE,CONFIG.NTSC, m - 1);

  sc0.cls();
  sc0.show_curs(false);
  sc0.draw_cls_curs();
  sc0.locate(0,0);
  sc0.refresh();
}

/***bc scr PALETTE
Sets the color space ("palette") for the current screen mode, as well as
coefficients for the color conversion function `RGB()`.
\usage PALETTE pal[, hw, sw, vw, f]
\args
@pal	colorspace number [`0` or `1`]
@hw	hue weight for color conversion [`0` to `7`]
@sw	saturation weight for color conversion [`0` to `7`]
@vw	value weight for color conversion [`0` to `7`]
@f	conversion fix-ups enabled [`0` or `1`]
\note
The default component weights depend on the color space. They are:
\table header
|Color space | H | S | V
| 0 | 7 | 3 | 6
| 1 | 7 | 4 | 7
\endtable

Conversion fix-ups are enabled by default.
\ref RGB() SCREEN
***/
void ipalette() {
  int32_t p, hw, sw, vw, f;
  if (getParam(p, 0, CSP_NUM_COLORSPACES - 1, I_NONE)) return;
  vs23.setColorSpace(p);
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(hw, 0, 7, I_COMMA) ) return;
    if (getParam(sw, 0, 7, I_COMMA) ) return;
    if (getParam(vw, 0, 7, I_COMMA) ) return;
    if (getParam(f, 0, 1, I_NONE)) return;
    csp.setColorConversion(p, hw, sw, vw, !!f);
  }
}

/***bc scr BORDER
Sets the color of the screen border.

The color can be set individually for each screen column.
\usage BORDER uv, y[, x, w]
\args
@uv	UV component of the border color [`0` to `255`]
@y	Y component of the border color [`0` to `255`]
@x	first column to set [default: `0`]
@w	number of columns to set [default: all]
\note
There is no direct correspondence between border and screen color values.
\bugs
There is no way to find the allowed range of values for `x` and `w` without
trial and error.
***/
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

/***bc bg BG
Defines a tiled background's properties.
\desc
Using `BG`, you can define a tiled background's map and tile size, tile set,
window size and position, and priority, as well as turn it on or off.

`BG OFF` turns off all backgrounds.
\usage
BG bg [TILES w, h] [PATTERN px, py, pw] [SIZE tx, ty] [WINDOW wx, wy, ww, wh]
      [PRIO priority] [<ON|OFF>]
BG OFF
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@w	background map width, in tiles
@h	background map height, in tiles
@px	tile set X coordinate, pixels [`0` to `PSIZE(0)-1`]
@py	tile set Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@tx	tile width, pixels [`8` (default) to `32`]
@ty	tile height, pixels [`8` (default) to `32`]
@wx	window X coordinate, pixels [`0` (default) to `PSIZE(0)-1`]
@wy	window Y coordinate, pixels [`0` (default) to `PSIZE(1)-1`]
@ww	window width, pixels [`0` to `PSIZE(0)-wx` (default)]
@wh	window height, pixels [`0` to `PSIZE(1)-wy` (default)]
@priority Background priority [`0` to `{MAX_BG_m1}`, default: `bg`]
\note
* The `BG` command's attributes can be specified in any order, but it is
  usually a good idea to place the `ON` attribute at the end if used.
* A background cannot be turned on unless at least the `TILES` attribute
  is set. In order to get sensible output, it will probably be necessary
  to specify the `PATTERN` attribute as well.
\bugs
If a background cannot be turned on, no error is generated.
\ref LOAD_BG LOAD_PCX MOVE_BG SAVE_BG
***/
void ibg() {
#ifdef USE_BG_ENGINE
  int32_t m;
  int32_t w, h, px, py, pw, tx, ty, wx, wy, ww, wh, prio;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetBgs();
    return;
  }

  if (getParam(m, 0, MAX_BG, I_NONE)) return;

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
    if (getParam(prio, 0, MAX_PRIO, I_NONE)) return;
    vs23.setBgPriority(m, prio);
    break;
  default:
    cip--;
    if (!end_of_statement())
      SYNTAX_T("exp BG parameter");
    return;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg LOAD BG
Loads a background map from storage.
\usage LOAD BG bg, file$
\args
@bg	background number. [`0` to `{MAX_BG_m1}`]
@file$	name of background map file
\ref SAVE_BG
***/
void iloadbg() {
#ifdef USE_BG_ENGINE
  int32_t bg;
  uint8_t w, h, tsx, tsy;
  BString filename;

  cip++;

  if (getParam(bg, 0, MAX_BG, I_COMMA))
    return;
  if (!(filename = getParamFname()))
    return;
  
  Unifile f = Unifile::open(filename, UFILE_READ);
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

/***bc bg SAVE BG
Saves a background map to storage.
\usage SAVE BG bg TO file$
\args
@bg	background number. [`0` to `{MAX_BG_m1}`]
@file$	name of background map file
\bugs
Does not check if the specified background is properly defined.
\ref LOAD_BG
***/
void isavebg() {
#ifdef USE_BG_ENGINE
  int32_t bg;
  uint8_t w, h;
  BString filename;

  cip++;

  if (!(filename = getParamFname()))
    return;
  if (getParam(bg, 0, MAX_BG, I_TO))
    return;
  
  Unifile f = Unifile::open(filename, UFILE_OVERWRITE);
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

/***bc bg MOVE BG
Scrolls a tiled background.
\usage MOVE BG bg TO pos_x, pos_y
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@pos_x	top/left corner offset horizontal, pixels
@pos_y	top/left corner offset vertical, pixels
\note
There are no restrictions on the coordinates that can be used; backgrounds
maps wrap around if the display window extends beyond the map boundaries.
\ref BG
***/
void BASIC_FP imovebg() {
#ifdef USE_BG_ENGINE
  int32_t bg, x, y;
  if (getParam(bg, 0, MAX_BG, I_TO)) return;
  // XXX: arbitrary limitation?
  if (getParam(x, INT32_MIN, INT32_MAX, I_COMMA)) return;
  if (getParam(y, INT32_MIN, INT32_MAX, I_NONE)) return;
  
  vs23.scroll(bg, x, y);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg SPRITE
Defines a sprite's properties.
\desc
Using `SPRITE`, you can define a sprite's appearance, size, animation frame, color key,
prority, special effects, and turn it on and off.

`SPRITE OFF` turns all sprites off.
\usage
SPRITE num [PATTERN pat_x, pat_y][SIZE w, h][FRAME frame_x, frame_y]
           [FLAGS flags][KEY key][PRIO priority][<ON|OFF>]

SPRITE OFF
\args
@pat_x X 	coordinate of the top/left sprite pattern, pixels
@pat_y Y	coordinate of the top/left sprite pattern, pixels
@w		sprite width, pixels [`0` to `{MAX_SPRITE_W_m1}`]
@h		sprite height, pixels [`0` to `{MAX_SPRITE_H_m1}`]
@frame_x	X coordinate of the pattern section to be used,
                in multiples of sprite width
@frame_y	Y coordinate of the pattern section to be used,
                in multiples of sprite height
@flags		special effect flags [`0` to `7`]
@key		key color value to be used for transparency masking +
                [`0` to `255`]
@priority	sprite priority in relation to background layers +
                [`0` to `{MAX_BG_m1}`]

\sec FLAGS
The `FLAGS` attribute is the sum of any of the following bit values:
\table header
| Bit value | Effect
| `1` | Sprite opacity (opaque if set)
| `2` | Horizontal flip
| `4` | Vertical flip
\endtable
\note
The `SPRITE` command's attributes can be specified in any order, but it is
usually a good idea to place the `ON` attribute at the end if used.
\ref LOAD_PCX MOVE_SPRITE
***/
void BASIC_INT isprite() {
#ifdef USE_BG_ENGINE
  int32_t num, pat_x, pat_y, w, h, frame_x, frame_y, flags, key, prio;
  bool set_frame = false, set_opacity = false;

  if (*cip == I_OFF) {
    ++cip;
    vs23.resetSprites();
    return;
  }

  if (getParam(num, 0, MAX_SPRITES, I_NONE)) return;
  
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
    if (getParam(w, 1, MAX_SPRITE_W, I_COMMA)) return;
    if (getParam(h, 1, MAX_SPRITE_H, I_NONE)) return;
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
    if (getParam(prio, 0, MAX_PRIO, I_NONE)) return;
    vs23.setSpritePriority(num, prio);
    break;
  default:
    // XXX: throw an error if nothing has been done
    cip--;
    if (!end_of_statement())
      SYNTAX_T("exp sprite parameter");
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

/***bc bg MOVE SPRITE
Moves a sprite.
\usage MOVE SPRITE num TO pos_x, pos_y
\args
@num Sprite number [`0` to `{MAX_SPRITES_m1}`]
@pos_x Sprite position X coordinate, pixels
@pos_y Sprite position X coordinate, pixels
\note
There are no restrictions on the coordinates that can be used; sprites
that are placed completely outside the dimensions of the current screen
mode will not be drawn.
\ref SPRITE
***/
void BASIC_FP imovesprite() {
#ifdef USE_BG_ENGINE
  int32_t num, pos_x, pos_y;
  if (getParam(num, 0, MAX_SPRITES, I_TO)) return;
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
    SYNTAX_T("exp BG or SPRITE");
}

/***bc bg PLOT
Sets the value of one or more background tiles.
\usage PLOT bg, x, y, <tile|tile$>
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@x	tile X coordinate [`0` to background width in tiles]
@y	tile Y coordinate [`0` to background height in tiles]
@tile	tile identifier [`0` to `255`]
@tile$	tile identifiers
\note
If the numeric `tile` argument is given, a single tile will be set.
If a string of tiles (`tile$`) is passed, a tile will be set for each of the
elements of the string, starting from the specified tile coordinates and
proceeding horizontally.
\ref BG PLOT_MAP
***/
/***bc bg PLOT MAP
Creates a mapping from one tile to another.
\desc
Tile numbers depend on the locations of graphics data in memory and do not
usually represent printable characters, often making it difficult to handle
them in program text. By remapping a printable character to a tile as
represented by the graphics data, it is possible to use that human-readable
character in your program, making it easier to write and to understand.
\usage PLOT bg MAP from TO to
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@from	tile number to remap [`0` to `255`]
@to	tile number to map to [`0` to `255`]
\example
====
----
PLOT 0 MAP ASC("X") TO 1
PLOT 0 MAP ASC("O") TO 3
...
PLOT 0,5,10,"XOOOXXOO"
----
====
\ref PLOT
***/
void iplot() {
#ifdef USE_BG_ENGINE
  int32_t bg, x, y, t;
  if (getParam(bg, 0, MAX_BG, I_NONE)) return;
  if (!vs23.bgTiles(bg)) {
    // BG not defined
    err = ERR_RANGE;
    return;
  }
  if (*cip == I_MAP) {
    ++cip;
    if (getParam(x, 0, 255, I_TO)) return;
    if (getParam(y, 0, 255, I_NONE)) return;
    vs23.mapBgTile(bg, x, y);
  } else if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
  } else {
    if (getParam(x, 0, INT_MAX, I_COMMA)) return;
    if (getParam(y, 0, INT_MAX, I_COMMA)) return;
    if (is_strexp()) {
      BString dat = istrexp();
      vs23.setBgTiles(bg, x, y, (const uint8_t *)dat.c_str(), dat.length());
    } else {
      if (getParam(t, 0, 255, I_NONE)) return;
      vs23.setBgTile(bg, x, y, t);
    }
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg FRAMESKIP
Sets the number of frames that should be skipped before a new frame is rendered
by the BG/sprite engine.

This serves to increase the available CPU power to BASIC by reducing the load
imposed by the graphics subsystem.
\usage FRAMESKIP frm
\args
@frm	number of frames to be skipped [`0` (default) to `60`]
\note
It is not possible to mitigate flickering caused by overloading the graphics
engine using `FRAMESKIP` because each frame that is actually rendered must
be so within a single TV frame.
***/
void iframeskip() {
#ifdef USE_BG_ENGINE
  int32_t skip;
  if (getParam(skip, 0, 60, I_NONE)) return;
  vs23.setFrameskip(skip);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc sys CREDITS
Prints information on Engine BASIC's components, its authors and
licensing conditions.
\usage CREDITS
***/
void icredits() {
  c_puts_P(__credits);
}

/***bc sys XYZZY
Run Z-machine program.
\usage XYZZY file_name$
\args
@file_name$	name of a z-code file
***/
#include <azip.h>
void ixyzzy() {
  if (!is_strexp()) {
    PRINT_P("Nothing happens.\n");
    return;
  }
  BString game = getParamFname();
  if (err)
    return;
  if (!Unifile::exists(game.c_str())) {
    err = ERR_FILE_OPEN;
    return;
  }
  AZIP azip;
  azip.load(game.c_str());
  azip.run();
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

/***bf io PAD
Get the state of the game controller(s) and cursor pad.
\desc
`PAD()` can be used to query actual game controllers, or a "virtual" controller
simulated with cursor and letter keys on the keyboard.

The special controller number `0` returns the combined state of all (real and
virtual) controllers, making it easier to write programs that work with and
without a game controller.
\usage state = PAD(num[, state])
\args
@num Number of the game controller: +
     `0`: all controllers combined +
     `1`: cursor pad +
     `2`: PSX controller
@state	`0`: current button state (default) +
        `1`: button-change events
\ret
Bit field representing the button states of the requested controller(s). The
value is the sum of any of the following bit values:
\table header
| Bit value | PSX Controller | Keyboard
| `1` (aka `<<LEFT>>`) | kbd:[&#x25c4;] button | kbd:[Left] key
| `2` (aka `<<DOWN>>`) | kbd:[&#x25bc;] button | kbd:[Down] key
| `4` (aka `<<RIGHT>>`) | kbd:[&#x25ba;] button | kbd:[Right] key
| `8` (aka `<<UP>>`) | kbd:[&#x25b2;] button | kbd:[Up] key
| `16` | kbd:[Start] button | n/a
| `32` | kbd:[Select] button | n/a
| `256` | kbd:[&#x25a1;] button | kbd:[Z] key
| `512` | kbd:[&#x2715;] button | kbd:[X] key
| `1024` | kbd:[&#x25cb;] button | kbd:[S] key
| `2048` | kbd:[&#x25b3;] button | kbd:[A] key
| `4096` | kbd:[R1] button | n/a
| `8192` | kbd:[L1] button | n/a
| `16384` | kbd:[R2] button | n/a
| `32768` | kbd:[L2] button | n/a
\endtable

Depending on `state`, these bit values are set if the respective buttons are
currently pressed (`0`) or newly pressed (`1`).

If `state` is `1`, additional values are returned:

* `RET(1)` contains buttons that are newly released,
* `RET(2)` contains the current button state.
\note
WARNING: Using `PAD()` to retrieve button events (`state` is not `0`) interacts with
the event handling via `ON PAD`. It is not recommended to use both at the
same time.
\ref ON_PAD UP DOWN LEFT RIGHT
***/
num_t BASIC_INT npad() {
  int32_t num;
  int32_t state = 0;

  if (checkOpen()) return 0;

  if (getParam(num, 0, 2, I_NONE)) return 0;

  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(state, 0, 3, I_NONE)) return 0;
  }

  if (checkClose())
    return 0;

  int ps = pad_state(num);
  if (state) {
    int cs = ps ^ event_pad_last[num];
    retval[1] = cs & ~ps;
    retval[2] = ps;
    event_pad_last[num] = ps;
    ps &= cs;
  }
  return ps;
}

void BASIC_INT event_handle_pad()
{
  for (int i = 0; i < MAX_PADS; ++i) {
    if (event_pad_proc_idx[i] == NO_PROC)
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

/***bc sys EDIT
Runs the ASCII text editor.
\usage EDIT [file$]
\args
@file$	name of file to be edited [default: new file]
***/
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

/***bc bas LOAD
Load a program from storage.
\usage LOAD file$
\args
@file$	name of the BASIC program
\ref LOAD_BG LOAD_PCX SAVE
***/
/***bc bas MERGE
Merge a program in storage with the program currently in memory.
\usage MERGE file$[, line_num]
\args
@file$		name of the BASIC program to be merged
@line_num	line number at which to contine execution [default: first line]
\note
If called from within a program, `MERGE` will continue execution from line `line_num`
after the merge (or the beginning of the program if `line_num` is not specified).
\bugs
When called from within a program, `MERGE` resets all variables. This probably
limits its usefulness.
\ref CHAIN
***/
/***bc bas CHAIN
Loads a new program and runs it, keeping the current set of variables.
\usage CHAIN file$[, line_num]
\args
@file$		name of the BASIC program to be executed
@line_num	line number at which execution will start [default: first line]
\note
Unlike `EXEC`, `CHAIN` does not allow returning to the original program.
\ref EXEC LOAD
***/
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
  if (*(cip-1) == I_LOAD || *(cip-1) == I_RUN) {
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
    SYNTAX_T("exp file name");
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
      initialize_proc_pointers();
      initialize_label_pointers();
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
#ifdef USE_BG_ENGINE
      vs23.resetSprites();
      vs23.resetBgs();
#endif
      inew(NEW_VAR);
    }
    // もしプログラムの実行中なら（cipがリストの中にあり、clpが末尾ではない場合）
    if (cip >= listbuf && cip < listbuf + size_list && *clp && !flgCmd) {
      // エラーメッセージを表示
      sc0.setColor(COL(PROC), COL(BG));
      c_puts_P(errmsg[err]);
      sc0.setColor(COL(FG), COL(BG));
      PRINT_P(" in ");
      putnum(getlineno(clp), 0); // 行番号を調べて表示
      if (err_expected) {
        PRINT_P(" (");
        c_puts_P(err_expected);
        PRINT_P(")");
      }
      newline();

      // リストの該当行を表示
      putnum(getlineno(clp), 0);
      c_putch(' ');
      int mark = putlist(clp);
      newline();
      if (mark >= 0) {
        for (int i = 0; i < mark; ++i)
          c_putch(' ');
        sc0.setColor(COL(PROC), COL(BG));
        c_putch('^');
        sc0.setColor(COL(FG), COL(BG));
        if (mark < 3)
          newline();
        else
          sc0.locate(0, sc0.c_y());
      }
      //err = 0;
      //return;
    } else {                   // 指示の実行中なら
      c_puts_P(errmsg[err]);     // エラーメッセージを表示
      if (err_expected) {
        PRINT_P(" (");
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

/***bf scr RGB
Converts RGB value to hardware color value.
\desc
All Engine BASIC commands that require a color value expect that value
to be in the video controller's format, which depends on the <<PALETTE>>
setting.

RGB() can be used to calculate hardware color values from the more easily
understood RGB format, and can also help with programs that need to work
with different color palettes.
\usage color = RGB(red, green, blue)
\args
@red	red component [0 to 255]
@green	green component [0 to 255]
@blue	blue component [0 to 255]
\ret YUV color value
\note
The color conversion method used is optimized for use with pixel art.
Its results can be tweaked by setting conversion coefficients with
the `PALETTE` command.
\ref PALETTE COLOR
***/
num_t BASIC_FP nrgb() {
  int32_t r, g, b;
  if (checkOpen() ||
      getParam(r, 0, 255, I_COMMA) ||
      getParam(g, 0, 255, I_COMMA) ||
      getParam(b, 0, 255, I_CLOSE)) {
    return 0;
  }
  return csp.colorFromRgb(r, g, b);
}

static inline bool is_var(unsigned char tok)
{
  return tok >= I_VAR && tok <= I_STRLSTREF;
}

void BASIC_FP icall();

int BASIC_INT get_filenum_param() {
  int32_t f = getparam();
  if (f < 0 || f >= MAX_USER_FILES) {
    E_VALUE(0, MAX_USER_FILES - 1);
    return -1;
  } else
    return f;
}

BString BASIC_INT ilrstr(bool right) {
  BString value;
  int len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(len, I_CLOSE)) goto out;
  if (len < 0) {
    E_ERR(VALUE, "negative substring length");
    goto out;
  }

  if (right) {
    value = value.substring(_max(0, (int)value.length() - len), value.length());
  } else
    value = value.substring(0, len);

out:
  return value;
}

/***bf bas LEFT$
Returns a specified number of leftmost characters in a string.
\usage s$ = LEFT$(l$, num)
\args
@l	any string expression
@num	number of characters to return [min `0`]
\ret Substring of at most the length specified in `num`.
\note
If `l$` is shorter than `num` characters, the return value is `l$`.
\ref MID$() RIGHT$()
***/
static BString BASIC_INT sleft() {
  return ilrstr(false);
}
/***bf bas RIGHT$
Returns a specified number of rightmost characters in a string.
\usage s$ = RIGHT$(r$, num)
\args
@r	any string expression
@num	number of characters to return [min `0`]
\ret Substring of at most `num` characters.
\note
If `r$` is shorter than `num` characters, the return value is `r$`.
\ref LEFT$() MID$()
***/
static BString BASIC_INT sright() {
  return ilrstr(true);
}

/***bf bas MID$
Returns part of a string (a substring).
\usage s$ = MID$(m$, start[, len])
\args
@m$	any string expression
@start	position of the first character in the substring being returned
@len	number of characters in the substring [default: `LEN(m$)-start`]
\ret Substring of at most `len` characters.
\note
* If `m$` is shorter than `len` characters, the return value is `m$`.
* Unlike with other BASIC implementations, `start` is zero-based, i.e. the
  first character is 0, not 1.
\bugs
`MID$()` cannot be used as the target of an assignment, as is possible in
other BASIC implementations.
\ref LEFT$() LEN() RIGHT$()
***/
BString BASIC_INT smid() {
  BString value;
  int32_t start;
  int32_t len;

  if (checkOpen()) goto out;

  value = istrexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    goto out;
  }

  if (getParam(start, I_NONE)) goto out;
  if (start < 0) {
    E_ERR(VALUE, "negative string offset");
    goto out;
  }
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(len, I_NONE)) goto out;
  } else {
    len = value.length() - start;
  }
  if (checkClose()) goto out;

  value = value.substring(start, start + len);

out:
  return value;
}

/***bf fs DIR$
Returns the next entry of an open directory.
\usage d$ = DIR$(dir_num)
\args
@dir_num	number of an open directory
\ret Returns the directory entry as the value of the function.

Also returns the entry's size in `RET(0)` and `0` or `1` in RET(1)
depending on whether the entry is a directory itself.
\error
An error is generated if `dir_num` is not open or not a directory.
***/
BString sdir()
{
  int32_t fnum = getparam();
  if (err)
    goto out;
  if (fnum < 0 || fnum >= MAX_USER_FILES) {
    E_VALUE(0, MAX_USER_FILES - 1);
out:
    return BString(F(""));
  }
  if (!user_files[fnum] || !*user_files[fnum]) {
    err = ERR_FILE_NOT_OPEN;
    goto out;
  }
  if (!user_files[fnum]->isDirectory()) {
    err = ERR_NOT_DIR;
    goto out;
  }
  auto dir_entry = user_files[fnum]->next();
  retval[0] = dir_entry.size;
  retval[1] = dir_entry.is_directory;
  return dir_entry.name;
}

/***bf fs INPUT$
Returns a string of characters read from a specified file.
\usage dat$ = INPUT$(len, [#]file_num)
\args
@len		number of bytes to read
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Data read from file.
\ref INPUT
***/
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

/***bf bas CHR$
Returns the character corresponding to a specified ASCII code.
\usage char = CHR$(val)
\args
@val	ASCII code
\ret Single-character string.
\ref ASC()
***/
static BString schr() {
  int32_t nv;
  BString value;
  if (checkOpen()) return value;
  if (getParam(nv, 0,255, I_NONE)) return value; 
  value = BString((char)nv);
  checkClose();
  return value;
}

/***bf bas STR$
Returns a string representation of a number.
\usage s$ = STR$(num)
\args
@num	any numeric expression
\ret String representation of `num`.
\ref VAL()
***/
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

/***bf fs CWD$
Returns the current working directory.
\usage dir$ = CWD$()
\ret Current working directory.
\ref CHDIR
***/
static BString scwd() {
  if (checkOpen() || checkClose()) return BString();
  return Unifile::cwd();
}

/***bn bas INKEY$
Reads a character from the keyboard.
\usage c$ = INKEY$
\ret
Returns either

* an empty string if there is no keyboard input,
* a single-character string for regular keys,
* a two-character string for extended keys.

An "extended key" is a key that does not have a common ASCII representation,
such as cursor or function keys.
\note
`INKEY$` is not consistent with the Engine BASIC convention of following
functions with parentheses to distinguish them from constants and variables
in order to remain compatible with other BASIC implementations.
\ref INKEY()
***/
static BString BASIC_INT sinkey() {
  int32_t c = iinkey();
  if (c > 0 && c < 0x100) {
    return BString((char)c);
  } else if (c >= 0x100) {
    return BString((char)(c >> 8)) + BString((char)(c & 255));
  } else
    return BString();
}

/***bf bas POPF$
Remove an element from the beginning of a string list.
\usage e$ = POPF$(~list$)
\args
@~list$	reference to a string list
\ret Value of the element removed.
\error
An error is generated if `~list$` is empty.
\ref POPB$()
***/
static BString BASIC_INT spopf() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).front();
    str_lst.var(*cip++).pop_front();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp string list reference");
    return value;
  }
  checkClose();
  return value;
}

/***bf bas POPB$
Remove an element from the end of a string list.
\usage e$ = POPB$(~list$)
\args
@~list$	reference to a string list
\ret Value of the element removed.
\error
An error is generated if `~list$` is empty.
\ref POPF$()
***/
static BString BASIC_INT spopb() {
  BString value;
  if (checkOpen()) return value;
  if (*cip++ == I_STRLSTREF) {
    value = str_lst.var(*cip).back();
    str_lst.var(*cip++).pop_back();
  } else {
    if (is_var(cip[-1]))
      err = ERR_TYPE;
    else
      SYNTAX_T("exp string list reference");
    return value;
  }
  checkClose();
  return value;
}

/***bf snd INST$
Returns the name of the specified wavetable synthesizer instrument.
\usage name$ = INST$(num)
\args
@num	instrument number
\ret
Instrument name.

If an instrument is not defined in the current sound font, an empty
string is returned.
\ref SOUND_FONT
***/
static BString sinst() {
#ifdef HAVE_TSF
  return sound.instName(getparam());
#else
  err = ERR_NOT_SUPPORTED;
  return BString();
#endif
}

/***bf bas RET$
Returns one of the string return values returned by the last function
call.
\usage rval$ = RET$(num)
\args
@num	number of the string return value [`0` to `{MAX_RETVALS_m1}`]
\ret String return value requested.
\ref RET() RETURN
***/
static BString BASIC_INT sret() {
  int32_t n = getparam();
  if (n < 0 || n >= MAX_RETVALS) {
    E_VALUE(0, MAX_RETVALS-1);
    return BString(F(""));
  }
  return retstr[n];
}

/***bf bas ERROR$
Returns the error message associated with a given error number.
\usage msg$ = ERROR$(num)
\args
@num	error number [`0` to `{max_err}`]
\ret Error message.
\ref ON_ERROR RET()
***/
static BString serror() {
  uint32_t code = getparam();
  if (code >= sizeof(errmsg) / sizeof(*errmsg)) {
    E_VALUE(0, sizeof(errmsg) / sizeof(*errmsg) - 1);
    return BString(F(""));
  } else
    return BString(FPSTR(errmsg[code]));
}

/***bf bas STRING$
Returns a string of a specified length made up of a repeating character.
\usage s$ = STRING$(count, char$)
\args
@count	number of characters [at least `0`]
@char$	any non-empty string expression
\note
* If `char$` contains more than one character, only the first will be considered.
* If `count` is `0`, an empty string will be returned.
\example
====
----
PRINT STRING$(5, "-");
PRINT "Hello";
PRINT STRING$(5, "-")
----
====
***/
static BString sstring() {
  BString out;
  int32_t count;
  int32_t c;
  if (checkOpen()) return out;
  if (getParam(count, I_COMMA)) return out;
  if (count < 0) {
    E_ERR(VALUE, "negative length");
    return out;
  }
  if (is_strexp()) {
    BString cs = istrexp();
    if (err)
      return cs;
    if (cs.length() < 1) {
      E_ERR(VALUE, "need min 1 character");
      return cs;
    }
    c = cs[0];
    if (checkClose()) return cs;
  } else {
    if (getParam(c, 0, 255, I_CLOSE)) return out;
  }
  if (!out.reserve(count)) {
    err = ERR_OOM;
    return out;
  }
  memset(out.begin(), c, count);
  out.begin()[count] = 0;
  out.resetLength(count);
  
  return out;
}

typedef BString (*strfun_t)();
#include "strfuntbl.h"

static inline bool BASIC_FP is_strexp() {
  // XXX: does not detect string comparisons (numeric)
  return ((*cip >= STRFUN_FIRST && *cip < STRFUN_LAST) ||
          *cip == I_STR ||
          ((*cip == I_SVAR || *cip == I_LSVAR) && cip[2] != I_SQOPEN) ||
          *cip == I_STRARR ||
          *cip == I_STRLST ||
          *cip == I_STRSTR ||
          *cip == I_INPUTSTR ||
          (*cip == I_NET && (cip[1] == I_INPUTSTR || cip[1] == I_GETSTR)) ||
          *cip == I_ERRORSTR
         );
}

BString BASIC_INT istrvalue()
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
    value = str_arr.var(i).var(dims, idxs);
    break;

  case I_STRLST:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != 1) {
      SYNTAX_T("invalid list index");
    } else {
      value = str_lst.var(i).var(idxs[0]);
    }
    break;

  case I_INPUTSTR:	value = sinput(); break;
  case I_ERRORSTR:	value = serror(); break;
  case I_NET:
#ifndef HAVE_NETWORK
    err = ERR_NOT_SUPPORTED;
#else
    if (*cip == I_INPUTSTR) {
      ++cip;
      value = snetinput();
    } else if (*cip == I_GETSTR) {
      ++cip;
      value = snetget();
    } else
      SYNTAX_T("exp network function");
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
      SYNTAX_T("exp string expr");
    break;
  }
  if (err)
    return BString();
  else
    return value;
}

BString BASIC_INT istrexp()
{
  BString value, tmp;
  
  value = istrvalue();

  for (;;) switch(*cip) {
/***bo sop + (strings)
String concatenation operator.
\usage a$ + b$
\res Concatenation of `a$` and `b$`.
***/
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

/***bf bas LEN
Returns the number of characters in a string or the number of elements in
a list.
\usage
sl = LEN(string$)

ll = LEN(~list)
\args
@string$	any string expression
@~list		reference to a numeric or string list
\ret Length in characters (string) or elements (list).
***/
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

// see getrnd() for basicdoc
num_t BASIC_FP nrnd() {
  num_t value = getparam(); //括弧の値を取得
  if (!err)
    value = getrnd(value);
  return value;
}

/***bf m ABS
Returns the absolute value of a number.
\usage a = ABS(num)
\args
@num	any numeric expression
\ret Absolute value of `num`.
***/
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

/***bf m SIN
Returns the sine of a specified angle.
\usage s = SIN(angle)
\args
@angle	an angle expressed in radians
\ret Sine of `angle`.
\ref COS() TAN()
***/
num_t BASIC_FP nsin() {
  return sin(getparam());
}
/***bf m COS
Returns the cosine of a specified angle.
\usage c = COS(angle)
\args
@angle	an angle expressed in radians
\ret Cosine of `angle`.
\ref SIN() TAN()
***/
num_t BASIC_FP ncos() {
  return cos(getparam());
}
/***bf m EXP
Returns _e_ raised to a specified power, where _e_ is the base of natural
logarithms.
\usage ex = EXP(pow)
\args
@pow	Power to raise _e_ to.
\ret Exponential value of `pow`.
***/
num_t BASIC_FP nexp() {
  return exp(getparam());
}
/***bf m ATN
Arc tangent function.
\usage a = ATN(x)
\args
@x	any numeric expression
\desc
`ATN()` calculates the principal value of the arc tangent of `x`; that  is the
value whose tangent is `x`.
\ret The principal value of the arc tangent of `x` in radians.
\ref ATN2()
***/
num_t BASIC_FP natn() {
  return atan(getparam());
}

/***bf m ATN2
Arc tangent function of two variables.
\usage a2 = ATN2(x, y)
\args
@x	any numeric expression
@y	any numeric expression
\desc
Calculates the principal value of the arc tangent of `y`/`x`, using the signs of
the two arguments to determine the quadrant of the result.
\ret Arc tangent of `y`/`x`.
\ref ATN()
***/
num_t BASIC_FP natn2() {
  num_t value, value2;
  if (checkOpen() || getParam(value, I_COMMA) || getParam(value2, I_CLOSE))
    return 0;
  return atan2(value, value2);
}

/***bf m SQR
Square root function.
\usage s = SQR(num)
\args
@num	any numeric expression larger than or equal to `0`
\ret The nonnegative square root of `num`.
***/
num_t BASIC_FP nsqr() {
  return sqrt(getparam());
}

/***bf m TAN
Returns the tangent of a specified angle.
\usage t = TAN(angle)
\args
@angle	an angle expressed in radians
\ret Tangent of `angle`.
\ref COS() SIN()
***/
num_t BASIC_FP ntan() {
  return tan(getparam());
}

/***bf m LOG
Returns the natural logarithm of a numeric expression.
\usage l = LOG(num)
\args
@num	any numeric expression
\ret The natural logarithm of `num`.
***/
num_t BASIC_FP nlog() {
  return log(getparam());
}

/***bf m INT
Returns the largest integer less than or equal to a numeric expression.
\usage i = INT(num)
\args
@num	any numeric expression
\ret `num` rounded down to the nearest integer.
***/
num_t BASIC_FP nint() {
  return floor(getparam());
}

/***bf m SGN
Returns a value indicating the sign of a numeric expression.
\usage s = SGN(num)
\args
@num	any numeric expression
\ret
`1` if the expression is positive, `0` if it is zero, or `-1` if it is
negative.
***/
num_t BASIC_FP nsgn() {
  num_t value = getparam();
  return (0 < value) - (value < 0);
}

/***bf sys SYS
Retrieve internal system addresses and parameters.
\usage a = SYS(item)
\args
@item	number of the item of information to be retrieved
\ret Requested information.
\sec ITEMS
The following internal information can be retrieved using `SYS()`:
\table
| `0` | memory address of BASIC program buffer
| `1` | memory address of current font
\endtable
***/
num_t nsys() {
  int32_t item = getparam();
  if (err)
    return 0;
  switch (item) {
  case 0:	return (uint32_t)listbuf;
  case 1:	return (uint32_t)sc0.getfontadr();
  default:	E_VALUE(0, 1); return 0;
  }
}

/***bf io GPIN
Reads the state of a general-purpose I/O pin.
\usage s = GPIN(pin)
\args
@pin	pin number [`0` to `15`]
\ret State of pin: `0` for low, `1` for high.
\note
`GPIN()` allows access to pins on the I2C I/O extender only.
\ref GPOUT
***/
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

/***bf io ANA
Reads value from the analog input pin.
\usage v = ANA()
\ret Analog value read.
***/
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

/***bf io SREAD
Reads bytes from the serial port.

WARNING: This function is currently useless because the serial port
receive pin is used for sound output in the BASIC Engine. It may be
removed in the future if it turns out that there is no viable way
to support serial input.
\usage b = SREAD()
\ret Byte of data read, or `-1` if there was no data available.
\ref SREADY()
***/
num_t BASIC_INT nsread() {
  if (checkOpen()||checkClose()) return 0;
  return Serial.read();
}

/***bf io SREADY
Checks for available bytes on the serial port.

WARNING: This function is currently useless because the serial port
receive pin is used for sound output in the BASIC Engine. It may be
removed in the future if it turns out that there is no viable way
to support serial input.
\usage n = SREADY()
\ret Number of bytes available to read.
\ref SREAD()
***/
num_t BASIC_INT nsready() {
  if (checkOpen()||checkClose()) return 0;
  return Serial.available();
}

/***bf scr CSIZE
Returns the dimensions of the current screen mode in text characters.
\usage size = CSIZE(dimension)
\args
@dimension	requested screen dimension: +
                `0`: text screen width +
                `1`: text screen height
\ret Size in characters.
\ref FONT SCREEN
***/
num_t BASIC_FP ncsize() {
  // 画面サイズ定数の参照
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.getWidth();
  case 1:	return sc0.getHeight();
  default:	E_VALUE(0, 1); return 0;
  }
}

/***bf scr PSIZE
Returns the dimensions of the current screen mode in pixels.
\usage size = PSIZE(dimension)
\args
@dimension	requested screen dimension: +
                `0`: screen width +
                `1`: visible screen height +
                `2`: total screen height
\ret Size in pixels.
\ref SCREEN
***/
num_t BASIC_FP npsize() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.getGWidth();
  case 1:	return sc0.getGHeight();
  case 2:	return vs23.lastLine();
  default:	E_VALUE(0, 2); return 0;
  }
}

/***bf scr POS
Returns the text cursor position.
\usage position = POS(axis)
\args
@axis	requested axis: +
        `0`: horizontal +
        `1`: vertical
\ret Position in characters.
***/
num_t BASIC_FP npos() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.c_x();
  case 1:	return sc0.c_y();
  default:	E_VALUE(0, 1); return 0;
  }
}

/***bf fs COMPARE
Compares two files.
\usage result = COMPARE(file1$, file2$)
\args
@file1$	name of first file to compare
@file2$ name of second file to compare
\ret
If both files are equal, the return value is `0`. Otherwise, the return
value is `-1` or `1`.

If the files are not equal, the sign of the result is determined by the sign
of the difference between the first pair of bytes that differ in `file1$`
and `file2$`.
***/
num_t ncompare() {
  if (checkOpen()) return 0;
  BString one = getParamFname();
  if (err)
    return 0;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return 0;
  }
  BString two = getParamFname();
  if (err || checkClose()) return 0;
  return bfs.compare(one.c_str(), two.c_str());
}

/***bn io UP
Value of the "up" direction for input devices.
\ref PAD() DOWN LEFT RIGHT
***/
num_t BASIC_FP nup() {
  // カーソル・スクロール等の方向
  return psxUp;
}

/***bn io RIGHT
Value of the "right" direction for input devices.
\ref PAD() UP DOWN LEFT
***/
num_t BASIC_FP nright() {
  return psxRight;
}
/***bn io LEFT
Value of the "left" direction for input devices.
\ref PAD() UP DOWN RIGHT
***/
num_t BASIC_FP nleft() {
  return psxLeft;
}

/***bf bg TILECOLL
Checks if a sprite has collided with a background tile.
\usage res = TILECOLL(spr, bg, tile)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
@bg	background number [`0` to `{MAX_BG_m1}`]
@tile	tile number [`0` to `255`]
\ret
If the return value is `0`, no collision has taken place. Otherwise
the return value is the sum of `64` and any of the following
bit values:
\table header
| Bit value | Meaning
| `1` (aka `<<LEFT>>`) | The tile is left of the sprite.
| `2` (aka `<<DOWN>>`) | The tile is below the sprite.
| `4` (aka `<<RIGHT>>`) | The tile is right of the sprite.
| `8` (aka `<<UP>>`) | The tile is above the sprite.
\endtable

Note that it is possible that none of these bits are set even if
a collision has taken place (i.e., the return value is `64`) if
the sprite's size is larger than the tile size of the given
background layer, and the sprite covers the tile completely.
\bugs
WARNING: This function has never been tested.

Unlike `SPRCOLL()`, tile collision detection is not pixel-accurate.
\ref SPRCOLL()
***/
num_t BASIC_FP ntilecoll() {
#ifdef USE_BG_ENGINE
  int32_t a, b, c;
  if (checkOpen()) return 0;
  if (getParam(a, 0, MAX_SPRITES, I_COMMA)) return 0;
  if (getParam(b, 0, MAX_BG, I_COMMA)) return 0;
  if (getParam(c, 0, 255, I_NONE)) return 0;
  if (checkClose()) return 0;
  return vs23.spriteTileCollision(a, b, c);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf bg SPRCOLL
Checks if a sprite has collided with another sprite.
\usage res = SPRCOLL(spr1, spr2)
\args
@spr1	sprite number 1 [`0` to `{MAX_SPRITES_m1}`]
@spr2	sprite number 2 [`0` to `{MAX_SPRITES_m1}`]
\ret
If the return value is `0`, no collision has taken place. Otherwise
the return value is the sum of `64` and any of the following
bit values:
\table header
| Bit value | Meaning
| `1` (aka `<<LEFT>>`) | Sprite 2 is left of sprite 1.
| `2` (aka `<<DOWN>>`) | Sprite 2 is below sprite 1.
| `4` (aka `<<RIGHT>>`) | Sprite 2 is right of sprite 1.
| `8` (aka `<<UP>>`) | Sprite 2 is above sprite 1.
\endtable

It is possible that none of these bits are set even if
a collision has taken place (i.e., the return value is `64`) if
sprite 1 is larger than sprite 2 and covers it completely.
\note
* Collisions are only detected if both sprites are enabled.
* Sprite/sprite collision detection is pixel-accurate, i.e. a collision is
only detected if visible pixels of two sprites overlap.
\bugs
Does not return an intuitive direction indication if sprite 2 is larger than
sprite 1 and covers it completely. (It will indicate that sprite 2 is
up/left of sprite 1.)
\ref TILECOLL()
***/
num_t BASIC_FP nsprcoll() {
#ifdef USE_BG_ENGINE
  int32_t a, b;
  if (checkOpen()) return 0;
  if (getParam(a, 0, MAX_SPRITES, I_COMMA)) return 0;
  if (getParam(b, 0, MAX_SPRITES, I_NONE)) return 0;
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

/***bf bg SPRX
Returns the horizontal position of a given sprite.
\usage p = SPRX(spr)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
\ret
X coordinate of sprite `spr`.
\ref MOVE_SPRITE SPRY()
***/
num_t BASIC_FP nsprx() {
#ifdef USE_BG_ENGINE
  int32_t spr;

  if (checkOpen() ||
      getParam(spr, 0, MAX_SPRITES, I_CLOSE))
    return 0;

  return vs23.spriteX(spr);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}
/***bf bg SPRY
Returns the vertical position of a given sprite.
\usage p = SPRY(spr)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
\ret
Y coordinate of sprite `spr`.
\ref MOVE_SPRITE SPRX()
***/
num_t BASIC_FP nspry() {
#ifdef USE_BG_ENGINE
  int32_t spr;

  if (checkOpen() ||
      getParam(spr, 0, MAX_SPRITES, I_CLOSE))
    return 0;

  return vs23.spriteY(spr);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf bg BSCRX
Returns the horizontal scrolling offset of a given background layer.
\usage p = BSCRX(bg)
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
\ret
X scrolling offset of background `bg`.
\ref BG BSCRY()
***/
num_t BASIC_FP nbscrx() {
#ifdef USE_BG_ENGINE
  int32_t bg;

  if (checkOpen() ||
      getParam(bg, 0, MAX_BG-1, I_CLOSE))
    return 0;

  return vs23.bgScrollX(bg);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}
/***bf bg BSCRY
Returns the vertical scrolling offset of a given background layer.
\usage p = BSCRY(bg)
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
\ret
Y scrolling offset of background `bg`.
\ref BG BSCRX()
***/
num_t BASIC_FP nbscry() {
#ifdef USE_BG_ENGINE
  int32_t bg;

  if (checkOpen() ||
      getParam(bg, 0, MAX_BG-1, I_CLOSE))
    return 0;

  return vs23.bgScrollY(bg);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf snd PLAY
Checks if a sound is playing a on wavetable synthesizer channel.
\usage p = PLAY(channel)
\args
@channel	sound channel +
                [`0` to `{SOUND_CHANNELS_m1}`, or `-1` to check all channels]
\ret `1` if sound is playing, `0` otherwise.
\ref PLAY SOUND
***/
num_t BASIC_FP nplay() {
#ifdef HAVE_MML
  int32_t a, b;
  if (checkOpen()) return 0;
  if (getParam(a, -1, SOUND_CHANNELS - 1, I_CLOSE)) return 0;
  if (a == -1) {
    b = 0;
    for (int i = 0; i < SOUND_CHANNELS; ++i) {
      b |= sound.isPlaying(i);
    }
    return b;
  } else
    return sound.isPlaying(a);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

#ifndef ESP8266
int try_malloc() {
  uint32_t total = 0;
  void **foo = (void **)calloc(128, sizeof(void *));
  if (!foo)
    return total;
  total += 128 * sizeof(void *);
  int bs = 8192;
  int cnt = 0;
  for (;;) {
    foo[cnt] = malloc(bs);
    if (!foo[cnt]) {
      bs /= 2;
      if (!bs)
        break;
      continue;
    }
    total += bs;
    cnt++;
  }
  while (cnt) {
    free(foo[--cnt]);
  }
  free(foo);
  return total;
}
#endif

/***bf bas FREE
Get free memory size.
\usage bytes = FREE()
\ret Number of bytes free.
***/
num_t BASIC_FP nfree() {
  if (checkOpen()||checkClose()) return 0;
#ifdef ESP8266
  return umm_free_heap_size();
#else
  return try_malloc();
#endif
}

/***bf io INKEY
Reads a character from the keyboard and returns its numeric value.
\usage c = INKEY()
\ret Key code [`0` to `65535`]
\ref INKEY$
***/
num_t BASIC_FP ninkey() {
  if (checkOpen()||checkClose()) return 0;
  return iinkey(); // キー入力値の取得
}

/***bf sys TICK
Returns the elapsed time since power-on.
\usage tim = TICK([unit])
\args
@unit	unit of time [`0` for milliseconds, `1` for seconds, default: `0`]
\ret Time elapsed.
***/
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

/***bf scr FRAME
Returns the number of video frames since power-on.
\usage fr = FRAME()
\ret Frame count.
\ref VSYNC
***/
num_t BASIC_FP nframe() {
  if (checkOpen()||checkClose()) return 0;
  return vs23.frame();
}

/***bf scr VREG
Read a video controller register.
\usage v = VREG(reg)
\args
@reg	video controller register number
\ret Register contents.
\note
Valid register numbers are: `$01`, `$53`, `$84`, `$86`, `$9F` and `$B7`.
\ref VREG
***/
static const uint8_t vs23_read_regs[] PROGMEM = {
  0x01, 0x9f, 0x84, 0x86, 0xb7, 0x53
};
num_t BASIC_INT nvreg() {
#ifdef USE_VS23
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
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}
  
/***bf scr VPEEK
Read a location in video memory.
\usage v = VPEEK(video_addr)
\args
@video_addr	Byte address in video memory. [`0` to `131071`]
\ret Memory content.
\ref VPOKE VREG()
***/
num_t BASIC_FP nvpeek() {
#ifdef USE_VS23
  num_t value = getparam();
  if (value < 0 || value > 131071) {
    E_VALUE(0, 131071);
    return 0;
  } else
    return SpiRamReadByte(value);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

#define basic_bool(x) ((x) ? -1 : 0)

/***bf fs EOF
Checks whether the end of a file has been reached.
\usage e = EOF(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret `-1` if the end of the file has been reached, `0` otherwise.
***/
num_t BASIC_INT neof() {
  int32_t a = get_filenum_param();
  if (!err)
    return basic_bool(!user_files[a]->available());
  else
    return 0;
}

/***bf fs LOF
Returns the length of a file in bytes.
\usage l = LOF(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Length of file.
***/
num_t BASIC_INT nlof() {
  int32_t a = get_filenum_param();
  if (!err)
    return user_files[a]->fileSize();
  else
    return 0;
}

/***bf fs LOC
Returns the current position within a file.
\usage l = LOC(file_num)
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
\ret Position at which the next byte will be read or written.
***/
num_t BASIC_INT nloc() {
  int32_t a = get_filenum_param();
  if (!err)
    return user_files[a]->position();
  else
    return 0;
}
  
/***bf bas POPF
Remove an element from the beginning of a numeric list.
\usage e = POPF(~list)
\args
@~list	reference to a numeric list
\ret Value of the element removed.
\error
An error is generated if `~list` is empty.
\ref POPB()
***/
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
      SYNTAX_T("exp numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

/***bf bas POPB
Remove an element from the end of a numeric list.
\usage e = POPB(~list)
\args
@~list	reference to a numeric list
\ret Value of the element removed.
\error
An error is generated if `~list` is empty.
\ref POPF()
***/
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
      SYNTAX_T("exp numeric list reference");
    return 0;
  }
  if (checkClose()) return 0;
  return value;
}

/***bf bas VAL
Converts a number contained in a string to a numeric value.
\usage v = VAL(num$)
\args
@num$	a string expression containing the text representation of a number
\ret Numeric value of `num$`.
\ref STR$()
***/
num_t BASIC_INT nval() {
  if (checkOpen()) return 0;
  num_t value = strtonum(istrexp().c_str(), NULL);
  checkClose();
  return value;
}

/***bf bas INSTR
Get the position of the first occurrence of a string in another string.
\usage p = INSTR(haystack$, needle$)
\args
@haystack$	string in which to search
@needle$	string to search for
\ret
Position of first occurence of `needle$` (starting with `0`), or `-1` if it
could not be found.

NOTE: The meaning of the return value differs from other BASIC implementations,
in which `0` usually indicates that the string has not been found, and locations
found start at `1`.
***/
num_t BASIC_INT ninstr() {
  BString haystack, needle;
  if (checkOpen()) return 0;
  haystack = istrexp();
  if (err)
    return 0;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return 0;
  }
  needle = istrexp();
  if (checkClose()) return 0;
  char *res = strstr(haystack.c_str(), needle.c_str());
  if (!res)
    return -1;
  else
    return res - haystack.c_str();
}

typedef num_t (*numfun_t)();
#include "numfuntbl.h"

num_t BASIC_INT nsvar_a() {
  uint8_t i;
  int32_t a;
  // String character accessor 
  i = *cip++;
  if (*cip++ != I_SQOPEN) {
    // XXX: Can we actually get here?
    E_SYNTAX(I_SQOPEN);
    return 0;
  }
  if (getParam(a, 0, svar.var(i).length(), I_SQCLOSE))
    return 0;
  return svar.var(i)[a];
}

num_t BASIC_INT irel_string();

// Get value
num_t BASIC_FP ivalue() {
  num_t value = 0; // 値
  uint8_t i;   // 文字数
  int dims;
  static int idxs[MAX_ARRAY_DIMS];

  if (*cip >= NUMFUN_FIRST && *cip < NUMFUN_LAST) {
    value = numfuntbl[*cip++ - NUMFUN_FIRST]();
  } else if (is_strexp()) {
    // string comparison (or error)
    value = irel_string();
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
    value = nvar.var(*cip++);
    break;

  case I_LVAR:
    value = get_lvar(*cip++);
    break;

  case I_VARARR:
    i = *cip++;
    dims = get_array_dims(idxs);
    value = num_arr.var(i).var(dims, idxs);
    break;

  case I_SVAR:
    value = nsvar_a();
    break;

  //括弧の値の取得
  case I_OPEN: //「(」
    cip--;
    value = getparam(); //括弧の値を取得
    break;

  case I_CHAR: value = ncharfun(); break; //関数CHAR

  case I_SYS: value = nsys(); break;

/***bn io DOWN
Value of the "down" direction for input devices.
\ref PAD() UP LEFT RIGHT
***/
  case I_DOWN:  value = psxDown; break;

  case I_PLAY:	value = nplay(); break;

  case I_NUMLST:
    i = *cip++;
    dims = get_array_dims(idxs);
    if (dims != 1) {
      SYNTAX_T("invalid list index");
    } else {
      value = num_lst.var(i).var(idxs[0]);
    }
    break;

/***bn bas FN
Call a procedure and get its return value.

Evaluates to the first return value of a procedure.
\usage v = FN procedure[(argument[, argument ...])]
\args
@procedure	name of a procedure declared with `PROC`
@argument	a string or numeric expression
\ret First return value of called procedure.
\note
Most implementations of BASIC allow declaration of functions using `DEF FN`,
a syntax that is not supported by Engine BASIC.  To achieve the same result,
you will have to rewrite the function declaration:

.Standard BASIC
====
----
DEF FN f(x) = x * 2
----
====
.Engine BASIC
====
----
PROC f(x): RETURN @x * 2
----
====
\ref CALL
***/
  case I_FN: {
    unsigned char *lp;
    icall();
    i = gstki;
    if (err)
      break;
    for (;;) {
      lp = iexe(i);
      if (!lp || err)
        break;
      clp = lp;
      cip = clp + sizeof(line_desc_t);
      TRACE;
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
      SYNTAX_T("exp numeric expr");
    return 0;
  }

#if defined(ESP8266_NOWIFI) && !defined(HOSTED)
  // XXX: So, yes, it's weird that the I2S could would be responsible for
  // checking for system stack overflows, but it happens to have its data
  // buffer right below the stack, and knows where it ends...
  if (nosdk_i2s_check_guard()) {
    nosdk_i2s_clear_buf();
    err = ERR_STACKOF;
    return value;
  }
#endif

  while (1)
    switch (*cip) {
/***bo op ^
Exponentiation operator.
\usage a ^ b
\res `a` raised to the power of `b`.
\prec 1
***/
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
/***bo op - (unary)
Negation operator.
\usage -a
\res The negative of `a`.
\prec 2
***/
    case I_MINUS: //「-」
      return 0 - imul(); //値を取得して負の値に変換
      break;
/***bo op + (unary)
Identity operator.
\usage +a
\res `a`.
\note
This operator serves decorative purposes only.
\prec 2
***/
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
    switch(*cip++) { //中間コードで分岐

/***bo op *
Multiplication operator.
\usage a * b
\res Product of `a` and `b`.
\prec 3
***/
    case I_MUL: //掛け算の場合
      tmp = ivalue();
      value *= tmp; //掛け算を実行
      break;

/***bo op /
Division operator.
\usage a / b
\res Quotient of `a` and `b`.
\prec 3
***/
    case I_DIV: //割り算の場合
      tmp = ivalue();
      if (err)
        return -1;
      value /= tmp; //割り算を実行
      break;

/***bo op MOD
Modulus operator.
\usage a MOD b
\res Remainder of division of `a` and `b`.
\prec 3
***/
    case I_MOD: //剰余の場合
      tmp = ivalue();
      if (err)
        return -1;
      value = (int32_t)value % (int32_t)tmp; //割り算を実行
      break;

/***bo op <<
Bit-shift operator, left.
\usage a << b
\res `a` converted to an unsigned integer and shifted left by `b` bits.
\prec 3
***/
    case I_LSHIFT: // シフト演算 "<<" の場合
      tmp = ivalue();
      value =((uint32_t)value)<<(uint32_t)tmp;
      break;

/***bo op >>
Bit-shift operator, right.
\usage a >> b
\res `a` converted to an unsigned integer and shifted right by `b` bits.
\prec 3
***/
    case I_RSHIFT: // シフト演算 ">>" の場合
      tmp = ivalue();
      value =((uint32_t)value)>>(uint32_t)tmp;
      break;

    default:
      --cip;
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
/***bo op +
Numeric addition operator.
\usage a + b
\res Sum of `a` and `b`.
\prec 4
***/
    case I_PLUS: //足し算の場合
      cip++;
      tmp = imul();
      value += tmp; //足し算を実行
      break;

/***bo op -
Numeric subtraction operator.
\usage a - b
\res Difference of `a` and `b`.
\prec 4
***/
    case I_MINUS: //引き算の場合
      cip++;
      tmp = imul();
      value -= tmp; //引き算を実行
      break;

    default:
      if (!math_exceptions_disabled && !err && !finite(value)) {
        if (isinf(value))
          err = ERR_DIVBY0;
        else
          err = ERR_FP;
      }
      return value; //値を持ち帰る
    } //中間コードで分岐の末尾
}

num_t BASIC_INT irel_string() {
  BString lhs = istrexp();
  BString rhs;
  switch (*cip++) {
/***bo op = (strings)
String equality operator.
\usage a$ = b$
\res
`-1` if the value of `a$` is identical to the value of `b$`, `0` otherwise.
\prec 5
***/
  case I_EQ:
    rhs = istrexp();
    return basic_bool(lhs == rhs);
/***bo op <> (strings)
String inequality operator.
\usage
a$ <> b$

a$ >< b$
\res
`-1` if the value of `a$` is different from the value of `b$`, `0` otherwise.
\prec 5
***/
  case I_NEQ:
  case I_NEQ2:
    rhs = istrexp();
    return basic_bool(lhs != rhs);
/***bo op < (strings)
String less-than operator.
\usage a$ < b$
\res
`-1` if the value of `a$` precedes the value of `b$` when sorted
alphabetically, `0` otherwise.
\prec 5
***/
  case I_LT:
    rhs = istrexp();
    return basic_bool(lhs < rhs);
/***bo op > (strings)
String greater-than operator.
\usage a$ > b$
\res
`-1` if the value of `a$` succeeds the value of `b$` when sorted
alphabetically, `0` otherwise.
\prec 5
***/
  case I_GT:
    rhs = istrexp();
    return basic_bool(lhs > rhs);
  case I_SQOPEN: {
    int32_t i = iexp();
    if (*cip++ != I_SQCLOSE) {
      E_SYNTAX(I_SQCLOSE);
      return -1;
    }
    return lhs[i];
  }
  default:
    err = ERR_TYPE;
    return -1;
  }
}

num_t BASIC_FP irel() {
  num_t value, tmp;
  value = iplus();

  if (err)
    return -1;

  // conditional expression
  while (1)
    switch(*cip++) {
/***bo op =
Equality operator.
\usage a = b
\res `-1` if `a` and `b` are equal, `0` otherwise.
\prec 6
***/
    case I_EQ:
      tmp = iplus();
      value = basic_bool(value == tmp);
      break;
/***bo op <>
Inequality operator.
\usage
a <> b

a >< b
\res `-1` if `a` and `b` are not equal, `0` otherwise.
\prec 6
***/
    case I_NEQ:
    case I_NEQ2:
      tmp = iplus();
      value = basic_bool(value != tmp);
      break;
/***bo op <
Less-than operator.
\usage
a < b
\res `-1` if `a` is less than `b`, `0` otherwise.
\prec 6
***/
    case I_LT:
      tmp = iplus();
      value = basic_bool(value < tmp);
      break;
/***bo op \<=
Less-than-or-equal operator.
\usage
a <= b
\res `-1` if `a` is less than or equal to `b`, `0` otherwise.
\prec 6
***/
    case I_LTE:
      tmp = iplus();
      value = basic_bool(value <= tmp);
      break;
/***bo op >
Greater-than operator.
\usage
a > b
\res `-1` if `a` is greater than `b`, `0` otherwise.
\prec 6
***/
    case I_GT:
      tmp = iplus();
      value = basic_bool(value > tmp);
      break;
/***bo op >=
Greater-than-or-equal operator.
\usage
a >= b
\res `-1` if `a` is greater than or equal to `b`, `0` otherwise.
\prec 6
***/
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
/***bo op NOT
Bitwise inversion operator.
\usage NOT a
\res Bitwise inversion of `a`.
\prec 7
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical NOT" operator; instead, the bitwise operator is used.
***/
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
/***bo op AND
Bitwise AND operator.
\usage a AND b
\res Bitwise conjunction of `a` and `b`.
\prec 7
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical AND" operator; instead, the bitwise operator is used.
***/
      tmp = iand();
      value = ((int32_t)value)&((int32_t)tmp);
      break;
    default:
      cip--;
      return value;
    }
}

// Numeric expression parser
num_t BASIC_FP iexp() {
  num_t value, tmp;

  value = iand();
  if (err)
    return -1;

  while (1)
    switch(*cip++) {
/***bo op OR
Bitwise OR operator.
\usage a OR b
\res Bitwise inclusive disjunction of `a` and `b`.
\prec 8
\note
Like most BASIC implementations, Engine BASIC does not have a dedicated
"logical OR" operator; instead, the bitwise operator is used.
***/
    case I_OR:
      tmp = iand();
      value = ((int32_t)value) | ((int32_t)tmp);
      break;
/***bo op EOR
Bitwise exclusive-OR operator.
\usage a EOR b
\res Bitwise exclusive disjunction of `a` and `b`.
\prec 8
***/
    case I_XOR:
      tmp = iand();
      value = ((int32_t)value) ^ ((int32_t)tmp);
      break;
    default:
      cip--;
      return value;
    }
}

// Get number of line at top left of the screen
uint32_t getTopLineNum() {
  uint8_t* ptr = sc0.getScreenWindow();
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
  uint8_t* ptr = sc0.getScreenWindow()+sc0.getStride()*(sc0.getHeight()-1);
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

/***bc sys SYSINFO
Displays internal information about the system.

WARNING: This command may be renamed in the future to reduce namespace
pollution.
\usage SYSINFO
\desc
`SYSINFO` displays the following information:

* Program size,
* statistics on scalar, array and list variables, both numeric and string,
* loop and return stack depths,
* CPU stack pointer address,
* free memory size and
* video timing information (nominal and actual cycles per frame).
***/
void SMALL isysinfo() {
  char top = 't';
  uint32_t adr = (uint32_t)&top;

  PRINT_P("Program size: ");
  putnum(size_list, 0);
  newline();

  newline();
  PRINT_P("Variables:\n");
  
  PRINT_P(" Numerical: ");
  putnum(nvar.size(), 0);
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
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
#else
  putnum(try_malloc(), 0);
#endif
  newline();

#ifdef USE_VS23
  newline();
  PRINT_P("Video timing: ");
  putint(vs23.cyclesPerFrame(), 0);
  PRINT_P(" cpf (");
  putint(vs23.cyclesPerFrameCalculated(), 0);
  PRINT_P(" nominal)\n");
#endif
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
  TRACE;
}

/***bc bas GOTO
Branches to a specified line or label.
\usage GOTO <line_number|&label>
\args
@line_number	a BASIC program line number
@&label		a BASIC program label
\note
It is recommended to use `GOTO` sparingly as it tends to make programs
more difficult to understand. If possible, use loop constructs and
procedure calls instead.
\ref ON_GOTO
***/
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
    TRACE;
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
  gstk[gstki++].proc_idx = NO_PROC;

  clp = lp;                                 // 行ポインタを分岐先へ更新
  cip = ip;
  TRACE;
}

static void BASIC_FP do_gosub(uint32_t lineno) {
  uint8_t *lp = getlp(lineno);
  if (lineno != getlineno(lp)) {            // もし分岐先が存在しなければ
    err = ERR_ULN;                          // エラー番号をセット
    return;
  }
  do_gosub_p(lp, lp + sizeof(line_desc_t));
}

/***bc bas GOSUB
Calls a subroutine.
\desc
`GOSUB` puts the current position in the program on a stack and then
branches to the given location. It is then possible to return to
the statement after the `GOSUB` command by using `RETURN`.
\usage GOSUB <line_number|&label>
\args
@line_number	a BASIC program line number
@&label		a BASIC program label
\note
It may be more convenient to use `PROC` procedures and the `CALL` command
instead.
\example
====
----
FOR i = 1 TO 20
  GOSUB &Square
NEXT i
END
  
&Square:
PRINT i, i * i
RETURN
----
====
\ref CALL ON_GOSUB PROC RETURN
***/
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

/***bc bas ON GOTO
Branches to one of several locations, depending on the value of an expression.
\usage ON expression <GOTO|GOSUB> <line_number|&label>[, <line_number|&label> ...]
\args
@expression	any numeric expression
@line_number	a BASIC program line number
@&label		a BASIC program label
\desc
`ON GOTO` and `ON GOSUB` will branch to the first given line number or label
if `expression` is `1`, to the second if `expression` is `2`, and so forth. 
If `expression` is `0`, it will do nothing, and execution will continue with
the next statement.

`ON GOSUB` calls the given location as a subroutine, while `ON GOTO` simply
continues execution there.
\ref GOTO GOSUB
***/
/***bc bas ON GOSUB
Call one of several subroutines, depending on the value of an expression.
\ref ON_GOTO GOSUB
***/
static void BASIC_FP on_go(bool is_gosub, int cas)
{
  unsigned char *lp = NULL, *ip = NULL;
  --cas;
  for (;;) {
    if (*cip == I_LABEL) {
      ++cip;
      if (!cas) {
        label_t &lb = labels.label(*cip++);
        lp = lb.lp;
        ip = lb.ip;
      }
    } else {
      uint32_t line = iexp();
      if (!cas) {
        lp = getlp(line);
        if (line != getlineno(lp)) {
          err = ERR_ULN;
          return;
        }
        ip = lp + sizeof(line_desc_t);
      }
    }

    if (err)
      return;
    if (!cas && !is_gosub)
      break;
    if (*cip != I_COMMA) {
      if (lp)
        break;
      else
        return;
    }

    ++cip;
    --cas;
  }

  if (is_gosub)
    do_gosub_p(lp, ip);
  else {
    clp = lp;
    cip = ip;
    TRACE;
  }
}

void BASIC_INT ion()
{
  if (*cip == I_SPRITE) {
/***bc bg ON SPRITE
Defines an event handler for sprite collisions.

The handler is called once for each collision between two sprites.
The handler can be disabled using `ON SPRITE OFF`.
\usage
ON SPRITE CALL handler

ON SPRITE OFF
\args
@handler	a procedure defined with `PROC`
\sec HANDLER
The event handler will be passed three numeric arguments: The sprite IDs
of the involved sprites, and the direction of the collision, as would
be returned by `SPRCOLL()`.
\note
The handler will be called once for each sprite collision, but it has
to be re-enabled using `ON SPRITE CALL` each time to prevent event storms.
\bugs
WARNING: This command has never been tested.
\ref PROC SPRCOLL()
***/
    ++cip;
    if (*cip == I_OFF) {
      event_sprite_proc_idx = NO_PROC;
      ++cip;
    } else {
      if (*cip++ != I_CALL) {
        E_SYNTAX(I_CALL);
        return;
      }
      event_sprite_proc_idx = *cip++;
    }
  } else if (*cip == I_PLAY) {
/***bc snd ON PLAY
Defines an event handler triggered by the end of the current MML pattern.

The handler is called once the MML pattern of any sound channel has
finished playing. It can be disabled using `ON PLAY ... OFF`.
\usage
ON PLAY channel CALL handler

ON PLAY channel OFF
\args
@channel	a sound channel [`0` to `{SOUND_CHANNELS_m1}`]
@handler	a procedure defined with `PROC`
\sec HANDLER
The event handler will receive the number of the channel that has
ended as a numeric argument.
\note
All end-of-music handlers will be disabled when one of them is called,
and they therefore have to be re-enabled using `ON PLAY CALL` if so
desired.
\bugs
WARNING: This command has never been tested.

An event on one channel will disable handling on all channels. That is stupid.
\ref PLAY PLAY()
***/
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
/***bc io ON PAD
Defines a game controller input handler triggered by changes in the
state of the buttons of a given controller.

Game controller input event handling can be disabled using `ON PAD OFF`.
\usage
ON PAD num CALL handler

ON PAD OFF
\args
@num		number of the game controller [`0` to `{MAX_PADS_m1}`]
@handler	a procedure defined with `PROC`
\sec HANDLER
The event handler will receive two numeric arguments: the number of
the game controller that has changed state, and a bit pattern indicating
which buttons have changed.
\note
Unlike some other event handlers, the game controller handler remains
enabled after an event has been processed and does not have to be re-armed.
\bugs
WARNING: This command has never been tested.
\ref PAD()
***/
    ++cip;
    int pad = getparam();
    if (pad < 0 || pad >= MAX_PADS) {
      E_VALUE(0, MAX_PADS - 1);
      return;
    }
    if (*cip == I_OFF) {
      event_pad_enabled = false;
      memset(event_pad_proc_idx, -1, sizeof(event_pad_proc_idx));
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
/***bc bas ON ERROR
Enables error handling and, when a run-time error occurs, directs your
program to branch to an error-handling routine.

Error handling can be disabled using `ON ERROR OFF`.
\usage
ON ERROR GOTO location

ON ERROR OFF
\args
@location	a line number or a label
\sec HANDLER
The error code is passed to the handler in `RET(0)`, and the line number in `RET(1)`.

When an error has been forwarded from a sub-program run with `EXEC`, the line
number of the sub-program in which the error has been triggered is passed in `RET(2)`.
\note
Unlike other event handlers, `ON ERROR` does not call the error handler as a procedure,
and it is not possible to use `RETURN` to resume execution. `RESUME` must be used
instead.

When a program is executed using `EXEC` and does not define its own error handler, any
errors occurring there will be forwarded to the calling program's error handler, if
there is one.
\ref EXEC RESUME
***/
    ++cip;
    if (*cip == I_OFF) {
      ++cip;
      event_error_enabled = false;
      return;
    } else if (*cip++ != I_GOTO) {
      E_SYNTAX(I_GOTO);
      return;
    }
    event_error_enabled = true;
    if (*cip == I_LABEL) {
      ++cip;
      label_t &lb = labels.label(*cip++);
      if (!lb.lp || !lb.ip) {
        err = ERR_UNDEFLABEL;
        return;
      }
      event_error_lp = lb.lp;
      event_error_ip = lb.ip;
    } else {
      int line = iexp();
      event_error_lp = getlp(line);
      event_error_ip = event_error_lp + sizeof(line_desc_t);
    }
  } else {
    uint32_t cas = iexp();
    if (*cip == I_GOTO) {
      ++cip;
      on_go(false, cas);
    } else if (*cip == I_GOSUB) {
      ++cip;
      on_go(true, cas);
    } else {
      SYNTAX_T("exp GOTO or GOSUB");
    }
  }
}

/***bc bas RESUME
Resume program execution after an error has been handled.
\desc
When an error has been caught by a handler set up using `ON ERROR`,
it is possible to resume execution after the statement that caused the
error using `RESUME`.
\usage RESUME
\note
In other BASIC implementations, `RESUME` will retry the statement that
has caused the error, while continuing after the error is possible
using `RESUME NEXT`. In Engine BASIC, `RESUME` will always skip the
statement that generated the error.
\bugs
It is not possible to repeat the statement that has generated the error.
\ref ON_ERROR
***/
void iresume()
{
  if (!event_error_resume_lp) {
    err = ERR_CONT;
  } else {
    clp = event_error_resume_lp;
    cip = event_error_resume_ip;
    TRACE;
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

/***bc bas ERROR
Triggers an error.
\usage ERROR code
\args
@code	error code
\ref ON_ERROR
***/
void ierror() {
  err = iexp();
}

/***bc bas CALL
Calls a procedure.
\usage CALL procedure[(argument[, argument ...])]
\args
@procedure	name of a procedure declared with `PROC`
@argument	a string or numeric expression
\note
At least as many arguments must be given as defined in the procedure
declaration. Additional arguments can be accessed using `ARG()` and
`ARG$()`.
\ref ARG() ARG$() ARGC() FN PROC
***/
void BASIC_FP icall() {
  num_t n;
  uint8_t proc_idx = *cip++;

  struct proc_t &proc_loc = procs.proc(proc_idx);

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
  // Argument stack indices cannot be modified while arguments are evaluated
  // because that would mess with the stack frame of the caller.
  int new_astk_num_i = astk_num_i;
  int new_astk_str_i = astk_str_i;
  if (gstki > 0 && gstk[gstki-1].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki-1].proc_idx);
    new_astk_num_i += p.locc_num;
    new_astk_str_i += p.locc_str;
  }
  if (*cip == I_OPEN) {
    ++cip;
    if (*cip != I_CLOSE) for(;;) {
      if (is_strexp()) {
        BString b = istrexp();
        if (err)
          return;
        if (new_astk_str_i >= SIZE_ASTK)
          goto overflow;
        astk_str[new_astk_str_i++] = b;
        ++str_args;
      } else {
        n = iexp();
        if (err)
          return;
        if (new_astk_num_i >= SIZE_ASTK)
          goto overflow;
        astk_num[new_astk_num_i++] = n;
        ++num_args;
      }
      if (*cip != I_COMMA)
        break;
      ++cip;
    }

    if (checkClose())
      return;
  }
  astk_num_i = new_astk_num_i;
  astk_str_i = new_astk_str_i;
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
  TRACE;
  
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

/***bc bas RETURN
Return from a subroutine or procedure.

Returns from a subroutine called by `GOSUB` or from a procedure
defined by `PROC` and called with `CALL` or `FN`.

If specified, up to {MAX_RETVALS} return values of each type (numeric or
string) can be returned to the caller.
\usage RETURN [<value|value$>[, <value|value$> ...]]
\args
@value	numeric expression
@value$	string expression
\ref CALL FN GOSUB PROC
***/
void BASIC_FP ireturn() {
  if (!gstki) {    // もしGOSUBスタックが空なら
    err = ERR_GSTKUF; // エラー番号をセット
    return;
  }

  // Set return values, if any.
  if (!end_of_statement()) {
    int rcnt = 0, rscnt = 0;
    num_t my_retval[MAX_RETVALS];
    BString *my_retstr[MAX_RETVALS];	// don't want to always construct all strings
    do {
      if (is_strexp())
        my_retstr[rscnt++] = new BString(istrexp());
      else
        my_retval[rcnt++] = iexp();
    } while (*cip++ == I_COMMA && rcnt < MAX_RETVALS && rscnt < MAX_RETVALS);
    for (int i = 0; i < rcnt; ++i)
      retval[i] = my_retval[i];
    for (int i = 0; i < rscnt; ++i) {
      retstr[i] = *my_retstr[i];
      delete my_retstr[i];
    }
  }

  astk_num_i -= gstk[--gstki].num_args;
  astk_str_i -= gstk[gstki].str_args;
  if (gstki > 0 && gstk[gstki-1].proc_idx != NO_PROC) {
    // XXX: This can change if the parent procedure was called by this one
    // (directly or indirectly)!
    struct proc_t &p = procs.proc(gstk[gstki-1].proc_idx);
    astk_num_i -= p.locc_num;
    astk_str_i -= p.locc_str;
  }
  if (profile_enabled && gstk[gstki].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki].proc_idx);
    p.profile_total += ESP.getCycleCount() - p.profile_current;
  }
  clp = gstk[gstki].lp; //中間コードポインタを復帰
  cip = gstk[gstki].ip; //行ポインタを復帰
  TRACE;
  return;
}

/***bc bas DO
Repeats a block of statements while a condition is true or until a condition
becomes true.
\usage
DO
  statement_block
LOOP [<WHILE|UNTIL> condition]
\args
@statement_block one or more statements on one or more lines
@condition	a numeric expression
\note
If `condition` is prefixed by `WHILE`, the loop will repeat if `condition`
is "true" (non-zero). If `condition` is prefixed by `UNTIL`, the loop will
continue if `condition` is "false" (zero). In any other case, the loop will
be exited and execution will continue with the statement following the
`LOOP` command.

If no condition is specified, the loop will repeat forever.
\example
====
----
i=0
PRINT "Value of i at beginning of loop is ";i
DO
  i=i+1
LOOP WHILE i<10
PRINT "Value of i at end of loop is ";i
----
====
***/
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

/***bc bas WHILE
Executes a series of statements as long as a specified condition is "true".

\usage WHILE condition
\args
@condition	any numeric expression
\note
`condition` is evaluated at the start of the loop. If it is "false",
execution continues after the corresponding `WEND` statement. (Note that
all kinds of loops can be nested, so this may not be the nearest `WEND`
statement.)

If `condition` is "true", the statements following the `WHILE` statement
will be executed, until the corresponding `WEND` statement is reached,
at which point `condition` will be evaluated again to determine whether
to continue the loop.

The `WHILE` keyword can also form part of a `LOOP` command.
\ref LOOP
***/
void BASIC_FP iwhile() {
  if (lstki >= SIZE_LSTK) {
    err = ERR_LSTKOF;
    return;
  }
  lstk[lstki].lp = clp;
  lstk[lstki].ip = cip;
  num_t cond = iexp();
  if (cond) {
    lstk[lstki].vto = -1;
    lstk[lstki].vstep = -1;
    lstk[lstki].index = -2;
    lstk[lstki++].local = false;
  } else {
    unsigned char *newip = getWENDptr(cip);
    if (newip) {
      cip = newip;
    } else {
      err = ERR_WHILEWOW;
    }
  }
}

/***bc bas WEND
Iterates a `WHILE` loop.
\usage WEND
\ref WHILE
***/
void BASIC_FP iwend() {
  if (!lstki) {
    err = ERR_LSTKUF;
    return;
  }

  // Look for nearest WHILE.
  while (lstki) {
    if (lstk[lstki - 1].index == -2)
      break;
    lstki--;
  }
  
  if (!lstki) {
    err = ERR_LSTKUF;
    return;
  }

  unsigned char *tmp_ip = cip;
  unsigned char *tmp_lp = clp;

  // Jump to condition
  cip = lstk[lstki - 1].ip;
  clp = lstk[lstki - 1].lp;
  TRACE;

  num_t cond = iexp();
  // If the condition is true, continue with the loop
  if (cond)
    return;

  // If the condition is not true, pop the loop and
  // go back to the WEND.
  cip = tmp_ip;
  clp = tmp_lp;
  TRACE;
  lstki--;
}
  
/***bc bas FOR
Starts a loop repeating a block of statements a specified number of times.
\usage
FOR loop_variable = start TO end [STEP increment]
  statement_block
NEXT [loop_variable]
\args
@loop_variable	a numeric variable used as the loop counter
@start		initial value of the loop counter
@increment	amount the counter is changed each time through the loop +
                [default: `1`]
@statement_block one or more statements on one or more lines
\note
Both `end` and `increment` are only evaluated once, at the start of the
loop. Any changes to these expressions afterwards will not have any effect
on it.

If no loop variable is specified in the `NEXT` command, the top-most `FOR`
loop on the loop stack (that is, the one started last) will be iterated. If
it is specified, the `FOR` loop associated with the given variable will be
iterated, and any nested loops below it will be discarded.
\example
====
----
FOR i = 1 TO 15
  PRINT i
NEXT i
----
====
====
----
FOR i = 7 to -6 STEP -3
  PRINT i
NEXT i
----
====
***/
void BASIC_FP ifor() {
  int index;
  num_t vto, vstep; // FOR文の変数番号、終了値、増分

  // 変数名を取得して開始値を代入（例I=1）
  if (*cip == I_VAR) { // もし変数がなかったら
    index = *++cip; // 変数名を取得
    ivar();       // 代入文を実行
    lstk[lstki].local = false;
  } else if (*cip == I_LVAR) {
    index = *++cip;
    ilvar();
    lstk[lstki].local = true;
  } else {
    err = ERR_FORWOV;    // エラー番号をセット
  }
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
  lstk[lstki++].index = index;
}

/***bc bas LOOP
Iterates a `DO` loop.
\usage LOOP [<UNTIL|WHILE> condition]
\ref DO
***/
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
      TRACE;
    } else {
      // Pop loop off stack.
      lstki--;
    }
  } else {
    // Infinite loop.
    cip = lstk[lstki - 1].ip;
    clp = lstk[lstki - 1].lp;
    TRACE;
  }
}

/***bc bas NEXT
Increments and tests the counter in a `FOR` loop.
\usage NEXT [loop_variable]
\ref FOR
***/
void BASIC_FP inext() {
  int want_index;	// variable we want to NEXT, if specified
  bool want_local;
  int index;		// loop variable index we will actually use
  bool local;
  num_t vto;		// end of loop value
  num_t vstep;		// increment value

  if (!lstki) {    // FOR stack is empty
    err = ERR_LSTKUF;
    return;
  }

  if (*cip != I_VAR && *cip != I_LVAR)
    want_index = -1;		// just use whatever is TOS
  else {
    want_local = *cip++ == I_LVAR;
    want_index = *cip++;	// NEXT a specific loop variable
  }

  while (lstki) {
    // Get index of loop variable on top of stack.
    index = lstk[lstki - 1].index;
    local = lstk[lstki - 1].local;

    // Done if it's the one we want (or if none is specified).
    if (want_index < 0 || (want_index == index && want_local == local))
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

  num_t &loop_var = local ? get_lvar(index) : nvar.var(index);
  
  vstep = lstk[lstki - 1].vstep;
  loop_var += vstep;
  vto = lstk[lstki - 1].vto;

  // Is this loop finished?
  if (((vstep < 0) && (loop_var < vto)) ||
      ((vstep > 0) && (loop_var > vto))) {
    lstki--;  // drop it from FOR stack
    return;
  }

  // Jump to the start of the loop.
  cip = lstk[lstki - 1].ip;
  clp = lstk[lstki - 1].lp;
  TRACE;
}

/***bc bas IF
Executes a statement or statement block depending on specified conditions.
\usage
IF condition THEN
  statement_block
[ELSE IF condition THEN
  statement_block]
...
[ELSE
  statement_block]
ENDIF

IF condition THEN statements [ELSE statements]
\args
@condition		any numeric expression
@statement_block	one or more statements on one or more lines
@statements		one or more statements, separated by colons
\note
A `condition` is considered "false" if it is zero, and "true" in any other
case.

IMPORTANT: In many BASIC implementations, `ENDIF` is spelled `END IF` (that
is, as two commands). If the sequence `END IF` is entered or loaded in a
program, Engine BASIC will convert it to `ENDIF`.
***/
void BASIC_FP iif() {
  num_t condition;    // IF文の条件値
  uint8_t* newip;       // ELSE文以降の処理対象ポインタ

  condition = iexp(); // 真偽を取得
  if (err)
    return;

  bool have_goto = false;
  if (*cip == I_THEN) {
    ++cip;
    if (*cip == I_NUM)	// XXX: should be "if not command"
      have_goto = true;
  } else if (*cip == I_GOTO) {
    ++cip;
    have_goto = true;
  } else {
    SYNTAX_T("exp THEN or GOTO");
    return;
  }

  if (condition) {    // もし真なら
    if (have_goto)
      igoto();
    return;
  } else {
    newip = getELSEptr(cip);
    if (newip) {
      if (*newip == I_NUM) {
        do_goto(UNALIGNED_NUM_T(newip+1));
      } else {
        cip = newip;
      }
    }
  }
}

/***bc bas ENDIF
Ends a muli-line `IF` statement.
\ref IF
***/
void BASIC_FP iendif()
{
}

/***bc bas ELSE
Introduces the `ELSE` branch of an `IF` statement.
\ref IF
***/
void BASIC_FP ielse()
{
  // Special handling for "ELSE IF": Skip one level of nesting. This avoids
  // having to have an ENDIF for each ELSE at the end of an IF ... ELSE IF
  // ... cascade.
  int adjust = 0;
  if (*cip == I_IF)
    adjust = -1;
  uint8_t *newip = getELSEptr(cip, true, adjust);
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

/***bc bas END
Ends the program.
\usage END
\ref ENDIF
***/
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

/***bc fs CMD
Redirect screen I/O to a file.
\desc
`CMD` allows to redirect standard text output or input operations to
files.

Redirection can be disabled by replacing the file number with `OFF`. `CMD
OFF` turns off both input and output redirection.
\usage
CMD <INPUT|OUTPUT> file_num
CMD <INPUT|OUTPUT> OFF
CMD OFF
\args
@file_num	number of an open file to redirect to [`0` to `{MAX_USER_FILES_m1}`]
\note
Redirection will automatically be reset if a file redirected to is closed
(either explicitly or implicitly, by opening a new file using the currently
used file number), or when returning to the command prompt.
\ref OPEN CLOSE
***/
void icmd () {
  bool is_input;
  int32_t redir;
  if (*cip == I_OUTPUT) {
    is_input = false;
    ++cip;
  } else if (*cip == I_INPUT) {
    is_input = true;
    ++cip;
  } else if (*cip == I_OFF) {
    ++cip;
    redirect_input_file = -1;
    redirect_output_file = -1;
    return;
  } else {
    SYNTAX_T("exp INPUT, OUTPUT or OFF");
    return;
  }

  if (*cip == I_OFF) {
    ++cip;
    if (is_input)
      redirect_input_file = -1;
    else
      redirect_output_file = -1;
    return;
  } else
    getParam(redir, 0, MAX_USER_FILES, I_NONE);
  if (!user_files[redir] || !*user_files[redir]) {
    err = ERR_FILE_NOT_OPEN;
    if (is_input)
      redirect_input_file = -1;
    else
      redirect_output_file = -1;
  } else {
    if (is_input)
      redirect_input_file = redir;
    else
      redirect_output_file = redir;
  }
}

void iprint_() {
  iprint();
}

/***bc scr REDRAW
Redraw the text screen.
\desc
Overwrites anything displayed on-screen with the characters stored in
the text buffer.
\usage REDRAW
\ref CLS
***/
void irefresh() {
  sc0.refresh();
}

void isprint() {
  iprint(1);
}

/***bc bas NEW
Deletes the program in memory as well as all variables.
\usage NEW
\ref CLEAR DELETE
***/
void inew_() {
  inew();
}

/***bc bas CLEAR
Deletes all variables.
\usage CLEAR
\ref NEW
***/
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

/***bc bas STOP
Halts the program.

A program stopped by a `STOP` command can be resumed using `CONT`.
\usage STOP
\ref CONT
***/
void istop() {
  err = ERR_CTR_C;
}

/***bc fs CHDIR
Changes the current directory.
\usage CHDIR directory$
\args
@directory$	path to the new current directory
\ref CWD$()
***/
void ichdir() {
  BString new_cwd;
  if(!(new_cwd = getParamFname())) {
    return;
  }
  if (!Unifile::chDir(new_cwd.c_str()))
    err = ERR_FILE_OPEN;
}

/***bc fs OPEN
Opens a file or directory.
\usage
OPEN file$ [FOR <INPUT|OUTPUT|APPEND|DIRECTORY>] AS [#]file_num
\args
@file$		name of the file or directory
@file_num	file number to be used [`0` to `{MAX_USER_FILES_m1}`]
\sec MODES
* `*INPUT*` opens the file for reading. (This is the default if no mode is
  specified.)
* `*OUTPUT*` empties the file and opens it for writing.
* `*APPEND*` opens the file for writing while keeping its content.
* `*DIRECTORY*` opens `file$` as a directory.
\ref CLOSE DIR$() INPUT INPUT$() PRINT SEEK
***/
void iopen() {
  BString filename;
  int flags = UFILE_READ;
  int32_t filenum;

  if (!(filename = getParamFname()))
    return;
  
  if (*cip == I_FOR) {
    ++cip;
    switch (*cip++) {
    case I_OUTPUT:	flags = UFILE_OVERWRITE; break;
    case I_INPUT:	flags = UFILE_READ; break;
    case I_APPEND:	flags = UFILE_WRITE; break;
    case I_DIRECTORY:	flags = -1; break;
    default:		SYNTAX_T("exp file mode"); return;
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
  
  if (user_files[filenum]) {
    user_files[filenum]->close();
    if (redirect_output_file == filenum)
      redirect_output_file = -1;
    if (redirect_input_file == filenum)
      redirect_input_file = -1;
    delete user_files[filenum];
    user_files[filenum] = NULL;
  }

  Unifile f;
  if (flags == -1)
    f = Unifile::openDir(filename.c_str());
  else
    f = Unifile::open(filename, flags);
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

/***bc fs CLOSE
Closes an open file or directory.
\usage CLOSE [#]file_num
\args
@file_num	number of an open file or directory [`0` to `{MAX_USER_FILES_m1}`]
\ref OPEN
***/
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
    if (redirect_output_file == filenum)
      redirect_output_file = -1;
    if (redirect_input_file == filenum)
      redirect_input_file = -1;
    delete user_files[filenum];
    user_files[filenum] = NULL;
  }
}

/***bc fs SEEK
Sets the file position for the next read or write.
\usage SEEK file_num, position
\args
@file_num	number of an open file [`0` to `{MAX_USER_FILES_m1}`]
@position	position where the next read or write should occur
\error
The command will generate an error if `file_num` is not open or the operation
is unsuccessful.
\ref INPUT INPUT$() PRINT
***/
void iseek() {
  int32_t filenum, pos;

  if (*cip == I_SHARP)
    ++cip;

  if (getParam(filenum, 0, MAX_USER_FILES - 1, I_COMMA))
    return;

  if (!user_files[filenum] || !*user_files[filenum])
    err = ERR_FILE_NOT_OPEN;

  if (getParam(pos, 0, user_files[filenum]->fileSize(), I_NONE))
    return;

  if (!user_files[filenum]->seekSet(pos))
    err = ERR_FILE_SEEK;
}

/***bc sys PROFILE
Enables or disables the system and procedure profiler.
\desc
The system profiler shows a bar in the border of the screen indicating
how much CPU time is spent on the various tasks of the grahpics
and sound subsystems. It helps in determining the cause of system
overloads that lead to graphics and/or sound glitches.

`PROFILE ON` enables the profiler, `PROFILE OFF` disables it.

`PROFILE ON` also enables the procedure profiler that helps determine the
number of CPU cycles used in each BASIC procedure. After the program has
been run, the results can be viewed using `PROFILE LIST`.
\usage PROFILE <ON|OFF|LIST>
\bugs
It is not possible to switch the system and procedure profiling on and off
independently.
***/
void iprofile() {
  switch (*cip++) {
  case I_ON:
    profile_enabled = true;
    break;
  case I_OFF:
    profile_enabled = false;
    break;
  case I_LIST:
    for (int i = 0; i < procs.size(); ++i) {
      struct proc_t &p = procs.proc(i);
      sprintf(lbuf, "%10d %s", p.profile_total, proc_names.name(i));
      c_puts(lbuf); newline();
    }
    break;
  default:
    SYNTAX_T("exp ON, OFF or LIST");
    break;
  }
}

#ifdef ESP8266
#include <eboot_command.h>
#include <spi_flash.h>
#endif
/***bc sys BOOT
Boots the system from the specified flash page.
\usage BOOT page
\args
@page	flash page to boot from [`0` to `255`]
\note
The `BOOT` command does not verify if valid firmware code is installed at
the given flash page. Use with caution.
***/
void iboot() {
#if !defined(HOSTED) && defined(ESP8266)
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
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bas EXEC
Executes a BASIC program as a child process.
\usage EXEC program_file$
\args
@program_file$	name of the BASIC program file to be executed
\note
A program run via `EXEC` runs in its own program and variable space, and
nothing it does affects the calling program or its variables, with the
exception of the following:

* *Open files* are shared between the parent and child program. Any changes
  will be visible to both programs.
* *Error handlers* set in the parent program will be triggered by errors
  in the child program, unless they are handled by the child.
\bugs
There is no generalized way to share data between parent and child program.
\ref CHAIN
***/
void iexec() {
  BString file = getParamFname();
  int is_text = bfs.IsText((char *)file.c_str());
  if (is_text < 0) {
    err = -is_text;
    return;
  } else if (!is_text) {
    E_ERR(FORMAT, "not a BASIC program");
    return;
  }
  basic_ctx_t *old_bc = bc;
  bc = new basic_ctx_t;
  listbuf = NULL;
  loadPrgText((char *)file.c_str(), NEW_ALL);
  clp = listbuf;
  cip = clp + sizeof(line_desc_t);
  irun(clp);
  if (err) {
    if (old_bc->_event_error_enabled) {
      // This replicates the code in irun() because we have to get the error
      // line number in the context of the subprogram.
      int sub_line = getlineno(clp);
      free(listbuf);
      delete bc;
      bc = old_bc;
      retval[0] = err;
      retval[1] = getlineno(clp);
      retval[2] = sub_line;
      err = 0;
      event_error_enabled = false;
      event_error_resume_lp = NULL;
      clp = event_error_lp;
      cip = event_error_ip;
      err_expected = NULL;	// prevent stale "expected" messages
      return;
    } else {
      // Print the error in the context of the subprogram.
      error();
    }
  } 
  free(listbuf);
  delete bc;
  bc = old_bc;
}

void iextend();

typedef void (*cmd_t)();
#include "funtbl.h"

void BASIC_INT iextend() {
  if (*cip >= sizeof(funtbl_ext) / sizeof(funtbl_ext[0])) {
    err = ERR_SYS;
    return;
  }
  funtbl_ext[*cip++]();
}

// 中間コードの実行
// 戻り値      : 次のプログラム実行位置(行の先頭)
unsigned char* BASIC_FP iexe(int stk) {
  uint8_t c;               // 入力キー
  err = 0;

  while (*cip != I_EOL) { //行末まで繰り返す
    //強制的な中断の判定
    if ((c = sc0.peekKey())) { // もし未読文字があったら
      if (process_hotkeys(c)) { // 読み込んでもし[ESC],［CTRL_C］キーだったら
	err_expected = NULL;
	break;
      }
    }

    //中間コードを実行
    if (*cip < sizeof(funtbl)/sizeof(funtbl[0])) {
      funtbl[*cip++]();
    } else
      SYNTAX_T("exp command");

    pump_events();

    if (err || gstki < stk)
      return NULL;
  }

  return clp + *clp;
}

void iflash();

void SMALL resize_windows()
{
#ifdef USE_BG_ENGINE
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
    for (int i = 0; i < MAX_BG; ++i) {
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
      sc0.setScroll(true);
      sc0.locate(0,0);
      sc0.cls();
      restore_text_window = true;
    }
  }
#endif
}

void SMALL restore_windows()
{
#ifdef USE_BG_ENGINE
  if (restore_text_window) {
    restore_text_window = false;
    sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
    sc0.setScroll(true);
    sc0.locate(original_text_pos[0], original_text_pos[1]);
  }
  if (restore_bgs) {
    restore_bgs = false;
    for (int i = 0; i < MAX_BG; ++i) {
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

  case I_CHAIN:  if(ilrun()) {
      sc0.show_curs(0); irun(clp);
    }
    break;
  case I_RUN:
    if (is_strexp()) {
      // RUN with file name -> we need to check for arguments
      // Problem: loading a new program clobbers ibuf, so we won't be able
      // to parse our arguments afterwards. We therefore save the current
      // instruction pointer, parse the arguments, reset the instruction
      // pointer, and then call ilrun(), which will only look at the file
      // name and discard everything else.
      // XXX: This means the file name is evaluated twice. Not sure if that
      // will be a problem in practice.
      unsigned char *save_cip = cip;
      istrexp();	// dump file name
      int nr = 0, sr = 0;
      for (int i = 0; i < MAX_RETVALS; ++i) {
        retval[i] = 0;
        retstr[i] = BString();
      }
      while (*cip == I_COMMA) {
        ++cip;
        if (is_strexp()) {
          retstr[sr++] = istrexp();
        } else {
          retval[nr++] = iexp();
        }
        if (err)
          break;
      }
      if (err)
        break;
      cip = save_cip;
      ilrun();
    }
    sc0.show_curs(0);
    irun();
    break;
/***bc bas CONT
Continues an interrupted program.
\desc
`CONT` can be used to resume a program run that has been interrupted by
kbd:[Ctrl+C] or by the `STOP` command. It will reset text window and
tiled background layouts if they have been automatically adjusted when the
program was interrupted.
\usage CONT
\ref BG STOP WINDOW
***/
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

  bc = new basic_ctx_t;

  // try to mount SPIFFS, ignore failure (do not format)
#ifdef UNIFILE_USE_SPIFFS
#ifdef ESP8266_NOWIFI
  SPIFFS.begin(false);
#else
  SPIFFS.begin();
#endif
#elif defined(UNIFILE_USE_FASTROMFS)
  fs.mount();
#elif defined(UNIFILE_USE_FS)
  SPIFFS.begin();
#endif
  loadConfig();

  // Initialize SD card file system
  bfs.init(16);		// CS on GPIO16

  if (!Unifile::chDir(SD_PREFIX))
    Unifile::chDir(FLASH_PREFIX);
  else
    bfs.fakeTime();

  vs23.begin(CONFIG.interlace, CONFIG.lowpass, CONFIG.NTSC != 0);
  vs23.setLineAdjust(CONFIG.line_adjust);
  vs23.setColorSpace(0);

  psx.setupPins(0, 1, 2, 3, 1);

  size_list = 0;
  listbuf = NULL;
  memset(user_files, 0, sizeof(user_files));

  // Initialize execution environment
  inew();

  sc0.setCursorColor(CONFIG.cursor_color);
  sc0.init(SIZE_LINE, CONFIG.NTSC, CONFIG.mode - 1);
  sc0.reset_kbd(CONFIG.KEYBOARD);

  Wire.begin(2, 0);
  // ESP8266 Wire code assumes that SCL and SDA pins are set low, instead
  // of taking care of that itself. WTF?!?
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);

  icls();
  sc0.show_curs(0);

  // Want to make sure we get the right hue.
  csp.setColorConversion(0, 7, 2, 4, true);
  show_logo();
  vs23.setColorSpace(0);	// reset color conversion

  // Startup screen
  // Epigram
  sc0.setFont(fonts[1]);
  sc0.setColor(csp.colorFromRgb(72,72,72), COL(BG));
  srand(ESP.getCycleCount());
  c_puts_P(epigrams[random(sizeof(epigrams)/sizeof(*epigrams))]);
  newline();

  // Banner
  sc0.setColor(csp.colorFromRgb(192,0,0), COL(BG));
  static const char engine_basic[] PROGMEM = "Engine BASIC";
  c_puts_P(engine_basic);
  sc0.setFont(fonts[CONFIG.font]);

  // Platform/version
  sc0.setColor(csp.colorFromRgb(64,64,64), COL(BG));
  static const char __e[] PROGMEM = STR_EDITION;
  sc0.locate(sc0.getWidth() - strlen_P(__e), 7);
  c_puts_P(__e);
  static const char __v[] PROGMEM = STR_VARSION;
  sc0.locate(sc0.getWidth() - strlen_P(__v), 8);
  c_puts_P(__v);

  // Initialize file systems, format SPIFFS if necessary
#ifdef UNIFILE_USE_SPIFFS
  SPIFFS.begin();
#elif defined(UNIFILE_USE_FS)
  SPIFFS.begin(true);
#elif defined(UNIFILE_USE_FASTROMFS)
  if (!fs.mount()) {
    fs.mkfs();
    fs.mount();
  }
#endif
  // Free memory
  sc0.setColor(COL(FG), COL(BG));
  sc0.locate(0,2);
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
#else
  putnum(try_malloc(), 0);
#endif
  PRINT_P(" bytes free\n");

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
    redirect_input_file = -1;
    redirect_output_file = -1;
    rc = sc0.edit();
    if (rc) {
      textline = (char*)sc0.getText();
      int textlen = strlen(textline);
      if (!textlen) {
        free(textline);
	newline();
	continue;
      }
      if (textlen >= SIZE_LINE) {
        free(textline);
	err = ERR_LONG;
	newline();
	error();
	continue;
      }

      strcpy(lbuf, textline);
      free(textline);
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

#define CONFIG_FILE "/flash/.config"

// システム環境設定のロード
void loadConfig() {
  CONFIG.NTSC      =  0;
  CONFIG.line_adjust = 0;
  CONFIG.KEYBOARD  =  1;
  memcpy_P(CONFIG.color_scheme, default_color_scheme, sizeof(CONFIG.color_scheme));
  CONFIG.mode = SC_DEFAULT + 1;
  CONFIG.font = 0;
  CONFIG.cursor_color = 0x92;
  CONFIG.beep_volume = 15;
  
  Unifile f = Unifile::open(BString(F(CONFIG_FILE)), UFILE_READ);
  if (!f)
    return;
  f.read((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}

/***bc sys SAVE CONFIG
Saves the current set of configuration options as default.
\usage SAVE CONFIG
\note
The configuration will be saved as a file under the name `/flash/.config`.
\ref CONFIG
***/
void isaveconfig() {
  Unifile f = Unifile::open(BString(F(CONFIG_FILE)), UFILE_OVERWRITE);
  if (!f) {
    err = ERR_FILE_OPEN;
  }
  f.write((char *)&CONFIG, sizeof(CONFIG));
  f.close();
}

void syspanic(const char *txt) {
  redirect_output_file = -1;
  c_puts_P(txt);
  PRINT_P("\nSystem halted");
  Serial.println(txt);
  Serial.println(F("System halted"));
  for (;;);
}
