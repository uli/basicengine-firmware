#ifndef __BASIC_H
#define __BASIC_H

#include "ttconfig.h"

#include <stdint.h>
#include <string.h>
#include <sdfiles.h>
#include "BString.h"

#include "colorspace.h"
#include "video.h"
#include "variable.h"
#include "proc.h"
#include "sound.h"

#define SIZE_LINE 256    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 256    // 中間コード変換バッファサイズ

extern char lbuf[SIZE_LINE];
extern char tbuf[SIZE_LINE];
extern int32_t tbuf_pos;
extern unsigned char ibuf[SIZE_IBUF];

extern uint8_t err; // Error message index


uint8_t toktoi(bool find_prg_text = true);
int putlist(unsigned char* ip, uint8_t devno=0);

// メモリ書き込みポインタのクリア
static inline void cleartbuf() {
  tbuf_pos=0;
  memset(tbuf,0,SIZE_LINE);
}


void c_putch(uint8_t c, uint8_t devno = 0);
void c_puts(const char *s, uint8_t devno=0);
void c_puts_P(const char *s, uint8_t devno=0);
void screen_putch(uint8_t c, bool lazy = false);
extern bool screen_putch_disable_escape_codes;

#define PRINT_P(msg) c_puts_P(PSTR((msg)))

#define dbg_printf(x, y...) printf_P(PSTR(x), y)

void putnum(num_t value, int8_t d, uint8_t devno=0);
void putint(int value, int8_t d, uint8_t devno=0);
void putHexnum(uint32_t value, uint8_t d, uint8_t devno=0);
uint16_t BASIC_INT hex2value(char c);

void newline(uint8_t devno=0);

#define NUM_FONTS 4

BString getParamFname();
num_t getparam();

void get_input(bool numeric = false, uint8_t eoi = '\r');

uint32_t getPrevLineNo(uint32_t lineno);
uint32_t getNextLineNo(uint32_t lineno);
uint32_t getBottomLineNum();
uint32_t getTopLineNum();
char* getLineStr(uint32_t lineno, uint8_t devno = 3);

BString getstr(uint8_t eoi = '\r');

num_t& BASIC_FP get_lvar(uint8_t arg);
BString& get_lsvar(uint8_t arg);

void BASIC_FP ivar();
void BASIC_FP ilvar();

#define MAX_VAR_NAME 32  // maximum length of variable names
#define SIZE_GSTK 10     // GOSUB stack size
#define SIZE_LSTK 10     // FOR stack size
#define SIZE_ASTK 16	// argument stack

#define MAX_RETVALS 4

#define basic_bool(x) ((x) ? -1 : 0)

typedef struct {
  unsigned char ibuf[SIZE_IBUF];    // i-code conversion buffer

  int size_list;

  NumVariables nvar;
  VarNames nvar_names;
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
  Procedures procs;
  VarNames label_names;
  Labels labels;

  unsigned char *listbuf; // Pointer to program list area

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
  BString retstr[MAX_RETVALS];	    // multi-value returns (string)

  bool _event_error_enabled;
  unsigned char *_event_error_lp;
  unsigned char *_event_error_ip;
  unsigned char *_event_error_resume_lp;
  unsigned char *_event_error_resume_ip;
  
  bool math_exceptions_disabled;
  
  unsigned char *data_lp;
  unsigned char *data_ip;
  bool in_data;
} basic_ctx_t;

extern basic_ctx_t *bc;

#define ibuf bc->ibuf

#define size_list bc->size_list

#define nvar bc->nvar
#define nvar_names bc->nvar_names
#define svar bc->svar
#define svar_names bc->svar_names

#define num_arr bc->num_arr
#define num_arr_names bc->num_arr_names
#define str_arr bc->str_arr
#define str_arr_names bc->str_arr_names
#define str_lst bc->str_lst
#define str_lst_names bc->str_lst_names
#define num_lst bc->num_lst
#define num_lst_names bc->num_lst_names

#define proc_names bc->proc_names
#define procs bc->procs
#define label_names bc->label_names
#define labels bc->labels

#define listbuf bc->listbuf

#define clp bc->clp
#define cip bc->cip 

#define gstk bc->gstk
#define gstki bc->gstki

#define astk_num bc->astk_num
#define astk_num_i bc->astk_num_i
#define astk_str bc->astk_str
#define astk_str_i bc->astk_str_i

#define lstk bc->lstk
#define lstki bc->lstki

#define cont_clp bc->cont_clp
#define cont_cip bc->cont_cip

#define retval bc->retval
#define retstr bc->retstr

#define event_error_enabled bc->_event_error_enabled
#define event_error_lp bc->_event_error_lp
#define event_error_ip bc->_event_error_ip
#define event_error_resume_lp bc->_event_error_resume_lp
#define event_error_resume_ip bc->_event_error_resume_ip

#define math_exceptions_disabled bc->math_exceptions_disabled

#define data_lp bc->data_lp
#define data_ip bc->data_ip
#define in_data bc->in_data

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

int BASIC_FP token_size(uint8_t *code);
num_t BASIC_FP iexp();
BString istrexp(void);
bool BASIC_FP is_strexp();

void BASIC_FP do_call(uint8_t proc_idx);

BString sinput();

// キーワードテーブル
#include "kwtbl.h"

// Keyword count
#define SIZE_KWTBL (sizeof(kwtbl) / sizeof(const char*))
#define SIZE_KWTBL_EXT (sizeof(kwtbl_ext) / sizeof(const char *))

// i-code(Intermediate code) assignment
#include "kwenum.h"

extern uint8_t err;
extern const char *err_expected;

extern void E_SYNTAX(unsigned char token);
#define SYNTAX_T(exp) do { static const char __msg[] PROGMEM = exp; \
                           err = ERR_SYNTAX; err_expected = __msg; \
                      } while(0)
extern void E_VALUE(int32_t from, int32_t to);

#define E_ERR(code, exp) do { \
  static const char __msg[] PROGMEM = exp; \
  err = ERR_ ## code; err_expected = __msg; \
  } while(0)


#ifdef FLOAT_NUMS
// コマンド引数取得(uint32_t,引数チェックなし)
static inline uint8_t BASIC_FP getParam(int32_t& prm, token_t next_token) {
  num_t p = iexp();
  prm = p;
  if (!err && next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}
// コマンド引数取得(int32_t,引数チェックあり)
static inline uint8_t BASIC_FP getParam(int32_t& prm, int32_t v_min,  int32_t v_max, token_t next_token) {
  prm = iexp();
  if (!err && (prm < v_min || prm > v_max))
    E_VALUE(v_min, v_max);
  else if (next_token != I_NONE && *cip++ != next_token) {
    E_SYNTAX(next_token);
  }
  return err;
}
#endif

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

static inline bool end_of_statement()
{
  return *cip == I_EOL || *cip == I_COLON || *cip == I_ELSE || *cip == I_IMPLICITENDIF || *cip == I_SQUOT;
}

void* BASIC_INT sanitize_addr(uint32_t vadr, int type);

#ifdef ESP8266
extern "C" size_t umm_free_heap_size( void );
#else
extern int try_malloc();
#endif

#define MAX_USER_FILES 16
extern Unifile *user_files[MAX_USER_FILES];

extern sdfiles bfs;

#include <tscreenBase.h>
#include <tTVscreen.h>

extern tTVscreen sc0;

#define NEW_ALL		0
#define NEW_PROG	1
#define NEW_VAR		2

extern int redirect_output_file;
extern int redirect_input_file;

static inline uint16_t c_getch() {
  if (redirect_input_file >= 0)
    return user_files[redirect_input_file]->read();
  else
    return sc0.get_ch();
}

#define c_kbhit( ) sc0.isKeyIn()

#include <TKeyboard.h>
extern TKeyboard kb;
uint8_t BASIC_FP process_hotkeys(uint16_t c, bool dont_dump = false);

void c_puts(const char *s, uint8_t devno);
void c_puts_P(const char *s, uint8_t devno);

num_t BASIC_FP ivalue();

void iprint(uint8_t devno = 0,uint8_t nonewln = 0);
void iloadbg();

int32_t ncharfun();
num_t nvreg();
num_t nplay();

#define COL_BG		0
#define COL_FG		1
#define COL_KEYWORD	2
#define COL_LINENUM	3
#define COL_NUM		4
#define COL_VAR		5
#define COL_LVAR	6
#define COL_OP		7
#define COL_STR		8
#define COL_PROC	9
#define COL_COMMENT	10
#define COL_BORDER	11

#define CONFIG_COLS	12

typedef struct {
  int8_t NTSC;        // NTSC設定 (0,1,2,3)
  int8_t line_adjust;	// number of lines to add to default
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  uint8_t color_scheme[CONFIG_COLS][3];
  bool interlace;
  bool lowpass;
  uint8_t mode;
  uint8_t font;
  uint8_t cursor_color;
  uint8_t beep_volume;
} SystemConfig;
extern SystemConfig CONFIG;

#define COL(n)	(csp.colorFromRgb(CONFIG.color_scheme[COL_ ## n]))

void BASIC_INT event_handle_play(int ch);
extern bool event_play_enabled;
extern uint8_t event_play_proc_idx[SOUND_CHANNELS];

#define MAX_PADS 3
extern bool event_pad_enabled;
extern uint8_t event_pad_proc_idx[MAX_PADS];
extern int event_pad_last[MAX_PADS];
void event_handle_pad();

void event_handle_sprite();
extern uint8_t event_sprite_proc_idx;

#ifdef USE_BG_ENGINE
extern bool restore_text_window;
extern bool restore_bgs;
#endif

void SMALL resize_windows();
void SMALL restore_windows();

void BASIC_FP init_stack_frame();
void BASIC_FP push_num_arg(num_t n);

extern const uint8_t *fonts[NUM_FONTS];

void BASIC_FP pump_events(void);

void basic_init_io();
void basic_init_input();

#endif
