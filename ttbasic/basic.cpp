/*
   TOYOSHIKI Tiny BASIC for Arduino
   (C)2012 Tetsuya Suzuki
   GNU General Public License
   2017/03/22, Modified by Tamakichi、for Arduino STM32
   Modified for BASIC Engine by Ulrich Hecht
   (C) 2017-2019 Ulrich Hecht
 */

#include <Arduino.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <utf8.h>
#include "ttconfig.h"
#include "video.h"
#include "sound.h"

#include "eb_bg.h"
#include "eb_img.h"
#include "eb_input.h"
#include "eb_sys.h"

#include "epigrams.h"

#include <fonts.h>

#ifdef ESP8266

#ifdef ESP8266_NOWIFI
#define STR_EDITION "ESP8266"
#else
#define STR_EDITION "ESP8266 WiFi"
#endif

#elif defined(ESP32)

#define STR_EDITION "ESP32"

#elif defined(H3)

#define STR_EDITION "H3"

#elif defined(SDL)

#ifdef __linux__
#define STR_EDITION "Linux/SDL"
#else
#define STR_EDITION _("unknown/SDL")
#endif

#else

#define STR_EDITION _("unknown")

#endif

#include "version.h"

#include "basic.h"
#include "net.h"

// size by which the list buffer is incremented when full
#define LISTBUF_INC 128

struct unaligned_num_t {
  num_t n;
} __attribute__((packed));

#define UNALIGNED_NUM_T(ip) (reinterpret_cast<struct unaligned_num_t *>(ip)->n)

// Definition of TOYOSHIKI TinyBASIC program usage area

// *** SD card management *****************
sdfiles bfs;

#define DECL_TABLE
#include "numfuntbl.h"
#include "strfuntbl.h"
#include "funtbl.h"
#undef DECL_TABLE
#include "eb_basic.h"
#include "eb_basic_int.h"

std::vector<Basic::cmd_t> Basic::funtbl;
std::vector<Basic::numfun_t> Basic::numfuntbl;
std::vector<Basic::strfun_t> Basic::strfuntbl;

#ifdef FLOAT_NUMS
// Get command arguments (int32_t, no argument check)
uint8_t BASIC_FP Basic::getParam(int32_t &prm, token_t next_token) {
  num_t p = iexp();
  prm = p;
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}
// Get command argument (int32_t, with argument check)
uint8_t BASIC_FP Basic::getParam(int32_t &prm, int32_t v_min, int32_t v_max,
                                 token_t next_token) {
  prm = iexp();
  if (!err && (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}
#endif

// Get command argument (num_t, with argument check)
uint8_t BASIC_FP Basic::getParam(num_t &prm, num_t v_min, num_t v_max,
                                 token_t next_token) {
  prm = iexp();
  if (!err && (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

// Get command argument (uint32_t, with argument check)
uint32_t BASIC_FP Basic::getParam(uint32_t &prm, uint32_t v_min, uint32_t v_max,
                                  token_t next_token) {
  prm = (uint32_t)(int64_t)iexp();
  if (!err && (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

// Get command arguments (uint32_t, no argument check)
uint8_t BASIC_FP Basic::getParam(uint32_t &prm, token_t next_token) {
  prm = (uint32_t)(int64_t)iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}

// Get command arguments (num_t, no argument check)
uint8_t BASIC_FP Basic::getParam(num_t &prm, token_t next_token) {
  prm = iexp();
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    if (next_token == I_OPEN || next_token == I_CLOSE)
      err = ERR_PAREN;
    else
      E_SYNTAX(next_token);
  }
  return err;
}

// prototype
void isaveconfig();
void mem_putch(utf8_int32_t c);
num_t BASIC_FP iexp(void);
void error(uint8_t flgCmd);

// Convert a single digit hexadecimal character to an integer
uint16_t BASIC_INT hex2value(char c) {
  if (c <= '9' && c >= '0')
    return c - '0';
  else if (c <= 'f' && c >= 'a')
    return c - 'a' + 10;
  else if (c <= 'F' && c >= 'A')
    return c - 'A' + 10;
  return 0;
}

int redirect_output_file = -1;
int redirect_input_file = -1;

// Character output
extern inline void c_putch(utf8_int32_t c, uint8_t devno) {
  if (devno == 0 && redirect_output_file < 0) {
      screen_putch(c);
  } else if (devno == 2)
    sc0.gputch(c);
  else {
    char utfbuf[4];
    char *start = utfbuf;
    char *end = (char *)utf8catcodepoint((void *)start, c, 4);
    while (start < end) {
      if (devno == 0)
        putc(*start++, user_files[redirect_output_file].f);
      if (devno == 1)
        Serial.write(*start++);
      else if (devno == 3)
        mem_putch(*start++);
      else if (devno == 4)
        bfs.putch(*start++);
    }
  }
}

void BASIC_INT newline(uint8_t devno) {
  if (devno == 0) {
    if (redirect_output_file >= 0) {
      putc('\n', user_files[redirect_output_file].f);
      return;
    }
    sc0.newLine();
#ifdef BUFFERED_SCREEN
    process_events();
#endif
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

// Support function for tick
void Basic::iclt() {
  //systick_uptime_millis = 0;
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
| `mod` < 0 | A random fractional number between 0 and 1. A specific
              value of `mod` will always yield the same random number.
\endtable
***/
num_t BASIC_FP getrnd(int value) {
  if (value < 0)
    randomSeed(value);
  if (value <= 1)
    return ((num_t)random(RAND_MAX) / (num_t)RAND_MAX);
  else
    return random(value);
}

#ifdef ESP8266
#define __FLASH__ ICACHE_RODATA_ATTR
#endif

// List formatting condition
// Intermediate code without trailing blank
// clang-format off
const token_t i_nsa[] BASIC_DAT = {
  I_COMMA, I_SEMI, I_COLON, I_SQUOT,I_QUEST,
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_DOLLAR, I_LSHIFT, I_RSHIFT, I_POW,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2, I_LT,
  I_SQOPEN, I_SQCLOSE,
  I_INKEYSTR,
  I_UP, I_DOWN, I_LEFT, I_RIGHT, I_TAB,
};

// Intermediate code which eliminates previous space when former is constant or variable
const token_t i_nsb[] BASIC_DAT = {
  I_MINUS, I_PLUS, I_MUL, I_DIV, I_OPEN, I_CLOSE, I_LSHIFT, I_RSHIFT, I_POW,
  I_GTE, I_SHARP, I_GT, I_EQ, I_LTE, I_NEQ, I_NEQ2,I_LT,
  I_COMMA, I_SEMI, I_COLON, I_SQUOT, I_EOL, I_SQOPEN, I_SQCLOSE,
};

// insert a blank before intermediate code
const token_t i_sf[] BASIC_DAT  = {
  I_CLS, I_COLOR, I_DATE, I_END, I_FILES, I_TO, I_STEP,I_QUEST,I_AND, I_OR, I_XOR,
  I_GET,I_TIME,I_GOSUB,I_GOTO,I_INPUT,I_LET,I_LIST,I_ELSE,I_THEN,
  I_LOAD,I_LOCATE,I_NEW,I_DOUT,I_POKE,I_PRINT,I_REFLESH,I_REM,I_RENUM,I_CLT,
  I_RETURN,I_RUN,I_SAVE,I_SET,I_WAIT,
  I_PSET, I_LINE, I_RECT, I_CIRCLE, I_BLIT, I_SWRITE, I_SPRINT,I_SMODE,
  I_BEEP, I_CSCROLL, I_MOD, I_SIZE, I_FOR,
};

// tokens that can be functions (no space before paren) or something else
const token_t i_dual[] BASIC_DAT = {
  I_CHAR, I_SYS, I_PLAY,
};
// clang-format on

// exception search function
char sstyle(token_t code, const token_t *table, uint32_t count) {
  while (count--)  //中間コードの数だけ繰り返す
    // if there is a corresponding intermediate code
#ifdef LOWMEM
    if (code == pgm_read_byte(&table[count]))
#else
    if (code == table[count])
#endif
      return 1;
  return 0;
}

// exception search macro
#define nospacea(c) sstyle(c, i_nsa, sizeof(i_nsa) / sizeof(token_t))
#define nospaceb(c) ((c) == I_EOL || (c) == I_IMPLICITENDIF || sstyle(c, i_nsb, sizeof(i_nsb) / sizeof(token_t)))
#define spacef(c)   sstyle(c, i_sf, sizeof(i_sf) / sizeof(token_t))
#define dual(c)     sstyle(c, i_dual, sizeof(i_dual) / sizeof(token_t))
#define isfun(c)    (dual(c) || numfuntbl[c] || strfuntbl[c])

// Error message definition
uint8_t err;               // Error message index
const char *err_expected;  // Clarification for the user

#define ESTR(n, s) static const char _errmsg_##n[] PROGMEM = s;
#include "errdef.h"

#undef ESTR
#define ESTR(n, s) _errmsg_##n,
static const char *const errmsg[] PROGMEM = {
#include "errdef.h"
};

#undef ESTR

#include "error.h"

void SMALL E_VALUE(int32_t from, int32_t to) {
  err = ERR_VALUE;
  if (from == INT32_MIN)
    sprintf_P(tbuf, _("max %d"), (int)to);
  else if (to == INT32_MAX)
    sprintf_P(tbuf, _("min %d"), (int)from);
  else
    sprintf_P(tbuf, _("from %d to %d"), (int)from, (int)to);
  err_expected = tbuf;
}

void E_SYNTAX(token_t token) {
  err = ERR_SYNTAX;
  if (token < kwtbl.size() && kwtbl[token]) {
    strcpy_P(tbuf, _("expected \""));
    strcat_P(tbuf, kwtbl[token]);
    strcat_P(tbuf, PSTR("\""));
    err_expected = tbuf;
  } else
    err_expected = NULL;
}

// RAM mapping
char lbuf[SIZE_LINE];  // Command input buffer
char tbuf[SIZE_LINE];  // Text display buffer
int32_t tbuf_pos = 0;

// BASIC line number descriptor.
// NB: A lot of code relies on next being the first element.
typedef struct {
#ifdef LOWMEM
  uint8_t  next;
#else
  uint32_t next;
#endif
  union {
    num_t raw_line;
    struct {
      uint32_t line;
      int8_t indent;
    };
  };
} __attribute__((packed)) line_desc_t;

#define icodes_per_line_desc() (sizeof(line_desc_t) / sizeof(icode_t))

Basic *bc = NULL;

// メモリへの文字出力
inline void mem_putch(utf8_int32_t c) {
  if (tbuf_pos < SIZE_LINE) {
    tbuf[tbuf_pos] = c;
    tbuf_pos++;
  }
}

// Standard C library (about) same functions
// XXX: We pull in ctype anyway, can do away with these.
char c_isprint(utf8_int32_t c) {
  //return(c >= 32 && c <= 126);
  return (c >= 32 && c != 127);
}

// Delete whitespace on the right side of the string
char *tlimR(char *str) {
  uint32_t len = strlen(str);
  for (uint32_t i = len - 1; i > 0; i--) {
    if (str[i] == ' ') {
      str[i] = 0;
    } else {
      break;
    }
  }
  return str;
}

void c_puts(const char *s, uint8_t devno) {
  utf8_int32_t c;
  while (*s) {
    s = (const char *)utf8codepoint(s, &c);
    c_putch(c, devno);
  }
}

void c_puts_P(const char *s, uint8_t devno) {
  utf8_int32_t c;
  while (pgm_read_byte(s)) {
    s = (const char *)utf8codepoint(s, &c);
    c_putch(c, devno);
  }
}

int c_printf(const char *f, ...) {
  char *out;
  va_list ap;
  va_start(ap, f);
  int ret = vasprintf(&out, f, ap);
  va_end(ap);
  c_puts(out);

  return ret;
}

// Print numeric specified columns
// arguments
//  value : Output target value
//  d     : Digits (0 means not specified)
//  devno : Output device
// function
// 'SNNNNN' S: Sign N: Number or blank
// If the digit is specified by d, blank complements
//
#ifdef FLOAT_NUMS
static const char num_prec_fmt[] PROGMEM = "%%%s%d.%dg";
#else
static const char num_fmt[] PROGMEM = "%%%s%dd";
#endif
void putnum(num_t value, int8_t d, uint8_t devno) {
  char f[13];
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

// Output in hexadecimal
// Arguments:
//   value: Output target number
//   d: Digits (0 is not specified)
//   devno: Output device
// function
//   'XXXX' X: number
//   0 complement when digit is specified by d.
//   Does not consider sign.
//
void putHexnum(uint32_t value, uint8_t d, uint8_t devno) {
  char s[] = "%0.X";
  s[2] = '0' + d;
  sprintf(lbuf, s, value);
  c_puts(lbuf, devno);
}

// Binary output
// Arguments:
//   value: Output target number
//   d: Digits (0 is not specified)
//   devno: Output device
// function
//   'BBBBBBBBBBBBBBBB' B: Number
//   0 complement when digit is specified by d
//   Does not consider sign.
//
void putBinnum(uint32_t value, uint8_t d, uint8_t devno = 0) {
  uint32_t bin = (uint32_t)value;  // Interpreted as unsigned hexadecimal
  uint32_t b;
  uint32_t dig = 0;

  for (uint8_t i = 0; i < 16; i++) {
    b = (bin >> (15 - i)) & 1;
    lbuf[i] = b ? '1' : '0';
    if (!dig && b)
      dig = 16 - i;
  }
  lbuf[16] = 0;

  if (d > dig)
    dig = d;
  c_puts(&lbuf[16 - dig], devno);
}

void get_input(bool numeric, uint8_t eoi) {
  utf8_int32_t c;       //文字
  uint32_t len;  //文字数

  len = 0;  //文字数をクリア
  while (1) {
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
    } else if (len < SIZE_LINE - 1 &&
               (!numeric || c == '.' || c == '+' || c == '-' || isDigit(c) ||
                c == 'e' || c == 'E')) {
      if (utf8catcodepoint(&lbuf[len], c, SIZE_LINE - len))
        len += utf8codepointsize(c);
      else {
        err = ERR_LONG;
        return;
      }
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
  lbuf[len] = 0;  //終端を置く
}

// 数値の入力
num_t getnum(uint8_t eoi = '\r') {
  num_t value;

  get_input(true, eoi);
  value = strtonum(lbuf, NULL);
  // XXX: say "EXTRA IGNORED" if there is unused input?

  return value;
}

BString getstr(uint8_t eoi) {
  get_input(false, eoi);
  return BString(lbuf);
}

// キーワード検索
//[戻り値]
//  該当なし   : -1
//  見つかった : キーワードコード
int lookup(char *str) {
  for (uint32_t i = 0; i < kwtbl.size(); ++i) {
    if (kwtbl[i]) {
      int l = strlen_P(kwtbl[i]);

      if (strncasecmp_P(str, kwtbl[i], l))
        continue;

      // Don't match if
      // - keyword separation is not optional, and
      // - the keyword ends in a letter, and
      // - the keyword is not ELSE (to allow "ELSEIF"), and
      // - the keyword is not FN, and
      // - the character after the keyword is valid in an identifier.

      // NB: There are keywords other than FN that ECMA BASIC-2 does not
      // allow in identifiers, but they are extremely arbitrary and have
      // therefore been omitted here.
      // FN is here because there are tests relying on it being glued to
      // function identifiers.
      if (!CONFIG.keyword_sep_optional && isAlpha(str[l - 1]) &&
          i != I_ELSE && i != I_FN &&
          (isAlphaNumeric(str[l]) || str[l] == '_'))
        continue;

      return i;
    }
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
           var_len < MAX_VAR_NAME - 1);
  vname[--var_len] = 0;  // terminate C string
  return var_len;
}

uint32_t getlineno(icode_t *lp);

//
// Convert text to intermediate code
// [Return value]
// 0 or intermediate code number of items
//
// If find_prg_text is true (default), variable and procedure names
// are designated as belonging to the program; if not, they are considered
// temporary, even if the input line starts with a number.
unsigned int BASIC_INT SMALL Basic::toktoi(bool find_prg_text) {
  unsigned int i;
  int key;
  unsigned int len = 0;      // length of sequence of intermediate code
  char *ptok;                // pointer to the inside of one word
  char *s = lbuf;            // pointer to the inside of the string buffer
  char c;                    // Character used to enclose string (")
  num_t value;               // constant
  uint32_t hex;              // hexadecimal constant
  uint16_t hcnt;             // hexadecimal digit count
  uint8_t var_len;           // variable name length
  char vname[MAX_VAR_NAME];  // variable name
  bool had_if = false;
  int implicit_endif = 0;

  bool is_prg_text = false;
  bool is_require = false;

  while (*s) {           //文字列1行分の終端まで繰り返す
    while (isspace(*s))  //空白を読み飛ばす
      s++;

    if (is_require && *s != '"') {
      SYNTAX_T("expected module name");
      return 0;
    }

    if (!*s)  // Abort if nothing left after skipping whitespace.
      break;

    bool isext = false;
    key = lookup(s);
    if (key < 0) {
    } else {
      s += strlen_P(kwtbl[key]);
    }
    if (key >= 0) {
      if (key == I_REQUIRE)
        is_require = true;
      // 該当キーワードあり
      if (len >= SIZE_IBUF - 2) {  // もし中間コードが長すぎたら
        err = ERR_IBUFOF;          // エラー番号をセット
        return 0;                  // 0を持ち帰る
      }
      // translate "END IF" to "ENDIF"
      if (key == I_IF && len > 0 && ibuf[len - 1] == I_END)
        ibuf[len - 1] = I_ENDIF;
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
        ibuf[len++] = key;  // 中間コードを記録
      }
      if (had_if && !isext && (key == I_THEN || key == I_GOTO)) {
        had_if = false;  // prevent multiple implicit endifs for "THEN GOTO"
        while (isspace(*s))
          s++;
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

    // Try hexadecimal conversion $XXXX
    if (key == I_DOLLAR) {
      if (isHexadecimalDigit(*s)) {
        hex = 0;
        hcnt = 0;  // Number of digits
        do {
          hex = (hex << 4) + hex2value(*s++);
          hcnt++;
        } while (isHexadecimalDigit(*s));

        if (hcnt > 8) {  // Overflow check
          err = ERR_VOF;
          return 0;
        }

        if (len >= SIZE_IBUF - 5) {  // もし中間コードが長すぎたら
          err = ERR_IBUFOF;          // エラー番号をセット
          return 0;                  // 0を持ち帰る
        }
        //s = ptok;               // 文字列の処理ずみの部分を詰める
        len--;                    // I_DALLARを置き換えるために格納位置を移動
        ibuf[len++] = I_HEXNUM;   //中間コードを記録
#if TOKEN_TYPE == uint32_t
        ibuf[len++] = hex;
#else
        ibuf[len++] = hex & 255;  //定数の下位バイトを記録
        ibuf[len++] = hex >> 8;   //定数の上位バイトを記録
        ibuf[len++] = hex >> 16;
        ibuf[len++] = hex >> 24;
#endif
      }
    }

    if (isext) {
      find_prg_text = false;
      continue;
    }

    //コメントへの変換を試みる
    if (key == I_REM || key == I_SQUOT) {  // もし中間コードがI_REMなら
      while (isspace(*s))                  // 空白を読み飛ばす
        s++;
      ptok = s;  // コメントの先頭を指す

      for (i = 0; *ptok++; i++) {}     // コメントの文字数を得る
      if (len >= SIZE_IBUF - 3 - i) {  // もし中間コードが長すぎたら
        err = ERR_IBUFOF;              // エラー番号をセット
        return 0;                      // 0を持ち帰る
      }

      ibuf[len++] = i;       // コメントの文字数を記録
      while (i--) {          // コメントの文字数だけ繰り返す
        ibuf[len++] = *s++;  // コメントを記録
      }
      break;                 // 文字列の処理を打ち切る（終端の処理へ進む）
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
Procedures can only be called using `CALL` or `FN`. If a `PROC` instruction
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
      if (len >= SIZE_IBUF - 2) {  //もし中間コードが長すぎたら
        err = ERR_IBUFOF;
        return 0;
      }

      while (isspace(*s))
        s++;
      s += parse_identifier(s, vname);

      int idx = proc_names.assign(vname, true);
      ibuf[len++] = idx;
      if (procs.reserve(proc_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_LABEL) {
      if (len >= SIZE_IBUF - 2) {  //もし中間コードが長すぎたら
        err = ERR_IBUFOF;
        return 0;
      }

      while (isspace(*s))
        s++;
      s += parse_identifier(s, vname);

      index_t idx = label_names.assign(vname, true);
      ibuf[len++] = idx;
      if (labels.reserve(label_names.varTop())) {
        err = ERR_OOM;
        return 0;
      }
    } else if (key == I_CALL || key == I_FN) {
      while (isspace(*s))
        s++;
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
    ptok = s;  // Points to the beginning of a word
    if (isDigit(*ptok) || *ptok == '.') {
      if (len >= SIZE_IBUF - icodes_per_num() - 2) {
        // the intermediate code is too long
        err = ERR_IBUFOF;
        return 0;
      }
      value = strtonum(ptok, &ptok);
      if (s == ptok) {
        // nothing parsed, most likely a random single period
        SYNTAX_T(_("invalid number"));
        return 0;
      }
      s = ptok;             // Stuff the processed part of the character string
      ibuf[len++] = I_NUM;  // Record intermediate code
      UNALIGNED_NUM_T(ibuf + len) = value;
      len += icodes_per_num();
      if (find_prg_text) {
        is_prg_text = true;
        find_prg_text = false;
      }
    } else if (*s == '"') {
      // Attempt to convert to a character string

      c = *s++;  // Remember " and go to the next character
      ptok = s;  // Points to the beginning of the string

      // Get the number of characters in a string
      for (i = 0; *ptok && (*ptok != c); i++)
        ptok++;

      if (len >= SIZE_IBUF - 3 - i) {  // if the intermediate code is too long
        err = ERR_IBUFOF;
        return 0;
      }

      BString sstr;
      ibuf[len++] = I_STR;
      ibuf[len++] = i;  // record the number of characters in the string
      while (i--) {
        sstr.concat(*s);
        ibuf[len++] = *s++;  // Record character
      }

      if (is_require) {
        if (eb_load_module(sstr.c_str()) < 0)
          return 0;
        is_require = false;
      }

      if (*s == c)  // If the character is ", go to the next character
        s++;
    } else if (*ptok == '@' || *ptok == '~' || isAlpha(*ptok)) {
      // Try converting to variable
      bool is_local = false;
      bool is_list = false;
      if (*ptok == '@') {
        is_local = true;
        ++ptok;
        ++s;
      } else if (*ptok == '~') {
        is_list = true;
        ++ptok;
        ++s;
      }

      var_len = parse_identifier(ptok, vname);
      if (len >= SIZE_IBUF - 3) {  // if the intermediate code is too long
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
            (is_list && str_lst.reserve(str_lst_names.varTop())))
          goto oom;
        s += var_len + 2;
        ptok += 2;
      } else if (*p == '$') {
        token_t tok;
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
            (is_list && str_lst.reserve(str_lst_names.varTop())))
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
            (is_list && num_lst.reserve(num_lst_names.varTop())))
          goto oom;
        s += var_len + 1;
        ptok++;
      } else {
        // Convert to intermediate code
        token_t tok;
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
            (is_list && num_lst.reserve(num_lst_names.varTop())))
          goto oom;
        s += var_len;
      }
    } else if (*ptok == '|') {
      ++ptok;
      ++s;
      s += parse_identifier(s, vname);
      void *sym = get_symbol(vname);
      if (!sym) {
        err = ERR_UNDEFSYM;
        strcpy(tbuf, vname);
        err_expected = tbuf;
        return 0;
      }
      ibuf[len++] = I_NFC;
      *(uintptr_t *)(ibuf + len) = (uintptr_t)sym;
      len += icodes_per_ptr();
      ptok++;
    } else {  // if none apply
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
int Basic::list_free() {
  icode_t *lp;

  // Move the pointer to the end of the list
  for (lp = listbuf; *lp; lp += *lp) {}

  return listbuf + size_list - lp - 1;  // Calculate the rest and return it
}

// Get line numbere by line pointer
uint32_t BASIC_FP getlineno(icode_t *lp) {
  line_desc_t *ld = (line_desc_t *)lp;
  if (ld->next == 0)  //もし末尾だったら
    return (uint32_t)-1;
  return ld->line;
}

// Search line by line number
icode_t *BASIC_FP Basic::getlp(uint32_t lineno) {
  icode_t *lp;  //ポインタ

  for (lp = listbuf; *lp; lp += *lp)  // Repeat from top to bottom
    if (getlineno(lp) >= lineno)
      break;

  return lp;  //ポインタを持ち帰る
}

// Get line index from line number
uint32_t Basic::getlineIndex(uint32_t lineno) {
  icode_t *lp;  //ポインタ
  uint32_t index = 0;
  uint32_t rc = INT32_MAX;
  for (lp = listbuf; *lp; lp += *lp) {  // 先頭から末尾まで繰り返す
    if (getlineno(lp) >= lineno) {      // もし指定の行番号以上なら
      rc = index;
      break;  // 繰り返しを打ち切る
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
icode_t *BASIC_INT Basic::getELSEptr(icode_t *p, bool endif_only, int adjust) {
  icode_t *rc = NULL;
  icode_t *lp;
  index_t lifstki = 1 + adjust;
  icode_t *stlp = clp;
  icode_t *stip = cip;

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (lp = p;;) {
    switch (*lp) {
    case I_IF:  // IF命令
      lp++;
      lifstki++;
      break;
    case I_ELSE:  // ELSE命令
      if (lifstki == 1 && !endif_only) {
        // Found the highest-level ELSE, we're done.
        rc = lp + 1;
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
        clp = stlp;
        cip = stip;
        err = ERR_NOENDIF;
        return NULL;
      }
      lp = cip = clp + icodes_per_line_desc();
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
    default:  // その他
      lp += token_size(lp);
      break;
    }
  }
DONE:
  return rc;
}

icode_t *BASIC_INT Basic::getWENDptr(icode_t *p) {
  icode_t *rc = NULL;
  icode_t *lp;
  index_t lifstki = 1;

  for (lp = p;;) {
    switch (*lp) {
    case I_WHILE:
      if (lp[-1] != I_LOOP && lp[-1] != I_EXIT)
        lifstki++;
      lp++;
      break;
    case I_WEND:
      if (lifstki == 1) {
        // Found the highest-level WEND, we're done.
        rc = lp + 1;
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
      lp = cip = clp + icodes_per_line_desc();
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
uint32_t Basic::countLines(uint32_t st, uint32_t ed) {
  icode_t *lp;  //ポインタ
  uint32_t cnt = 0;
  uint32_t lineno;
  for (lp = listbuf; *lp; lp += *lp) {
    lineno = getlineno(lp);
    if (lineno == (uint32_t)-1)
      break;
    if ((lineno >= st) && (lineno <= ed))
      cnt++;
  }
  return cnt;
}

// Insert i-code to the list
// Insert [ibuf] in [listbuf]
//  [ibuf]: [1: data length] [1: I_NUM] [2: line number] [intermediate code]
//
void Basic::inslist() {
  icode_t *insp;     // 挿入位置ポインタ
  icode_t *p1, *p2;  // 移動先と移動元ポインタ
  int len;                 // 移動の長さ

  cont_clp = cont_cip = NULL;
  procs.reset();
  labels.reset();

  // Empty check (If this is the case, it may be impossible to delete lines
  // when only line numbers are entered when there is insufficient space ..)
  // @Tamakichi)
  if (list_free() < (int)*ibuf) {  // If the vacancy is insufficient
    int inc_mem = (*ibuf + LISTBUF_INC - 1) / LISTBUF_INC * LISTBUF_INC;
    listbuf = (icode_t *)realloc(listbuf, (size_list + inc_mem) * sizeof(icode_t));
    if (!listbuf) {
      err = ERR_OOM;
      size_list = 0;
      return;
    }
    size_list += inc_mem;
  }

  // Convert I_NUM literal to line number descriptor.
  line_desc_t *ld = (line_desc_t *)ibuf;
  num_t lin = ld->raw_line;
  ld->line = lin;
  ld->indent = 0;

  insp = getlp(getlineno(ibuf));  // 挿入位置ポインタを取得

  // 同じ行番号の行が存在したらとりあえず削除
  if (getlineno(insp) == getlineno(ibuf)) {  // もし行番号が一致したら
    p1 = insp;                               // p1を挿入位置に設定
    p2 = p1 + *p1;                           // p2を次の行に設定
    while ((len = *p2) != 0) {  // 次の行の長さが0でなければ繰り返す
      while (len--)             // 次の行の長さだけ繰り返す
        *p1++ = *p2++;          // 前へ詰める
    }
    *p1 = 0;  // リストの末尾に0を置く
  }

  // 行番号だけが入力された場合はここで終わる
  // もし長さが4（[長さ][I_NUM][行番号]のみ）なら
  if (*ibuf == icodes_per_num() + 2)
    return;

  // 挿入のためのスペースを空ける

  for (p1 = insp; *p1; p1 += *p1) {}  // p1をリストの末尾へ移動
  len = p1 - insp + 1;                // 移動する幅を計算
  p2 = p1 + *ibuf;  // p2を末尾より1行の長さだけ後ろに設定
  while (len--)     // 移動する幅だけ繰り返す
    *p2-- = *p1--;  // 後ろへズラす

  // 行を転送する
  len = *ibuf;      // 中間コードの長さを設定
  p1 = insp;        // 転送先を設定
  p2 = ibuf;        // 転送元を設定
  while (len--)     // 中間コードの長さだけ繰り返す
    *p1++ = *p2++;  // 転送
}

#define MAX_INDENT  10
#define INDENT_STEP 2

static int8_t indent_level;

// tokens that increase indentation
inline bool is_indent(icode_t *c) {
  return (*c == I_IF && c[-1] != I_ELSE) || (*c == I_DO && c[-1] != I_EXIT) ||
         (*c == I_WHILE && c[-1] != I_EXIT) ||
         (*c == I_FOR && c[1] != I_OUTPUT && c[1] != I_INPUT &&
          c[1] != I_APPEND && c[1] != I_DIRECTORY && c[-1] != I_EXIT);
}
// tokens that reduce indentation
inline bool is_unindent(token_t c) {
  return c == I_ENDIF || c == I_IMPLICITENDIF || c == I_NEXT || c == I_LOOP ||
         c == I_WEND;
}
// tokens that temporarily reduce indentation
inline bool is_reindent(token_t c) {
  return c == I_ELSE;
}

void SMALL recalc_indent_line(icode_t *lp) {
  bool re_indent = false;
  bool skip_indent = false;
  line_desc_t *ld = (line_desc_t *)lp;
  icode_t *ip = lp + icodes_per_line_desc();

  re_indent = is_reindent((token_t)*ip);  // must be reverted at the end of the line
  if (is_unindent((token_t)*ip) || re_indent) {
    skip_indent = true;  // don't do this again in the main loop
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
      else if (is_unindent((token_t)*ip))
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

void SMALL Basic::recalc_indent() {
  icode_t *lp = listbuf;
  indent_level = 0;

  while (*lp) {
    recalc_indent_line(lp);
    if (err)
      break;
    lp += *lp;
  }
}

// Text output of specified intermediate code line record
int SMALL Basic::putlist(icode_t *ip, uint8_t devno) {
  int mark = -1;
  unsigned char i;
  index_t var_code;
  line_desc_t *ld = (line_desc_t *)ip;
  ip += icodes_per_line_desc();

  for (i = 0; i < ld->indent && i < MAX_INDENT; ++i)
    c_putch(' ', devno);

  while (*ip != I_EOL) {
    // keyword processing
    if ((*ip < kwtbl.size() && kwtbl[*ip])) {
      // if it is a keyword
      const char *kw = kwtbl[*ip];

      if (isAlpha(pgm_read_byte(&kw[0])))
        sc0.setColor(COL(KEYWORD), COL(BG));
      else if (*ip == I_LABEL)
        sc0.setColor(COL(PROC), COL(BG));
      else
        sc0.setColor(COL(OP), COL(BG));
      // indent single-quote comment unless at start of line
      if (*ip == I_SQUOT && ip != (icode_t *)&ld[1]) {
        c_putch(' ', devno);
        c_putch(' ', devno);
      }
      c_puts_P(kw, devno);
      sc0.setColor(COL(FG), COL(BG));

      if (ip[1] != I_EOL && ip[1] != I_IMPLICITENDIF && ip[1] != I_COLON &&
          (ip[1] != I_OPEN || !isfun((token_t)*ip)))
        if ((!nospacea((token_t)*ip) || spacef((token_t)ip[1])) &&
            *ip != I_COLON && *ip != I_SQUOT && *ip != I_REM && *ip != I_LABEL)
          c_putch(' ', devno);

      if (*ip == I_REM || *ip == I_SQUOT) {  //もし中間コードがI_REMなら
        // Comments are really strings without quotes and need thus be UTF-8
        // decoded.
        // XXX: This should probably be factored out.
        goto handle_comment_strings;
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

      ip++;  //ポインタを次の中間コードへ進める
    } else if (*ip == I_NUM) {  //Process of constant
      ip++;                     //Advance the pointer to the value
      num_t n = UNALIGNED_NUM_T(ip);
      sc0.setColor(COL(NUM), COL(BG));
      putnum(n, 0, devno);  //値を取得して表示
      sc0.setColor(COL(FG), COL(BG));
      ip += icodes_per_num();  //ポインタを次の中間コードへ進める
      if (!nospaceb((token_t)*ip))  //もし例外にあたらなければ
        c_putch(' ', devno);        //空白を表示
    } else if (*ip == I_HEXNUM) {   //Processing hexadecimal constants
      ip++;                         //ポインタを値へ進める
      sc0.setColor(COL(NUM), COL(BG));
      c_putch('$', devno);  //空白を表示
#if TOKEN_TYPE == uint32_t
      uint32_t num = ip[0];
      ++ip;
#else
      uint32_t num = ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24);
      ip += 4;  //ポインタを次の中間コードへ進める
#endif
      int digits;
      if (num < 256)
        digits = 2;
      else if (num < 65536)
        digits = 4;
      else
        digits = 8;
      putHexnum(num, digits, devno);  //値を取得して表示
      sc0.setColor(COL(FG), COL(BG));
      if (!nospaceb((token_t)*ip))  //もし例外にあたらなければ
        c_putch(' ', devno);        //空白を表示
    } else if (*ip == I_VAR || *ip == I_LVAR) {  //もし定数なら
      if (*ip == I_LVAR) {
        sc0.setColor(COL(LVAR), COL(BG));
        c_putch('@', devno);
      } else
        sc0.setColor(COL(VAR), COL(BG));
      ip++;  //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(nvar_names.name(var_code), devno);
      sc0.setColor(COL(FG), COL(BG));

      if (!nospaceb((token_t)*ip))  //もし例外にあたらなければ
        c_putch(' ', devno);        //空白を表示
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
      ip++;  //ポインタを変数番号へ進める
      var_code = *ip++;
      c_puts(svar_names.name(var_code), devno);
      c_putch('$', devno);
      sc0.setColor(COL(FG), COL(BG));

      if (!nospaceb((token_t)*ip))  //もし例外にあたらなければ
        c_putch(' ', devno);        //空白を表示
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
handle_comment_strings:
      icode_t type = *ip;

      char c; //文字列の括りに使われている文字（「"」または「'」）

      ip++;                       //ポインタを文字数へ進める

      if (type == I_STR) {
        //文字列の括りに使われている文字を調べる
        c = '\"';                   //文字列の括りを仮に「"」とする
        for (i = *ip; i; i--)       //文字数だけ繰り返す
          if (*(ip + i) == '\"') {  //もし「"」があれば
            c = '\'';               //文字列の括りは「'」
            break;                  //繰り返しを打ち切る
          }

        sc0.setColor(COL(STR), COL(BG));

        //文字列を表示する
        c_putch(c, devno);  //文字列の括りを表示

      } else
        sc0.setColor(COL(COMMENT), COL(BG));

      if (type == I_REM)
        c_putch(' ', devno);

      i = *ip++;  //文字数を取得してポインタを文字列へ進める
      while (i) {  //文字数だけ繰り返す
        // String constants are stored as UTF-8, with the bytes cast to
        // icode_t. We create a temp string with "real" bytes and have that
        // decoded.
        utf8_int32_t codepoint;

        char tmp_utf8[4];
        char *tmp_ptr = tmp_utf8;

        // the first byte is sufficient to find out how big the codepoint is
        *tmp_ptr = *ip;
        int cpsize = utf8codepointcalcsize(tmp_utf8);
        i -= cpsize;

        while (cpsize--) {
          *tmp_ptr++ = *ip++;
        }

        utf8codepoint(tmp_utf8, &codepoint);
        c_putch(codepoint, devno);  //ポインタを進めながら文字を表示
      }

      if (type == I_REM || type == I_SQUOT) {
        sc0.setColor(COL(FG), COL(BG));
        return mark;
      }

      c_putch(c, devno);  //文字列の括りを表示
      sc0.setColor(COL(FG), COL(BG));
      // XXX: Why I_VAR? Such code wouldn't make sense anyway.
      if (*ip == I_VAR || *ip == I_ELSE || *ip == I_AS || *ip == I_TO ||
          *ip == I_THEN || *ip == I_FOR)
        c_putch(' ', devno);
    } else if (*ip == I_IMPLICITENDIF) {
      // invisible
      ip++;
    } else if (*ip == I_NFC) {
      c_putch('|', devno);
      ++ip;
      void *addr = *((void **)ip);
      ip += icodes_per_ptr();
      const char *sym = get_name(addr);
      c_puts(sym, devno);
      if (!nospaceb((token_t)*ip))
        c_putch(' ', devno);
    } else {          //どれにも当てはまらなかった場合
      err = ERR_SYS;  //エラー番号をセット
      return mark;
    }
    if (ip <= cip)
      mark = sc0.c_x();
  }
  return mark;
}

// Get argument in parenthesis
num_t BASIC_FP Basic::getparam() {
  num_t value;  //値
  if (checkOpen())
    return 0;
  if (getParam(value, I_NONE))
    return 0;
  if (checkClose())
    return 0;
  return value;  //値を持ち帰る
}

// INPUT handler
void SMALL Basic::iinput() {
  int dims = 0;
  int idxs[MAX_ARRAY_DIMS];
  num_t value;
  BString str_value;
  short index;  // Array subscript or variable number
  int32_t filenum = -1;
  uint8_t eoi;  // end-of-input character

  if (*cip == I_SHARP) {
    cip++;
    if (getParam(filenum, 0, MAX_USER_FILES, I_COMMA))
      return;
    if (!user_files[filenum].f) {
      err = ERR_FILE_NOT_OPEN;
      return;
    }
  } else if (is_strexp() && *cip != I_SVAR && *cip != I_STRARR) {
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
  for (;;) {
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
          SYNTAX_T(_("invalid list index"));
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
          c = getc(user_files[filenum].f);
          if (isdigit(c) || c == '.')
            str_value += (char)c;
          else if (isspace(c) || c == eoi)
            break;
          else {
            if (c >= 0)
              ungetc(c, user_files[filenum].f);
            break;
          }
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
          SYNTAX_T(_("invalid list index"));
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
        while ((c = getc(user_files[filenum].f)) >= 0 && c != eoi) {
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

    default:  // 以上のいずれにも該当しなかった場合
      SYNTAX_T(_("expected variable"));
      //return;            // 終了
      goto DONE;
    }  // 中間コードで分岐の末尾

    //値の入力を連続するかどうか判定する処理
    if (end_of_statement())
      goto DONE;
    else {
      switch (*cip) {  // 中間コードで分岐
      case I_COMMA:    // コンマの場合
        cip++;         // 中間コードポインタを次へ進める
        break;         // 打ち切る
      default:         // 以のいずれにも該当しなかった場合
        SYNTAX_T(_("expected separator"));
        //return;           // 終了
        goto DONE;
      }  // 中間コードで分岐の末尾
    }
  }  // 無限に繰り返すの末尾

DONE:
  sc0.show_curs(0);
}

int BASIC_FP token_size(icode_t *code) {
  switch (*code) {
  case I_STR:    return code[1] + 2;
  case I_NUM:    return icodes_per_num() + 1;
  case I_HEXNUM: return 4 / sizeof(icode_t) + 1;
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
  case I_LABEL: return 2;
  case I_EOL:
  case I_REM:
  case I_SQUOT: return -1;
  case I_GOTO:
  case I_GOSUB:
  case I_COMMA:
  case I_RESTORE:
    if (code[1] == I_LABEL)
      return 3;
    else
      return 1;
  case I_NFC: return icodes_per_ptr() + 1;
  default: return 1;
  }
}

void find_next_token(icode_t **start_clp, icode_t **start_cip,
                     token_t tok) {
  icode_t *sclp = *start_clp;
  icode_t *scip = *start_cip;
  int next;

  *start_clp = *start_cip = NULL;

  if (!*sclp)
    return;

  if (!scip)
    scip = sclp + icodes_per_line_desc();

  while (*scip != tok) {
    next = token_size(scip);
    if (next < 0) {
      sclp += *sclp;
      if (!*sclp)
        return;
      scip = sclp + icodes_per_line_desc();
    } else
      scip += next;
  }

  *start_clp = sclp;
  *start_cip = scip;
}

void Basic::initialize_proc_pointers(void) {
  icode_t *lp, *ip;

  lp = listbuf;
  ip = NULL;

  for (int i = 0; i < procs.size(); ++i) {
    procs.proc(i).lp = NULL;
  }

  for (;;) {
    find_next_token(&lp, &ip, I_PROC);
    if (!lp)
      return;

    index_t proc_id = ip[1];
    ip += 2;

    proc_t &pr = procs.proc(proc_id);

    if (pr.lp) {
      err = ERR_DUPPROC;
      clp = lp;
      cip = ip;
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
            clp = lp;
            cip = ip;
            return;
          }
          pr.args_num[pr.argc_num] = *ip++;
          pr.argc_num++;
          break;
        case I_SVAR:
          if (pr.argc_str >= MAX_PROC_ARGS) {
            err = ERR_ASTKOF;
            clp = lp;
            cip = ip;
            return;
          }
          pr.args_str[pr.argc_str] = *ip++;
          pr.argc_str++;
          break;
        default:
          SYNTAX_T(_("expected variable"));
          clp = lp;
          cip = ip;
          return;
        }
      } while (*ip++ == I_COMMA);

      if (ip[-1] != I_CLOSE) {
        if (ip[-1] == I_COMMA)
          err = ERR_UNDEFARG;
        else
          E_SYNTAX(I_CLOSE);
        clp = lp;
        cip = ip;
        return;
      }
    }

    pr.lp = lp;
    pr.ip = ip;
  }
}

void Basic::initialize_label_pointers(void) {
  icode_t *lp, *ip;

  lp = listbuf;
  ip = NULL;

  for (int i = 0; i < labels.size(); ++i) {
    labels.label(i).lp = NULL;
  }

  for (;;) {
    find_next_token(&lp, &ip, I_LABEL);
    if (!lp)
      return;

    index_t label_id = ip[1];
    ip += 2;

    label_t &lb = labels.label(label_id);
    if (lb.lp) {
      err = ERR_DUPLABEL;
      clp = lp;
      cip = ip;
      return;
    }

    lb.lp = lp;
    lb.ip = ip;
  }
}

bool BASIC_INT Basic::find_next_data() {
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
    data_ip = data_lp + icodes_per_line_desc();
  }

  while (*data_ip != I_DATA && (!in_data || *data_ip != I_COMMA)) {
    in_data = false;
    next = token_size(data_ip);
    if (next < 0) {
      data_lp += *data_lp;
      if (!*data_lp)
        return false;
      data_ip = data_lp + icodes_per_line_desc();
    } else
      data_ip += next;
  }
  in_data = true;
  return true;
}

/***bc bas DATA
Specifies values to be read by subsequent `READ` instructions.
\usage DATA expr[, expr...]
\args
@expr	a numeric or string expression
\note
When a DATA instruction is executed, nothing happens.
\ref READ RESTORE
***/
void Basic::idata() {
  int next;

  // Skip over the DATA statement
  while (*cip != I_EOL && *cip != I_COLON) {
    next = token_size(cip);
    if (next < 0) {
      clp += *clp;
      if (!*clp)
        return;
      cip = clp + icodes_per_line_desc();
    } else
      cip += next;
  }
}

static icode_t *data_cip_save;
void BASIC_INT Basic::data_push() {
  data_cip_save = cip;
  cip = data_ip + 1;
}

void BASIC_INT Basic::data_pop() {
  if (err) {
    // XXX: You would actually want to know both locations (READ and DATA).
    clp = data_lp;
    return;
  }
  if (!end_of_statement() && *cip != I_COMMA) {
    clp = data_lp;
    SYNTAX_T(_("malformed DATA"));
    return;
  }
  data_ip = cip;
  cip = data_cip_save;
}

/***bc bas READ
Reads values specified in `DATA` instructions and assigns them to variables.
\usage READ var[, var...]
\args
@var	numeric or string variable
\note
Data is read from the element that follows the current "data pointer".
The data pointer points to the beginning of the program initially. With every
element read it is forwarded to the next one. It can be set directly with
the `RESTORE` command.
\ref DATA RESTORE
***/
void BASIC_INT Basic::iread() {
  num_t value;
  BString svalue;
  index_t index;

  if (!find_next_data()) {
    err = ERR_OOD;
    return;
  }

  auto data_exp = [&]() {
    num_t v = 0;
    data_push();
    if (*cip != I_COMMA && !end_of_statement()) {
      v = iexp();
    }
    data_pop();
    return v;
  };

  auto data_strexp = [&]() {
    BString v;
    data_push();
    if (*cip != I_COMMA && !end_of_statement()) {
      v = istrexp();
    }
    data_pop();
    return v;
  };

  for (;;)
    switch (*cip++) {
    case I_VAR:
      value = data_exp();
      if (err)
        return;
      nvar.var(*cip++) = value;
      break;

    case I_VARARR:
    case I_NUMLST: {
      bool is_list = cip[-1] == I_NUMLST;
      int idxs[MAX_ARRAY_DIMS];
      int dims = 0;

      index = *cip++;
      dims = get_array_dims(idxs);
      if (dims < 0 || (is_list && dims != 1))
        return;

      value = data_exp();
      if (err)
        return;

      num_t &n = is_list ? num_lst.var(index).var(idxs[0]) :
                           num_arr.var(index).var(dims, idxs);
      if (err)
        return;
      n = value;
      break;
    }

    case I_SVAR:
      svalue = data_strexp();
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

      svalue = data_strexp();
      if (err)
        return;

      BString &s = is_list ? str_lst.var(index).var(idxs[0]) :
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
        SYNTAX_T(_("expected variable"));
      return;
    }
}

/***bc bas RESTORE
Sets the data pointer to a given location.
\usage RESTORE [location]
\args
@location	a label or line number [default: start of program]
\ref DATA READ
***/
void Basic::irestore() {
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
    data_ip = data_lp + icodes_per_line_desc();
  }
}

static bool trace_enabled = false;

void Basic::do_trace() {
  putnum(getlineno(clp), 0);
  c_putch(' ');
}
#define TRACE          \
  do {                 \
    if (trace_enabled) \
      do_trace();      \
  } while (0)

void Basic::itron() {
  trace_enabled = true;
}
void Basic::itroff() {
  trace_enabled = false;
}

void Basic::clear_execution_state(bool clear) {
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

  gstki = 0;  // GOSUBスタックインデクスを0に初期化
  lstki = 0;  // FORスタックインデクスを0に初期化
  astk_num_i = 0;
  astk_str_i = 0;
  data_lp = data_ip = NULL;
  in_data = false;
  if (clear)
    inew(NEW_VAR);
}

/***bc bas RUN
Runs the progam in memory.
\desc
If a file name is given, that program will be loaded to memory first.  If a
line number is given, execution will begin at the specified line. Otherwise,
execution will start at the first line.
\usage
RUN [line_num]

RUN file$
\args
@file$		name of the BASIC program to be loaded and run
@line_num	line number at which execution will start [default: first line]
\note
`RUN` behaves slightly differently when executed from within a program than
when used in direct mode.

In direct mode, `RUN` clears all variables, whereas within a program, it
does not.  The purpose of this is to allow a program to restart itself with
all loop and subroutine stacks cleared, but with the variables still intact.
\bugs
* The syntax `RUN file$` does not work from within a program. Use `CHAIN` instead.
\ref CHAIN CLEAR EXEC GOTO LOAD
***/
void Basic::irun_() {
  int32_t lineno;

  if (end_of_statement()) {
    err = 0;
    lineno = 0;
  } else if (getParam(lineno, I_NONE))
    return;

  icode_t *lp = getlp(lineno);
  if (!lp) {
    err = ERR_ULN;
    return;
  }

  clear_execution_state(false);
  clp = lp;
  cip = clp + icodes_per_line_desc();
  err = ERR_CHAIN;
}

// RUN command handler
void BASIC_FP Basic::irun(icode_t *start_clp, bool cont, bool clear,
                          icode_t *start_cip) {
  icode_t *lp;  // 行ポインタの一時的な記憶場所
  if (cont) {
    if (!start_clp) {
      clp = cont_clp;
      cip = cont_cip;
    } else {
      clp = start_clp;
      cip = start_cip ? start_cip : clp + icodes_per_line_desc();
    }
    goto resume;
  }

  clear_execution_state(clear);
  if (err)
    return;

  if (start_clp != NULL) {
    clp = start_clp;
  } else {
    clp = listbuf;  // 行ポインタをプログラム保存領域の先頭に設定
  }

  while (*clp) {  // 行ポインタが末尾を指すまで繰り返す
    TRACE;
    cip = clp + icodes_per_line_desc();  // 中間コードポインタを行番号の後ろに設定

  resume:
    lp = iexe();  // 中間コードを実行して次の行の位置を得る
    if (err) {    // もしエラーを生じたら
      event_error_resume_lp = NULL;
      if (err == ERR_CHAIN) {
        // reset continue pointers, but keep running
        cont_cip = cont_clp = NULL;
      } else if (event_error_enabled) {
        retval[0] = err;
        retval[1] = getlineno(clp);
        retval[2] = -1;
        err = 0;
        err_expected = NULL;  // prevent stale "expected" messages
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
      clp = lp;  // 行ポインタを次の行の位置へ移動
  }
}

num_t BASIC_FP imul();

bool Basic::get_range(uint32_t &start, uint32_t &end) {
  start = 0;
  end = UINT32_MAX;
  if (!end_of_statement()) {
    if (*cip == I_MINUS) {
      // -<num> -> from start to line <num>
      cip++;
      if (getParam(end, I_NONE))
        return false;
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
          if (getParam(end, start, UINT32_MAX, I_NONE))
            return false;
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
void SMALL Basic::ilist(uint8_t devno, BString *search, bool show_lines) {
  uint32_t lineno;     // start line number
  uint32_t endlineno;  // end line number
  uint32_t prnlineno;  // output line number
  icode_t *lp;

  if (!get_range(lineno, endlineno))
    return;

  // Skip until we reach the start line.
  for (lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) {}

  screen_putch_disable_escape_codes = true;
  pixel_t saved_fg_color = sc0.getFgColor();
  pixel_t saved_bg_color = sc0.getBgColor();

  //リストを表示する
  while (*lp) {                 // 行ポインタが末尾を指すまで繰り返す
    prnlineno = getlineno(lp);  // 行番号取得
    if (prnlineno > endlineno)  // 表示終了行番号に達したら抜ける
      break;
    if (search) {
      char *l = getLineStr(prnlineno);
      if (!strstr(l, search->c_str())) {
        lp += *lp;
        continue;
      }
    }

    if (show_lines) {
      sc0.setColor(COL(LINENUM), COL(BG));
      putnum(prnlineno, 0, devno);  // 行番号を表示
      sc0.setColor(COL(FG), COL(BG));
      c_putch(' ', devno); // 空白を入れる
    }

    putlist(lp, devno);  // 行番号より後ろを文字列に変換して表示
    if (err)             // もしエラーが生じたら
      break;             // 繰り返しを打ち切る
    newline(devno);      // 改行
    lp += *lp;           // 行ポインタを次の行へ進める
  }

  sc0.setColor(saved_fg_color, saved_bg_color);
  screen_putch_disable_escape_codes = false;
}

void Basic::isearch() {
  BString needle = istrexp();
  if (!err)
    ilist(0, &needle);
}

// Argument 0: all erase, 1: erase only program, 2: erase variable area only
void Basic::inew(uint8_t mode) {
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
  if (mode == NEW_ALL || mode == NEW_VAR) {
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

    gstki = 0;  //GOSUBスタックインデクスを0に初期化
    lstki = 0;  //FORスタックインデクスを0に初期化
    astk_num_i = 0;
    astk_str_i = 0;

    bool direct_mode = !(cip >= listbuf && cip < listbuf + size_list);

    if (listbuf)
      free(listbuf);
    // XXX: Should we be more generous here to avoid fragmentation?
    listbuf = (icode_t *)calloc(1, LISTBUF_INC * sizeof(icode_t));
    if (!listbuf) {
      err = ERR_OOM;
      size_list = 0;
      // If this fails, we're in deep shit...
      return;
    }
    size_list = LISTBUF_INC;
    clp = listbuf;  //行ポインタをプログラム保存領域の先頭に設定
    if (!direct_mode) {
      cip = clp + icodes_per_line_desc();
      *cip = I_EOL;
    }
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
void SMALL Basic::irenum() {
  int32_t startLineNo = 10;  // 開始行番号
  int32_t increase = 10;     // 増分
  icode_t *ptr;              // プログラム領域参照ポインタ
  uint32_t len;              // 行長さ
  uint32_t i;                // 中間コード参照位置
  num_t newnum;              // 新しい行番号
  uint32_t num;              // 現在の行番号
  uint32_t index;            // 行インデックス
  uint32_t cnt;              // プログラム行数
  int toksize;

  if (!end_of_statement()) {
    startLineNo = iexp();
    if (*cip == I_COMMA) {
      cip++;
      increase = iexp();
    }
  }

  // 引数の有効性チェック
  cnt = countLines() - 1;
  if (startLineNo < 0 || increase <= 0) {
    err = ERR_RANGE;
    return;
  }
  if (startLineNo + increase * cnt > INT32_MAX) {
    err = ERR_RANGE;
    return;
  }

  // ブログラム中のGOTOの飛び先行番号を付け直す
  for (clp = listbuf; *clp; clp += *clp) {
    ptr = clp;
    len = *ptr;
    ptr += icodes_per_line_desc();
    i = 0;
    // 行内検索
    while (i < len - 1) {
      switch (ptr[i]) {
      case I_GOTO:
      case I_GOSUB:
      case I_THEN:
      case I_RESTORE:
        i++;
        if (ptr[i] == I_NUM) {  // XXX: I_HEXNUM? :)
          num = UNALIGNED_NUM_T(&ptr[i + 1]);
          index = getlineIndex(num);
          if (index == INT32_MAX) {
            // 該当する行が見つからないため、変更は行わない
            i += icodes_per_num() + 1;
            continue;
          } else {
            // とび先行番号を付け替える
            newnum = startLineNo + increase * index;
            UNALIGNED_NUM_T(&ptr[i + 1]) = newnum;
            i += icodes_per_num() + 1;
            continue;
          }
        } else if (ptr[i] == I_LABEL) {
          ++i;
        }
        break;
      default:
        toksize = token_size(ptr + i);
        if (toksize < 0)
          i = len + 1;  // skip rest of line
        else
          i += toksize;  // next token
        break;
      }
    }
  }

  // 各行の行番号の付け替え
  index = 0;
  for (clp = listbuf; *clp; clp += *clp) {
    newnum = startLineNo + increase * index;
    line_desc_t *ld = (line_desc_t *)clp;
    ld->line = newnum;
    index++;
  }
}

/***bc bas SAVE
Saves the BASIC program in memory to a file.
\usage SAVE file$ [LINE <ON|OFF>]
\args
@file$	name of file to be saved
\note
* Programs are saved without line numbers if `LINE OFF` is specified.
  Otherwise, line numbers are included.
* BASIC programs are saved in plain text (UTF-8) format.
\ref SAVE_BG SAVE_CONFIG SAVE_IMAGE
***/
void Basic::isave() {
  BString fname;
  int8_t rc;
  bool show_lines = true;

  if (*cip == I_BG) {
    isavebg();
    return;
  } else if (*cip == I_PCX || *cip == I_IMAGE) {
    ++cip;
    isavepcx();
    return;
  } else if (*cip == I_CONFIG) {
    ++cip;
    isaveconfig();
    return;
  }

  if (!(fname = getParamFname())) {
    return;
  }

  if (*cip == I_LINE) {
    ++cip;
    if (*cip == I_OFF)
      show_lines = false;
    else if (*cip == I_ON)
      show_lines = true;
    else {
      SYNTAX_T(_("expected OFF or ON"));
      return;
    }
    ++cip;
  }

  // SDカードへの保存
  rc = bfs.tmpOpen((char *)fname.c_str(), 1);
  if (rc == SD_ERR_INIT) {
    err = ERR_SD_NOT_READY;
    return;
  } else if (rc == SD_ERR_OPEN_FILE) {
    err = ERR_FILE_OPEN;
    return;
  }
  ilist(4, NULL, show_lines);
  bfs.tmpClose();
}

#include "codepages.h"

void Basic::transcodeLineToUTF8(char *lbuf, encoding_t enc) {
  const utf8_int32_t *table;

  switch (enc) {
  case ENC_CP437: table = cp437_to_utf8; break;
  case ENC_PETASCII: table = petascii_to_utf8; break;
  case ENC_AMSTRAD: table = amstradcpc_to_utf8; break;
  default:
    err = ERR_NOT_SUPPORTED;
    err_expected = "conversion not implemented";
    return;
  }

  uint8_t *in_buf = (uint8_t *)strdup(lbuf);
  const uint8_t *in = in_buf;

  void *out = (void *)lbuf;
  memset(lbuf, 0, SIZE_LINE);

  while (*in && out) {
    uint8_t c = 0;

    if (*in == '\\' && in[1] == 'x') {
      in += 2;
      while (isHexadecimalDigit(*in)) {
        c = c << 4 | hex2value(*in++);
      }
    } else
      c = *in++;

    out = utf8catcodepoint(out, table[c], SIZE_LINE - (in - in_buf));
  }

  free(in_buf);
}

// テキスト形式のプログラムのロード
// 引数
//   fname  :  ファイル名
//   newmode:  初期化モード 0:初期化する 1:変数を初期化しない 2:追記モード
// 戻り値
//   0:正常終了
//   1:異常終了
uint8_t SMALL Basic::loadPrgText(char *fname, uint8_t newmode, encoding_t enc) {
  int32_t len;
  uint8_t rc = 0;
  uint32_t last_line = 0;

  cont_clp = cont_cip = NULL;
  procs.reset();
  labels.reset();

  err = bfs.tmpOpen(fname, 0);
  if (err)
    return 1;

  if (newmode != NEW_VAR)
    inew(newmode);
  while (bfs.readLine(lbuf)) {
    if (enc != ENC_UTF8)
      transcodeLineToUTF8(lbuf, enc);

    char *sbuf = lbuf;
    while (isspace(*sbuf))
      sbuf++;

    if (!isDigit(*sbuf)) {
      // Insert a line number before tokenizing.
      if (strlen(lbuf) > SIZE_LINE - 12) {
        err = ERR_LONG;
        error(true);
        rc = 1;
        break;
      }
      memmove(lbuf + 11, lbuf, strlen(lbuf) + 1);
      memset(lbuf, ' ', 11);
      last_line += 1;
      int lnum_size = sprintf(lbuf, "%u", (unsigned int)last_line);
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
      SYNTAX_T(_("invalid program line"));
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

WARNING: Do not confuse with `REMOVE`, which deletes files.
\usage DELETE range
\args
@range	a range of BASIC program lines
\note
* Using `DELETE` does not affect variables.
* When called from a running program, execution will continue at the next
  program line, i.e. instructions following `DELETE` on the same line are
  disregarded.
\ref REMOVE
***/
void SMALL Basic::idelete() {
  uint32_t sNo, eNo;
  icode_t *lp;       // 削除位置ポインタ
  icode_t *p1, *p2;  // 移動先と移動元ポインタ
  int32_t len;       // 移動の長さ

  cont_clp = cont_cip = NULL;
  procs.reset();
  labels.reset();

  uint32_t current_line = getlineno(clp);

  if (!get_range(sNo, eNo))
    return;
  if (!end_of_statement()) {
    SYNTAX_T(_("expected end of statement"));
    return;
  }

  if (eNo < sNo) {
    err = ERR_RANGE;
    return;
  }

  if (eNo == sNo) {
    lp = getlp(eNo);  // 削除位置ポインタを取得
    if (getlineno(lp) == sNo) {
      // 削除
      p1 = lp;                    // p1を挿入位置に設定
      p2 = p1 + *p1;              // p2を次の行に設定
      while ((len = *p2) != 0) {  // 次の行の長さが0でなければ繰り返す
        while (len--)             // 次の行の長さだけ繰り返す
          *p1++ = *p2++;          // 前へ詰める
      }
      *p1 = 0;  // リストの末尾に0を置く
    }
  } else {
    for (uint32_t i = sNo; i <= eNo; i++) {
      lp = getlp(i);  // 削除位置ポインタを取得
      if (!*lp)
        break;
      if (getlineno(lp) == i) {     // もし行番号が一致したら
        p1 = lp;                    // p1を挿入位置に設定
        p2 = p1 + *p1;              // p2を次の行に設定
        while ((len = *p2) != 0) {  // 次の行の長さが0でなければ繰り返す
          while (len--)             // 次の行の長さだけ繰り返す
            *p1++ = *p2++;          // 前へ詰める
        }
        *p1 = 0;  // リストの末尾に0を置く
      }
    }
  }

  initialize_proc_pointers();
  initialize_label_pointers();
  // continue on the next line, in the likely case the DELETE command didn't
  // delete itself
  clp = getlp(current_line + 1);
  cip = clp + icodes_per_line_desc();
  TRACE;
}

/***bc scr CLS
Clears the current text window.
\usage CLS
\ref WINDOW
***/
void Basic::icls() {
  if (redirect_output_file < 0) {
    sc0.cls();
    sc0.locate(0, 0);
  }
}

static bool profile_enabled;

void BASIC_FP Basic::init_stack_frame() {
  if (gstki > 0 && gstk[gstki - 1].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki - 1].proc_idx);
    astk_num_i += p.locc_num;
    astk_str_i += p.locc_str;
  }
  gstk[gstki].num_args = 0;
  gstk[gstki].str_args = 0;
}

void BASIC_FP Basic::push_num_arg(num_t n) {
  if (astk_num_i >= SIZE_ASTK) {
    err = ERR_ASTKOF;
    return;
  }
  astk_num[astk_num_i++] = n;
  gstk[gstki].num_args++;
}

void BASIC_FP Basic::do_call(index_t proc_idx) {
  struct proc_t &proc_loc = procs.proc(proc_idx);

  if (!proc_loc.lp || !proc_loc.ip) {
    err = ERR_UNDEFPROC;
    return;
  }

  if (gstki >= SIZE_GSTK) {  // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;        // エラー番号をセット
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

#define EVENT_PROFILE_SAMPLES 7
uint32_t event_profile[EVENT_PROFILE_SAMPLES];

void BASIC_INT Basic::draw_profile(void) {
  int x = 0;
  int bw = vs23.borderWidth();
  int scale = 1000000 / 60 / bw + 1;

  for (int i = 1; i < EVENT_PROFILE_SAMPLES; ++i) {
    int pixels = (event_profile[i] - event_profile[i - 1]) / scale;
    if (x + pixels > bw)
      pixels = bw - x;
    if (pixels > 0)
      vs23.setBorder(0x70, (i * 0x4c) % 256, x, pixels);
    x += pixels;
  }

  if (x < bw)
    vs23.setBorder(0x20, 0, x, bw - x);
}

void BASIC_FP process_events(void) {
  static uint32_t last_frame;
#ifdef SDL
  static bool extra_poll = false;
  // Polling input once after each frame introduces considerable input lag.
  // Event handling on SDL is relatively expensive, however, so we can only
  // afford a single extra poll. Fortunately, that seems to be good enough
  // to prevent excessive lag.
  if (!extra_poll) {
    extra_poll = true;
    platform_process_events();
  }
#endif

  if (vs23.frame() == last_frame) {
#if defined(HAVE_TSF) && !defined(HOSTED) && !defined(SDL)
    // Wasn't able to get this to work without underruns in SDL-based builds.
    // Doing the rendering in the SDL callback instead, which is probably
    // the right thing to do in the first place.
    if (sound.needSamples())
      sound.render();
#endif
    return;
  }

#ifdef SDL
  extra_poll = false;
#endif

  last_frame = vs23.frame();

  event_profile[0] = micros();

  // Do this before the updateBg() call to make sure it is included in
  // the next screen update.
  sc0.updateCursor();

  event_profile[1] = micros();

#ifdef USE_BG_ENGINE
  vs23.updateBg();
#endif

  event_profile[2] = micros();

#if defined(HAVE_TSF) && !defined(HOSTED) && !defined(SDL)
  if (sound.needSamples())
    sound.render();
#endif
  sound.pumpEvents();
  event_profile[3] = micros();
#ifdef HAVE_MML
  if (event_play_enabled) {
    for (int i = 0; i < MML_CHANNELS; ++i) {
      if (bc && !bc->basic_events_disabled && sound.isFinished(i))
        bc->event_handle_play(i);
    }
  }
#endif

  event_profile[4] = micros();

#ifdef USE_BG_ENGINE
  if (bc && !bc->basic_events_disabled && event_sprite_proc_idx != NO_PROC)
    bc->event_handle_sprite();
#endif
  event_profile[5] = micros();
  if (bc && !bc->basic_events_disabled && event_pad_enabled)
    bc->event_handle_pad();
  event_profile[6] = micros();

  if (bc && profile_enabled)
    bc->draw_profile();

#if defined(HOSTED) || defined(H3) || defined(__DJGPP__) || defined(SDL)
  platform_process_events();
#endif
}

/***bf bas RET
Returns one of the numeric return values of the last function call.
\usage rval = RET(num)
\args
@num	number of the numeric return value [`0` to `{MAX_RETVALS_m1}`]
\ret Requested return value.
\ref RETURN RET$()
***/
num_t BASIC_FP Basic::nret() {
  int32_t r;

  if (checkOpen())
    return 0;
  if (getParam(r, 0, MAX_RETVALS - 1, I_NONE))
    return 0;
  if (checkClose())
    return 0;

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
num_t BASIC_FP Basic::narg() {
  int32_t a;
  if (astk_num_i == 0) {
    err = ERR_UNDEFARG;
    return 0;
  }
  uint16_t argc = gstk[gstki - 1].num_args;

  if (checkOpen())
    return 0;
  if (getParam(a, 0, argc - 1, I_NONE))
    return 0;
  if (checkClose())
    return 0;

  return astk_num[astk_num_i - argc + a];
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
BString Basic::sarg() {
  int32_t a;
  if (astk_str_i == 0) {
    err = ERR_UNDEFARG;
    return BString();
  }
  uint16_t argc = gstk[gstki - 1].str_args;

  if (checkOpen())
    return BString();
  if (getParam(a, 0, argc - 1, I_NONE))
    return BString();
  if (checkClose())
    return BString();

  return BString(astk_str[astk_str_i - argc + a]);
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
num_t BASIC_FP Basic::nargc() {
  int32_t type = getparam();
  if (!gstki)
    return 0;
  if (type == 0)
    return gstk[gstki - 1].num_args;
  else
    return gstk[gstki - 1].str_args;
}

/***bf bas HEX$
Returns a string containing the hexadecimal representation of a number.
\usage h$ = HEX$(num)
\args
@num	numeric expression
\ret Hexadecimal number as text.
\ref BIN$()
***/
BString Basic::shex() {
  uint32_t value;  // 値
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
\note
Numbers are converted to an unsigned 32-bit value first.
\ref HEX$()
***/
BString Basic::sbin() {
  uint32_t value;  // 値

  if (checkOpen())
    goto out;
  if (getParam(value, I_NONE))
    goto out;
  if (checkClose())
    goto out;
  return BString(value, 2);
out:
  return BString();
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
num_t BASIC_FP SMALL Basic::nmap() {
  int32_t value, l1, h1, l2, h2, rc;
  if (checkOpen())
    return 0;
  if (getParam(value, I_COMMA) || getParam(l1, I_COMMA) ||
      getParam(h1, I_COMMA) || getParam(l2, I_COMMA) || getParam(h2, I_NONE))
    return 0;
  if (checkClose())
    return 0;
  if (l1 >= h1 || l2 >= h2) {
    err = ERR_RANGE;
    return 0;
  } else if (value < l1 || value > h1) {
    E_VALUE(l1, h1);
  }
  rc = (value - l1) * (h2 - l2) / (h1 - l1) + l2;
  return rc;
}

/***bf bas ASC
Returns the Unicode codepoint for the first character in a string expression.
\usage val = ASC(s$)
\args
@s$	string expression
\ret Unicode codepoint of the first character.
\error
Generates an error if `s$` is empty.
\note
* Unlike its counterparts in single-byte BASIC implementations, this
  function may return values larger than 255.
* To get access to individual bytes without UTF-8 decoding, use the
  `a$[num]` syntax.
\ref CHR$()
***/
num_t BASIC_INT Basic::nasc() {
  int32_t value;

  if (checkOpen())
    return 0;
  BString a = istrexp();
  if (a.lengthMB() < 1) {
    E_ERR(VALUE, _("empty string"));
    return 0;
  }
  value = a.codepointAt(0);
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

You have to separate expressions by either a semicolon (`;`) or a comma
(`,`). When you use a semicolon, the expressions are printed without
separation. When you use a comma, spaces will be inserted until the next
tabulator stop is reached.
\bugs
`TAB()` does not work when output is redirected to a file using
`CMD`.
\ref CMD GPRINT
***/
void Basic::iprint(uint8_t devno, uint8_t nonewln) {
  num_t value;  //値
  int32_t filenum;
  BString str;
  BString format = F("%0.9g");  // default format without USING

  while (!end_of_statement()) {
    if (is_strexp()) {
      str = istrexp();
      c_puts(str.c_str(), devno);
    } else
      switch (*cip) {  //中間コードで分岐
      case I_USING: {  //「#
        cip++;
        str = istrexp();
        bool had_point = false;  // encountered a decimal point
        bool had_digit = false;  // encountered a #
        int leading = 0;
        int trailing = 0;        // decimal places
        BString prefix, suffix;  // random literal characters
        for (unsigned int i = 0; i < str.lengthMB(); ++i) {
          switch (str.codepointAt(i)) {
          case '#':
            if (!had_point)
              leading++;
            else
              trailing++;
            had_digit = true;
            break;
#ifdef FLOAT_NUMS
          case '.':
            if (had_point) {
              E_ERR(USING, _("multiple periods"));
              return;
            } else
              had_point = true;
            break;
#endif
          case '%':
            if (!had_point && !had_digit)
              prefix += F("%%");
            else
              suffix += F("%%");
            break;
          default:
            if (!had_point && !had_digit)
              prefix += str.substringMB(i, i + 1);
            else
              suffix += str.substringMB(i, i + 1);
            break;
          }
        }
#ifdef FLOAT_NUMS
        format = prefix + '%' + (trailing + leading + (trailing ? 1 : 0)) +
                 '.' + trailing + 'f' + suffix;
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
        if (!user_files[filenum].f) {
          err = ERR_FILE_NOT_OPEN;
          return;
        }
        bfs.setTempFile(user_files[filenum].f);
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

      case I_COMMA: break;

      default:  // anything else is assumed to be a numeric expression
        value = iexp();
        if (err) {
          newline();
          return;
        }
        sprintf(lbuf, format.c_str(), value);
        c_puts(lbuf, devno);
        break;
      }

    if (err) {
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
        SYNTAX_T(_("expected separator"));
        newline(devno);
        return;
      }
    }
  }
  if (!nonewln)
    newline(devno);
}

// ファイル名引数の取得
BString Basic::getParamFname() {
  BString fname = istrexp();
  if (fname.length() >= SD_PATH_LEN)
    err = ERR_LONGPATH;
  return fname;
}

/***bc scr SAVE IMAGE
Saves a portion of pixel memory as an image file.
\usage SAVE IMAGE image$ [POS x, y] [SIZE w, h]
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
\ref LOAD_IMAGE
***/
void SMALL Basic::isavepcx() {
  BString fname;
  int32_t x = 0, y = 0;
  int32_t w = sc0.getGWidth();
  int32_t h = sc0.getGHeight();

  if (!(fname = getParamFname())) {
    return;
  }

  for (;;) {
    if (*cip == I_POS) {
      cip++;
      if (getParam(x, 0, sc0.getGWidth() - 1, I_COMMA))
        return;
      if (getParam(y, 0, vs23.lastLine() - 1, I_NONE))
        return;
    } else if (*cip == I_SIZE) {
      cip++;
      if (getParam(w, 0, sc0.getGWidth() - x - 1, I_COMMA))
        return;
      if (getParam(h, 0, vs23.lastLine() - y - 1, I_NONE))
        return;
    } else
      break;
  }

  err = bfs.saveBitmap((char *)fname.c_str(), x, y, w, h);
  return;
}

/***bc scr LOAD IMAGE
Loads an image file in whole or in parts to pixel memory.
\usage
LOAD IMAGE image$ [AS <BG bg|SPRITE *range*>] [TO dest_x, dest_y] [OFF x, y]
           [SIZE width, height] [KEY col] [SCALE scale_x, scale_y]
\args
@bg		background number [`0` to `{MAX_BG_m1}`]
@range		sprite range [limits between `0` and `{MAX_SPRITES_m1}`]
@dest_x		X-coordinate of destination, pixels +
                [`0` (default) to `PSIZE(0)-w`]
@dest_y		Y-coordinate of destination, pixels +
                [`0` (default) to `PSIZE(2)-h`]
@x		offset within image file, X axis, pixels [default: `0`]
@y		offset within image file, Y axis, pixels [default: `0`]
@width		width of image portion to be loaded, pixels [default: image width]
@height		height of image portion to be loaded, pixels [default: image height]
@col		color key for transparency [default: no transparency]
@scale_x	scaling factor [default: `1`]
@scale_y	scaling factor [default: `1`]
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

If a color key is specified, pixels of the given color will not be drawn.

If the scaling factor for a dimension is `-1`, it will be resized to the
screen size. If it is `-2`, it will be resized so that the aspect ratio is
preserved.

If the scaling factor for both dimensions is `-2`, the image will be fit to
the screen while preserving the aspect ratio.

IMPORTANT: Resizing is not supported for images in PCX format.
\ref BG SAVE_IMAGE SPRITE
***/
void SMALL Basic::ildbmp() {
  BString fname;
  int32_t dx = -1, dy = -1;
  int32_t x = 0, y = 0, w = -1, h = -1;
#ifdef USE_BG_ENGINE
  uint32_t spr_from, spr_to;
  bool define_bg = false, define_spr = false;
  int32_t bg = 0;
#endif
  uint32_t key = 0;  // no keying
  double scale_x = 1, scale_y = 1;

  if (!(fname = getParamFname())) {
    return;
  }

  for (;;) {
    if (*cip == I_AS) {
#ifdef USE_BG_ENGINE
      cip++;
      if (*cip == I_BG) {  // AS BG ...
        ++cip;
        dx = dy = -1;
        if (getParam(bg, I_NONE))
          return;
        define_bg = true;
      } else if (*cip == I_SPRITE) {  // AS SPRITE ...
        ++cip;
        dx = dy = -1;
        define_spr = true;
        if (!get_range(spr_from, spr_to))
          return;
        if (spr_to == UINT32_MAX)
          spr_to = MAX_SPRITES - 1;
        if (spr_to > MAX_SPRITES - 1 || spr_from > spr_to) {
          err = ERR_RANGE;
          return;
        }
      } else {
        SYNTAX_T(_("expected BG or SPRITE"));
        return;
      }
#else
      err = ERR_NOT_SUPPORTED;
      return;
#endif
    } else if (*cip == I_TO) {
      // TO dx,dy
      cip++;
      if (getParam(dx, I_COMMA))
        return;
      if (getParam(dy, I_NONE))
        return;
    } else if (*cip == I_OFF) {
      // OFF x,y
      cip++;
      if (getParam(x, 0, INT32_MAX, I_COMMA))
        return;
      if (getParam(y, 0, INT32_MAX, I_NONE))
        return;
    } else if (*cip == I_SIZE) {
      // SIZE w,h
      cip++;
      if (getParam(w, 0, INT32_MAX, I_COMMA))
        return;
      if (getParam(h, 0, INT32_MAX, I_NONE))
        return;
    } else if (*cip == I_KEY) {
      // KEY c
      cip++;
      if (getParam(key, 0, UINT32_MAX, I_NONE))
        return;
    } else if (*cip == I_SCALE) {
      // SCALE x,y
      cip++;
      if (getParam(scale_x, I_COMMA) ||
          getParam(scale_y, I_NONE))
        return;
    } else {
      break;
    }
  }

  // 画像のロード
  struct eb_image_spec loc = {
    .dst_x = dx,
    .dst_y = dy,
    .src_x = x,
    .src_y = y,
    .w = w,
    .h = h,
    .scale_x = scale_x,
    .scale_y = scale_y,
    .key = key,
  };

  err = eb_load_image(fname.c_str(), &loc);
  if (!err) {
#ifdef USE_BG_ENGINE
    if (define_bg)
      eb_bg_set_pattern(bg, loc.dst_x, loc.dst_y, loc.w / vs23.bgTileSizeX(bg));
    if (define_spr) {
      for (uint32_t i = spr_from; i < spr_to + 1; ++i) {
        eb_sprite_set_pattern(i, loc.dst_x, loc.dst_y);
      }
    }
#endif
    retval[0] = loc.dst_x;
    retval[1] = loc.dst_y;
    retval[2] = loc.w;
    retval[3] = loc.h;
  }
}

#include <list>
#include "basic_native.h"

/***bc sys EDIT
Runs the ASCII text editor.
\usage EDIT [file$]
\args
@file$	name of file to be edited [default: new file]
***/
void Basic::iedit() {
  BString fn;
  std::list<BString> args;

  if (getenv("EDITOR"))
    args.push_back(getenv("EDITOR"));
  else
    args.push_back("atto");

  if (is_strexp() && (fn = getParamFname()))
    args.push_back(fn);
  if (err)
    return;

  sc0.show_curs(1);
  shell_list(args);
  sc0.show_curs(0);
}

/***bc bas LOAD
Load a program from a file.
\usage LOAD file$ [FONT n]
\args
@file$	name of the BASIC program
@font	font (encoding) used for UTF-8 conversion [`0` to `{NUM_FONTS_m1}`]
\note
If you specify the `FONT` parameter, `LOAD` will convert the program from
the single-byte encoding for that font to UTF-8. Use this to load programs
written for the BASIC Engine ESP8266 platform on next-generation systems.
\ref LOAD_BG LOAD_IMAGE SAVE
***/
/***bc bas MERGE
Merge a program in a file with the program currently in memory.
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
uint8_t SMALL Basic::ilrun() {
  uint32_t lineno = (uint32_t)-1;
  icode_t *lp;
  int8_t fg;  // File format 0: Binary format 1: Text format
  bool islrun = true;
  bool ismerge = false;
  uint8_t newmode = NEW_PROG;
  BString fname;

  // Command identification
  if (*(cip - 1) == I_LOAD || *(cip - 1) == I_RUN) {
    islrun = false;
    lineno = 0;
    newmode = NEW_ALL;
  } else if (cip[-1] == I_MERGE) {
    islrun = false;
    ismerge = true;
    newmode = NEW_VAR;
  }

  // Get file name
  if (is_strexp()) {
    if (!(fname = getParamFname())) {
      return 0;
    }
  } else {
    SYNTAX_T(_("expected file name"));
    return 0;
  }

  if (islrun || ismerge) {
    // LRUN or MERGE
    // Obtain the second argument line number
    if (*cip == I_COMMA) {
      islrun = true;  // MERGE + line number => run!
      cip++;
      if (getParam(lineno, I_NONE))
        return 0;
    } else {
      lineno = 0;
    }
  }

  // Load program from storage
  fg = bfs.IsText((char *)fname.c_str());  // Format check
  if (fg < 0) {
    // Abnormal form (形式異常)
    err = -fg;
  } else if (fg == 0) {
    // Binary format load from SD card
    err = ERR_NOT_SUPPORTED;
  } else if (fg == 1) {
    // Text format load from SD card
    int32_t encoding = ENC_UTF8;
    if (*cip == I_FONT) {
      ++cip;
      if (getParam(encoding, 0, NUM_FONTS, I_NONE))
        return 0;
      ++encoding;
    }
    loadPrgText((char *)fname.c_str(), newmode, (encoding_t)encoding);
  }
  if (err)
    return 0;

  // Processing of line number
  if (lineno == 0) {
    clp = listbuf;  // Set the line pointer to start of program buffer
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
    if (islrun || (cip >= listbuf && cip < listbuf + size_list)) {
      initialize_proc_pointers();
      initialize_label_pointers();
      cip = clp + icodes_per_line_desc();
      if (islrun)
        err = ERR_CHAIN;
    }
  }
  return 1;
}

// output error message
// Arguments:
// flgCmd: set to false at program execution, true at command line
void SMALL Basic::error(uint8_t flgCmd) {
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
      c_puts_P(_(errmsg[err]));
      sc0.setColor(COL(FG), COL(BG));
      PRINT_P(_(" in "));
      putnum(getlineno(clp), 0);  // 行番号を調べて表示
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
    } else {                  // 指示の実行中なら
      sc0.setColor(COL(PROC), COL(BG));
      c_puts_P(_(errmsg[err]));  // エラーメッセージを表示
      sc0.setColor(COL(FG), COL(BG));
      if (err_expected) {
        PRINT_P(" (");
        c_puts_P(err_expected);
        PRINT_P(")");
      }
      newline();  // 改行
      //err = 0;               // エラー番号をクリア
      //return;
    }
  }
  c_puts_P(_(errmsg[0]));  //「OK」を表示
  newline();            // 改行
  err = 0;              // エラー番号をクリア
  err_expected = NULL;
}

/***bf bas ERROR$
Returns the error message associated with a given error number.
\usage msg$ = ERROR$(num)
\args
@num	error number [`0` to `{max_err}`]
\ret Error message.
\ref ON_ERROR RET()
***/
BString Basic::serror() {
  uint32_t code = getparam();
  if (code >= sizeof(errmsg) / sizeof(*errmsg)) {
    E_VALUE(0, sizeof(errmsg) / sizeof(*errmsg) - 1);
    return BString(F(""));
  } else
    return BString(_(errmsg[code]));
}

bool BASIC_FP Basic::is_strexp() {
  // XXX: does not detect string comparisons (numeric)
  return (strfuntbl[*cip] != NULL ||
          *cip == I_STR ||
          ((*cip == I_SVAR || *cip == I_LSVAR) && cip[2] != I_SQOPEN) ||
          *cip == I_STRARR ||
          *cip == I_STRLST ||
          *cip == I_STRSTR ||
          *cip == I_INPUTSTR ||
          (*cip == I_NET && (cip[1] == I_INPUTSTR || cip[1] == I_GETSTR)) ||
          *cip == I_ERRORSTR);
}

BString Basic::iextstrvalue() {
  BString value;

  std::vector<eb_param_t> params;
  token_t sf = (token_t)cip[-1];

  parse_params(sf, eb_strfun_syntax(sf), params);

  if (!err) {
    basic_events_disabled = true;
    value = handle_strfun(sf, &params[0]);
    basic_events_disabled = false;
  }

  delete_params(params);

  return value;
}

BString BASIC_INT Basic::istrvalue() {
  BString value;
  int len, dims;
  index_t i;
  int idxs[MAX_ARRAY_DIMS];

  if (strfuntbl[*cip] != NULL) {
    return (this->*strfuntbl[*cip++])();
  } else
    switch (*cip++) {
    case I_STR:
      len = value.fromBasic((TOKEN_TYPE *)cip);
      cip += len;
      if (!len)
        err = ERR_OOM;
      break;

    case I_SVAR:  value = svar.var(*cip++); break;
    case I_LSVAR: value = get_lsvar(*cip++); break;

    case I_STRARR:
      i = *cip++;
      dims = get_array_dims(idxs);
      value = str_arr.var(i).var(dims, idxs);
      break;

    case I_STRLST:
      i = *cip++;
      dims = get_array_dims(idxs);
      if (dims != 1) {
        SYNTAX_T(_("invalid list index"));
      } else {
        value = str_lst.var(i).var(idxs[0]);
      }
      break;

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
        SYNTAX_T(_("expected network function"));
#endif
      break;

    case I_NFC: value = snfc(); break;

    default:
      cip--;
      // Check if a numeric expression follows, so we can give a more
      // helpful error message.
      err = 0;
      iexp();
      if (!err)
        err = ERR_TYPE;
      else
        SYNTAX_T(_("expected string expression"));
      break;
    }
  if (err)
    return BString();
  else
    return value;
}

BString BASIC_INT Basic::istrexp() {
  BString value, tmp;

  value = istrvalue();

  for (;;)
    switch (*cip) {
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
    default: return value;
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
num_t BASIC_INT Basic::nlen() {
  int32_t value;
  if (checkOpen())
    return 0;
  if (*cip == I_STRLSTREF) {
    ++cip;
    value = str_lst.var(*cip++).size();
  } else if (*cip == I_NUMLSTREF) {
    ++cip;
    value = num_lst.var(*cip++).size();
  } else {
    value = istrexp().lengthMB();
  }
  checkClose();
  return value;
}

/***bf bas BLEN
Returns the number of bytes in a byte string.
\usage
sl = BLEN(string$)
\args
@string$	any string expression
\ret Length in bytes.
***/
num_t BASIC_INT Basic::nblen() {
  int32_t value;
  if (checkOpen())
    return 0;
  value = istrexp().length();
  checkClose();
  return value;
}

// see getrnd() for basicdoc
num_t BASIC_FP Basic::nrnd() {
  num_t value = getparam();  //括弧の値を取得
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
num_t BASIC_FP Basic::nabs() {
  num_t value = getparam();  //括弧の値を取得
  if (value == INT32_MIN) {
    err = ERR_VOF;
    return 0;
  }
  if (value < 0)
    value *= -1;  //正負を反転
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
num_t BASIC_INT Basic::nval() {
  if (checkOpen())
    return 0;
  num_t value = strtonum(istrexp().c_str(), NULL);
  checkClose();
  return value;
}

num_t BASIC_INT Basic::nsvar_a(BString &value) {
  int32_t a;
  // String character accessor
  if (*cip++ != I_SQOPEN) {
    // XXX: Can we actually get here?
    E_SYNTAX(I_SQOPEN);
    return 0;
  }
  if (getParam(a, 0, value.length(), I_SQCLOSE))
    return 0;
  return (uint8_t)value[a];
}

num_t Basic::iextvalue() {
  num_t value = 0;

  std::vector<eb_param_t> params;
  token_t nf = (token_t)cip[-1];

  parse_params(nf, eb_numfun_syntax(nf), params);

  if (!err) {
    basic_events_disabled = true;
    value = handle_numfun(nf, &params[0]);
    basic_events_disabled = false;
  }

  delete_params(params);

  return value;
}

// Get value
num_t BASIC_FP Basic::ivalue() {
  num_t value = 0;  // 値
  index_t i;        // 文字数
  int dims;
  static int idxs[MAX_ARRAY_DIMS];

  if (numfuntbl[*cip] != NULL) {
    value = (this->*numfuntbl[*cip++])();
  } else if (is_strexp()) {
    // string comparison (or error)
    value = irel_string();
  } else
    switch (*cip++) {
    //定数の取得
    case I_NUM:  // 定数
      value = UNALIGNED_NUM_T(cip);
      cip += icodes_per_num();
      break;

    case I_HEXNUM:  // 16進定数
      // get the constant
#if TOKEN_TYPE == uint32_t
      value = cip[0];
      ++cip;
#else
      value = (uint32_t)cip[0] | ((uint32_t)cip[1] << 8) | ((uint32_t)cip[2] << 16) | ((uint32_t)cip[3] << 24);
      cip += 4;
#endif
      break;

    //変数の値の取得
    case I_VAR:  //変数
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
      value = nsvar_a(svar.var(*cip++));
      break;

    case I_LSVAR:
      value = nsvar_a(get_lsvar(*cip++));
      break;

    case I_STRARR:
      i = *cip++;
      dims = get_array_dims(idxs);
      value = nsvar_a(str_arr.var(i).var(dims, idxs));
      break;

    case I_STRLST:
      i = *cip++;
      dims = get_array_dims(idxs);
      if (dims != 1) {
        SYNTAX_T(_("invalid list index"));
      } else {
        value = nsvar_a(str_lst.var(i).var(idxs[0]));
      }
      break;

    //括弧の値の取得
    case I_OPEN:  //「(」
      cip--;
      value = getparam();  //括弧の値を取得
      break;

    case I_CHAR: value = ncharfun(); break;  //関数CHAR

    case I_SYS:  value = nsys(); break;

    case I_PLAY: value = nplay(); break;

    case I_NUMLST:
      i = *cip++;
      dims = get_array_dims(idxs);
      if (dims != 1) {
        SYNTAX_T(_("invalid list index"));
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
      icode_t *lp;
      icall();
      i = gstki;
      if (err)
        break;
      for (;;) {
        lp = iexe(i);
        if (!lp || err)
          break;
        clp = lp;
        cip = clp + icodes_per_line_desc();
        TRACE;
      }
      value = retval[0];
      break;
    }

    case I_NFC: value = nnfc(); break;

    case I_VREG: value = nvreg(); break;

    default:
      cip--;
      if (is_strexp())
        err = ERR_TYPE;
      else
        SYNTAX_T(_("expected numeric expression"));
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

// Get number of line at top left of the screen
uint32_t getTopLineNum() {
  utf8_int32_t *ptr = sc0.getScreenWindow();
  uint32_t n = 0;
  int rc = -1;
  while (isDigit(*ptr)) {
    n *= 10;
    n += *ptr - '0';
    if (n > INT32_MAX) {
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
  utf8_int32_t *ptr =
          sc0.getScreenWindow() + sc0.getStride() * (sc0.getHeight() - 1);
  uint32_t n = 0;
  int rc = -1;
  while (isDigit(*ptr)) {
    n *= 10;
    n += *ptr - '0';
    if (n > INT32_MAX) {
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
uint32_t Basic::getPrevLineNo(uint32_t lineno) {
  icode_t *lp, *prv_lp = NULL;
  int32_t rc = -1;
  for (lp = listbuf; *lp && (getlineno(lp) < lineno); lp += *lp) {
    prv_lp = lp;
  }
  if (prv_lp)
    rc = getlineno(prv_lp);
  return rc;
}

// Get the number of the line succeeding the specified line
uint32_t Basic::getNextLineNo(uint32_t lineno) {
  icode_t *lp;
  int32_t rc = -1;

  lp = getlp(lineno);
  if (lineno == getlineno(lp)) {
    // Move to the next line
    lp += *lp;
    rc = getlineno(lp);
  }
  return rc;
}

// Get the program text of the specified line
char *Basic::getLineStr(uint32_t lineno, uint8_t devno) {
  icode_t *lp = getlp(lineno);
  if (lineno != getlineno(lp))
    return NULL;

  // Output of specified line text to line buffer
  if (devno == 3)
    cleartbuf();

  if (devno == 0)
    sc0.setColor(COL(LINENUM), COL(BG));
  putnum(lineno, 0, devno);

  if (devno == 0) {
    screen_putch_disable_escape_codes = true;
    sc0.setColor(COL(FG), COL(BG));
  }
  c_putch(' ', devno);
  putlist(lp, devno);

  if (devno == 3)
    c_putch(0, devno);  // zero-terminate tbuf
  else if (devno == 0)
    screen_putch_disable_escape_codes = false;

  return tbuf;
}

void BASIC_FP Basic::do_goto(uint32_t line) {
  icode_t *lp = getlp(line);
  if (line != getlineno(lp)) {  // もし分岐先が存在しなければ
    err = ERR_ULN;              // エラー番号をセット
    return;
  }

  // Update the line pointer to the branch destination.
  clp = lp;
  // Update the intermediate code pointer to the beginning of the
  // intermediate code.
  cip = clp + icodes_per_line_desc();
  TRACE;
}

/***bc bas GOTO
Branches to a specified line or label.
\usage GOTO <line_number|&label>
\args
@line_number	a BASIC program line number
@&label		a BASIC program label
\note
Try not to use `GOTO` often as it tends to make programs
more difficult to understand. If possible, use loop constructs and
procedure calls instead.
\ref ON_GOTO
***/
void BASIC_FP Basic::igoto() {
  uint32_t lineno;  // 行番号

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
    if (err)
      return;
    do_goto(lineno);
  }
}

void BASIC_FP Basic::do_gosub_p(icode_t *lp, icode_t *ip) {
  //ポインタを退避
  if (gstki >= SIZE_GSTK) {  // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;        // エラー番号をセット
    return;
  }
  gstk[gstki].lp = clp;  // 行ポインタを退避
  gstk[gstki].ip = cip;  // 中間コードポインタを退避
  gstk[gstki].num_args = 0;
  gstk[gstki].str_args = 0;
  gstk[gstki++].proc_idx = NO_PROC;

  clp = lp;  // 行ポインタを分岐先へ更新
  cip = ip;
  TRACE;
}

void BASIC_FP Basic::do_gosub(uint32_t lineno) {
  icode_t *lp = getlp(lineno);
  if (lineno != getlineno(lp)) {  // もし分岐先が存在しなければ
    err = ERR_ULN;                // エラー番号をセット
    return;
  }
  do_gosub_p(lp, lp + icodes_per_line_desc());
}

/***bc bas GOSUB
Calls a subroutine.
\desc
`GOSUB` puts the current position in the program on a stack and then jumps
to the given location. It is then possible to return to the instruction after
the `GOSUB` command by using `RETURN`.
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
void BASIC_FP Basic::igosub() {
  uint32_t lineno;  // 行番号

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
the next instruction.

`ON GOSUB` calls the given location as a subroutine, while `ON GOTO` simply
continues execution there.
\ref GOTO GOSUB
***/
/***bc bas ON GOSUB
Call one of several subroutines, depending on the value of an expression.
\ref ON_GOTO GOSUB
***/
void BASIC_FP Basic::on_go(bool is_gosub, int cas) {
  icode_t *lp = NULL, *ip = NULL;
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
        ip = lp + icodes_per_line_desc();
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

void BASIC_INT Basic::ion() {
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

The handler is called when the MML pattern of any sound channel has
finished playing. It can be disabled using `ON PLAY ... OFF`.
\usage
ON PLAY channel CALL handler

ON PLAY channel OFF
\args
@channel	a sound channel [`0` to `{MML_CHANNELS_m1}`]
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
    if (ch < 0 || ch >= MML_CHANNELS) {
      E_VALUE(0, MML_CHANNELS - 1);
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

When an error has been forwarded from a sub-program that has been started
with `EXEC`, the line number of the sub-program in which the error has been
triggered is passed in `RET(2)`.
\note
Unlike other event handlers, `ON ERROR` does not call the error handler as a procedure,
and it is not possible to use `RETURN` to resume execution. `RESUME` must be used
instead.

When a program is executed using `EXEC` and does not define its own error
handler, any errors occurring in the child program will be forwarded to the
parent program's error handler, if there is one.
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
      event_error_ip = event_error_lp + icodes_per_line_desc();
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
      SYNTAX_T(_("expected GOTO or GOSUB"));
    }
  }
}

/***bc bas RESUME
Resume program execution after an error has been handled.
\desc
When an error has been caught by a handler set up using `ON ERROR`,
it is possible to resume execution after the instruction that caused the
error using `RESUME`.
\usage RESUME
\note
In other BASIC implementations, `RESUME` will retry the instruction that has
caused the error, and it is possible to continue after the error using
`RESUME NEXT`. In Engine BASIC, `RESUME` will always skip the instruction that
generated the error.
\bugs
It is not possible to repeat the instruction that has generated the error.
\ref ON_ERROR
***/
void Basic::iresume() {
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
void Basic::ierror() {
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
void BASIC_FP Basic::icall() {
  num_t n;
  index_t proc_idx = *cip++;

  struct proc_t &proc_loc = procs.proc(proc_idx);

  if (!proc_loc.lp || !proc_loc.ip) {
    err = ERR_UNDEFPROC;
    return;
  }

  if (gstki >= SIZE_GSTK) {  // もしGOSUBスタックがいっぱいなら
    err = ERR_GSTKOF;        // エラー番号をセット
    return;
  }

  int num_args = 0;
  int str_args = 0;
  // Argument stack indices cannot be modified while arguments are evaluated
  // because that would mess with the stack frame of the caller.
  int new_astk_num_i = astk_num_i;
  int new_astk_str_i = astk_str_i;
  if (gstki > 0 && gstk[gstki - 1].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki - 1].proc_idx);
    new_astk_num_i += p.locc_num;
    new_astk_str_i += p.locc_str;
  }
  if (*cip == I_OPEN) {
    ++cip;
    QList<BString> strargs;
    QList<num_t> numargs;

    if (*cip != I_CLOSE)
      for (;;) {
        if (is_strexp()) {
          BString b = istrexp();
          if (err)
            return;
          if (new_astk_str_i >= SIZE_ASTK)
            goto overflow;
          strargs.push_back(b);
          ++str_args;
        } else {
          n = iexp();
          if (err)
            return;
          if (new_astk_num_i >= SIZE_ASTK)
            goto overflow;
          numargs.push_back(n);
          ++num_args;
        }
        if (*cip != I_COMMA)
          break;
        ++cip;
      }

    if (checkClose())
      return;

    for (int i = 0; i < strargs.size(); ++i) {
      astk_str[new_astk_str_i++] = strargs[i];
    }
    for (int i = 0; i < numargs.size(); ++i) {
      astk_num[new_astk_num_i++] = numargs[i];
    }
  }
  astk_num_i = new_astk_num_i;
  astk_str_i = new_astk_str_i;
  if (num_args < proc_loc.argc_num || str_args < proc_loc.argc_str) {
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

void Basic::iproc() {
  err = ERR_PROCWOC;
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
void BASIC_FP Basic::ireturn() {
  if (!gstki) {        // もしGOSUBスタックが空なら
    err = ERR_GSTKUF;  // エラー番号をセット
    return;
  }

  // Set return values, if any.
  if (!end_of_statement()) {
    int rcnt = 0, rscnt = 0;
    num_t my_retval[MAX_RETVALS];
    BString *my_retstr[MAX_RETVALS];  // don't want to always construct all strings
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
  if (gstki > 0 && gstk[gstki - 1].proc_idx != NO_PROC) {
    // XXX: This can change if the parent procedure was called by this one
    // (directly or indirectly)!
    struct proc_t &p = procs.proc(gstk[gstki - 1].proc_idx);
    astk_num_i -= p.locc_num;
    astk_str_i -= p.locc_str;
  }
  if (profile_enabled && gstk[gstki].proc_idx != NO_PROC) {
    struct proc_t &p = procs.proc(gstk[gstki].proc_idx);
    p.profile_total += ESP.getCycleCount() - p.profile_current;
  }
  clp = gstk[gstki].lp;  //中間コードポインタを復帰
  cip = gstk[gstki].ip;  //行ポインタを復帰
  TRACE;
  return;
}

/***bc bas DO
Repeats a block of instructions while a condition is true or until a condition
becomes true.
\usage
DO
  instruction_block
LOOP [<WHILE|UNTIL> condition]
\args
@instruction_block one or more instructions on one or more lines
@condition	a numeric expression
\note
If `condition` is prefixed by `WHILE`, the loop will repeat if `condition`
is "true" (non-zero). If `condition` is prefixed by `UNTIL`, the loop will
continue if `condition` is "false" (zero). In any other case, the loop will
be exited and execution will continue with the instruction following the
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
void BASIC_FP Basic::ido() {
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
Executes a series of instructions as long as a specified condition is "true".

\usage WHILE condition
\args
@condition	any numeric expression
\note
`condition` is evaluated at the start of the loop. If it is "false",
execution continues after the corresponding `WEND` instruction. (Note that
all kinds of loops can be nested, so this may not be the nearest `WEND`
instruction.)

If `condition` is "true", the instructions following the `WHILE` instruction
will be executed, until the corresponding `WEND` instruction is reached,
at which point `condition` will be evaluated again to determine whether
to continue the loop.

The `WHILE` keyword can also form part of a `LOOP` command.
\ref LOOP
***/
void BASIC_FP Basic::iwhile() {
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
    icode_t *newip = getWENDptr(cip);
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
void BASIC_FP Basic::iwend() {
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

  icode_t *tmp_ip = cip;
  icode_t *tmp_lp = clp;

  // Jump to condition
  cip = lstk[lstki - 1].ip;
  clp = lstk[lstki - 1].lp;
  TRACE;

  num_t cond = iexp();
  // If the condition is true, continue with the loop
  if (cond || err) {
    return;
  }

  // If the condition is not true, pop the loop and
  // go back to the WEND.
  cip = tmp_ip;
  clp = tmp_lp;
  TRACE;
  lstki--;
}

/***bc bas FOR
Starts a loop repeating a block of instructions a specified number of times.
\usage
FOR loop_variable = start TO end [STEP increment]
  instruction_block
NEXT [loop_variable]
\args
@loop_variable	a numeric variable used as the loop counter
@start		initial value of the loop counter
@increment	amount the counter is changed each time through the loop +
                [default: `1`]
@instruction_block one or more instructions on one or more lines
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
void BASIC_FP Basic::ifor() {
  int index;
  num_t vto, vstep;  // FOR文の変数番号、終了値、増分

  // 変数名を取得して開始値を代入（例I=1）
  if (*cip == I_VAR) {  // もし変数がなかったら
    index = *++cip;     // 変数名を取得
    ivar();             // 代入文を実行
    lstk[lstki].local = false;
  } else if (*cip == I_LVAR) {
    index = *++cip;
    ilvar();
    lstk[lstki].local = true;
  } else {
    err = ERR_FORWOV;  // エラー番号をセット
  }
  if (err)  // もしエラーが生じたら
    return;

  // 終了値を取得（例TO 5）
  if (*cip == I_TO) {   // もしTOだったら
    cip++;              // 中間コードポインタを次へ進める
    vto = iexp();       // 終了値を取得
  } else {              // TOではなかったら
    err = ERR_FORWOTO;  //エラー番号をセット
    return;
  }

  // 増分を取得（例STEP 1）
  if (*cip == I_STEP) {  // もしSTEPだったら
    cip++;               // 中間コードポインタを次へ進める
    vstep = iexp();      // 増分を取得
  } else                 // STEPではなかったら
    vstep = 1;           // 増分を1に設定

  // 繰り返し条件を退避
  if (lstki >= SIZE_LSTK) {  // もしFORスタックがいっぱいなら
    err = ERR_LSTKOF;        // エラー番号をセット
    return;
  }
  lstk[lstki].lp = clp;  // 行ポインタを退避
  lstk[lstki].ip = cip;  // 中間コードポインタを退避

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
void BASIC_FP Basic::iloop() {
  token_t cond;

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

  cond = (token_t)*cip;
  if (cond == I_WHILE || cond == I_UNTIL) {
    ++cip;
    num_t exp = iexp();

    if ((cond == I_WHILE && exp != 0) || (cond == I_UNTIL && exp == 0)) {
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
void BASIC_FP Basic::inext() {
  int want_index;  // variable we want to NEXT, if specified
  bool want_local;
  int index;       // loop variable index we will actually use
  bool local;
  num_t vto;       // end of loop value
  num_t vstep;     // increment value

  if (!lstki) {  // FOR stack is empty
    err = ERR_LSTKUF;
    return;
  }

  if (*cip != I_VAR && *cip != I_LVAR)
    want_index = -1;  // just use whatever is TOS
  else {
    want_local = *cip++ == I_LVAR;
    want_index = *cip++;  // NEXT a specific loop variable
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
    err = ERR_LSTKUF;  // XXX: have something more descriptive
    return;
  }

  num_t &loop_var = local ? get_lvar(index) : nvar.var(index);

  vstep = lstk[lstki - 1].vstep;
  loop_var += vstep;
  vto = lstk[lstki - 1].vto;

  // Is this loop finished?
  if (((vstep < 0) && (loop_var < vto)) || ((vstep > 0) && (loop_var > vto))) {
    lstki--;  // drop it from FOR stack
    return;
  }

  // Jump to the start of the loop.
  cip = lstk[lstki - 1].ip;
  clp = lstk[lstki - 1].lp;
  TRACE;
}

/***bc bas EXIT
Exits a DO, FOR or WHILE loop.
\usage
EXIT DO

EXIT FOR [loop_variable]

EXIT WHILE
\args
@loop_variable	the counter variable of a FOR loop
\desc
Exits the top-most loop of the specified type, without regard for its loop
condition, or the specified `FOR` loop if there is a `loop_variable` argument.

The matching loop is dropped from the loop stack, and execution resumes
after the end of the loop (the nearest following `LOOP`, `NEXT` or `WEND`
statement, or the nearest `NEXT loop_variable` statement).
\note
`EXIT` only works as intended in loops with a "single end", i.e. loops that are
not iterated using more than one `LOOP`, `NEXT` or `WEND` statement.
\ref DO FOR WHILE
***/
void BASIC_FP Basic::iexit() {
  int want_index;  // variable we want to EXIT (for EXIT FOR)
  bool want_local = false;
  int index;       // loop variable index we will actually use (for EXIT FOR)
  bool local;

  if (!lstki) {  // loop stack is empty
    E_ERR(NOEXIT, _("no active loops"));
    return;
  }

  token_t exit_what = (token_t)*cip++;
  if (exit_what != I_FOR && exit_what != I_DO && exit_what != I_WHILE) {
    SYNTAX_T(_("unknown loop type"));
    return;
  }

  token_t search_for = I_NEXT;

  if (exit_what == I_DO) {
    search_for = I_LOOP;
    want_index = -1;
  } else if (exit_what == I_WHILE) {
    search_for = I_WEND;
    want_index = -2;
  } else if (*cip != I_VAR && *cip != I_LVAR)
    want_index = -42;  // just use whatever is TOS
  else {
    want_local = *cip++ == I_LVAR;
    want_index = *cip++;  // NEXT a specific loop variable
  }

  while (lstki) {
    // Get index of loop variable on top of stack.
    index = lstk[lstki - 1].index;
    local = lstk[lstki - 1].local;

    // Done if it's the one we want (or if none is specified).
    if ((want_index == -42 && index >= 0) ||		// found unspecific FOR loop
        (want_index == index && want_local == local))	// found something else that matches
      break;

    // If nothing that matches can be found we assume we want to EXIT to a
    // loop higher up the stack.
    lstki--;
  }

  if (!lstki) {
    // Didn't find anything that matches the EXIT.
    E_ERR(NOEXIT, _("no matching loop"));
    return;
  }

  lstki--;  // drop the loop from the loop stack

  // Jump past the end of the loop.
  icode_t *lp = cip;
  for (; clp;) {
    if (*lp == search_for) {
      icode_t *tlp = lp + 1;
      if ((want_index == -1 || want_index == -2) ||	// LOOP or WEND
          (want_index == -42 && *tlp != I_LVAR && *tlp != I_VAR) ||	// unspecific NEXT
          (local && *tlp++ == I_LVAR && *tlp++ == index) ||	// specific NEXT (local counter)
          (!local && *tlp++ == I_VAR && *tlp++ == index)) {	// specific NEXT (global counter)
        cip = tlp;
        TRACE;
        return;
      }
      // not the loop end we were looking for
      lp += token_size(lp);
    } else {
      switch (*lp) {
      case I_EOL:
      case I_REM:
      case I_SQUOT:
        // Continue at next line.
        clp += *clp;
        if (!*clp) {
          E_ERR(NOEXIT, _("cannot find end of loop"));
          return;
        }
        lp = cip = clp + icodes_per_line_desc();
        break;
      default:
        lp += token_size(lp);
        break;
      }
    }
  }
}

/***bc bas IF
Executes a instruction or instruction block depending on specified conditions.
\usage
IF condition THEN
  instruction_block
[ELSE IF condition THEN
  instruction_block]
...
[ELSE
  instruction_block]
ENDIF

IF condition THEN instructions [ELSE instructions]
\args
@condition		any numeric expression
@instruction_block	one or more instructions on one or more lines
@instructions		one or more instructions, separated by colons
\note
A `condition` is considered "false" if it is zero, and "true" in any other
case.

IMPORTANT: In many BASIC implementations, `ENDIF` is spelled `END IF` (that
is, as two commands). If the sequence `END IF` is entered or loaded in a
program, Engine BASIC will convert it to `ENDIF`.
***/
void BASIC_FP Basic::iif() {
  num_t condition;  // IF文の条件値
  icode_t *newip;   // ELSE文以降の処理対象ポインタ

  condition = iexp();  // 真偽を取得
  if (err)
    return;

  bool have_goto = false;
  if (*cip == I_THEN) {
    ++cip;
    if (*cip == I_NUM)  // XXX: should be "if not command"
      have_goto = true;
  } else if (*cip == I_GOTO) {
    ++cip;
    have_goto = true;
  } else {
    SYNTAX_T(_("expected THEN or GOTO"));
    return;
  }

  if (condition) {  // もし真なら
    if (have_goto)
      igoto();
    return;
  } else {
    newip = getELSEptr(cip);
    if (newip) {
      if (*newip == I_NUM) {
        do_goto(UNALIGNED_NUM_T(newip + 1));
      } else {
        cip = newip;
      }
    }
  }
}

/***bc bas ENDIF
Ends a multi-line `IF` instruction.
\ref IF
***/
void BASIC_FP Basic::iendif() {
}

/***bc bas ELSE
Introduces the `ELSE` branch of an `IF` instruction.
\ref IF
***/
void BASIC_FP Basic::ielse() {
  // Special handling for "ELSE IF": Skip one level of nesting. This avoids
  // having to have an ENDIF for each ELSE at the end of an IF ... ELSE IF
  // ... cascade.
  int adjust = 0;
  if (*cip == I_IF)
    adjust = -1;
  icode_t *newip = getELSEptr(cip, true, adjust);
  if (newip)
    cip = newip;
}

// スキップ
void BASIC_FP Basic::iskip() {
  while (*cip != I_EOL)  // I_EOLに達するまで繰り返す
    cip++;               // 中間コードポインタを次へ進める
}

void BASIC_FP Basic::ilabel() {
  ++cip;
}

/***bc bas END
Ends the program.
\usage END
\ref ENDIF
***/
void Basic::iend() {
  while (*clp)    // 行の終端まで繰り返す
    clp += *clp;  // 行ポインタを次へ進める
  while (*cip != I_EOL)
    ++cip;
}

void Basic::ecom() {
  err = ERR_COM;
}

void Basic::esyntax() {
  cip--;
  err = ERR_SYNTAX;
}

void Basic::iprint_() {
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
void Basic::irefresh() {
  sc0.refresh();
}

/***bc bas NEW
Deletes the program in memory as well as all variables.
\usage NEW
\ref CLEAR DELETE
***/
void Basic::inew_() {
  inew();
}

/***bc bas CLEAR
Deletes all variables.
\usage CLEAR
\ref NEW
***/
void Basic::iclear() {
  inew(NEW_VAR);
}

void BASIC_FP Basic::inil() {
}

void Basic::eunimp() {
  err = ERR_NOT_SUPPORTED;
}

void Basic::ilist_() {
  ilist();
}

void Basic::ilrun_() {
  if (*cip == I_PCX || *cip == I_IMAGE) {
    cip++;
    ildbmp();
  } else if (*cip == I_BG) {
    iloadbg();
  } else if (*cip == I_CONFIG) {
    iloadconfig();
  } else if (*cip == I_FONT) {
    iloadfont();
  } else if (*cip == I_MOD) {
    ++cip;
    iloadmod();
  } else
    ilrun();
}

void Basic::imerge() {
  ilrun();
}

/***bc bas STOP
Halts the program.

A program stopped by a `STOP` command can be resumed using `CONT`.
\usage STOP
\ref CONT
***/
void Basic::istop() {
  err = ERR_CTR_C;
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
void Basic::iprofile() {
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
      sprintf(lbuf, "%10u %s", (unsigned int)p.profile_total, proc_names.name(i));
      c_puts(lbuf);
      newline();
    }
    break;
  default:
    SYNTAX_T(_("expected ON, OFF or LIST"));
    break;
  }
}

int Basic::exec(const char *filename) {
    free(listbuf);
    listbuf = NULL;
    if (loadPrgText((char *)filename, NEW_ALL) == 0) {
      clp = listbuf;
      cip = clp + icodes_per_line_desc();
      irun(clp);
    }
    return err;
}

void Basic::exec_sub(const char *filename) {
  Basic *sub = new Basic;
  err = sub->exec(filename);

  if (err) {
    if (event_error_enabled) {
      // This replicates the code in irun() because we have to get the error
      // line number in the context of the subprogram.
      int sub_line = getlineno(sub->clp);
      delete sub;
      retval[0] = err;
      retval[1] = getlineno(clp);
      retval[2] = sub_line;
      err = 0;
      event_error_enabled = false;
      event_error_resume_lp = NULL;
      clp = event_error_lp;
      cip = event_error_ip;
      err_expected = NULL;  // prevent stale "expected" messages
      return;
    } else {
      // Print the error in the context of the subprogram.
      error();
    }
  }
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
void Basic::iexec() {
  BString file = getParamFname();
  int is_text = bfs.IsText((char *)file.c_str());
  if (is_text < 0) {
    err = -is_text;
    return;
  } else if (!is_text) {
    E_ERR(FORMAT, _("not a BASIC program"));
    return;
  }
  exec_sub(file.c_str());
}

void Basic::parse_params(token_t token, const token_t *syntax, std::vector<eb_param_t> &params) {
  if (!syntax) {
    SYNTAX_T("type mismatch");
    return;
  }

  token_t el;
  eb_param_t param;

  while ((el = *syntax++) != I_EOL) {
    param.type = el;
    switch (el) {
    case I_VAR:
    case I_NUM: {
        num_t value = iexp();
        param.num = value;
        params.push_back(param);
      }
      break;
    case I_STR: {
        BString value = istrexp();
        param.str = strdup(value.c_str());
        params.push_back(param);
      }
      break;
    default:
      if (*cip++ != el) {
        E_SYNTAX(el);
        return;
      }
      break;
    }
  }
}

void Basic::delete_params(std::vector<eb_param_t> &params) {
  for (auto p : params) {
    if (p.type == I_STR)
      free(p.str);
  }
}

void Basic::iextcmd(void) {
  std::vector<eb_param_t> params;
  token_t cmd = (token_t)cip[-1];

  parse_params(cmd, eb_command_syntax(cmd), params);

  if (!err) {
    basic_events_disabled = true;
    handle_command(cmd, &params[0]);
    basic_events_disabled = false;
  }

  delete_params(params);
}

// execute intermediate code
// Return value: next program execution position (line start)
icode_t *BASIC_FP Basic::iexe(index_t stk) {
  utf8_int32_t c;  // 入力キー
  err = 0;

  while (*cip != I_EOL) {
    //強制的な中断の判定
    if ((c = sc0.peekKey())) {  // If there are unread characters
      if (process_hotkeys(c)) {
        err_expected = NULL;
        break;
      }
    }

    // Execute intermediate code
    if (funtbl[*cip] != NULL) {
      (this->*funtbl[*cip++])();
    } else
      SYNTAX_T(_("expected command"));

    process_events();

    if (err || gstki < stk)
      return NULL;
  }

  return clp + *clp;
}

extern "C" void hook_data_abort(void)
{
  bc->exit(ERR_EXCEPTION_OFFSET + ERR_DATA_ABORT);
}

//Command precessor
uint8_t SMALL Basic::icom() {
  uint8_t rc = 1;
  cip = ibuf;  // Set the intermediate code pointer to the beginning of the intermediate code buffer

  // Branch by the intermediate code pointed to by the intermediate code pointer
  int exit_code = setjmp(jump);
  if (exit_code == 0) switch (*cip++) {
  case I_LOAD:
  case I_MERGE:
    ilrun_(); break;

  case I_CHAIN:
    if (ilrun()) {
      if (err == ERR_CHAIN)
        err = 0;
      sc0.show_curs(0);
      irun(clp);
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
      icode_t *save_cip = cip;
      istrexp();  // dump file name
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
      sc0.show_curs(0);
      irun();
    } else {
      sc0.show_curs(0);
      irun_();
      if (err == ERR_CHAIN)
        err = 0;
      irun(clp);
    }
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
    initialize_proc_pointers();
    initialize_label_pointers();
    igoto();
    if (!err) {
      restore_windows();
      sc0.show_curs(0);
      irun(clp, true, false);
    }
    break;
  case I_GOSUB:
    initialize_proc_pointers();
    initialize_label_pointers();

    // See I_CALL.
    while (*clp)
      clp += *clp;

    igosub();
    if (!err) {
      restore_windows();
      sc0.show_curs(0);
      irun(clp, true, false);
    }
    break;
  case I_CALL:
    initialize_proc_pointers();
    initialize_label_pointers();

    // icall() puts the current clp/cip on the call stack; we want irun() to
    // end execution after the procedure has completed, which it does if we
    // make clp point to the end of the program here.
    while (*clp)
      clp += *clp;

    icall();

    if (!err) {
      restore_windows();
      sc0.show_curs(0);
      irun(clp, true, false, cip);
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
  case I_RENUM:  irenum(); break;

  case I_DELETE: idelete(); break;
  case I_FORMAT: iformat(); break;
  case I_FLASH:  iflash(); break;

  default: {
      cip--;
      sc0.show_curs(0);
      index_t gstki_save = gstki;
      index_t lstki_save = lstki;
      index_t astk_num_i_save = astk_num_i;
      index_t astk_str_i_save = astk_str_i;
      iexe();
      gstki = gstki_save;
      lstki = lstki_save;
      astk_num_i = astk_num_i_save;
      astk_str_i = astk_str_i_save;
      break;
    }
  } else {
    if (exit_code > ERR_EXCEPTION_OFFSET)
      err = exit_code - ERR_EXCEPTION_OFFSET;
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

void Basic::autoexec() {
  struct stat st;
  BString autoexec(F("autoexec.bas"));
  if (_stat(autoexec.c_str(), &st) == 0) {
    sc0.peekKey();	// update internal key state
    if (eb_pad_state(0) == 0) {
      exec_sub(autoexec.c_str());
    } else {
      PRINT_P(_("Skipping autoexec.bas\n"));
    }
  }
}

#ifdef __DJGPP__
extern "C" {
#include <dpmi.h>
};
#endif

std::vector<const char *> kwtbl;

/*
   TOYOSHIKI Tiny BASIC
   The BASIC entry point
 */
void SMALL Basic::basic() {
  int len;  // Length of intermediate code
  char *textline;     // input line
  uint8_t rc;

  basic_init_environment();

  kwtbl.assign(kwtbl_init, kwtbl_init + sizeof(kwtbl_init) / sizeof(*kwtbl_init));

  funtbl.assign(funtbl_init, funtbl_init + sizeof(funtbl_init) / sizeof(*funtbl_init));
  numfuntbl.assign(numfuntbl_init, numfuntbl_init + sizeof(numfuntbl_init) / sizeof(*numfuntbl_init));
  strfuntbl.assign(strfuntbl_init, strfuntbl_init + sizeof(strfuntbl_init) / sizeof(*strfuntbl_init));

  basic_init_file_early();

  vs23.begin(CONFIG.interlace, CONFIG.lowpass, CONFIG.NTSC != 0);
  vs23.setLineAdjust(CONFIG.line_adjust);
  vs23.setColorSpace(DEFAULT_COLORSPACE);

  basic_init_input();

  size_list = 0;
  listbuf = NULL;
  memset(user_files, 0, sizeof(user_files));

  // Initialize execution environment
  inew();

  sc0.setCursorColor(CONFIG.cursor_color);
  sc0.init(SIZE_LINE, CONFIG.NTSC, CONFIG.mode - 1);
  sc0.reset_kbd(CONFIG.KEYBOARD);

  // See if we have the required fonts for the configured language.
  int rec_font = eb_load_lang_resources(CONFIG.language);
  if (rec_font < 0)
    CONFIG.language = 0;  // fall back to English

  basic_init_io();

  icls();
  sc0.show_curs(0);

  // Want to make sure we get the right hue.
  csp.setColorConversion(0, 7, 2, 4, true);
  show_logo();
  vs23.setColorSpace(DEFAULT_COLORSPACE);  // reset color conversion

  // Startup screen
  // Epigram
  eb_font(rec_font > 0 ? rec_font : 1);
  sc0.setColor(csp.colorFromRgb(72, 72, 72), COL(BG));
  srand(ESP.getCycleCount());
  c_puts_P(epigrams[random(sizeof(epigrams) / sizeof(*epigrams))]);
  newline();

  // Banner
  sc0.setColor(csp.colorFromRgb(192, 0, 0), COL(BG));
  static const char engine_basic[] PROGMEM = "Engine BASIC";
  c_puts_P(engine_basic);
  eb_font(rec_font > 0 ? rec_font : CONFIG.font);

  // Platform/version
  sc0.setColor(csp.colorFromRgb(64, 64, 64), COL(BG));
  sc0.locate(sc0.getWidth() - strlen_P(STR_EDITION), 7);
  c_puts_P(STR_EDITION);
  static const char __v[] PROGMEM = STR_VARSION;
  sc0.locate(sc0.getWidth() - strlen_P(__v), 8);
  c_puts_P(__v);

  basic_init_file_late();
  // Free memory
  sc0.setColor(COL(FG), COL(BG));
  sc0.locate(0, 2);

  uint64_t free_mem = getFreeMemory();
  if (free_mem == (uint64_t)-1)
    PRINT_P(_("unknown memory size\n"));
  else if (free_mem < 1048576) {
    putnum(free_mem, 0);
    PRINT_P(_(" bytes free\n"));
  } else {
    putnum(free_mem / 1048576, 0);
    PRINT_P(_(" MB free\n"));
  }

  PRINT_P(_("Directory "));
  char *cwd = new char[FILENAME_MAX];
  if (_getcwd(cwd, FILENAME_MAX) == NULL)
    c_puts_P(_("none"));
  else
    c_puts(cwd);
  delete[] cwd;
  newline();

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

  autoexec();
  sc0.show_curs(1);
  err_expected = NULL;
  error();  // "OK" or display an error message and clear the error number

  sc0.forget();

  // Enter one line from the terminal and execute
  while (1) {
    redirect_input_file = -1;
    redirect_output_file = -1;

    rc = sc0.edit();

    if (rc) {
      textline = (char *)sc0.getText();
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
      tlimR((char *)lbuf);
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
    if (*ibuf == I_NUM) {  // the beginning of the code buffer is a line number
      *ibuf = len;         // overwrite the token with the length
      inslist();           // Insert one line of intermediate code into the list
      recalc_indent();
      if (err)
        error();  // display program mode error message
      continue;
    }

    // If the intermediate code is a direct mode command
    if (icom())      // execute
      error(false);  // display direct mode error message
  }
}

Basic::Basic() {
  init_tcc();
  listbuf = NULL;
  event_error_enabled = false;
  basic_events_disabled = false;
}

Basic::~Basic() {
  free(listbuf);
}
